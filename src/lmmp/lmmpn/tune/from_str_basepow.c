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

/* 字符串→limb 入口门：FROM_STR_BASEPOW_THRESHOLD 只在 from_str_/from_decimal_
   入口读一次（结果 limb 估计值 <T 走 basecase，否则进幂表分治），不在递归内，
   绑 T=n 强制双路对任意 n 干净（sub_n=0），无 deployed 扫描。
   该阈值由两个成本结构不同的消费方共享：from_str_（任意基数）每次调用重建
   幂表（无缓存语境，主评分形状），from_decimal_ 用全局缓存十进制幂表（缓存
   语境，副评分形状，高路更廉、最优阈值更低）。两形状独立评分，主推荐以
   无缓存形状为准，缓存语境推荐值另行输出供写回时联合裁决。 */
enum {
    MIN_N = 8, MAX_N = 512, STEP_N = 4, CANDIDATE_MIN = MIN_N, CANDIDATE_MAX = MAX_N, CANDIDATE_STEP = 4,
    CANDIDATE_LAST = CANDIDATE_MIN + (CANDIDATE_MAX - CANDIDATE_MIN) / CANDIDATE_STEP * CANDIDATE_STEP
};

enum { SHAPE_DEC_CACHED = 0, SHAPE_FROM_STR_UNCACHED = 1, SHAPE_COUNT = 2 };

typedef struct {
    mp_ptr out;
    const mp_byte_t* src;   /* 字节位值数字串（basecase 与 from_str_ 用） */
    const char* src_chars;  /* 同一数字串的 ASCII 形式（from_decimal_ 用） */
    mp_size_t len;
    int shape;
    mp_size_t bind_threshold;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_FROM_STR_BASEPOW_THRESHOLD = context->bind_threshold;
}

/* 输入只读、输出全写：批次前仅触碰输出首 limb */
static void reset_outputs(void* opaque) {
    measure_context* context = opaque;
    context->out[0] = 0;
}

static mp_limb_t call_basecase(void* opaque) {
    measure_context* context = opaque;
    mp_size_t limbs = lmmp_from_str_basecase_(context->out, context->src, context->len, DEC_BASE, 0);
    return (mp_limb_t)limbs ^ context->out[0] ^ context->out[limbs - 1];
}

static mp_limb_t call_entry(void* opaque) {
    measure_context* context = opaque;
    mp_size_t limbs;
    if (context->shape == SHAPE_FROM_STR_UNCACHED)
        limbs = lmmp_from_str_(context->out, context->src, context->len, DEC_BASE);
    else
        limbs = lmmp_from_decimal_(context->out, context->src_chars, context->len);
    return (mp_limb_t)limbs ^ context->out[0] ^ context->out[limbs - 1];
}

/* 双路互比：limb 数与内容全等（high 绑 T=n 强制入口走幂表分治） */
static mp_size_t check_pair(measure_context* low, measure_context* high) {
    mp_size_t saved = lmmp_tune_FROM_STR_BASEPOW_THRESHOLD;
    mp_size_t limbs_low = lmmp_from_str_basecase_(low->out, low->src, low->len, DEC_BASE, 0);
    lmmp_tune_FROM_STR_BASEPOW_THRESHOLD = high->bind_threshold;
    mp_size_t limbs_high;
    if (high->shape == SHAPE_FROM_STR_UNCACHED)
        limbs_high = lmmp_from_str_(high->out, high->src, high->len, DEC_BASE);
    else
        limbs_high = lmmp_from_decimal_(high->out, high->src_chars, high->len);
    lmmp_tune_FROM_STR_BASEPOW_THRESHOLD = saved;
    if (limbs_low != limbs_high || lmmp_cmp_(low->out, high->out, limbs_low) != 0)
        return (mp_size_t)-1;
    return limbs_low;
}

/* 往返锚点：用对侧 to_decimal_ 转回数字串，须与源串逐位相等（首位非零已保证） */
static int check_roundtrip(const mp_byte_t* src, mp_size_t len, mp_srcptr limbs_buf, mp_size_t limbs, char* rt) {
    mp_size_t digits = lmmp_to_decimal_(rt, limbs_buf, limbs);
    if (digits != len)
        return 0;
    for (mp_size_t i = 0; i < len; ++i)
        if (rt[i] != (char)('0' + src[i]))
            return 0;
    return 1;
}

