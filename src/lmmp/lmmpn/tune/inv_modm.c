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

/* 牛顿步残差积 plain ↔ 梅森分发门：INV_MODM_THRESHOLD 在 newton_ 每步内部
   （na<T || mn>=na+nr → plain mul，否则 mul_mersenne_ 分发 dc/fft），不是
   根分派，无法用低/高曲线复用评分 → 全 deployed 扫描：逐 (根 na, 候选 T)
   绑 T 直调完整 newton_ 实测。INV_NEWTON 默认 21 钉死梯子深度；
   MUL_MERSENNE_* 不绑定、沿默认分发梯子。
   另测 T=极大（全 plain）/T=极小（全 mersenne）两条参考曲线。
   恒等式锚点 a*(in+B^na) ∈ [B^2na-2a, B^2na)。 */
enum { MIN_N = 32, MAX_N = 512, STEP_N = 32, CANDIDATE_MIN = 16, CANDIDATE_MAX = 256, CANDIDATE_STEP = 16 };

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_size_t na;
    mp_size_t bind_threshold;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_INV_MODM_THRESHOLD = context->bind_threshold;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->na);
}

static mp_limb_t call_newton(void* opaque) {
    measure_context* context = opaque;
    lmmp_invappr_newton_(context->dst, context->numa, context->na);
    return context->dst[0] ^ context->dst[context->na - 1];
}

/* 恒等式锚点：in = floor((B^2na-1)/a) - B^na + [0|-1]。
   上界：p = a*(in+B^na) <= B^2na-1；精确性：库除法算 q=floor((B^2na-1)/a)，
   要求 in == q-B^na 或 q-B^na-1。scratch 需 6na+8 limb。 */
