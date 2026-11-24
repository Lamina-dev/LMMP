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
#include "impl/str_conv.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

/* limb→字符串 入口门：TO_STR_BASEPOW_THRESHOLD 只在 to_str_/to_decimal_ 入口读
   一次（na<T 走 basecase，否则进幂表分治），不在递归内，绑 T=na 强制双路对
   任意 na 干净（sub_n=0），无 deployed 扫描。阈值由两个成本结构不同的消费方
   共享：to_str_（任意基数）每次调用重建幂表（本网格 np<527 不建逆元），主评分
   形状；to_decimal_ 用全局缓存十进制幂表，副评分形状。两形状独立评分，主推荐
   以无缓存形状为准。
   注意：本阈值为首个 runtime-with-fallback 登记——to_str_basecase_ 的栈数组在
   生产构建按该阈值定长，tune 构建回退为 TO_STR_BASEPOW_TUNE_STACK_MAX 常量，
   basecase 测量含更大栈帧的结构差异。部署约束 TO_STR_DIVIDE <= 本阈值+1
  （入口 na>=T 进幂表分治、其递归以 na<T 调 basecase，栈容量上限即本阈值）：
   推荐选择域排除 TO_STR_DIVIDE-1 以下的候选，其测量与 badness 诊断保留。 */
enum {
    MIN_N = 4, MAX_N = 512, STEP_N = 4, CANDIDATE_MIN = MIN_N, CANDIDATE_MAX = MAX_N, CANDIDATE_STEP = 4,
    CANDIDATE_LAST = CANDIDATE_MIN + (CANDIDATE_MAX - CANDIDATE_MIN) / CANDIDATE_STEP * CANDIDATE_STEP
};

_Static_assert(MAX_N <= 1024, "grid must stay within TO_STR_BASEPOW_TUNE_STACK_MAX");

/* 选择域下界：最小满足 TO_STR_DIVIDE <= T+1 的候选（生产栈容量约束） */
static int select_floor(void) {
    mp_size_t need = TO_STR_DIVIDE_THRESHOLD > 1 ? TO_STR_DIVIDE_THRESHOLD - 1 : 1;
    int floor = CANDIDATE_MIN;
    while (floor < (int)need && floor + CANDIDATE_STEP <= CANDIDATE_LAST)
        floor += CANDIDATE_STEP;
    return floor;
}

enum { SHAPE_DEC_CACHED = 0, SHAPE_TO_STR_UNCACHED = 1, SHAPE_COUNT = 2 };

typedef struct {
    mp_byte_t* dst;
    mp_srcptr numa;
    mp_size_t na;
    int shape;
    mp_size_t bind_threshold;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_TO_STR_BASEPOW_THRESHOLD = context->bind_threshold;
}

/* 输入只读、输出全写：批次前仅触碰输出首字节 */
static void reset_outputs(void* opaque) {
    measure_context* context = opaque;
    context->dst[0] = 0;
}

static mp_limb_t call_basecase(void* opaque) {
    measure_context* context = opaque;
    mp_size_t digits = lmmp_to_str_basecase_(context->dst, context->numa, context->na, DEC_BASE, 0);
    return (mp_limb_t)digits ^ context->dst[0] ^ context->dst[digits - 1];
}

static mp_limb_t call_entry(void* opaque) {
    measure_context* context = opaque;
    mp_size_t digits;
    if (context->shape == SHAPE_TO_STR_UNCACHED)
        digits = lmmp_to_str_(context->dst, context->numa, context->na, DEC_BASE);
    else
        digits = lmmp_to_decimal_((char*)context->dst, context->numa, context->na);
    return (mp_limb_t)digits ^ context->dst[0] ^ context->dst[digits - 1];
}

/* 双路互比：位数与数字值全等（high 绑 T=na 强制入口走幂表分治；
   to_str_ 输出位值、to_decimal_ 输出 '0'+位值，比较时统一减 '0'） */
static mp_size_t check_pair(measure_context* low, measure_context* high) {
    mp_size_t saved = lmmp_tune_TO_STR_BASEPOW_THRESHOLD;
    mp_size_t digits_low = lmmp_to_str_basecase_(low->dst, low->numa, low->na, DEC_BASE, 0);
    lmmp_tune_TO_STR_BASEPOW_THRESHOLD = high->bind_threshold;
    mp_size_t digits_high;
    if (high->shape == SHAPE_TO_STR_UNCACHED)
        digits_high = lmmp_to_str_(high->dst, high->numa, high->na, DEC_BASE);
    else
        digits_high = lmmp_to_decimal_((char*)high->dst, high->numa, high->na);
    lmmp_tune_TO_STR_BASEPOW_THRESHOLD = saved;
    if (digits_low != digits_high)
        return (mp_size_t)-1;
    mp_byte_t off = high->shape == SHAPE_TO_STR_UNCACHED ? 0 : '0';
    for (mp_size_t i = 0; i < digits_low; ++i)
        if (low->dst[i] != high->dst[i] - off)
            return (mp_size_t)-1;
    return digits_low;
}

