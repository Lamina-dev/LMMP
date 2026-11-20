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

/* 候选下界=分派地板：mul_ 档2 的 toom42 分支靠档1 nb<MUL_TOOM22_THRESHOLD 兜住
   toom42 契约门 nb>=20，阈值低于 20 会让 (35,19) 类形状违约（Release UB）。
   若将来要支持更小的平衡乘法阈值，须先把该耦合与不平衡分派条件分离。
   扫描起点 5：toom22 契约 2*nb>na+4 在平衡形状上要求 n>=5。 */
enum { MIN_N = 5, MAX_N = 48, CANDIDATE_MIN = 20, CANDIDATE_MAX = MAX_N + 4 };

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
    lmmp_tune_MUL_TOOM22_THRESHOLD = context->threshold;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, 2 * context->n);
}

static mp_limb_t fingerprint(mp_srcptr p, mp_size_t n) {
    return p[0] + p[n - 1];
}

/* 递归阈值只强制根层（设计稿 §4.2）：低路直调 hard 分派（n<=19 查表、其余 basecase），
   高路直调 toom22 内核，两路的子问题都沿默认运行时阈值分派。若用 T=n 经完整入口强制
   高路，子问题会被同一边界值压回 basecase（如 n=48 的子问题 24 本应再入 toom22），
   系统性抬高 high 耗时、把交叉点假性推高。
   子问题经宏首分支按同一边界值（MUL_TOOM22_THRESHOLD）分派，根层两级子乘规模为
   ceil(n/2)（v0/vm1）与 floor(n/2)（vinf）：候选 T 与默认阈值分居任一级两侧时，
   部署子路径（递归↔hard）与测量路径不同，此类候选逐个按 T 绑定实测部署路径；
   更深层节点 <=MAX_N/4=12 恒低于候选与默认阈值下界 20，两级判据即全路径等价。 */
static mp_limb_t call_mul_low(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_mul_hard_n_(context->dst, context->numa, context->numb, context->n);
    return fingerprint(context->dst, 2 * context->n);
}

static mp_limb_t call_mul_high(void* opaque) {
    measure_context* context = opaque;
    lmmp_mul_toom22_(context->dst, context->numa, context->n, context->numb, context->n);
    return fingerprint(context->dst, 2 * context->n);
}

static void print_info(void) {
    printf("program=tune_lmmpn_mul_toom22\n");
    printf("parameter=MUL_TOOM22_THRESHOLD\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)MUL_TOOM22_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_MUL_TOOM22_THRESHOLD);
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
        lmmp_tune_usage("tune_lmmpn_mul_toom22");
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
        lmmp_tune_print_current("MUL_TOOM22_THRESHOLD", (unsigned long long)MUL_TOOM22_THRESHOLD,
                                (unsigned long long)lmmp_tune_MUL_TOOM22_THRESHOLD);
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
    mp_size_t saved_threshold = lmmp_tune_MUL_TOOM22_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_mul_toom22\n");
    printf("# parameter=MUL_TOOM22_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)MUL_TOOM22_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# candidate_range=%d..%d input_range=%d..%d\n", CANDIDATE_MIN, CANDIDATE_MAX, MIN_N, MAX_N);
    lmmp_tune_print_env_comment();
    printf("# n low_us low_mad high_us high_mad retries stable\n");
    printf("# deployed n candidate us mad retries stable  (candidates whose deployed subproblems reroute)\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t n = MIN_N; n <= MAX_N; ++n) {
        fill_random(numa, n, (mp_limb_t)(options.seed + 4 * n));
        fill_random(numb, n, (mp_limb_t)(options.seed + 4 * n + 1));

        measure_context low_context = {.dst = low, .numa = numa, .numb = numb, .n = n, .threshold = saved_threshold};
        measure_context high_context = {.dst = high, .numa = numa, .numb = numb, .n = n, .threshold = saved_threshold};
        lmmp_tune_mul_hard_n_(low, numa, numb, n);
        lmmp_mul_toom22_(high, numa, n, numb, n);
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
        /* 复用规则：部署子路径与本行高路测量一致的候选直接复用（子问题同走递归或
           同走 hard），其余候选逐个按 T 绑定实测部署路径。根层两级子乘规模为
           ceil(n/2)（v0/vm1）与 floor(n/2)（vinf），更深节点 <=MAX_N/4=12 恒低于
           候选与默认阈值下界，无第三级跨越。 */
        mp_size_t sub_n = n - (n >> 1);
        mp_size_t deploy_max = n < (mp_size_t)CANDIDATE_MAX ? n : (mp_size_t)CANDIDATE_MAX;
        mp_size_t deploy_lo, deploy_hi;
        if (sub_n < saved_threshold) {
            /* 测量两级子乘走 hard：T<=sub_n 的候选部署时至少 ceil 半递归，
               score_point 跳过，须实测。 */
            lmmp_tune_score_point(scores, counts, CANDIDATE_MIN, CANDIDATE_MAX, n, sub_n, low_us, high_us);
            deploy_lo = CANDIDATE_MIN;
            deploy_hi = sub_n < deploy_max ? sub_n : deploy_max;
        } else {
            /* 测量两级子乘走递归：T<=n>>1 时部署两级同走递归，复用 high_us；
               T>n>>1 的候选会把 floor 半子乘（奇 n 在 T=ceil(n/2) 处率先翻回
               hard）或两级一并压回，须实测。 */
            for (int candidate = CANDIDATE_MIN; candidate <= CANDIDATE_MAX; ++candidate) {
                if ((mp_size_t)candidate > n) {
                    scores[candidate] += low_us;
                    ++counts[candidate];
                } else if ((mp_size_t)candidate <= (n >> 1)) {
                    scores[candidate] += high_us;
                    ++counts[candidate];
                }
            }
            deploy_lo = (n >> 1) + 1;
            deploy_hi = deploy_max;
        }
        for (mp_size_t candidate = deploy_lo; candidate <= deploy_hi; ++candidate) {
            measure_context deploy_context = {
                .dst = high, .numa = numa, .numb = numb, .n = n, .threshold = candidate};
            lmmp_tune_MUL_TOOM22_THRESHOLD = candidate;
            lmmp_mul_toom22_(high, numa, n, numb, n);
            lmmp_tune_MUL_TOOM22_THRESHOLD = saved_threshold;
            if (lmmp_cmp_(low, high, 2 * n) != 0) {
                fprintf(stderr, "mul deployed mismatch at n=%llu candidate=%llu\n", (unsigned long long)n,
                        (unsigned long long)candidate);
                result = EXIT_FAILURE;
                break;
            }
            lmmp_tune_bench deploy_bench = {
                .bind = bind_default, .reset = reset_output, .call = call_mul_high, .context = &deploy_context};
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
            printf("# note: candidates whose deployed subproblems reroute are measured on their bound path\n");
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            lmmp_tune_recommend("MUL_TOOM22_THRESHOLD", scores, counts, CANDIDATE_MIN, CANDIDATE_MAX,
                                (unsigned long long)saved_threshold);
        }
    }
    lmmp_tune_MUL_TOOM22_THRESHOLD = saved_threshold;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
