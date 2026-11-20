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

enum { MIN_N = 8, MAX_N = 128, CANDIDATE_MIN = 8, CANDIDATE_MAX = 128 };

/* mullo_dc_ 递归自身：T=n 强制根层后，子问题 t=n-m 若仍 >= 默认阈值，本应
   再入 dc 却被同一边界值压回 basecase，高路耗时被抬高（forced_root 注记）。
   等价窗边界随默认阈值变化，运行时按根层分块公式计算，勿硬编码。
   subproblem_n_ 镜像 lmmp_mullo_dc_ 的根层分块（仅用于注记，不参与测量）。 */
static mp_size_t subproblem_n_(mp_size_t n) {
    mp_size_t m;
    if (n < MUL_TOOM33_THRESHOLD) {
        m = 25 * n / 36;
    } else if (n < MUL_TOOM44_THRESHOLD) {
        m = 31 * n / 40;
    } else if (n < MUL_TOOM6H_THRESHOLD) {
        m = 32 * n / 39;
    } else if (n < MUL_TOOM8H_THRESHOLD) {
        m = 27 * n / 31;
    } else {
        m = 9 * n / 10;
    }
    return n - m;
}

/* 网格内最大的干净强制规模（连续等价前缀的上端）：子问题 t < 默认阈值的最后一个 n。
   前缀之外的行因根层分块档位切换可能 individually 干净，由逐行 forced_root 判定。 */
static mp_size_t equiv_max_n_(mp_size_t default_threshold) {
    for (mp_size_t n = MIN_N; n <= MAX_N; ++n) {
        if (subproblem_n_(n) >= default_threshold)
            return n - 1;
    }
    return MAX_N;
}

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_srcptr numb;
    mp_ptr tp;
    mp_size_t n;
    mp_size_t threshold;
    mp_size_t force;
} measure_context;

/* 低路直调 basecase 叶内核（无递归），批次前恢复默认阈值即可。 */
static void bind_default(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_MULLO_BASECASE_THRESHOLD = context->threshold;
}

/* 高路经完整 dc 入口以 T=n 强制根层走分治；子问题语义见 EQUIV_MAX_N 注记。 */
static void bind_force_dc(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_MULLO_BASECASE_THRESHOLD = context->n;
}

/* 部署路径实测：按候选阈值 T 绑定后走完整 dc 入口（根层与深层子问题一致按 T 分派）。 */
static void bind_force_candidate(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_MULLO_BASECASE_THRESHOLD = context->force;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->n);
}

static mp_limb_t fingerprint(mp_srcptr p, mp_size_t n) {
    return p[0] + p[n - 1];
}

static mp_limb_t call_mullo_low(void* opaque) {
    measure_context* context = opaque;
    lmmp_mullo_basecase_(context->dst, context->numa, context->numb, context->n);
    return fingerprint(context->dst, context->n);
}

static mp_limb_t call_mullo_high(void* opaque) {
    measure_context* context = opaque;
    lmmp_mullo_dc_(context->dst, context->numa, context->numb, context->tp, context->n);
    return fingerprint(context->dst, context->n);
}

