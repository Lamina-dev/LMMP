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

/* binvert 根分派斜率阈值 BINVERT_UNBALANCE_SLOPE_THRESHOLD（定点 1/256）：
   条件 256*n >= na*T 为真走线性递推 unbalance（high），为假走高位补零平衡
   牛顿 n_dc（low）。分派只读一次、两条后端内部都不再读该阈值（unbalance_
   内部直调 binvert_n_dc_，n_dc 自递归不看分派）⟹ 强制绑定全网格干净、
   sub_n=0。二元斜率拟合：固定 (na,n) 网格（na 列 × 比例 r=n/na 行，
   r=1+1/16..2 步 1/16）逐点配对测量两分派耗时；候选斜率 T 按部署选择式
   256*n >= na*T ? high : low 对全网格累计求 argmin（成本加权的分界线
   拟合），另附逐列首胜比例的过原点最小二乘斜率作交叉诊断。恒等式
   oracle a*x == 1 (mod B^n) 同 tune_numth_binvert_modm。MODM/CACHE/
   MULHI/MULLO_DC 保持默认（两分派共同、按部署形态）。 */
enum {
    R_STEP = 16,                    /* 比例步长 1/16：n = na + j*na/16 */
    R_COUNT = 16,                   /* j=1..16：r=1+1/16 .. 2.0 */
    NA_COUNT = 9,
    CANDIDATE_MIN = 256,            /* 斜率 1.0 */
    CANDIDATE_MAX = 768,            /* 斜率 3.0 */
    CANDIDATE_STEP = 16,            /* 与网格比例步长对齐 */
    MAX_NA = 4096,
    MAX_N = 2 * MAX_NA,
    MAX_POINTS = NA_COUNT * R_COUNT
};

static const mp_size_t na_values[NA_COUNT] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_size_t na;
    mp_size_t n;
} measure_context;

static void bind_low(void* opaque) {
    measure_context* context = opaque;
    (void)context;
    /* 任意网格点 256*n < na*T ⟹ 恒走补零平衡牛顿 */
    lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD = 1 << 20;
}

