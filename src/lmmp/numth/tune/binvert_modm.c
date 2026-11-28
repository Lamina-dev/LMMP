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

/* binvert unbalance 主循环 k 更新的三档梯子（梯级尺寸恒为 na；判序与生产分派一致，
   MODM 先判：na<BINVERT_UNBALANCE_MODM_THRESHOLD 时 CACHE 不参与）：
   A=plain 直乘（na<BINVERT_UNBALANCE_MODM_THRESHOLD）
   B=梅森分发不缓存（na>=BINVERT_UNBALANCE_MODM_THRESHOLD 且
     na<BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD，m=mersenne_next_size((2na+1)>>1)）
   C=fft 缓存（na>=BINVERT_UNBALANCE_MODM_THRESHOLD 且
     na>=BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD，m=fft_next_size((2na+1)>>1)，
     预旋转 D' 变换跨块复用）
   档位在循环体内按 na 读取，一次调用内所有块同尺寸、无自递归 ⟹ 绑定全网格
   干净、sub_n=0。每点经完整 lmmp_binvert_ 入口三路测量（根分派绑
   BINVERT_UNBALANCE_SLOPE_THRESHOLD=1 固定走 unbalance）；binvert_n_dc_(na)
   前缀与 binvert_mulhi_ 每调用恰一次、三路对称共同计时；分块乘积的 mullo 走
   MULLO_DC_THRESHOLD（不绑定），网格 na<=2048<默认值 ⟹ 恒走 mullo_dc_，
   与部署一致。输入奇底数 [numa,na]，oracle=恒等式 a*x == 1 (mod B^n)。
   评分：MODM 轴配 CACHE 默认、CACHE 轴配 MODM 默认各一维 + 按部署选择式
   na<m ? A : (na>=c ? C : B) 求两阈值的二维联合 argmin。 */
enum {
    MIN_NA = 8, MAX_NA = 2048,
    /* 网格三段：8..256 步 8（A|B 边界细扫）、272..1024 步 16、1088..2048 步 32 */
    SEG1_LO = 8, SEG1_HI = 256, SEG1_STEP = 8,
    SEG2_LO = 272, SEG2_HI = 1024, SEG2_STEP = 16,
    SEG3_LO = 1088, SEG3_HI = 2048, SEG3_STEP = 32,
    SHAPE_COUNT = 2,
    MAX_SHAPE_K = 6, /* 最大形状 n=6na */
    MODM_CAND_MIN = 8, MODM_CAND_MAX = 512, MODM_CAND_STEP = 8,
    CACHE_CAND_MIN = 256, CACHE_CAND_MAX = 2048, CACHE_CAND_STEP = 32,
    MAX_N = MAX_SHAPE_K * MAX_NA,
    MAX_POINTS = SHAPE_COUNT * ((SEG1_HI - SEG1_LO) / SEG1_STEP + 1 + (SEG2_HI - SEG2_LO) / SEG2_STEP + 1 +
                                (SEG3_HI - SEG3_LO) / SEG3_STEP + 1)
};

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_size_t na;
    mp_size_t n;
    mp_size_t bind_modm;
    mp_size_t bind_cache;
} measure_context;

