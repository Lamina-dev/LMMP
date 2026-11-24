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
#include "tune/params.h"

/* 梯子顶端交叉：只测 toom8h ↔ FFT 一个边界。toom 梯子的子问题规模单调收缩
   且跨层坠落（8h 子问题 ~n/8 落入 6h 及以下，6h 子问题 ~n/6 落入更低阶），
   各层 toom 阈值保持默认、沿真实递归梯子被顺带覆盖，无需逐层设程序。
   网格与候选同域 512..5400、步长 8：下端远高于 toom8h 契约 nb>=79 与分派
   地板，候选低于现值时高路 FFT 测量仍代表部署路径（见下）。 */
enum { MIN_N = 512, MAX_N = 5400, STEP_N = 8, CANDIDATE_MIN = MIN_N, CANDIDATE_MAX = MAX_N, CANDIDATE_STEP = 1 };

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_srcptr numb;
    mp_size_t n;
    mp_size_t threshold;
} measure_context;

/* 两路均直调算法内核，不依赖运行时分派选路；每个测量批次前把阈值恢复为默认值，
   保证递归子问题沿真实部署梯子分派（只强制根层）。 */
static void bind_default(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_MUL_FFT_THRESHOLD = context->threshold;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, 2 * context->n);
}

static mp_limb_t fingerprint(mp_srcptr p, mp_size_t n) {
    return p[0] + p[n - 1];
}

/* 强制路径全网格干净（forced_root_equivalence=all_n）：FFT 引擎内部递归只按
   MUL_FFT_MODF_THRESHOLD（本程序不绑定）切分，点乘消费 lmmp_mul_n_ 时子问题
   规模远小于被调阈值，T=n 不会改写 FFT 内部任何决策；低路 toom8h 的自递归
   阈值 MUL_TOOM8H_THRESHOLD 同样不绑定，子问题恒沿默认梯子。故无需等价窗，
   也无需 deployed 扫描：sub_n=0，全部候选直接复用本行两路测量。 */
static mp_limb_t call_mul_low(void* opaque) {
    measure_context* context = opaque;
    lmmp_mul_toom8h_(context->dst, context->numa, context->n, context->numb, context->n);
    return fingerprint(context->dst, 2 * context->n);
}

static mp_limb_t call_mul_high(void* opaque) {
    measure_context* context = opaque;
    lmmp_mul_fft_(context->dst, context->numa, context->n, context->numb, context->n);
    return fingerprint(context->dst, 2 * context->n);
}

static void print_info(void) {
    printf("program=tune_lmmpn_mul_fft\n");
    printf("parameter=MUL_FFT_THRESHOLD\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)MUL_FFT_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_MUL_FFT_THRESHOLD);
    printf("candidate_range=%d..%d step=%d\n", CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP);
    printf("input_range=%d..%d step=%d\n", MIN_N, MAX_N, STEP_N);
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_mul_fft");
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
        lmmp_tune_print_current("MUL_FFT_THRESHOLD", (unsigned long long)MUL_FFT_THRESHOLD,
                                (unsigned long long)lmmp_tune_MUL_FFT_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t input_stride = MAX_N + 4;
    const mp_size_t output_stride = 2 * input_stride + 4;
    mp_ptr storage = alloc_limbs(2 * input_stride + 2 * output_stride);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;
    mp_ptr numb = storage + input_stride;
    mp_ptr low = storage + 2 * input_stride;
    mp_ptr high = low + output_stride;
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_MUL_FFT_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_mul_fft\n");
    printf("# parameter=MUL_FFT_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)MUL_FFT_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# candidate_range=%d..%d step=%d input_range=%d..%d step=%d\n", CANDIDATE_MIN, CANDIDATE_MAX,
           CANDIDATE_STEP, MIN_N, MAX_N, STEP_N);
    printf("# forced_root_equivalence=all_n (fft recursion keyed by MUL_FFT_MODF_THRESHOLD, unbound)\n");
    lmmp_tune_print_env_comment();
    printf("# n low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t n = MIN_N; n <= MAX_N; n += STEP_N) {
        fill_random(numa, n, (mp_limb_t)(options.seed + 4 * n));
        fill_random(numb, n, (mp_limb_t)(options.seed + 4 * n + 1));

        measure_context low_context = {.dst = low, .numa = numa, .numb = numb, .n = n, .threshold = saved_threshold};
        measure_context high_context = {.dst = high, .numa = numa, .numb = numb, .n = n, .threshold = saved_threshold};
        lmmp_mul_toom8h_(low, numa, n, numb, n);
        lmmp_mul_fft_(high, numa, n, numb, n);
        if (lmmp_cmp_(low, high, 2 * n) != 0) {
            fprintf(stderr, "mul result mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }

        lmmp_tune_bench low_bench = {
            .bind = bind_default, .reset = reset_output, .call = call_mul_low, .context = &low_context};
        lmmp_tune_bench high_bench = {
            .bind = bind_default, .reset = reset_output, .call = call_mul_high, .context = &high_context};
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
            lmmp_tune_recommend("MUL_FFT_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX,
                                (unsigned long long)saved_threshold);
        }
    }
    lmmp_tune_MUL_FFT_THRESHOLD = saved_threshold;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