static void print_info(void) {
    printf("program=tune_lmmpn_mullo_basecase\n");
    printf("parameter=MULLO_BASECASE_THRESHOLD\n");
    printf("consumer=lmmp_mullo_dc_\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)MULLO_BASECASE_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_MULLO_BASECASE_THRESHOLD);
    printf("candidate_range=%d..%d\n", CANDIDATE_MIN, CANDIDATE_MAX);
    printf("input_range=%d..%d\n", MIN_N, MAX_N);
    printf("forced_root_equivalence=n<=%llu\n",
           (unsigned long long)equiv_max_n_((mp_size_t)MULLO_BASECASE_THRESHOLD));
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_mullo_basecase");
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
        lmmp_tune_print_current("MULLO_BASECASE_THRESHOLD", (unsigned long long)MULLO_BASECASE_THRESHOLD,
                                (unsigned long long)lmmp_tune_MULLO_BASECASE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t input_stride = MAX_N + 4;
    const mp_size_t output_stride = input_stride;
    const mp_size_t tp_stride = 2 * input_stride + 4;
    mp_ptr storage = alloc_limbs(2 * input_stride + 2 * output_stride + tp_stride);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;
    mp_ptr numb = storage + input_stride;
    mp_ptr low = storage + 2 * input_stride;
    mp_ptr high = low + output_stride;
    mp_ptr tp = high + output_stride;
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    mp_size_t saved_threshold = lmmp_tune_MULLO_BASECASE_THRESHOLD;
    const mp_size_t equiv_max_n = equiv_max_n_(saved_threshold);

    printf("# program=tune_lmmpn_mullo_basecase\n");
    printf("# parameter=MULLO_BASECASE_THRESHOLD\n");
    printf("# consumer=lmmp_mullo_dc_\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)MULLO_BASECASE_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# candidate_range=%d..%d input_range=%d..%d\n", CANDIDATE_MIN, CANDIDATE_MAX, MIN_N, MAX_N);
    printf("# forced_root_equivalence_prefix=n<=%llu (default=%llu); per-row flag compares subproblem vs default\n",
           (unsigned long long)equiv_max_n, (unsigned long long)saved_threshold);
    lmmp_tune_print_env_comment();
    printf("# n low_us low_mad high_us high_mad retries stable forced_root\n");
    printf("# deployed n candidate us mad retries stable  (candidates whose deployed path recurses)\n");

    int result = EXIT_SUCCESS;
    unsigned stable_points = 0, unstable_points = 0;
    for (mp_size_t n = MIN_N; n <= MAX_N; ++n) {
        fill_random(numa, n, (mp_limb_t)(options.seed + 4 * n));
        fill_random(numb, n, (mp_limb_t)(options.seed + 4 * n + 1));

        measure_context low_context = {
            .dst = low, .numa = numa, .numb = numb, .tp = tp, .n = n, .threshold = saved_threshold};
        measure_context high_context = {
            .dst = high, .numa = numa, .numb = numb, .tp = tp, .n = n, .threshold = saved_threshold};
        lmmp_mullo_basecase_(low, numa, numb, n);
        lmmp_tune_MULLO_BASECASE_THRESHOLD = n;
        lmmp_mullo_dc_(high, numa, numb, tp, n);
        lmmp_tune_MULLO_BASECASE_THRESHOLD = saved_threshold;
        if (lmmp_cmp_(low, high, n) != 0) {
            fprintf(stderr, "mullo result mismatch at n=%llu\n", (unsigned long long)n);
            result = EXIT_FAILURE;
            break;
        }

        lmmp_tune_bench low_bench = {
            .bind = bind_default, .reset = reset_output, .call = call_mullo_low, .context = &low_context};
        lmmp_tune_bench high_bench = {
            .bind = bind_force_dc, .reset = reset_output, .call = call_mullo_high, .context = &high_context};
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
        /* 逐行判定：该行子问题按默认阈值是否会改走分治（前缀界只是连续干净段的上端） */
        mp_size_t sub_n = subproblem_n_(n);
        unsigned forced_root = sub_n >= saved_threshold;
        printf("%llu %.6f %.6f %.6f %.6f %u %u %u\n", (unsigned long long)n, low_us, low_mad, high_us, high_mad, retries,
               stable, forced_root);
        /* 复用规则：候选 T>n 计低路；sub_n<T<=n 时部署路径=根层分治+子问题 basecase，
           与本行高路测量一致，复用 high_us；T<=sub_n 时部署会让子问题再入分治，
           路径不同，逐候选按 T 绑定实测部署路径。 */
        lmmp_tune_score_point(scores, counts, CANDIDATE_MIN, CANDIDATE_MAX, n, sub_n, low_us, high_us);
        for (mp_size_t candidate = CANDIDATE_MIN; candidate <= CANDIDATE_MAX && candidate <= n && candidate <= sub_n;
             ++candidate) {
            measure_context deploy_context = {
                .dst = high, .numa = numa, .numb = numb, .tp = tp, .n = n, .force = candidate};
            lmmp_tune_MULLO_BASECASE_THRESHOLD = candidate;
            lmmp_mullo_dc_(high, numa, numb, tp, n);
            lmmp_tune_MULLO_BASECASE_THRESHOLD = saved_threshold;
            if (lmmp_cmp_(low, high, n) != 0) {
                fprintf(stderr, "mullo deployed mismatch at n=%llu candidate=%llu\n", (unsigned long long)n,
                        (unsigned long long)candidate);
                result = EXIT_FAILURE;
                break;
            }
            lmmp_tune_bench deploy_bench = {
                .bind = bind_force_candidate, .reset = reset_output, .call = call_mullo_high,
                .context = &deploy_context};
            double dep_us, dep_mad;
            unsigned dep_retries;
            if (!lmmp_tune_measure(&deploy_bench, &options, &dep_us, &dep_mad, &dep_retries)) {
                fprintf(stderr, "deployed measurement failed at n=%llu candidate=%llu\n", (unsigned long long)n,
                        (unsigned long long)candidate);
                result = EXIT_FAILURE;
                break;
            }
            bool dep_stable = dep_mad <= options.mad_limit;
            if (dep_stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("# deployed %llu %llu %.6f %.6f %u %u\n", (unsigned long long)n, (unsigned long long)candidate,
                   dep_us, dep_mad, dep_retries, dep_stable);
            scores[candidate] += dep_us;
            ++counts[candidate];
        }
        if (result != EXIT_SUCCESS)
            break;
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            printf("# note: candidates at or below a row's subproblem size are measured on their deployed\n");
            printf("#       recursive path (dc bound to the candidate); reused high rows cover the rest\n");
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            lmmp_tune_recommend("MULLO_BASECASE_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX,
                                (unsigned long long)saved_threshold);
        }
    }
    lmmp_tune_MULLO_BASECASE_THRESHOLD = saved_threshold;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
