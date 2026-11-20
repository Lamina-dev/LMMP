/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
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
#include "tune/kernels.h"
#include "tune/params.h"

/* 候选下界=契约地板：sqr_ 分派 n>=SQR_TOOM22_THRESHOLD 即入 sqr_toom2_，其契约
   要求 na>3，阈值低于 4 会让 n=3 走进违约形状（Release UB）。网格仍下探到
   toom2 契约最小规模 4，支撑小候选的取舍证据。 */
enum { MIN_N = 4, MAX_N = 48, CANDIDATE_MIN = MIN_N, CANDIDATE_MAX = MAX_N + 4 };

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_size_t n;
    mp_size_t threshold;
} measure_context;

/* 两路均直调算法内核，不依赖运行时分派选路；每个测量批次前把阈值恢复为默认值，
   保证递归子问题沿真实部署梯子分派（只强制根层）。 */
static void bind_default(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_SQR_TOOM22_THRESHOLD = context->threshold;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, 2 * context->n);
}

static mp_limb_t fingerprint(mp_srcptr p, mp_size_t n) {
    return p[0] + p[n - 1];
}

/* 递归阈值只强制根层（设计稿 §4.2）：低路直调 hard 分派（n<=19 查表、其余 basecase），
   高路直调 toom2 内核，两路的子问题都沿默认运行时阈值分派。若用 T=n 经完整入口强制
   高路，子问题会被同一边界值压回 basecase（如 n=48 的子问题 24 本应再入 toom2），
   系统性抬高 high 耗时、把交叉点假性推高。 */
static mp_limb_t call_sqr_low(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_sqr_hard_n_(context->dst, context->numa, context->n);
    return fingerprint(context->dst, 2 * context->n);
}

static mp_limb_t call_sqr_high(void* opaque) {
    measure_context* context = opaque;
    lmmp_sqr_toom2_(context->dst, context->numa, context->n);
    return fingerprint(context->dst, 2 * context->n);
}

static void print_info(void) {
    printf("program=tune_lmmpn_sqr_toom22\n");
    printf("parameter=SQR_TOOM22_THRESHOLD\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)SQR_TOOM22_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_SQR_TOOM22_THRESHOLD);
    printf("candidate_range=%d..%d\n", CANDIDATE_MIN, CANDIDATE_MAX);
    printf("input_range=%d..%d\n", MIN_N, MAX_N);
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_sqr_toom22");
        return EXIT_SUCCESS;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    if (options.action == LMMP_TUNE_INFO) {
        print_info();
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current("SQR_TOOM22_THRESHOLD", (unsigned long long)SQR_TOOM22_THRESHOLD,
                                (unsigned long long)lmmp_tune_SQR_TOOM22_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t input_stride = MAX_N + 4;
    const mp_size_t output_stride = 2 * input_stride + 4;
    mp_ptr storage = alloc_limbs(input_stride + 2 * output_stride);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;
    mp_ptr low = storage + input_stride;
    mp_ptr high = low + output_stride;
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_SQR_TOOM22_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_sqr_toom22\n");
    printf("# parameter=SQR_TOOM22_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)SQR_TOOM22_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# candidate_range=%d..%d input_range=%d..%d\n", CANDIDATE_MIN, CANDIDATE_MAX, MIN_N, MAX_N);
    lmmp_tune_print_env_comment();
    printf("# n low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t n = MIN_N; n <= MAX_N; ++n) {
        fill_random(numa, n, (mp_limb_t)(options.seed + 4 * n));

        measure_context low_context = {.dst = low, .numa = numa, .n = n, .threshold = saved_threshold};
        measure_context high_context = {.dst = high, .numa = numa, .n = n, .threshold = saved_threshold};
        lmmp_tune_sqr_hard_n_(low, numa, n);
        lmmp_sqr_toom2_(high, numa, n);
        if (lmmp_cmp_(low, high, 2 * n) != 0) {
            fprintf(stderr, "sqr result mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }

        lmmp_tune_bench low_bench = {
            .bind = bind_default, .reset = reset_output, .call = call_sqr_low, .context = &low_context};
        lmmp_tune_bench high_bench = {
            .bind = bind_default, .reset = reset_output, .call = call_sqr_high, .context = &high_context};
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
        printf("%llu %.6f %.6f %.6f %.6f %u %u\n", (unsigned long long)n, low_us, low_mad, high_us, high_mad, retries,
               stable);
        lmmp_tune_score_point(scores, counts, CANDIDATE_MIN, CANDIDATE_MAX, n, 0, low_us, high_us);
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            lmmp_tune_recommend("SQR_TOOM22_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX,
                                (unsigned long long)saved_threshold);
        }
    }
    lmmp_tune_SQR_TOOM22_THRESHOLD = saved_threshold;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
