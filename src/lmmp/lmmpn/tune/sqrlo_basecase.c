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


enum { MIN_N = 8, MAX_N = 256, CANDIDATE_MIN = 8, CANDIDATE_MAX = 256 };

/* sqrlo_dc_ 不递归自身：唯一递归子问题是交叉项 lmmp_mullo_dc_(t)，按 MULLO
   阈值（本程序不绑定，沿架构默认值）分派；全积 lmmp_sqr_(m) 走乘法梯子。
   故 T=n 强制根层对任意 n 都不污染子问题路径——与 mullo 程序不同，本程序
   不存在 forced_root 抬高区间（首版误套用 mullo 的等价窗，已纠正）。 */
enum { EQUIV_MAX_N = MAX_N };

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_ptr tp;
    mp_size_t n;
    mp_size_t threshold;
} measure_context;

/* 低路直调 basecase 叶内核（无递归），批次前恢复默认阈值即可。 */
static void bind_default(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_SQRLO_BASECASE_THRESHOLD = context->threshold;
}

/* 高路经完整 dc 入口以 T=n 强制根层走分治；子问题语义见 EQUIV_MAX_N 注记。 */
static void bind_force_dc(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_SQRLO_BASECASE_THRESHOLD = context->n;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->n);
}

static mp_limb_t fingerprint(mp_srcptr p, mp_size_t n) {
    return p[0] + p[n - 1];
}

static mp_limb_t call_sqrlo_low(void* opaque) {
    measure_context* context = opaque;
    lmmp_sqrlo_basecase_(context->dst, context->numa, context->n);
    return fingerprint(context->dst, context->n);
}

static mp_limb_t call_sqrlo_high(void* opaque) {
    measure_context* context = opaque;
    lmmp_sqrlo_dc_(context->dst, context->numa, context->tp, context->n);
    return fingerprint(context->dst, context->n);
}

static void print_info(void) {
    printf("program=tune_lmmpn_sqrlo_basecase\n");
    printf("parameter=SQRLO_BASECASE_THRESHOLD\n");
    printf("consumer=lmmp_sqrlo_dc_\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)SQRLO_BASECASE_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_SQRLO_BASECASE_THRESHOLD);
    printf("candidate_range=%d..%d\n", CANDIDATE_MIN, CANDIDATE_MAX);
    printf("input_range=%d..%d\n", MIN_N, MAX_N);
    printf("forced_root_equivalence=all_n (sqrlo_dc recurses only via mullo_dc_, unbound here)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_sqrlo_basecase");
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
        lmmp_tune_print_current("SQRLO_BASECASE_THRESHOLD", (unsigned long long)SQRLO_BASECASE_THRESHOLD,
                                (unsigned long long)lmmp_tune_SQRLO_BASECASE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t input_stride = MAX_N + 4;
    const mp_size_t output_stride = input_stride;
    const mp_size_t tp_stride = 2 * input_stride + 4;
    mp_ptr storage = alloc_limbs(input_stride + 3 * output_stride + tp_stride);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;
    mp_ptr low = storage + input_stride;
    mp_ptr high = low + output_stride;
    mp_ptr ref = high + output_stride;
    mp_ptr tp = ref + output_stride;
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_SQRLO_BASECASE_THRESHOLD;

    printf("# program=tune_lmmpn_sqrlo_basecase\n");
    printf("# parameter=SQRLO_BASECASE_THRESHOLD\n");
    printf("# consumer=lmmp_sqrlo_dc_\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)SQRLO_BASECASE_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# candidate_range=%d..%d input_range=%d..%d\n", CANDIDATE_MIN, CANDIDATE_MAX, MIN_N, MAX_N);
    printf("# forced_root_equivalence=all_n: T=n only forces root; subproblem mullo_dc_ keeps default MULLO=%llu\n",
           (unsigned long long)lmmp_tune_MULLO_BASECASE_THRESHOLD);
    lmmp_tune_print_env_comment();
    printf("# n low_us low_mad high_us high_mad retries stable forced_root\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t n = MIN_N; n <= MAX_N; ++n) {
        fill_random(numa, n, (mp_limb_t)(options.seed + 4 * n));

        measure_context low_context = {.dst = low, .numa = numa, .tp = tp, .n = n, .threshold = saved_threshold};
        measure_context high_context = {.dst = high, .numa = numa, .tp = tp, .n = n, .threshold = saved_threshold};
        lmmp_sqrlo_basecase_(low, numa, n);
        lmmp_tune_SQRLO_BASECASE_THRESHOLD = n;
        lmmp_sqrlo_dc_(high, numa, tp, n);
        lmmp_tune_SQRLO_BASECASE_THRESHOLD = saved_threshold;
        if (lmmp_cmp_(low, high, n) != 0) {
            fprintf(stderr, "sqrlo result mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }
        /* 恒等互比：sqrlo(a) == mullo(a,a)，参考路走 mullo basecase 独立实现 */
        lmmp_mullo_basecase_(ref, numa, numa, n);
        if (lmmp_cmp_(low, ref, n) != 0) {
            fprintf(stderr, "sqrlo identity mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }

        lmmp_tune_bench low_bench = {
            .bind = bind_default, .reset = reset_output, .call = call_sqrlo_low, .context = &low_context};
        lmmp_tune_bench high_bench = {
            .bind = bind_force_dc, .reset = reset_output, .call = call_sqrlo_high, .context = &high_context};
        double low_us, low_mad, high_us, high_mad;
        unsigned retries;
        if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                    &retries)) {
            fprintf(stderr, "measurement failed at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }
        unsigned stable = low_mad <= options.mad_limit && high_mad <= options.mad_limit;
        unsigned forced_root = n > EQUIV_MAX_N;
        printf("%llu %.6f %.6f %.6f %.6f %u %u %u\n", (unsigned long long)n, low_us, low_mad, high_us, high_mad, retries,
               stable, forced_root);
        lmmp_tune_score_point(scores, counts, CANDIDATE_MIN, CANDIDATE_MAX, n, low_us, high_us);
    }

    if (result == EXIT_SUCCESS) {
        printf("# note: all high rows are clean root-forced dc (subproblem mullo_dc_ along default ladder);\n");
        printf("#       flat badness band is a real near-tie region, check per-point deltas for crossing evidence\n");
        lmmp_tune_recommend("SQRLO_BASECASE_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX,
                            (unsigned long long)saved_threshold);
    }
    lmmp_tune_SQRLO_BASECASE_THRESHOLD = saved_threshold;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
