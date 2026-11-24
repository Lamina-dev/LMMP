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

/* dc ↔ 梅森/费马 CRT-FFT 切换门（平方侧）：SQRLO_DC_THRESHOLD 只出现在根层
   分派（lmmp_mullo_ 的 numa==numb 分支与 powlo 的 lmmp_sqrlo_n_）。低路
   sqrlo_dc_ 不递归自身：全积走平方梯子、交叉项经 mullo_dc_ 按
   MULLO_BASECASE_THRESHOLD 分派；高路 mullo_fft_(a,a) 走梅森平方引擎，
   内部按 MUL_FFT_MODF_THRESHOLD 切分。以上阈值均不绑定 ⟹ 全网格干净，
   sub_n=0，无等价窗、无 deployed 扫描。 */
/* 网格 1024..6144 为批准测量域：dc/fft 成本比在 ~4000..16000 被 FFT 阶梯量化
   锯齿钉在 1 附近（hn=2^k 边界处 dc 病态尖峰），等权评分下大网格会把推荐值
   钳到网格上界；本域内的推荐值应结合逐点比值与更大网格的 badness 回升点解读。 */
enum { MIN_N = 1024, MAX_N = 6144, STEP_N = 32, CANDIDATE_MIN = MIN_N, CANDIDATE_MAX = MAX_N, CANDIDATE_STEP = 1 };

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_ptr tp;
    mp_size_t n;
    mp_size_t threshold;
} measure_context;

/* 两路均直调算法内核，不依赖运行时分派选路；每个测量批次前把阈值恢复为默认值，
   保证递归子问题沿真实部署梯子分派。 */
static void bind_default(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_SQRLO_DC_THRESHOLD = context->threshold;
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
    lmmp_sqrlo_dc_(context->dst, context->numa, context->tp, context->n);
    return fingerprint(context->dst, context->n);
}

/* 高路与部署平方消费方同形：mullo_fft_ 同操作数入参，内部走梅森平方引擎 */
static mp_limb_t call_sqrlo_high(void* opaque) {
    measure_context* context = opaque;
    lmmp_mullo_fft_(context->dst, context->numa, context->numa, context->n, context->tp);
    return fingerprint(context->dst, context->n);
}

static void print_info(void) {
    printf("program=tune_lmmpn_sqrlo_dc\n");
    printf("parameter=SQRLO_DC_THRESHOLD\n");
    printf("consumer=lmmp_mullo_ (numa==numb branch) / lmmp_sqrlo_n_ in powlo\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)SQRLO_DC_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_SQRLO_DC_THRESHOLD);
    printf("candidate_range=%d..%d step=%d\n", CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP);
    printf("input_range=%d..%d step=%d\n", MIN_N, MAX_N, STEP_N);
    printf("forced_root_equivalence=all_n (dc cross term via MULLO_BASECASE, fft via MUL_FFT_MODF; both unbound)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_sqrlo_dc");
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
        lmmp_tune_print_current("SQRLO_DC_THRESHOLD", (unsigned long long)SQRLO_DC_THRESHOLD,
                                (unsigned long long)lmmp_tune_SQRLO_DC_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t input_stride = MAX_N + 4;
    const mp_size_t output_stride = input_stride;
    const mp_size_t tp_stride = 2 * input_stride + 4;
    /* ref2 承载完整平方 [ref2,2n]，容量须为 2*input_stride */
    mp_ptr storage = alloc_limbs(input_stride + 3 * output_stride + 2 * input_stride + tp_stride);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;
    mp_ptr low = storage + input_stride;
    mp_ptr high = low + output_stride;
    mp_ptr ref = high + output_stride;
    mp_ptr ref2 = ref + output_stride;
    mp_ptr tp = ref2 + 2 * input_stride;
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_SQRLO_DC_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_sqrlo_dc\n");
    printf("# parameter=SQRLO_DC_THRESHOLD\n");
    printf("# consumer=lmmp_mullo_ (numa==numb branch) / lmmp_sqrlo_n_ in powlo\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)SQRLO_DC_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# candidate_range=%d..%d step=%d input_range=%d..%d step=%d\n", CANDIDATE_MIN, CANDIDATE_MAX,
           CANDIDATE_STEP, MIN_N, MAX_N, STEP_N);
    printf("# forced_root_equivalence=all_n: threshold only gates root dispatch; dc cross term keeps default\n");
    printf("#   MULLO_BASECASE=%llu, fft internals keep MUL_FFT_MODF (both unbound)\n",
           (unsigned long long)lmmp_tune_MULLO_BASECASE_THRESHOLD);
    printf("# kernels called directly with preallocated tp (2n); deployed entry differs only by one temp alloc\n");
    lmmp_tune_print_env_comment();
    printf("# n low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t n = MIN_N; n <= MAX_N; n += STEP_N) {
        fill_random(numa, n, (mp_limb_t)(options.seed + 4 * n));

        measure_context low_context = {.dst = low, .numa = numa, .tp = tp, .n = n, .threshold = saved_threshold};
        measure_context high_context = {.dst = high, .numa = numa, .tp = tp, .n = n, .threshold = saved_threshold};
        lmmp_sqrlo_dc_(low, numa, tp, n);
        lmmp_mullo_fft_(high, numa, numa, n, tp);
        if (lmmp_cmp_(low, high, n) != 0) {
            fprintf(stderr, "sqrlo result mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }
        /* 恒等互比：sqrlo_dc_(a) == mullo_dc_(a,a)，一般乘法分治为独立参考路 */
        lmmp_mullo_dc_(ref, numa, numa, tp, n);
        if (lmmp_cmp_(low, ref, n) != 0) {
            fprintf(stderr, "sqrlo identity mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }
        /* 全积锚点：低 n limb 必须与 lmmp_sqr_ 完整平方一致（先于计时） */
        lmmp_sqr_(ref2, numa, n);
        if (lmmp_cmp_(low, ref2, n) != 0) {
            fprintf(stderr, "sqrlo full-square anchor mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }

        lmmp_tune_bench low_bench = {
            .bind = bind_default, .reset = reset_output, .call = call_sqrlo_low, .context = &low_context};
        lmmp_tune_bench high_bench = {
            .bind = bind_default, .reset = reset_output, .call = call_sqrlo_high, .context = &high_context};
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
        /* 两路内部都不按被调阈值递归，无同阈值子问题，sub_n=0。不稳定点只留诊断行、
           不参与评分：对该点所有候选同样跳过，各候选计分点集保持一致 */
        if (stable)
            lmmp_tune_score_point(scores, counts, CANDIDATE_MIN, CANDIDATE_MAX, n, 0, low_us, high_us);
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            printf("# note: fft ladder rounding (hn=fft_next_size) sawtooths the crossing band;\n");
            printf("#       flat badness band is a real near-tie region, prefer band intersection across rounds\n");
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) excluded from scoring\n", unstable_points);
            lmmp_tune_recommend("SQRLO_DC_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX,
                                (unsigned long long)saved_threshold);
        }
    }
    lmmp_tune_SQRLO_DC_THRESHOLD = saved_threshold;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
