/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>

#include "lmmp/lmmpn.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

/* basecase ↔ 牛顿求逆切换阈值：INV_NEWTON_THRESHOLD 出现在 invappr_ 根分派
   与牛顿梯子深度循环（while nr>=T，nr=(nr>>1)+1）。高路直调 newton_ 并绑
   T=n：梯子仅一步（种子 nr=(n>>1)+1<n 走 basecase），恰好代表候选
   (sub_n,n]（sub_n=(n>>1)+1）；T<=sub_n 的候选梯子更深，须绑候选经完整
   newton_ deployed 实测。INV_MODM/DIV_DIVIDE 保持默认。
   正确性：basecase 精确值 vs newton ±1 容差 + 恒等式
   a*(in+B^na) ∈ [B^2na-2a, B^2na)（newton 可返回精确值-1）。 */
enum {
    MIN_N = 5, MAX_N = 128, STEP_N = 1, CANDIDATE_MIN = MIN_N, CANDIDATE_MAX = MAX_N, CANDIDATE_STEP = 2,
    CANDIDATE_LAST = CANDIDATE_MIN + (CANDIDATE_MAX - CANDIDATE_MIN) / CANDIDATE_STEP * CANDIDATE_STEP
};

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_size_t na;
    mp_size_t bind_threshold;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_INV_NEWTON_THRESHOLD = context->bind_threshold;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->na);
}

static mp_limb_t call_basecase(void* opaque) {
    measure_context* context = opaque;
    lmmp_inv_basecase_(context->dst, context->numa, context->na);
    return context->dst[0] ^ context->dst[context->na - 1];
}

static mp_limb_t call_newton(void* opaque) {
    measure_context* context = opaque;
    lmmp_invappr_newton_(context->dst, context->numa, context->na);
    return context->dst[0] ^ context->dst[context->na - 1];
}

/* ±1 容差比较：|x-y|<=1 */
static int close_pm1(mp_srcptr x, mp_srcptr y, mp_size_t n) {
    int c = lmmp_cmp_(x, y, n);
    if (c == 0)
        return 1;
    /* 判相邻：较小者 +1 == 较大者 */
    mp_srcptr lo = c > 0 ? y : x;
    mp_srcptr hi = c > 0 ? x : y;
    mp_size_t i = 0;
    while (i < n && lo[i] == LIMB_MAX)
        ++i;
    if (i == n)
        return 0; /* lo 全 FF，+1 溢出 n limb，差不可能为 1（hi 也是 n limb） */
    if (lo[i] + 1 != hi[i])
        return 0;
    for (mp_size_t j = i + 1; j < n; ++j)
        if (lo[j] != hi[j])
            return 0;
    for (mp_size_t j = 0; j < i; ++j)
        if (hi[j] != 0)
            return 0;
    return 1;
}

/* 恒等式锚点：in = floor((B^2na-1)/a) - B^na + [0|-1]。
   上界：p = a*(in+B^na) <= B^2na-1（p+1 无进位）；
   下界/精确性：用库除法算 q=floor((B^2na-1)/a)，要求 in == q-B^na 或 q-B^na-1。
   scratch 需 6na+8 limb：[0,2na)=p，[2na,4na+1)=B^2na-1，[4na+2,5na+3)=q，
   [5na+3,6na+3)=余数。 */
static int check_identity(mp_srcptr numa, mp_size_t na, mp_srcptr in, mp_ptr scratch) {
    lmmp_mul_(scratch, numa, na, in, na);
    lmmp_add_n_(scratch + na, scratch + na, numa, na); /* p = a*in + a*B^na */
    mp_limb_t cy = lmmp_add_1_(scratch, scratch, 2 * na, 1);
    if (cy)
        return 0;
    lmmp_sub_1_(scratch, scratch, 2 * na, 1);
    /* N = B^2na-1 */
    mp_ptr N = scratch + 2 * na;
    for (mp_size_t i = 0; i < 2 * na; ++i) N[i] = LIMB_MAX;
    N[2 * na] = 0;
    mp_ptr q = scratch + 4 * na + 2;
    mp_ptr r = scratch + 5 * na + 3;
    lmmp_div_(q, r, N, 2 * na + 1, numa, na);
    /* q = B^na + in + [0|1]（na+1 limb，顶 limb ∈ {1,2}）：
       契约只允许 in 低估 1，in > q_low 直接拒绝，in < q_low 须恰为 q_low-1 */
    if (q[na] != 1 && q[na] != 2)
        return 0;
    int c = lmmp_cmp_(in, q, na);
    if (c == 0)
        return 1;
    if (c > 0)
        return 0;
    /* in == q_low-1 判相邻 */
    mp_size_t i = 0;
    while (i < na && q[i] == 0)
        ++i;
    if (i == na || q[i] - 1 != in[i])
        return 0;
    for (mp_size_t j = i + 1; j < na; ++j)
        if (q[j] != in[j])
            return 0;
    for (mp_size_t j = 0; j < i; ++j)
        if (in[j] != LIMB_MAX)
            return 0;
    return 1;
}

