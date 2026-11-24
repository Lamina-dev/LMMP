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
#include "lmmp/numth.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

/* divexact mu 主循环块积 dp×qp 的三档梯子（截断窗形态，窗口 ~dn+1）：
   A=plain 直乘（dn<DIVEXACT_MERSENNE_THRESHOLD 且 dn<DIVEXACT_MERSENNE_CACHE_THRESHOLD）
   B=梅森分发不缓存（DIVEXACT_MERSENNE_THRESHOLD<=dn<DIVEXACT_MERSENNE_CACHE_THRESHOLD，
     m=mersenne_next_size(dn+1)）
   C=fft 缓存（dn>=DIVEXACT_MERSENNE_CACHE_THRESHOLD，m=fft_next_size(dn+1)，
     预旋转 D' 变换跨块复用）
   档位在 lmmp_divexact_mu_run_ 入口读一次、无自递归 ⟹ 绑定全网格干净、
   sub_n=0。截断窗 m>=dn+1 经 Hensel 已知低段恢复高 dn limb（推导见
   numth/divexact.c 的 mu_run_ 头注释），B/C 可行性无条件。每点经完整
   lmmp_divexact_ 入口三路
   测量（根门绑 DIVEXACT_BASECASE_THRESHOLD=1 结构化跳过 basecase；
   binvert(dinv) 与块内 mullo 三路共同、对称计时）；输入构造与 oracle 同
   tune_numth_divexact。评分：DIVEXACT_MERSENNE 轴配 DIVEXACT_MERSENNE_CACHE
   默认、后者轴配前者默认各一维 + 按部署选择式
   dn>=c ? C : (dn>=m ? B : A) 求两阈值的二维联合 argmin。
   MULLO_DC（use_lcache 的 in 门）不绑定：网格 in<=3dn/4+1<默认值 ⟹ mullo
   缓存恒不启用，与部署默认一致；MUL/SQR_MERSENNE_* 内部门不绑定。 */
enum {
    MIN_DN = 128, MAX_DN = 4096, STEP_DN = 64, SHAPE_COUNT = 2,
    MRS_CAND_MIN = 64, MRS_CAND_MAX = 4096, MRS_CAND_STEP = 64,
    CACHE_CAND_MIN = 64, CACHE_CAND_MAX = 4096, CACHE_CAND_STEP = 64,
    MAX_NN = 4 * MAX_DN,
    MAX_POINTS = SHAPE_COUNT * ((MAX_DN - MIN_DN) / STEP_DN + 1)
};

typedef struct {
    mp_ptr dst;
    mp_srcptr np;
    mp_srcptr dp;
    mp_size_t nn;
    mp_size_t dn;
    mp_size_t bind_mrs;
    mp_size_t bind_cache;
} measure_context;

static void bind_thresholds(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_DIVEXACT_BASECASE_THRESHOLD = 1;
    lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD = context->bind_mrs;
    lmmp_tune_DIVEXACT_MERSENNE_CACHE_THRESHOLD = context->bind_cache;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->nn - context->dn + 2);
}