static void bind_thresholds(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD = 1;
    lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD = context->bind_modm;
    lmmp_tune_BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD = context->bind_cache;
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

/* 按绑定强制跑完整入口并做恒等式校验 */
static int check_rung(measure_context* context, mp_size_t bind_modm, mp_size_t bind_cache, mp_ptr pad, mp_ptr prod) {
    context->bind_modm = bind_modm;
    context->bind_cache = bind_cache;
    bind_thresholds(context);
    lmmp_zero(context->dst, context->n);
    lmmp_binvert_(context->dst, context->numa, context->na, context->n);
    return check_inverse(context->numa, context->na, context->dst, context->n, pad, prod);
}

static void print_info(void) {
    printf("program=tune_numth_binvert_modm\n");
    printf("parameters=BINVERT_UNBALANCE_MODM_THRESHOLD,BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD (unbalance k-update ladder)\n");
    printf("consumer=lmmp_binvert_unbalance_ k-update loop (via lmmp_binvert_ unbalance rung)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default_modm=%llu default_cache=%llu\n", (unsigned long long)BINVERT_UNBALANCE_MODM_THRESHOLD,
           (unsigned long long)BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD);
    printf("current_modm=%llu current_cache=%llu\n", (unsigned long long)lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD,
           (unsigned long long)lmmp_tune_BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD);
    printf("input=na %d..%d (segments %d..%d step %d, %d..%d step %d, %d..%d step %d) x shapes{n=3na (1 block), n=6na (4 blocks)}\n",
           MIN_NA, MAX_NA, SEG1_LO, SEG1_HI, SEG1_STEP, SEG2_LO, SEG2_HI, SEG2_STEP, SEG3_LO, SEG3_HI, SEG3_STEP);
    printf("modm_candidates=%d..%d step=%d; cache_candidates=%d..%d step=%d\n", MODM_CAND_MIN, MODM_CAND_MAX,
           MODM_CAND_STEP, CACHE_CAND_MIN, CACHE_CAND_MAX, CACHE_CAND_STEP);
    printf("forced_root_equivalence=all_n (ladder keyed by fixed na per call, no recursion)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_binvert_modm");
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
        lmmp_tune_print_current("BINVERT_UNBALANCE_MODM_THRESHOLD",
                                (unsigned long long)BINVERT_UNBALANCE_MODM_THRESHOLD,
                                (unsigned long long)lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD);
        lmmp_tune_print_current("BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD",
                                (unsigned long long)BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD,
                                (unsigned long long)lmmp_tune_BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    /* 网格 na 列表（三段拼接） */
    mp_size_t na_grid[MAX_POINTS];
    unsigned na_count = 0;
    for (mp_size_t na = SEG1_LO; na <= SEG1_HI; na += SEG1_STEP)
        na_grid[na_count++] = na;
    for (mp_size_t na = SEG2_LO; na <= SEG2_HI; na += SEG2_STEP)
        na_grid[na_count++] = na;
    for (mp_size_t na = SEG3_LO; na <= SEG3_HI; na += SEG3_STEP)
        na_grid[na_count++] = na;

    mp_ptr storage = alloc_limbs(MAX_NA + 4 + 3 * (MAX_N + 4) + 2 * MAX_N + 4);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;               /* [0, na) 奇底数 */
    mp_ptr dst = storage + MAX_NA + 4;   /* [0, n) 逆元 */
    mp_ptr pad = dst + MAX_N + 4;        /* 恒等式校验：底数补零到 n */
    mp_ptr prod = pad + MAX_N + 4;       /* 恒等式校验：全积 2n */
    /* 逐点三档曲线缓存：二维联合评分复用 */
    mp_size_t point_na[MAX_POINTS];
    double point_us[MAX_POINTS][3];
    unsigned point_count = 0;
    double modm_scores[MODM_CAND_MAX + 1] = {0};
    unsigned long modm_counts[MODM_CAND_MAX + 1] = {0};
    double cache_scores[CACHE_CAND_MAX + 1] = {0};
    unsigned long cache_counts[CACHE_CAND_MAX + 1] = {0};
    const mp_size_t saved_modm = lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD;
    const mp_size_t saved_cache = lmmp_tune_BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD;
    const mp_size_t saved_slope = lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_numth_binvert_modm\n");
    printf("# parameters=BINVERT_UNBALANCE_MODM_THRESHOLD,BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default_modm=%llu default_cache=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)BINVERT_UNBALANCE_MODM_THRESHOLD,
           (unsigned long long)BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# rungs: A=plain B=mersenne-dispatch C=fft-cache; per-point forced binding via full entry, dispatch pinned to unbalance\n");
    printf("# MULLO_DC keeps default (na<=%d<%llu, block mullo always dc_); MULHI_MERSENNE unbound (symmetric across rungs)\n",
           MAX_NA, (unsigned long long)lmmp_tune_MULHI_MERSENNE_THRESHOLD);
    lmmp_tune_print_env_comment();
    printf("# na n blocks a_us a_mad b_us b_mad c_us c_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (unsigned na_index = 0; na_index < na_count && result == EXIT_SUCCESS; ++na_index) {
        const mp_size_t na = na_grid[na_index];
        for (int shape = 0; shape < SHAPE_COUNT && result == EXIT_SUCCESS; ++shape) {
            const mp_size_t n = shape == 0 ? 3 * na : 6 * na;
            const mp_size_t blocks = n / na - 2; /* 循环块数（n=k*na 时 k-2 块，另有末块 mullo） */

            fill_random(numa, na, (mp_limb_t)(options.seed + 8 * n + na));
            numa[0] |= 1;

            measure_context ctx = {
                .dst = dst, .numa = numa, .na = na, .n = n,
                .bind_modm = saved_modm, .bind_cache = saved_cache};
            /* A=(na+1,na+1) B=(na,na+1) C=(na,na)：镜像 k 更新选择式强制档位 */
            const mp_size_t rung_modm[3] = {na + 1, na, na};
            const mp_size_t rung_cache[3] = {na + 1, na + 1, na};
            double us[3], mad[3];
            unsigned retries = 0;
            int all_stable = 1;
            lmmp_tune_bench bench = {
                .bind = bind_thresholds, .reset = reset_output, .call = call_entry, .context = &ctx};
            for (int rung = 0; rung < 3; ++rung) {
                ctx.bind_modm = rung_modm[rung];
                ctx.bind_cache = rung_cache[rung];
                unsigned r;
                if (!lmmp_tune_measure(&bench, &options, &us[rung], &mad[rung], &r)) {
                    fprintf(stderr, "measurement failed at na=%llu shape=%d rung=%d\n", (unsigned long long)na,
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
            printf("%llu %llu %llu %.6f %.6f %.6f %.6f %.6f %.6f %u %d\n", (unsigned long long)na,
                   (unsigned long long)n, (unsigned long long)blocks, us[0], mad[0], us[1], mad[1], us[2], mad[2],
                   retries, all_stable);

            /* 正确性：三档均满足 a*x == 1 (mod B^n) */
            for (int rung = 0; rung < 3; ++rung) {
                if (!check_rung(&ctx, rung_modm[rung], rung_cache[rung], pad, prod)) {
                    fprintf(stderr, "inverse identity mismatch at na=%llu shape=%d rung=%d\n",
                            (unsigned long long)na, shape, rung);
                    result = EXIT_FAILURE;
                    break;
                }
            }
            if (result != EXIT_SUCCESS)
                break;

            if (point_count < MAX_POINTS) {
                point_na[point_count] = na;
                for (int rung = 0; rung < 3; ++rung)
                    point_us[point_count][rung] = us[rung];
                ++point_count;
            }
            /* MODM 轴：候选 m 配默认 CACHE=c0，判序镜像生产分派（MODM 先判） */
            for (int m = MODM_CAND_MIN; m <= MODM_CAND_MAX; m += MODM_CAND_STEP) {
                double pick = na < (mp_size_t)m ? us[0] : (na >= saved_cache ? us[2] : us[1]);
                modm_scores[m] += pick;
                ++modm_counts[m];
            }
            /* CACHE 轴：候选 c 配默认 MODM=m0，判序镜像生产分派（MODM 先判） */
            for (int c = CACHE_CAND_MIN; c <= CACHE_CAND_MAX; c += CACHE_CAND_STEP) {
                double pick = na < saved_modm ? us[0] : (na >= (mp_size_t)c ? us[2] : us[1]);
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
            recommend_stepped("BINVERT_UNBALANCE_MODM_THRESHOLD", modm_scores, modm_counts, MODM_CAND_MIN,
                              MODM_CAND_MAX, MODM_CAND_STEP, saved_modm);
            recommend_stepped("BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD", cache_scores, cache_counts, CACHE_CAND_MIN,
                              CACHE_CAND_MAX, CACHE_CAND_STEP, saved_cache);
            /* 二维联合 argmin：部署选择式 na<m ? A : (na>=c ? C : B)（MODM 先判），逐点取档位曲线 */
            int best_m = MODM_CAND_MIN, best_c = CACHE_CAND_MIN;
            double best_total = -1.0;
            for (int m = MODM_CAND_MIN; m <= MODM_CAND_MAX; m += MODM_CAND_STEP) {
                for (int c = CACHE_CAND_MIN; c <= CACHE_CAND_MAX; c += CACHE_CAND_STEP) {
                    double total = 0;
                    for (unsigned p = 0; p < point_count; ++p) {
                        int rung = point_na[p] < (mp_size_t)m ? 0 : (point_na[p] >= (mp_size_t)c ? 2 : 1);
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
                int rung = point_na[p] < saved_modm ? 0 : (point_na[p] >= saved_cache ? 2 : 1);
                current_total += point_us[p][rung];
            }
            printf("# joint_best=(%d,%d) avg_us=%.6f\n", best_m, best_c, best_total / point_count);
            printf("# joint_current=(%llu,%llu) avg_us=%.6f\n", (unsigned long long)saved_modm,
                   (unsigned long long)saved_cache, current_total / point_count);
            printf("recommended joint (BINVERT_UNBALANCE_MODM,BINVERT_UNBALANCE_MODM_CACHE) = (%d,%d)\n", best_m,
                   best_c);
        }
    }
    lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD = saved_modm;
    lmmp_tune_BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD = saved_cache;
    lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD = saved_slope;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