/* 往返锚点：用对侧 from_decimal_ 转回 limb，须与原输入全等 */
static int check_roundtrip(mp_srcptr numa, mp_size_t na, const mp_byte_t* digits, mp_size_t ndigits, mp_ptr back) {
    mp_size_t limbs = lmmp_from_decimal_(back, (const char*)digits, ndigits);
    if (limbs != na || lmmp_cmp_(back, numa, na) != 0)
        return 0;
    return 1;
}

/* 候选按步长稀疏，公共 recommend 的逐整数遍历不适用，自行汇总 */
static void recommend_stepped(const char* label, const double* scores, const unsigned long* counts, int cmin,
                              int cmax, int cstep) {
    int floor = select_floor();
    int best = floor;
    for (int c = floor + cstep; c <= cmax; c += cstep)
        if (counts[c] && scores[c] / counts[c] < scores[best] / counts[best])
            best = c;
    double best_score = scores[best] / counts[best];
    int flat_low = best, flat_high = best;
    while (flat_low - cstep >= floor && scores[flat_low - cstep] / counts[flat_low - cstep] <= best_score * 1.01)
        flat_low -= cstep;
    while (flat_high + cstep <= cmax && scores[flat_high + cstep] / counts[flat_high + cstep] <= best_score * 1.01)
        flat_high += cstep;
    for (int c = cmin; c <= cmax; c += cstep)
        if (counts[c])
            printf("# badness shape=%s candidate=%d avg_us=%.6f\n", label, c, scores[c] / counts[c]);
    printf("# select_floor=%d (TO_STR_DIVIDE-1, production stack bound)\n", floor);
    printf("# recommended shape=%s value=%d avg_us=%.6f\n", label, best, best_score);
    printf("# flat_within_1pct shape=%s %d..%d\n", label, flat_low, flat_high);
    printf("# points shape=%s %lu\n", label, counts[best]);
}