/* 候选按步长稀疏，公共 recommend 的逐整数遍历不适用，自行汇总 */
static void recommend_stepped(const char* name, const double* scores, const unsigned long* counts, int cmin,
                              int cmax, int cstep, mp_size_t current) {
    int best = cmin;
    for (int c = cmin + cstep; c <= cmax; c += cstep)
        if (counts[c] && scores[c] / counts[c] < scores[best] / counts[best])
            best = c;
    double best_score = scores[best] / counts[best];
    int flat_low = best, flat_high = best;
    while (flat_low - cstep >= cmin && scores[flat_low - cstep] / counts[flat_low - cstep] <= best_score * 1.01)
        flat_low -= cstep;
    while (flat_high + cstep <= cmax && scores[flat_high + cstep] / counts[flat_high + cstep] <= best_score * 1.01)
        flat_high += cstep;
    for (int c = cmin; c <= cmax; c += cstep)
        if (counts[c])
            printf("# badness candidate=%d avg_us=%.6f\n", c, scores[c] / counts[c]);
    printf("# recommended=%d\n", best);
    printf("# recommended_avg_us=%.6f\n", best_score);
    printf("# flat_within_1pct=%d..%d\n", flat_low, flat_high);
    printf("# current=%llu\n", (unsigned long long)current);
    printf("# points=%lu\n", counts[best]);
    printf("recommended %s = %d\n", name, best);
}

