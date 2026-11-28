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

/* basecase ↔ 分治除法切换阈值：DIV_DIVIDE_THRESHOLD 出现在根层分派
   （lmmp_div_s_/lmmp_div_/inv_basecase_）与 divide_n_ 自递归时 hi、lo 两个子问题处的判别条件。
   高路直调 divide_ 并绑 T=n：子问题 hi=ceil(n/2)<T 走 basecase，首块
   nq=0 无递归，恰好代表候选 (sub_n,n]（sub_n=ceil(n/2)）；T<=sub_n 的
   候选会让子问题再入分治，须按候选绑 T 经完整 div_s_ 另行 deployed 实测。
   DIV_MULINV_L/N 保持默认：本网格 nb<=256<L=427，不触 mulinv。 */
enum {
    MIN_N = 6, MAX_N = 256, STEP_N = 2, CANDIDATE_MIN = MIN_N, CANDIDATE_MAX = MAX_N, CANDIDATE_STEP = 4,
    CANDIDATE_LAST = CANDIDATE_MIN + (CANDIDATE_MAX - CANDIDATE_MIN) / CANDIDATE_STEP * CANDIDATE_STEP
};

typedef struct {
    mp_ptr dstq;
    mp_ptr numa;   /* 被除数副本池基址，容量 LMMP_TUNE_BATCH_COPIES*na */
    mp_ptr cursor; /* 批内下一份被除数 */
    mp_srcptr numa_orig;
    mp_srcptr numb;
    mp_size_t na;
    mp_size_t nb;
    mp_limb_t inv21;
    mp_size_t bind_threshold; /* 每批次绑定的阈值：直调路=根规模 n，deployed=候选 */
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_DIV_DIVIDE_THRESHOLD = context->bind_threshold;
}

/* 除法破坏被除数：每段计时前铺 count 份副本，每次调用从原始被除数出发 */
static void reset_batch(void* opaque, mp_size_t count) {
    measure_context* context = opaque;
    for (mp_size_t i = 0; i < count; ++i)
        lmmp_copy(context->numa + i * context->na, context->numa_orig, context->na);
    context->cursor = context->numa;
}

static mp_ptr take_dividend(measure_context* context) {
    mp_ptr numa = context->cursor;
    context->cursor += context->na;
    return numa;
}

static mp_limb_t call_basecase(void* opaque) {
    measure_context* context = opaque;
    mp_ptr numa = take_dividend(context);
    mp_limb_t qh = lmmp_div_basecase_(context->dstq, numa, context->na, context->numb, context->nb, context->inv21);
    return qh ^ context->dstq[0] ^ numa[0];
}

static mp_limb_t call_divide(void* opaque) {
    measure_context* context = opaque;
    mp_ptr numa = take_dividend(context);
    mp_limb_t qh = lmmp_div_divide_(context->dstq, numa, context->na, context->numb, context->nb, context->inv21);
    return qh ^ context->dstq[0] ^ numa[0];
}

static mp_limb_t call_deployed(void* opaque) {
    measure_context* context = opaque;
    mp_ptr numa = take_dividend(context);
    mp_limb_t qh = lmmp_div_s_(context->dstq, numa, context->na, context->numb, context->nb);
    return qh ^ context->dstq[0] ^ numa[0];
}

/* 商余数互比：qh、[dstq,na-nb]、余数 [numa,nb] 全等 */
static int check_pair(measure_context* low, measure_context* high) {
    mp_limb_t qh_low = lmmp_div_basecase_(low->dstq, low->numa, low->na, low->numb, low->nb, low->inv21);
    mp_limb_t saved = lmmp_tune_DIV_DIVIDE_THRESHOLD;
    lmmp_tune_DIV_DIVIDE_THRESHOLD = high->nb; /* T=n 强制根层分治 */
    mp_limb_t qh_high = lmmp_div_divide_(high->dstq, high->numa, high->na, high->numb, high->nb, high->inv21);
    lmmp_tune_DIV_DIVIDE_THRESHOLD = saved;
    if (qh_low != qh_high || lmmp_cmp_(low->dstq, high->dstq, low->na - low->nb) != 0 ||
        lmmp_cmp_(low->numa, high->numa, low->nb) != 0)
        return 0;
    return 1;
}

/* 重建锚点：qfull*d + r == a，qfull = qh:[q,nq] 共 nq+1 limb（仅库内乘法/加法）。
   scratch 需 na+1 limb。 */