static void print_info(void) {
    printf("program=tune_lmmpn_to_str_basepow\n");
    printf("parameter=TO_STR_BASEPOW_THRESHOLD\n");
    printf("consumer=lmmp_to_str_ / lmmp_to_decimal_ entry gates\n");
    printf("binding_mode=runtime-with-fallback (basecase stack array uses TO_STR_BASEPOW_TUNE_STACK_MAX in tune builds)\n");
    printf("default=%llu\n", (unsigned long long)TO_STR_BASEPOW_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_TO_STR_BASEPOW_THRESHOLD);
    printf("candidate_range=%d..%d step=%d (scan upper bound %d)\n", CANDIDATE_MIN, CANDIDATE_LAST, CANDIDATE_STEP,
           CANDIDATE_MAX);
    printf("input_range=na %d..%d step=%d\n", MIN_N, MAX_N, STEP_N);
    printf("forced_root_equivalence=all_n (gate read once at entry, sub_n=0)\n");
    printf("primary_shape=to_str_uncached (per-call power table, no inverses at this grid)\n");
    printf("secondary_shape=decimal_cached (global cached power table)\n");
    printf("constraint=TO_STR_DIVIDE <= TO_STR_BASEPOW+1 (production basecase stack array bound); selection excludes candidates below TO_STR_DIVIDE-1\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_to_str_basepow");
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
        lmmp_tune_print_current("TO_STR_BASEPOW_THRESHOLD", (unsigned long long)TO_STR_BASEPOW_THRESHOLD,
                                (unsigned long long)lmmp_tune_TO_STR_BASEPOW_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    const mp_size_t max_digits = MAX_N * LIMB_BITS / 3 + 16; /* 位数上界（log10(2)<1/3） */
    mp_ptr storage = alloc_limbs(2 * (MAX_N + 4));
    mp_byte_t* digits_low = lmmp_alloc(max_digits);
    mp_byte_t* digits_high = lmmp_alloc(max_digits);
    double scores[SHAPE_COUNT][CANDIDATE_MAX + 1] = {{0}};
    unsigned long counts[SHAPE_COUNT][CANDIDATE_MAX + 1] = {{0}};
    mp_size_t saved_threshold = lmmp_tune_TO_STR_BASEPOW_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_to_str_basepow\n");
    printf("# parameter=TO_STR_BASEPOW_THRESHOLD\n");
    printf("# binding_mode=runtime-with-fallback\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)TO_STR_BASEPOW_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# candidate_range=%d..%d step=%d (scan upper bound %d) input=na %d..%d step=%d\n", CANDIDATE_MIN,
           CANDIDATE_LAST, CANDIDATE_STEP, CANDIDATE_MAX, MIN_N, MAX_N, STEP_N);
    printf("# high=complete entry (to_str_ uncached / to_decimal_ cached) with T=na; sub_n=0 no deployed scan\n");
    printf("# primary=to_str_uncached; secondary=decimal_cached\n");
    lmmp_tune_print_env_comment();
    printf("# shape na low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    for (mp_size_t na = MIN_N; na <= MAX_N && result == EXIT_SUCCESS; na += STEP_N) {
        mp_ptr numa = storage;
        mp_ptr anchor = storage + MAX_N + 4;
        fill_random(numa, na, options.seed + 2 * na);

        for (int shape = 0; shape < SHAPE_COUNT; ++shape) {
            measure_context low_context = {
                .dst = digits_low, .numa = numa, .na = na, .shape = shape, .bind_threshold = saved_threshold};
            measure_context high_context = {
                .dst = digits_high, .numa = numa, .na = na, .shape = shape, .bind_threshold = na};
            mp_size_t digits_cnt = check_pair(&low_context, &high_context);
            if (digits_cnt == (mp_size_t)-1) {
                fprintf(stderr, "to_str basepow pair mismatch at na=%llu shape=%d\n", (unsigned long long)na, shape);
                result = EXIT_FAILURE;
                break;
            }
            /* 往返锚点走缓存形状的 ASCII 输出（每点一次，幂表已热） */
            if (shape == SHAPE_DEC_CACHED &&
                !check_roundtrip(numa, na, digits_high, digits_cnt, anchor)) {
                fprintf(stderr, "to_str roundtrip mismatch at na=%llu\n", (unsigned long long)na);
                result = EXIT_FAILURE;
                break;
            }

            lmmp_tune_bench low_bench = {
                .bind = bind_threshold, .reset = reset_outputs, .call = call_basecase, .context = &low_context};
            lmmp_tune_bench high_bench = {
                .bind = bind_threshold, .reset = reset_outputs, .call = call_entry, .context = &high_context};
            double low_us, low_mad, high_us, high_mad;
            unsigned retries;
            bool stable;
            if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                        &retries, &stable)) {
                fprintf(stderr, "measurement failed at na=%llu shape=%d\n", (unsigned long long)na, shape);
                result = EXIT_FAILURE;
                break;
            }
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("%s %llu %.6f %.6f %.6f %.6f %u %u\n", shape == SHAPE_TO_STR_UNCACHED ? "to_str" : "dec",
                   (unsigned long long)na, low_us, low_mad, high_us, high_mad, retries, stable);
            /* 入口门无同阈值递归：全部候选按 na 与候选的相对位置直接计分 */
            for (int c = CANDIDATE_MIN; c <= CANDIDATE_MAX; c += CANDIDATE_STEP) {
                scores[shape][c] += na >= (mp_size_t)c ? high_us : low_us;
                ++counts[shape][c];
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            recommend_stepped("to_str_uncached", scores[SHAPE_TO_STR_UNCACHED], counts[SHAPE_TO_STR_UNCACHED],
                              CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP);
            recommend_stepped("decimal_cached", scores[SHAPE_DEC_CACHED], counts[SHAPE_DEC_CACHED], CANDIDATE_MIN,
                              CANDIDATE_MAX, CANDIDATE_STEP);
            /* 主推荐行：无缓存语境（保守端），缓存语境推荐值见上；
               同 recommend_stepped 的选择域下界 */
            int floor = select_floor();
            int best = floor;
            for (int c = floor + CANDIDATE_STEP; c <= CANDIDATE_MAX; c += CANDIDATE_STEP)
                if (counts[SHAPE_TO_STR_UNCACHED][c] &&
                    scores[SHAPE_TO_STR_UNCACHED][c] / counts[SHAPE_TO_STR_UNCACHED][c] <
                        scores[SHAPE_TO_STR_UNCACHED][best] / counts[SHAPE_TO_STR_UNCACHED][best])
                    best = c;
            printf("# current=%llu\n", (unsigned long long)saved_threshold);
            printf("recommended TO_STR_BASEPOW_THRESHOLD = %d (primary=to_str_uncached)\n", best);
        }
    }
    lmmp_tune_TO_STR_BASEPOW_THRESHOLD = saved_threshold;
    lmmp_free(digits_low);
    lmmp_free(digits_high);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
