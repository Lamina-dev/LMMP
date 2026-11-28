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
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

/* binvert_mulhi_（unbalance 的 a^-1 高半积提取）阈值：n=na 低于
   MULHI_MERSENNE_THRESHOLD 时走 lmmp_mul_n_ 直乘取高半，否则走梅森分发
   （m=mersenne_next_size(2na)，梅森折叠积直接给出高半）。每次 unbalance 调用恰
   读一次、无递归 ⟹ 强制绑定全网格干净、sub_n=0。两路经完整 lmmp_binvert_
   入口配对测量（根分派绑 BINVERT_UNBALANCE_SLOPE_THRESHOLD=1 固定走
   unbalance；binvert_n_dc_(na) 前缀与分块乘积对两路对称共同计时）。形状取
   n=3na（单块，mulhi 占比最大）。恒等式 oracle a*x == 1 (mod B^n) 同
   tune_numth_binvert_modm。MODM/CACHE 与 MULLO_DC 保持默认不绑定。 */
enum {
    MIN_NA = 32, MAX_NA = 4096, STEP_NA = 32,
    CANDIDATE_MIN = MIN_NA, CANDIDATE_MAX = MAX_NA, CANDIDATE_STEP = 1,
    SHAPE_K = 3 /* n = SHAPE_K * na */
};

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_size_t na;
    mp_size_t n;
} measure_context;

static void bind_low(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD = 1;
    lmmp_tune_MULHI_MERSENNE_THRESHOLD = context->na + 1; /* na < T：mul_n 直乘 */
}

static void bind_high(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD = 1;
    lmmp_tune_MULHI_MERSENNE_THRESHOLD = context->na; /* na >= T：梅森分发 */
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->n);
}

static mp_limb_t call_entry(void* opaque) {
    measure_context* context = opaque;
    lmmp_binvert_(context->dst, context->numa, context->na, context->n);
    return context->dst[0] ^ context->dst[context->n - 1];
}

/* 恒等式 oracle：a 补零到 n 后与 x 全乘，a*x == 1 (mod B^n) */
static int check_inverse(mp_srcptr numa, mp_size_t na, mp_srcptr x, mp_size_t n, mp_ptr pad, mp_ptr prod) {
    lmmp_copy(pad, numa, na);
    lmmp_zero(pad + na, n - na);
    lmmp_mul_(prod, pad, n, x, n);
    if (prod[0] != 1)
        return 0;
    for (mp_size_t i = 1; i < n; ++i)
        if (prod[i] != 0)
            return 0;
    return 1;
}

static void print_info(void) {
    printf("program=tune_numth_binvert_mulhi\n");
    printf("parameter=MULHI_MERSENNE_THRESHOLD (binvert_mulhi_ high-half product)\n");
    printf("consumer=lmmp_binvert_unbalance_ via lmmp_binvert_ unbalance rung\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)MULHI_MERSENNE_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_MULHI_MERSENNE_THRESHOLD);
    printf("input=na %d..%d step %d x shape n=%dna\n", MIN_NA, MAX_NA, STEP_NA, SHAPE_K);
    printf("candidate_range=%d..%d step=%d\n", CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP);
    printf("forced_root_equivalence=all_n (mulhi read once per unbalance call, no recursion)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_binvert_mulhi");
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
        lmmp_tune_print_current("MULHI_MERSENNE_THRESHOLD", (unsigned long long)MULHI_MERSENNE_THRESHOLD,
                                (unsigned long long)lmmp_tune_MULHI_MERSENNE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t max_n = SHAPE_K * MAX_NA;
    mp_ptr storage = alloc_limbs(MAX_NA + 4 + 3 * (max_n + 4) + 2 * max_n + 4);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;                 /* [0, na) 奇底数 */
    mp_ptr low = storage + MAX_NA + 4;     /* [0, n) 低路逆元 */
    mp_ptr high = low + max_n + 4;         /* [0, n) 高路逆元 */
    mp_ptr pad = high + max_n + 4;         /* 恒等式校验：底数补零到 n */
    mp_ptr prod = pad + max_n + 4;         /* 恒等式校验：全积 2n */
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    const mp_size_t saved_mulhi = lmmp_tune_MULHI_MERSENNE_THRESHOLD;
    const mp_size_t saved_slope = lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_numth_binvert_mulhi\n");
    printf("# parameter=MULHI_MERSENNE_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)MULHI_MERSENNE_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# low=mul_n direct high=mersenne dispatch; both via full lmmp_binvert_ entry, dispatch pinned to unbalance\n");
    printf("# MODM/CACHE/MULLO_DC keep defaults (symmetric or inactive at these sizes)\n");
    lmmp_tune_print_env_comment();
    printf("# na n low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t na = MIN_NA; na <= MAX_NA && result == EXIT_SUCCESS; na += STEP_NA) {
        const mp_size_t n = SHAPE_K * na;

        fill_random(numa, na, (mp_limb_t)(options.seed + 8 * n + na));
        numa[0] |= 1;

        /* 互比与恒等式：两路强制绑定各跑一次 */
        measure_context check_context = {.dst = low, .numa = numa, .na = na, .n = n};
        bind_low(&check_context);
        lmmp_zero(low, n);
        lmmp_binvert_(low, numa, na, n);
        bind_high(&check_context);
        check_context.dst = high;
        lmmp_zero(high, n);
        lmmp_binvert_(high, numa, na, n);
        if (lmmp_cmp_(low, high, n) != 0 || !check_inverse(numa, na, low, n, pad, prod)) {
            fprintf(stderr, "inverse identity mismatch at na=%llu\n", (unsigned long long)na);
            result = EXIT_FAILURE;
            break;
        }

        measure_context ctx = {.dst = low, .numa = numa, .na = na, .n = n};
        lmmp_tune_bench low_bench = {
            .bind = bind_low, .reset = reset_output, .call = call_entry, .context = &ctx};
        lmmp_tune_bench high_bench = {
            .bind = bind_high, .reset = reset_output, .call = call_entry, .context = &ctx};
        double low_us, low_mad, high_us, high_mad;
        unsigned retries;
        bool stable;
        if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                    &retries, &stable)) {
            fprintf(stderr, "measurement failed at na=%llu\n", (unsigned long long)na);
            result = EXIT_FAILURE;
            break;
        }
        if (stable)
            ++stable_points;
        else
            ++unstable_points;
        printf("%llu %llu %.6f %.6f %.6f %.6f %u %u\n", (unsigned long long)na, (unsigned long long)n, low_us,
               low_mad, high_us, high_mad, retries, stable);
        lmmp_tune_score_point(scores, counts, CANDIDATE_MIN, CANDIDATE_MAX, na, 0, low_us, high_us);
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            lmmp_tune_recommend("MULHI_MERSENNE_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX,
                                (unsigned long long)saved_mulhi);
        }
    }
    lmmp_tune_MULHI_MERSENNE_THRESHOLD = saved_mulhi;
    lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD = saved_slope;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