static int check_anchor(mp_srcptr numa_orig, mp_size_t na, mp_srcptr numb, mp_size_t nb, mp_limb_t qh,
                        mp_srcptr q, mp_srcptr r, mp_ptr qfull, mp_ptr scratch) {
    mp_size_t nq = na - nb;
    lmmp_copy(qfull, q, nq);
    qfull[nq] = qh;
    lmmp_mul_(scratch, qfull, nq + 1, numb, nb);
    /* r 零扩展到 na limb（借用 qfull 后段），整段相加自然传播进位 */
    lmmp_copy(qfull + nq + 1, r, nb);
    lmmp_zero(qfull + nq + 1 + nb, na - nb);
    lmmp_add_n_(scratch, scratch, qfull + nq + 1, na);
    return lmmp_cmp_(scratch, numa_orig, na) == 0;
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
    printf("program=tune_lmmpn_div_divide\n");
    printf("parameter=DIV_DIVIDE_THRESHOLD\n");
    printf("consumer=lmmp_div_s_/lmmp_div_ root dispatch, divide_n_ hi/lo recursion, inv_basecase_\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)DIV_DIVIDE_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_DIV_DIVIDE_THRESHOLD);
    printf("candidate_range=%d..%d step=%d (scan upper bound %d)\n", CANDIDATE_MIN, CANDIDATE_LAST, CANDIDATE_STEP,
           CANDIDATE_MAX);
    printf("input_range=na=2nb, nb %d..%d step=%d\n", MIN_N, MAX_N, STEP_N);
    printf("forced_root_equivalence=sub_n=ceil(n/2); candidates T<=sub_n via deployed scan\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_div_divide");
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
        lmmp_tune_print_current("DIV_DIVIDE_THRESHOLD", (unsigned long long)DIV_DIVIDE_THRESHOLD,
                                (unsigned long long)lmmp_tune_DIV_DIVIDE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t input_stride = 2 * MAX_N + 4;
    const mp_size_t pool_stride = LMMP_TUNE_BATCH_COPIES * 2 * MAX_N;
    mp_ptr storage = alloc_limbs(4 * input_stride + 2 * pool_stride);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa_orig = storage;
    mp_ptr numb = storage + input_stride;
    mp_ptr numa_low = numb + input_stride;
    mp_ptr q_low = numa_low + pool_stride;
    mp_ptr numa_high = q_low + input_stride;
    mp_ptr q_high = numa_high + pool_stride;
    mp_ptr qfull = alloc_limbs(3 * MAX_N + 8); /* 完整商 nq+1 + 零扩展余数 na */
    mp_ptr anchor = alloc_limbs(2 * MAX_N + 8); /* 重建锚点需要 na+1 limb */
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    double deployed_scores[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_DIV_DIVIDE_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0, unstable_deployed = 0;

    printf("# program=tune_lmmpn_div_divide\n");
    printf("# parameter=DIV_DIVIDE_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)DIV_DIVIDE_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# candidate_range=%d..%d step=%d (scan upper bound %d) input=na=2nb nb %d..%d step=%d\n",
           CANDIDATE_MIN, CANDIDATE_LAST, CANDIDATE_STEP, CANDIDATE_MAX, MIN_N, MAX_N, STEP_N);
    printf("# high=divide_ with T=n (sub_n=ceil(n/2), root-only); T<=sub_n candidates measured deployed via div_s_\n");
    printf("# DIV_MULINV_L/N keep defaults (nb<%llu never reaches mulinv)\n",
           (unsigned long long)lmmp_tune_DIV_MULINV_L_THRESHOLD);
    lmmp_tune_print_env_comment();
    printf("# nb low_us low_mad high_us high_mad retries stable sub_n\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t nb = MIN_N; nb <= MAX_N; nb += STEP_N) {
        mp_size_t na = 2 * nb;
        fill_random(numa_orig, na, (mp_limb_t)(options.seed + 4 * nb));
        fill_random(numb, nb, (mp_limb_t)(options.seed + 4 * nb + 1));
        mp_limb_t inv21 = lmmp_inv_2_1_(numb[nb - 1], numb[nb - 2]);
        mp_size_t sub_n = (nb + 1) / 2;

        measure_context low_context = {.dstq = q_low,
                                       .numa = numa_low,
                                       .cursor = numa_low,
                                       .numa_orig = numa_orig,
                                       .numb = numb,
                                       .na = na,
                                       .nb = nb,
                                       .inv21 = inv21,
                                       .bind_threshold = saved_threshold};
        measure_context high_context = {.dstq = q_high,
                                        .numa = numa_high,
                                        .cursor = numa_high,
                                        .numa_orig = numa_orig,
                                        .numb = numb,
                                        .na = na,
                                        .nb = nb,
                                        .inv21 = inv21,
                                        .bind_threshold = nb};
        lmmp_copy(numa_low, numa_orig, na);
        lmmp_copy(numa_high, numa_orig, na);
        if (!check_pair(&low_context, &high_context)) {
            fprintf(stderr, "div result mismatch at nb=%llu\n", (unsigned long long)nb);
            result = EXIT_FAILURE;
            break;
        }
        /* 锚点：basecase 结果重建 qh*B^nq*d + q*d + r == a */
        lmmp_copy(numa_low, numa_orig, na);
        mp_limb_t qh_anchor = lmmp_div_basecase_(q_low, numa_low, na, numb, nb, inv21);
        if (!check_anchor(numa_orig, na, numb, nb, qh_anchor, q_low, numa_low, qfull, anchor)) {
            fprintf(stderr, "div anchor mismatch at nb=%llu\n", (unsigned long long)nb);
            result = EXIT_FAILURE;
            break;
        }

        lmmp_tune_bench low_bench = {
            .bind = bind_threshold, .reset_batch = reset_batch, .call = call_basecase, .context = &low_context};
        lmmp_tune_bench high_bench = {
            .bind = bind_threshold, .reset_batch = reset_batch, .call = call_divide, .context = &high_context};
        double low_us, low_mad, high_us, high_mad;
        unsigned retries;
        bool stable;
        if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                    &retries, &stable)) {
            fprintf(stderr, "measurement failed at nb=%llu\n", (unsigned long long)nb);
            result = EXIT_FAILURE;
            break;
        }
        if (stable)
            ++stable_points;
        else
            ++unstable_points;
        printf("%llu %.6f %.6f %.6f %.6f %u %u %llu\n", (unsigned long long)nb, low_us, low_mad, high_us, high_mad,
               retries, stable, (unsigned long long)sub_n);
        /* 只累加步长候选（与 deployed 扫描同域），其余候选无测量不参与推荐 */
        for (int c = CANDIDATE_MIN; c <= CANDIDATE_MAX; c += CANDIDATE_STEP) {
            if ((mp_size_t)c <= sub_n)
                continue;
            scores[c] += nb >= (mp_size_t)c ? high_us : low_us;
            ++counts[c];
        }

        /* deployed 扫描：候选 T<=sub_n 时子问题再入分治，逐候选绑 T 经完整入口实测 */
        for (int candidate = CANDIDATE_MIN; candidate <= (int)sub_n && candidate <= CANDIDATE_MAX;
             candidate += CANDIDATE_STEP) {
            measure_context dep_context = {.dstq = q_high,
                                           .numa = numa_high,
                                           .cursor = numa_high,
                                           .numa_orig = numa_orig,
                                           .numb = numb,
                                           .na = na,
                                           .nb = nb,
                                           .inv21 = inv21,
                                           .bind_threshold = (mp_size_t)candidate};
            lmmp_tune_bench dep_bench = {
                .bind = bind_threshold, .reset_batch = reset_batch, .call = call_deployed, .context = &dep_context};
            double dep_us, dep_mad;
            unsigned dep_retries;
            if (!lmmp_tune_measure(&dep_bench, &options, &dep_us, &dep_mad, &dep_retries)) {
                fprintf(stderr, "deployed measurement failed at nb=%llu T=%d\n", (unsigned long long)nb, candidate);
                result = EXIT_FAILURE;
                break;
            }
            /* 与配对测量同一规则：重试耗尽仍超限的样本保留计分，但计入不稳定并在汇总中提示 */
            if (dep_mad > options.mad_limit)
                ++unstable_deployed;
            deployed_scores[candidate] += dep_us;
            ++counts[candidate];
            printf("# deployed nb=%llu T=%d us=%.6f mad=%.6f retries=%u\n", (unsigned long long)nb, candidate, dep_us,
                   dep_mad, dep_retries);
        }
        if (result != EXIT_SUCCESS)
            break;
    }

    if (result == EXIT_SUCCESS) {
        /* 合并两来源：复用段 (sub_n,n] + deployed 段 T<=sub_n */
        for (int candidate = CANDIDATE_MIN; candidate <= CANDIDATE_MAX; candidate += CANDIDATE_STEP)
            scores[candidate] += deployed_scores[candidate];
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            if (unstable_deployed > 0)
                printf("# note: %u unstable deployed sample(s) retained in scoring\n", unstable_deployed);
            recommend_stepped("DIV_DIVIDE_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP,
                              saved_threshold);
        }
    }
    lmmp_tune_DIV_DIVIDE_THRESHOLD = saved_threshold;
    lmmp_free(qfull);
    lmmp_free(anchor);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