static void bind_high(void* opaque) {
    measure_context* context = opaque;
    (void)context;
    /* 256*n >= na*1 恒真 ⟹ 恒走 unbalance */
    lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD = 1;
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
    printf("program=tune_numth_binvert_dispatch\n");
    printf("parameter=BINVERT_UNBALANCE_SLOPE_THRESHOLD (root dispatch slope, fixed-point 1/256)\n");
    printf("consumer=lmmp_binvert_ unbalance vs zero-padded n_dc dispatch\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu (slope %.4f)\n", (unsigned long long)BINVERT_UNBALANCE_SLOPE_THRESHOLD,
           BINVERT_UNBALANCE_SLOPE_THRESHOLD / 256.0);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD);
    printf("input=na {16,32,64,128,256,512,1024,2048,4096} x ratio n/na = 1+1/16..2 step 1/16\n");
    printf("candidate_range=%d..%d step=%d (slopes %.4f..%.4f)\n", CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP,
           CANDIDATE_MIN / 256.0, CANDIDATE_MAX / 256.0);
    printf("forced_root_equivalence=all_n (dispatch read once, backends never re-read it)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_binvert_dispatch");
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
        lmmp_tune_print_current("BINVERT_UNBALANCE_SLOPE_THRESHOLD",
                                (unsigned long long)BINVERT_UNBALANCE_SLOPE_THRESHOLD,
                                (unsigned long long)lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    mp_ptr storage = alloc_limbs(MAX_NA + 4 + 3 * (MAX_N + 4) + 2 * MAX_N + 4);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage;               /* [0, na) 奇底数 */
    mp_ptr low = storage + MAX_NA + 4;   /* [0, n) 低路（补零 n_dc）逆元 */
    mp_ptr high = low + MAX_N + 4;       /* [0, n) 高路（unbalance）逆元 */
    mp_ptr pad = high + MAX_N + 4;       /* 恒等式校验：底数补零到 n */
    mp_ptr prod = pad + MAX_N + 4;       /* 恒等式校验：全积 2n */
    double scores[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    const mp_size_t saved_slope = lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;
    /* 逐列首胜比例（诊断 + 最小二乘斜率拟合） */
    double column_cross[NA_COUNT];

    printf("# program=tune_numth_binvert_dispatch\n");
    printf("# parameter=BINVERT_UNBALANCE_SLOPE_THRESHOLD (fixed-point 1/256)\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)BINVERT_UNBALANCE_SLOPE_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# low=zero-padded n_dc high=unbalance; both via full lmmp_binvert_ entry\n");
    printf("# MODM/CACHE/MULHI/MULLO_DC keep defaults (shared by both dispatches)\n");
    lmmp_tune_print_env_comment();
    printf("# na n ratio16 low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (int na_index = 0; na_index < NA_COUNT && result == EXIT_SUCCESS; ++na_index) {
        const mp_size_t na = na_values[na_index];
        column_cross[na_index] = -1.0;
        for (int j = 1; j <= R_COUNT && result == EXIT_SUCCESS; ++j) {
            const mp_size_t n = na + j * na / R_STEP;
            const double ratio16 = 16.0 * n / (double)na;

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
                fprintf(stderr, "inverse identity mismatch at na=%llu n=%llu\n", (unsigned long long)na,
                        (unsigned long long)n);
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
                fprintf(stderr, "measurement failed at na=%llu n=%llu\n", (unsigned long long)na,
                        (unsigned long long)n);
                result = EXIT_FAILURE;
                break;
            }
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("%llu %llu %.4f %.6f %.6f %.6f %.6f %u %u\n", (unsigned long long)na, (unsigned long long)n,
                   ratio16, low_us, low_mad, high_us, high_mad, retries, stable);

            /* 部署评分：候选 T 在该点走 256*n >= na*T ? high : low */
            for (int t = CANDIDATE_MIN; t <= CANDIDATE_MAX; t += CANDIDATE_STEP) {
                double pick = 256 * n >= na * (mp_size_t)t ? high_us : low_us;
                scores[t] += pick;
                ++counts[t];
            }
            /* 逐列首胜：unbalance 首次严格快于 n_dc 的比例 */
            if (column_cross[na_index] < 0.0 && high_us < low_us)
                column_cross[na_index] = ratio16 / 16.0;
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            for (int na_index = 0; na_index < NA_COUNT; ++na_index) {
                const mp_size_t na = na_values[na_index];
                if (column_cross[na_index] > 0.0)
                    printf("# crossover na=%llu ratio=%.4f slope_fp=%d\n", (unsigned long long)na,
                           column_cross[na_index], (int)(column_cross[na_index] * 256.0 + 0.5));
                else
                    printf("# crossover na=%llu none-within-grid\n", (unsigned long long)na);
            }
            /* 过原点最小二乘：n* = r_c*na ⟹ 斜率 = sum(r_c*na^2)/sum(na^2) */
            double num = 0, den = 0;
            for (int na_index = 0; na_index < NA_COUNT; ++na_index) {
                if (column_cross[na_index] > 0.0) {
                    const double na_d = (double)na_values[na_index];
                    num += column_cross[na_index] * na_d * na_d;
                    den += na_d * na_d;
                }
            }
            if (den > 0)
                printf("# lsq_slope_fit=%.4f slope_fp=%d\n", num / den, (int)(num / den * 256.0 + 0.5));

            int best = CANDIDATE_MIN;
            for (int t = CANDIDATE_MIN + CANDIDATE_STEP; t <= CANDIDATE_MAX; t += CANDIDATE_STEP)
                if (counts[t] && scores[t] / counts[t] < scores[best] / counts[best])
                    best = t;
            double best_score = scores[best] / counts[best];
            int flat_low = best, flat_high = best;
            while (flat_low - CANDIDATE_STEP >= CANDIDATE_MIN && counts[flat_low - CANDIDATE_STEP] &&
                   scores[flat_low - CANDIDATE_STEP] / counts[flat_low - CANDIDATE_STEP] <= best_score * 1.01)
                flat_low -= CANDIDATE_STEP;
            while (flat_high + CANDIDATE_STEP <= CANDIDATE_MAX && counts[flat_high + CANDIDATE_STEP] &&
                   scores[flat_high + CANDIDATE_STEP] / counts[flat_high + CANDIDATE_STEP] <= best_score * 1.01)
                flat_high += CANDIDATE_STEP;
            for (int t = CANDIDATE_MIN; t <= CANDIDATE_MAX; t += CANDIDATE_STEP)
                if (counts[t])
                    printf("# badness candidate=%d slope=%.4f avg_us=%.6f\n", t, t / 256.0,
                           scores[t] / counts[t]);
            printf("# recommended=%d slope=%.4f\n", best, best / 256.0);
            printf("# recommended_avg_us=%.6f\n", best_score);
            printf("# flat_within_1pct=%d..%d (slopes %.4f..%.4f)\n", flat_low, flat_high, flat_low / 256.0,
                   flat_high / 256.0);
            if (saved_slope >= CANDIDATE_MIN && saved_slope <= CANDIDATE_MAX && counts[saved_slope])
                printf("# current=%llu slope=%.4f current_avg_us=%.6f\n", (unsigned long long)saved_slope,
                       saved_slope / 256.0, scores[saved_slope] / (double)counts[saved_slope]);
            else
                printf("# current=%llu slope=%.4f current_avg_us=n/a (off candidate grid)\n",
                       (unsigned long long)saved_slope, saved_slope / 256.0);
            printf("# points=%lu\n", counts[best]);
            printf("recommended BINVERT_UNBALANCE_SLOPE_THRESHOLD = %d (slope %.4f)\n", best, best / 256.0);
        }
    }
    lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD = saved_slope;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