static int check_identity(mp_srcptr numa, mp_size_t na, mp_srcptr in, mp_ptr scratch) {
    lmmp_mul_(scratch, numa, na, in, na);
    lmmp_add_n_(scratch + na, scratch + na, numa, na);
    mp_limb_t cy = lmmp_add_1_(scratch, scratch, 2 * na, 1);
    if (cy)
        return 0;
    lmmp_sub_1_(scratch, scratch, 2 * na, 1);
    mp_ptr N = scratch + 2 * na;
    for (mp_size_t i = 0; i < 2 * na; ++i) N[i] = LIMB_MAX;
    N[2 * na] = 0;
    mp_ptr q = scratch + 4 * na + 2;
    mp_ptr r = scratch + 5 * na + 3;
    lmmp_div_(q, r, N, 2 * na + 1, numa, na);
    /* q = B^na + in + [0|1]（na+1 limb，顶 limb ∈ {1,2}） */
    if (q[na] != 1 && q[na] != 2)
        return 0;
    int c = lmmp_cmp_(in, q, na);
    if (c == 0)
        return 1;
    if (c < 0)
            return 0;
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

static void print_info(void) {
    printf("program=tune_lmmpn_inv_modm\n");
    printf("parameter=INV_MODM_THRESHOLD\n");
    printf("consumer=invappr_newton_ per-step residual product gate (dispatch mersenne dc/fft)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)INV_MODM_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_INV_MODM_THRESHOLD);
    printf("candidate_range=%d..%d step=%d\n", CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP);
    printf("input_range=root na %d..%d step=%d (full deployed scan)\n", MIN_N, MAX_N, STEP_N);
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_inv_modm");
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
        lmmp_tune_print_current("INV_MODM_THRESHOLD", (unsigned long long)INV_MODM_THRESHOLD,
                                (unsigned long long)lmmp_tune_INV_MODM_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t stride = MAX_N + 4;
    mp_ptr storage = alloc_limbs(3 * stride + 6 * MAX_N + 16);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;
    mp_ptr out = numa + stride;
    mp_ptr ref = out + stride;
    mp_ptr scratch = ref + stride;
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_INV_MODM_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_inv_modm\n");
    printf("# parameter=INV_MODM_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)INV_MODM_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# candidate_range=%d..%d step=%d input=root na %d..%d step=%d\n", CANDIDATE_MIN, CANDIDATE_MAX,
           CANDIDATE_STEP, MIN_N, MAX_N, STEP_N);
    printf("# full deployed scan: threshold gates every Newton step, no root-only curve reuse\n");
    printf("# INV_NEWTON=%llu pins ladder depth; MUL_MERSENNE_* unbound (default dispatch)\n",
           (unsigned long long)lmmp_tune_INV_NEWTON_THRESHOLD);
    lmmp_tune_print_env_comment();
    printf("# n candidate us mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t n = MIN_N; n <= MAX_N && result == EXIT_SUCCESS; n += STEP_N) {
        fill_random(numa, n, (mp_limb_t)(options.seed + 4 * n));
        /* 参考输出：默认阈值；锚点先于计时 */
        lmmp_invappr_newton_(ref, numa, n);
        if (!check_identity(numa, n, ref, scratch)) {
            fprintf(stderr, "inv identity mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }
        /* 参考曲线：全 plain（T=MAX_N+1）与全 mersenne（T=1） */
        for (int pass = 0; pass < 2; ++pass) {
            measure_context ctx = {.dst = out,
                                   .numa = numa,
                                   .na = n,
                                   .bind_threshold = pass == 0 ? MAX_N + 1 : 1};
            lmmp_tune_bench bench = {.bind = bind_threshold, .reset = reset_output, .call = call_newton, .context = &ctx};
            double us, mad;
            unsigned retries;
            if (!lmmp_tune_measure(&bench, &options, &us, &mad, &retries)) {
                fprintf(stderr, "reference measurement failed at n=%llu pass=%d\n", (unsigned long long)n, pass);
                result = EXIT_FAILURE;
                break;
            }
            printf("# ref n=%llu %s us=%.6f mad=%.6f retries=%u\n", (unsigned long long)n,
                   pass == 0 ? "all_plain" : "all_mersenne", us, mad, retries);
        }
        if (result != EXIT_SUCCESS)
            break;

        for (int candidate = CANDIDATE_MIN; candidate <= CANDIDATE_MAX; candidate += CANDIDATE_STEP) {
            measure_context ctx = {.dst = out, .numa = numa, .na = n, .bind_threshold = (mp_size_t)candidate};
            /* 正确性：候选路径结果与参考 ±1 容差（此处用恒等式独立校验） */
            lmmp_tune_INV_MODM_THRESHOLD = (mp_size_t)candidate;
            lmmp_invappr_newton_(out, numa, n);
            lmmp_tune_INV_MODM_THRESHOLD = saved_threshold;
            if (!check_identity(numa, n, out, scratch)) {
                fprintf(stderr, "candidate identity mismatch at n=%llu T=%d\n", (unsigned long long)n, candidate);
                result = EXIT_FAILURE;
                break;
            }
            lmmp_tune_bench bench = {.bind = bind_threshold, .reset = reset_output, .call = call_newton, .context = &ctx};
            double us, mad;
            unsigned retries;
            if (!lmmp_tune_measure(&bench, &options, &us, &mad, &retries)) {
                fprintf(stderr, "measurement failed at n=%llu T=%d\n", (unsigned long long)n, candidate);
                result = EXIT_FAILURE;
                break;
            }
            int stable = mad <= options.mad_limit;
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("%llu %d %.6f %.6f %u %d\n", (unsigned long long)n, candidate, us, mad, retries, stable);
            scores[candidate] += us;
            ++counts[candidate];
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            /* 候选按步长稀疏，recommend 的逐整数遍历不适用，自行汇总 */
            int best = CANDIDATE_MIN;
            for (int c = CANDIDATE_MIN + CANDIDATE_STEP; c <= CANDIDATE_MAX; c += CANDIDATE_STEP)
                if (counts[c] && scores[c] / counts[c] < scores[best] / counts[best])
                    best = c;
            double best_score = scores[best] / counts[best];
            int flat_low = best, flat_high = best;
            while (flat_low - CANDIDATE_STEP >= CANDIDATE_MIN &&
                   scores[flat_low - CANDIDATE_STEP] / counts[flat_low - CANDIDATE_STEP] <= best_score * 1.01)
                flat_low -= CANDIDATE_STEP;
            while (flat_high + CANDIDATE_STEP <= CANDIDATE_MAX &&
                   scores[flat_high + CANDIDATE_STEP] / counts[flat_high + CANDIDATE_STEP] <= best_score * 1.01)
                flat_high += CANDIDATE_STEP;
            for (int c = CANDIDATE_MIN; c <= CANDIDATE_MAX; c += CANDIDATE_STEP)
                printf("# badness candidate=%d avg_us=%.6f\n", c, scores[c] / counts[c]);
            printf("# recommended=%d\n", best);
            printf("# recommended_avg_us=%.6f\n", best_score);
            printf("# flat_within_1pct=%d..%d\n", flat_low, flat_high);
            printf("# current=%llu\n", (unsigned long long)saved_threshold);
            printf("# points=%lu\n", counts[best]);
            printf("recommended INV_MODM_THRESHOLD = %d\n", best);
        }
    }
    lmmp_tune_INV_MODM_THRESHOLD = saved_threshold;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