static void print_info(void) {
    printf("program=tune_lmmpn_inv_newton\n");
    printf("parameter=INV_NEWTON_THRESHOLD\n");
    printf("consumer=lmmp_invappr_ root dispatch + newton ladder depth; inv_prediv_/lmmp_inv_ upstream\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)INV_NEWTON_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_INV_NEWTON_THRESHOLD);
    printf("candidate_range=%d..%d step=%d (scan upper bound %d)\n", CANDIDATE_MIN, CANDIDATE_LAST, CANDIDATE_STEP,
           CANDIDATE_MAX);
    printf("input_range=%d..%d step=%d\n", MIN_N, MAX_N, STEP_N);
    printf("forced_root_equivalence=sub_n=(n>>1)+1; candidates T<=sub_n via deployed scan\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_inv_newton");
        return EXIT_SUCCESS;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    setvbuf(stdout, NULL, _IOLBF, 4096);
    if (options.action == LMMP_TUNE_INFO) {
        print_info();
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current("INV_NEWTON_THRESHOLD", (unsigned long long)INV_NEWTON_THRESHOLD,
                                (unsigned long long)lmmp_tune_INV_NEWTON_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t stride = MAX_N + 4;
    mp_ptr storage = alloc_limbs(4 * stride + 6 * MAX_N + 16);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;
    mp_ptr low = numa + stride;
    mp_ptr high = low + stride;
    mp_ptr ref = high + stride;
    mp_ptr scratch = ref + stride; /* 3*MAX_N+8 */
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    double deployed_scores[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_INV_NEWTON_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0, unstable_deployed = 0;

    printf("# program=tune_lmmpn_inv_newton\n");
    printf("# parameter=INV_NEWTON_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)INV_NEWTON_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# candidate_range=%d..%d step=%d (scan upper bound %d) input_range=%d..%d step=%d\n", CANDIDATE_MIN,
           CANDIDATE_LAST, CANDIDATE_STEP, CANDIDATE_MAX, MIN_N, MAX_N, STEP_N);
    printf("# high=newton_ with T=n (one Newton step, sub_n=(n>>1)+1); T<=sub_n via deployed scan\n");
    printf("# INV_MODM=%llu DIV_DIVIDE=%llu keep defaults\n", (unsigned long long)lmmp_tune_INV_MODM_THRESHOLD,
           (unsigned long long)lmmp_tune_DIV_DIVIDE_THRESHOLD);
    lmmp_tune_print_env_comment();
    printf("# n low_us low_mad high_us high_mad retries stable sub_n\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t n = MIN_N; n <= MAX_N; n += STEP_N) {
        fill_random(numa, n, (mp_limb_t)(options.seed + 4 * n));
        mp_size_t sub_n = (n >> 1) + 1;

        lmmp_inv_basecase_(ref, numa, n);
        lmmp_inv_basecase_(low, numa, n);
        lmmp_tune_INV_NEWTON_THRESHOLD = n;
        lmmp_invappr_newton_(high, numa, n);
        lmmp_tune_INV_NEWTON_THRESHOLD = saved_threshold;
        if (!close_pm1(low, high, n) || !check_identity(numa, n, ref, scratch) ||
            !check_identity(numa, n, high, scratch)) {
            fprintf(stderr, "inv result/identity mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }

        measure_context low_context = {.dst = low, .numa = numa, .na = n, .bind_threshold = saved_threshold};
        measure_context high_context = {.dst = high, .numa = numa, .na = n, .bind_threshold = n};
        lmmp_tune_bench low_bench = {
            .bind = bind_threshold, .reset = reset_output, .call = call_basecase, .context = &low_context};
        lmmp_tune_bench high_bench = {
            .bind = bind_threshold, .reset = reset_output, .call = call_newton, .context = &high_context};
        double low_us, low_mad, high_us, high_mad;
        unsigned retries;
        bool stable;
        if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                    &retries, &stable)) {
            fprintf(stderr, "measurement failed at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }
        if (stable)
            ++stable_points;
        else
            ++unstable_points;
        printf("%llu %.6f %.6f %.6f %.6f %u %u %llu\n", (unsigned long long)n, low_us, low_mad, high_us, high_mad,
               retries, stable, (unsigned long long)sub_n);
        for (int c = CANDIDATE_MIN; c <= CANDIDATE_MAX; c += CANDIDATE_STEP) {
            if ((mp_size_t)c <= sub_n)
                continue;
            scores[c] += n >= (mp_size_t)c ? high_us : low_us;
            ++counts[c];
        }

        for (int candidate = CANDIDATE_MIN; candidate <= (int)sub_n && candidate <= CANDIDATE_MAX;
             candidate += CANDIDATE_STEP) {
            measure_context dep_context = {.dst = high, .numa = numa, .na = n, .bind_threshold = (mp_size_t)candidate};
            lmmp_tune_bench dep_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_newton, .context = &dep_context};
            double dep_us, dep_mad;
            unsigned dep_retries;
            if (!lmmp_tune_measure(&dep_bench, &options, &dep_us, &dep_mad, &dep_retries)) {
                fprintf(stderr, "deployed measurement failed at n=%llu T=%d\n", (unsigned long long)n, candidate);
                result = EXIT_FAILURE;
                break;
            }
            /* 与配对测量同一规则：重试耗尽仍超限的样本保留计分，但计入不稳定并在汇总中提示 */
            if (dep_mad > options.mad_limit)
                ++unstable_deployed;
            deployed_scores[candidate] += dep_us;
            ++counts[candidate];
            printf("# deployed n=%llu T=%d us=%.6f mad=%.6f retries=%u\n", (unsigned long long)n, candidate, dep_us,
                   dep_mad, dep_retries);
        }
        if (result != EXIT_SUCCESS)
            break;
    }

    if (result == EXIT_SUCCESS) {
        for (int candidate = CANDIDATE_MIN; candidate <= CANDIDATE_MAX; candidate += CANDIDATE_STEP)
            scores[candidate] += deployed_scores[candidate];
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            if (unstable_deployed > 0)
                printf("# note: %u unstable deployed sample(s) retained in scoring\n", unstable_deployed);
            recommend_stepped("INV_NEWTON_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP,
                              saved_threshold);
        }
    }
    lmmp_tune_INV_NEWTON_THRESHOLD = saved_threshold;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