static mp_limb_t call_entry(void* opaque) {
    measure_context* context = opaque;
    lmmp_divexact_(context->dst, context->np, context->nn, context->dp, context->dn);
    return context->dst[0] ^ context->dst[context->nn - context->dn] ^ context->np[0];
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
    while (flat_low - cstep >= cmin && counts[flat_low - cstep] &&
           scores[flat_low - cstep] / counts[flat_low - cstep] <= best_score * 1.01)
        flat_low -= cstep;
    while (flat_high + cstep <= cmax && counts[flat_high + cstep] &&
           scores[flat_high + cstep] / counts[flat_high + cstep] <= best_score * 1.01)
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

/* 按绑定强制跑完整入口并与构造商 q 互比（oracle 锚点） */
static int check_rung(measure_context* context, mp_srcptr q, mp_size_t qa, mp_size_t bind_mrs, mp_size_t bind_cache) {
    context->bind_mrs = bind_mrs;
    context->bind_cache = bind_cache;
    bind_thresholds(context);
    lmmp_zero(context->dst, context->nn - context->dn + 2);
    lmmp_divexact_(context->dst, context->np, context->nn, context->dp, context->dn);
    return lmmp_cmp_(context->dst, q, qa) == 0 && context->dst[qa] == 0;
}

static void print_info(void) {
    printf("program=tune_numth_divexact_modm\n");
    printf("parameters=DIVEXACT_MERSENNE_THRESHOLD,DIVEXACT_MERSENNE_CACHE_THRESHOLD (mu block-product ladder)\n");
    printf("consumer=lmmp_divexact_mu_run_ dp*qp block loop (via lmmp_divexact_ unbalance rung)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default_mrs=%llu default_cache=%llu\n", (unsigned long long)DIVEXACT_MERSENNE_THRESHOLD,
           (unsigned long long)DIVEXACT_MERSENNE_CACHE_THRESHOLD);
    printf("current_mrs=%llu current_cache=%llu\n", (unsigned long long)lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD,
           (unsigned long long)lmmp_tune_DIVEXACT_MERSENNE_CACHE_THRESHOLD);
    printf("input=dn %d..%d step %d x shapes{nn=2dn (2 blocks), nn=4dn (4 blocks)}\n", MIN_DN, MAX_DN, STEP_DN);
    printf("mrs_candidates=%d..%d step=%d; cache_candidates=%d..%d step=%d\n", MRS_CAND_MIN, MRS_CAND_MAX,
           MRS_CAND_STEP, CACHE_CAND_MIN, CACHE_CAND_MAX, CACHE_CAND_STEP);
    printf("forced_root_equivalence=all_n (ladder read once per mu_run_ entry, no recursion)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_divexact_modm");
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
        lmmp_tune_print_current("DIVEXACT_MERSENNE_THRESHOLD", (unsigned long long)DIVEXACT_MERSENNE_THRESHOLD,
                                (unsigned long long)lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD);
        lmmp_tune_print_current("DIVEXACT_MERSENNE_CACHE_THRESHOLD",
                                (unsigned long long)DIVEXACT_MERSENNE_CACHE_THRESHOLD,
                                (unsigned long long)lmmp_tune_DIVEXACT_MERSENNE_CACHE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t stride = MAX_NN + 4;
    mp_ptr storage = alloc_limbs(3 * stride + MAX_DN + 4);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr np_orig = storage;        /* [0, nn) 构造被除数 d*q */
    mp_ptr q_ref = storage + stride; /* [0, qa) 构造商 */
    mp_ptr d_buf = storage + 2 * stride;
    mp_ptr dst = storage + 2 * stride + MAX_DN + 4;
    /* 逐点三档曲线缓存：二维联合评分复用 */
    mp_size_t point_dn[MAX_POINTS];
    double point_us[MAX_POINTS][3];
    unsigned point_count = 0;
    double mrs_scores[MRS_CAND_MAX + 1] = {0};
    unsigned long mrs_counts[MRS_CAND_MAX + 1] = {0};
    double cache_scores[CACHE_CAND_MAX + 1] = {0};
    unsigned long cache_counts[CACHE_CAND_MAX + 1] = {0};
    const mp_size_t saved_mrs = lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD;
    const mp_size_t saved_cache = lmmp_tune_DIVEXACT_MERSENNE_CACHE_THRESHOLD;
    const mp_size_t saved_bc = lmmp_tune_DIVEXACT_BASECASE_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_numth_divexact_modm\n");
    printf("# parameters=DIVEXACT_MERSENNE_THRESHOLD,DIVEXACT_MERSENNE_CACHE_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default_mrs=%llu default_cache=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)DIVEXACT_MERSENNE_THRESHOLD, (unsigned long long)DIVEXACT_MERSENNE_CACHE_THRESHOLD,
           options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# rungs: A=plain B=mersenne-dispatch C=fft-cache; per-point forced binding via full entry, root gate pinned off\n");
    printf("# MULLO_DC keeps default (in<=3dn/4+1<%llu, mullo cache always off); MUL/SQR_MERSENNE_* unbound\n",
           (unsigned long long)lmmp_tune_MULLO_DC_THRESHOLD);
    lmmp_tune_print_env_comment();
    printf("# dn nn in blocks a_us a_mad b_us b_mad c_us c_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t dn = MIN_DN; dn <= MAX_DN && result == EXIT_SUCCESS; dn += STEP_DN) {
        for (int shape = 0; shape < SHAPE_COUNT && result == EXIT_SUCCESS; ++shape) {
            const mp_size_t nn = shape == 0 ? 2 * dn : 4 * dn;
            const mp_size_t qn = nn - dn + 1;
            const mp_size_t qa = qn - 1;
            const mp_size_t blocks = (qn - 1) / dn + 1;
            const mp_size_t in = (qn - 1) / blocks + 1;

            fill_random(q_ref, qa, (mp_limb_t)(options.seed + 8 * nn + dn));
            fill_random(d_buf, dn, (mp_limb_t)(options.seed + 8 * nn + dn + 1));
            d_buf[0] |= 1;
            if (qa >= dn)
                lmmp_mul_(np_orig, q_ref, qa, d_buf, dn);
            else
                lmmp_mul_(np_orig, d_buf, dn, q_ref, qa);

            measure_context ctx = {
                .dst = dst, .np = np_orig, .dp = d_buf, .nn = nn, .dn = dn,
                .bind_mrs = saved_mrs, .bind_cache = saved_cache};
            /* A=(dn+1,dn+1) B=(dn,dn+1) C=(dn,dn)：镜像 ms_mode 选择式强制档位 */
            const mp_size_t rung_mrs[3] = {dn + 1, dn, dn};
            const mp_size_t rung_cache[3] = {dn + 1, dn + 1, dn};
            double us[3], mad[3];
            unsigned retries = 0;
            int all_stable = 1;
            lmmp_tune_bench bench = {
                .bind = bind_thresholds, .reset = reset_output, .call = call_entry, .context = &ctx};
            for (int rung = 0; rung < 3; ++rung) {
                ctx.bind_mrs = rung_mrs[rung];
                ctx.bind_cache = rung_cache[rung];
                unsigned r;
                if (!lmmp_tune_measure(&bench, &options, &us[rung], &mad[rung], &r)) {
                    fprintf(stderr, "measurement failed at dn=%llu shape=%d rung=%d\n", (unsigned long long)dn,
                            shape, rung);
                    result = EXIT_FAILURE;
                    break;
                }
                if (r > retries)
                    retries = r;
                if (mad[rung] > options.mad_limit)
                    all_stable = 0;
            }
            if (result != EXIT_SUCCESS)
                break;
            if (all_stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("%llu %llu %llu %llu %.6f %.6f %.6f %.6f %.6f %.6f %u %d\n", (unsigned long long)dn,
                   (unsigned long long)nn, (unsigned long long)in, (unsigned long long)blocks, us[0], mad[0], us[1],
                   mad[1], us[2], mad[2], retries, all_stable);

            /* 正确性：三档结果与构造商全等 */
            for (int rung = 0; rung < 3; ++rung) {
                if (!check_rung(&ctx, q_ref, qa, rung_mrs[rung], rung_cache[rung])) {
                    fprintf(stderr, "rung result mismatch at dn=%llu shape=%d rung=%d\n", (unsigned long long)dn,
                            shape, rung);
                    result = EXIT_FAILURE;
                    break;
                }
            }
            if (result != EXIT_SUCCESS)
                break;

            if (point_count < MAX_POINTS) {
                point_dn[point_count] = dn;
                for (int rung = 0; rung < 3; ++rung)
                    point_us[point_count][rung] = us[rung];
                ++point_count;
            }
            /* DIVEXACT_MERSENNE 轴：候选 m 配默认 DIVEXACT_MERSENNE_CACHE=c0 */
            for (int m = MRS_CAND_MIN; m <= MRS_CAND_MAX; m += MRS_CAND_STEP) {
                double pick = dn >= (mp_size_t)m ? (dn >= saved_cache ? us[2] : us[1]) : us[0];
                mrs_scores[m] += pick;
                ++mrs_counts[m];
            }
            /* DIVEXACT_MERSENNE_CACHE 轴：候选 c 配默认 DIVEXACT_MERSENNE=m0 */
            for (int c = CACHE_CAND_MIN; c <= CACHE_CAND_MAX; c += CACHE_CAND_STEP) {
                double pick = dn >= (mp_size_t)c ? us[2] : (dn >= saved_mrs ? us[1] : us[0]);
                cache_scores[c] += pick;
                ++cache_counts[c];
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            recommend_stepped("DIVEXACT_MERSENNE_THRESHOLD", mrs_scores, mrs_counts, MRS_CAND_MIN, MRS_CAND_MAX,
                              MRS_CAND_STEP, saved_mrs);
            recommend_stepped("DIVEXACT_MERSENNE_CACHE_THRESHOLD", cache_scores, cache_counts, CACHE_CAND_MIN,
                              CACHE_CAND_MAX, CACHE_CAND_STEP, saved_cache);
            /* 二维联合 argmin：部署选择式 dn>=c ? C : (dn>=m ? B : A)，逐点取档位曲线 */
            int best_m = MRS_CAND_MIN, best_c = CACHE_CAND_MIN;
            double best_total = -1.0;
            for (int m = MRS_CAND_MIN; m <= MRS_CAND_MAX; m += MRS_CAND_STEP) {
                for (int c = CACHE_CAND_MIN; c <= CACHE_CAND_MAX; c += CACHE_CAND_STEP) {
                    double total = 0;
                    for (unsigned p = 0; p < point_count; ++p) {
                        int rung = point_dn[p] >= (mp_size_t)c ? 2 : (point_dn[p] >= (mp_size_t)m ? 1 : 0);
                        total += point_us[p][rung];
                    }
                    if (best_total < 0 || total < best_total) {
                        best_total = total;
                        best_m = m;
                        best_c = c;
                    }
                }
            }
            double current_total = 0;
            for (unsigned p = 0; p < point_count; ++p) {
                int rung = point_dn[p] >= saved_cache ? 2 : (point_dn[p] >= saved_mrs ? 1 : 0);
                current_total += point_us[p][rung];
            }
            printf("# joint_best=(%d,%d) avg_us=%.6f\n", best_m, best_c, best_total / point_count);
            printf("# joint_current=(%llu,%llu) avg_us=%.6f\n", (unsigned long long)saved_mrs,
                   (unsigned long long)saved_cache, current_total / point_count);
            printf("recommended joint (DIVEXACT_MERSENNE,DIVEXACT_MERSENNE_CACHE) = (%d,%d)\n", best_m, best_c);
        }
    }
    lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD = saved_mrs;
    lmmp_tune_DIVEXACT_MERSENNE_CACHE_THRESHOLD = saved_cache;
    lmmp_tune_DIVEXACT_BASECASE_THRESHOLD = saved_bc;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