/* 候选按步长稀疏，公共 recommend 的逐整数遍历不适用，自行汇总 */
static void recommend_stepped(const char* label, const char* name, const double* scores, const unsigned long* counts,
                              int cmin, int cmax, int cstep, mp_size_t current) {
    int best = cmin;
    for (int c = cmin + cstep; c <= cmax; c += cstep)
        if (counts[c] && scores[c] / counts[c] < scores[best] / counts[best])
            best = c;
    double best_score = scores[best] / counts[best];
    int flat_low = best, flat_high = best;
    while (flat_low - cstep >= cmin && scores[flat_low - cstep] / counts[flat_low - cstep] <= best_score * 1.01)
        flat_low -= cstep;
    while (flat_high + cstep <= cmax && scores[flat_high + cstep] / counts[flat_high + cstep] <= best_score * 1.01)
        flat_high += cstep;
    for (int c = cmin; c <= cmax; c += cstep)
        if (counts[c])
            printf("# badness shape=%s candidate=%d avg_us=%.6f\n", label, c, scores[c] / counts[c]);
    printf("# recommended shape=%s value=%d avg_us=%.6f\n", label, best, best_score);
    printf("# flat_within_1pct shape=%s %d..%d\n", label, flat_low, flat_high);
    printf("# points shape=%s %lu\n", label, counts[best]);
    (void)name;
    (void)current;
}

static void print_info(void) {
    printf("program=tune_lmmpn_from_str_basepow\n");
    printf("parameter=FROM_STR_BASEPOW_THRESHOLD\n");
    printf("consumer=lmmp_from_str_ / lmmp_from_decimal_ entry gates\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)FROM_STR_BASEPOW_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_FROM_STR_BASEPOW_THRESHOLD);
    printf("candidate_range=%d..%d step=%d (scan upper bound %d)\n", CANDIDATE_MIN, CANDIDATE_LAST, CANDIDATE_STEP,
           CANDIDATE_MAX);
    printf("input_range=decimal digits, result limb estimate n %d..%d step=%d\n", MIN_N, MAX_N, STEP_N);
    printf("forced_root_equivalence=all_n (gate read once at entry, sub_n=0)\n");
    printf("primary_shape=from_str_uncached (per-call power table)\n");
    printf("secondary_shape=decimal_cached (global cached power table)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_lmmpn_from_str_basepow");
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
        lmmp_tune_print_current("FROM_STR_BASEPOW_THRESHOLD", (unsigned long long)FROM_STR_BASEPOW_THRESHOLD,
                                (unsigned long long)lmmp_tune_FROM_STR_BASEPOW_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    /* 最大数字串长度：MAX_N limb 对应的十进制位数上取齐 */
    mp_size_t max_len = 0;
    while (lmmp_from_str_len_(0, max_len, DEC_BASE) <= MAX_N) ++max_len;
    max_len += 2;

    mp_ptr storage = alloc_limbs(3 * (MAX_N + 4));
    mp_byte_t* src = lmmp_alloc(max_len);
    char* src_chars = lmmp_alloc(max_len + 2);
    char* rt = lmmp_alloc(max_len + 2);
    double scores[SHAPE_COUNT][CANDIDATE_MAX + 1] = {{0}};
    unsigned long counts[SHAPE_COUNT][CANDIDATE_MAX + 1] = {{0}};
    mp_size_t saved_threshold = lmmp_tune_FROM_STR_BASEPOW_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_lmmpn_from_str_basepow\n");
    printf("# parameter=FROM_STR_BASEPOW_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)FROM_STR_BASEPOW_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# candidate_range=%d..%d step=%d (scan upper bound %d) input=result limb estimate n %d..%d step=%d\n",
           CANDIDATE_MIN, CANDIDATE_LAST, CANDIDATE_STEP, CANDIDATE_MAX, MIN_N, MAX_N, STEP_N);
    printf("# high=complete entry (from_str_ uncached / from_decimal_ cached) with T=n; sub_n=0 no deployed scan\n");
    printf("# primary=from_str_uncached; secondary=decimal_cached\n");
    lmmp_tune_print_env_comment();
    printf("# shape n low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    mp_size_t len_probe = 1;
    for (mp_size_t n = MIN_N; n <= MAX_N && result == EXIT_SUCCESS; n += STEP_N) {
        while (lmmp_from_str_len_(0, len_probe, DEC_BASE) < n) ++len_probe;
        if (lmmp_from_str_len_(0, len_probe, DEC_BASE) != n)
            continue; /* 估计值跳过该整数（步进 0/1，理论上不发生） */
        mp_size_t len = len_probe;

        /* 随机数字串，最高位非零防入口裁剪；字节值与 ASCII 两份同值 */
        mp_limb_t seed = options.seed + 2 * n;
        for (mp_size_t i = 0; i < len; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            src[i] = (mp_byte_t)(seed % 10);
            src_chars[i] = (char)('0' + src[i]);
        }
        if (src[len - 1] == 0) {
            src[len - 1] = 1;
            src_chars[len - 1] = '1';
        }

        for (int shape = 0; shape < SHAPE_COUNT; ++shape) {
            mp_ptr low_out = storage;
            mp_ptr high_out = storage + MAX_N + 4;
            measure_context low_context = {.out = low_out,
                                           .src = src,
                                           .src_chars = src_chars,
                                           .len = len,
                                           .shape = shape,
                                           .bind_threshold = saved_threshold};
            measure_context high_context = {.out = high_out,
                                            .src = src,
                                            .src_chars = src_chars,
                                            .len = len,
                                            .shape = shape,
                                            .bind_threshold = n};
            mp_size_t limbs = check_pair(&low_context, &high_context);
            if (limbs == (mp_size_t)-1) {
                fprintf(stderr, "from_str basepow pair mismatch at n=%llu shape=%d\n", (unsigned long long)n, shape);
                result = EXIT_FAILURE;
                break;
            }
            if (!check_roundtrip(src, len, low_out, limbs, rt)) {
                fprintf(stderr, "from_str roundtrip mismatch at n=%llu shape=%d\n", (unsigned long long)n, shape);
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
                fprintf(stderr, "measurement failed at n=%llu shape=%d\n", (unsigned long long)n, shape);
                result = EXIT_FAILURE;
                break;
            }
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("%s %llu %.6f %.6f %.6f %.6f %u %u\n", shape == SHAPE_FROM_STR_UNCACHED ? "from_str" : "dec",
                   (unsigned long long)n, low_us, low_mad, high_us, high_mad, retries, stable);
            /* 入口门无同阈值递归：全部候选按 n 与候选的相对位置直接计分 */
            for (int c = CANDIDATE_MIN; c <= CANDIDATE_MAX; c += CANDIDATE_STEP) {
                scores[shape][c] += n >= (mp_size_t)c ? high_us : low_us;
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
            recommend_stepped("from_str_uncached", "FROM_STR_BASEPOW_THRESHOLD", scores[SHAPE_FROM_STR_UNCACHED],
                              counts[SHAPE_FROM_STR_UNCACHED], CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP,
                              saved_threshold);
            recommend_stepped("decimal_cached", "FROM_STR_BASEPOW_THRESHOLD", scores[SHAPE_DEC_CACHED],
                              counts[SHAPE_DEC_CACHED], CANDIDATE_MIN, CANDIDATE_MAX, CANDIDATE_STEP,
                              saved_threshold);
            /* 主推荐行：无缓存语境（保守端），缓存语境推荐值见上 */
            int best = CANDIDATE_MIN;
            for (int c = CANDIDATE_MIN + CANDIDATE_STEP; c <= CANDIDATE_MAX; c += CANDIDATE_STEP)
                if (counts[SHAPE_FROM_STR_UNCACHED][c] &&
                    scores[SHAPE_FROM_STR_UNCACHED][c] / counts[SHAPE_FROM_STR_UNCACHED][c] <
                        scores[SHAPE_FROM_STR_UNCACHED][best] / counts[SHAPE_FROM_STR_UNCACHED][best])
                    best = c;
            printf("# current=%llu\n", (unsigned long long)saved_threshold);
            printf("recommended FROM_STR_BASEPOW_THRESHOLD = %d (primary=from_str_uncached)\n", best);
        }
    }
    lmmp_tune_FROM_STR_BASEPOW_THRESHOLD = saved_threshold;
    lmmp_free(src);
    lmmp_free(src_chars);
    lmmp_free(rt);
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
