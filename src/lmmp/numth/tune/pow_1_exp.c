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

/* 单 limb 底数的幂指数阈值 POW_1_EXP_THRESHOLD：lmmp_pow_ 在 n==1、exp>2 时，
   exp<=POW_1_EXP_THRESHOLD 走逐次 mul_1_ 连乘，否则走 lmmp_pow_1_（按底数位宽
   分档的窗口法）。阈值只在根层读一次，pow_1_ 内部用 sqr_/mul_1_ 的独立阈值，
   不引用本阈值，所以强制绑定对任意 exp 干净，sub_n=0、无 deployed 扫描。
   每点配对：low 绑 POW_1_EXP_THRESHOLD=exp 强制连乘；high 绑
   POW_1_EXP_THRESHOLD=exp-1 强制 pow_1_。两路都经完整入口 lmmp_pow_，
   对 base 只读、dst 分离，无需副本池。oracle=两路结果逐 limb 全等。
   底数覆盖 u4/u8/u16/u32/u64 各档、纯 2 幂（pow_1_ 有置位捷径）与带尾零底数。
   评分：候选 c 逐点取 exp<=c ? low : high，等权平均；另给一份剔除纯 2 幂
   底数的评分（2/8 的 pow_1_ 是 O(exp/64) 置位，恒快，会把推荐值拉向极小）。 */

enum {
    EXP_FINE_MAX = 40, /* exp 3..40 步 1 */
    CAND_MIN = 2, CAND_MAX = 128,
    MAX_RN = 136, /* u64 底数 exp=128 的结果上界 + 富余 */
    BASE_COUNT = 12,
    EXP_TAIL_COUNT = 4,
    EXP_COUNT = (EXP_FINE_MAX - 3 + 1) + EXP_TAIL_COUNT,
    POINT_COUNT = BASE_COUNT * EXP_COUNT,
    TIME_BUDGET_MS = 60000 /* 单程序时间预算（用户裁决：约 1 分钟，加硬超时） */
};

static const mp_limb_t bases[BASE_COUNT] = {
    2, 3, 8, 15, 17, 255, 257, 65537, 0xFFFFFFFFul, 0x100000001ull, 0xFEDCBA9876543211ull, (3ull << 40)};
static const ulong exp_tail[EXP_TAIL_COUNT] = {48, 64, 96, 128};

/* 纯 2 幂底数：pow_1_ 走置位捷径，与一般底数不同类 */
static int is_pow2_base(mp_limb_t b) {
    return b == 2 || b == 8;
}

typedef struct {
    mp_ptr dst;
    mp_limb_t base;
    ulong exp;
    mp_size_t cap;
    mp_size_t bind_t;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_POW_1_EXP_THRESHOLD = context->bind_t;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->cap);
}

static mp_limb_t call_entry(void* opaque) {
    measure_context* context = opaque;
    mp_size_t rn = lmmp_pow_(context->dst, context->cap, &context->base, 1, context->exp);
    return context->dst[0] ^ context->dst[rn - 1] ^ context->base;
}

/* 按绑定强制跑完整入口，返回实际长度；两路互比用 */
static mp_size_t run_forced(measure_context* context, mp_size_t bind_t) {
    context->bind_t = bind_t;
    bind_threshold(context);
    lmmp_zero(context->dst, context->cap);
    return lmmp_pow_(context->dst, context->cap, &context->base, 1, context->exp);
}

static void print_info(void) {
    printf("program=tune_numth_pow_1_exp\n");
    printf("parameter=POW_1_EXP_THRESHOLD (root gate, n==1, exp>2)\n");
    printf("consumer=lmmp_pow_ root dispatch only (sequential mul_1 vs pow_1_)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)POW_1_EXP_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_POW_1_EXP_THRESHOLD);
    printf("candidates=%d..%d step=1\n", CAND_MIN, CAND_MAX);
    printf("input=base x exp: 12 bases (u4/u8/u16/u32/u64, pow2, trailing-zero) x exp 3..40 step1 + {48,64,96,128}\n");
    printf("forced_root_equivalence=all_exp (gate read once at root, pow_1_ uses independent thresholds)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    /* 时间预算：504 点全为亚微秒级调用，默认 5 样本×5ms 块超 60 s；
       按框架先例降为 3 样本×2ms（约 15-20 s），读数靠多轮合并。 */
    options.samples = 3;
    options.min_ms = 2.0;
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_pow_1_exp");
        return EXIT_SUCCESS;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    setvbuf(stdout, NULL, _IOLBF, 4096);
#if defined(LMMP_WINDOWS)
    SetProcessAffinityMask(GetCurrentProcess(), (DWORD_PTR)0xFFF);
#endif
    if (options.action == LMMP_TUNE_INFO) {
        print_info();
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current("POW_1_EXP_THRESHOLD", (unsigned long long)POW_1_EXP_THRESHOLD,
                                (unsigned long long)lmmp_tune_POW_1_EXP_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    mp_ptr storage = alloc_limbs(2 * MAX_RN + 4);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr dst_low = storage;
    mp_ptr dst_high = storage + MAX_RN + 2;
    double scores_all[CAND_MAX + 1] = {0};
    double scores_nopow2[CAND_MAX + 1] = {0};
    unsigned long counts[CAND_MAX + 1] = {0};
    const mp_size_t saved_t = lmmp_tune_POW_1_EXP_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;

    printf("# program=tune_numth_pow_1_exp\n");
    printf("# parameter=POW_1_EXP_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)POW_1_EXP_THRESHOLD, options.seed, options.samples, options.min_ms, options.mad_limit);
    printf("# low=sequential forced (POW_1_EXP_THRESHOLD=exp); high=pow_1_ forced (POW_1_EXP_THRESHOLD=exp-1)\n");
    lmmp_tune_print_env_comment();
    printf("# base exp low_us low_mad high_us high_mad retries stable\n");

    int result = EXIT_SUCCESS;
    const double deadline = bench_clock() + TIME_BUDGET_MS * 1e-3;
    for (int bi = 0; bi < BASE_COUNT && result == EXIT_SUCCESS; ++bi) {
        const mp_limb_t base = bases[bi];
        for (int ei = 0; ei < EXP_COUNT && result == EXIT_SUCCESS; ++ei) {
            if (bench_clock() > deadline) {
                fprintf(stderr, "time budget exceeded (%d ms); grid/samples must be reduced\n", TIME_BUDGET_MS);
                result = EXIT_FAILURE;
                break;
            }
            const ulong exp = ei < (EXP_FINE_MAX - 3 + 1) ? (ulong)(3 + ei) : exp_tail[ei - (EXP_FINE_MAX - 3 + 1)];
            const mp_size_t cap = lmmp_pow_size_(&base, 1, exp);
            if (cap > MAX_RN) {
                fprintf(stderr, "cap overflow at base=%llu exp=%llu\n", (unsigned long long)base,
                        (unsigned long long)exp);
                result = EXIT_FAILURE;
                break;
            }
            measure_context low_context = {.dst = dst_low, .base = base, .exp = exp, .cap = cap, .bind_t = exp};
            measure_context high_context = {.dst = dst_high, .base = base, .exp = exp, .cap = cap, .bind_t = exp - 1};

            mp_size_t rl = run_forced(&low_context, exp);
            mp_size_t rh = run_forced(&high_context, exp - 1);
            if (rl != rh || lmmp_cmp_(dst_low, dst_high, rl) != 0) {
                fprintf(stderr, "pow_1 result mismatch at base=%llu exp=%llu\n", (unsigned long long)base,
                        (unsigned long long)exp);
                result = EXIT_FAILURE;
                break;
            }

            lmmp_tune_bench low_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_entry, .context = &low_context};
            lmmp_tune_bench high_bench = {
                .bind = bind_threshold, .reset = reset_output, .call = call_entry, .context = &high_context};
            double low_us, low_mad, high_us, high_mad;
            unsigned retries;
            bool stable;
            if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                        &retries, &stable)) {
                fprintf(stderr, "measurement failed at base=%llu exp=%llu\n", (unsigned long long)base,
                        (unsigned long long)exp);
                result = EXIT_FAILURE;
                break;
            }
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            printf("%llu %llu %.6f %.6f %.6f %.6f %u %u\n", (unsigned long long)base, (unsigned long long)exp, low_us,
                   low_mad, high_us, high_mad, retries, stable);

            const int np2 = !is_pow2_base(base);
            for (int c = CAND_MIN; c <= CAND_MAX; ++c) {
                const double chosen = exp <= (ulong)c ? low_us : high_us;
                scores_all[c] += chosen;
                if (np2)
                    scores_nopow2[c] += chosen;
                ++counts[c];
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            printf("# === scoring over all bases ===\n");
            lmmp_tune_recommend("POW_1_EXP_THRESHOLD", scores_all, counts, CAND_MIN, CAND_MAX, saved_t);
            printf("# === scoring excluding pure power-of-2 bases (2,8) ===\n");
            lmmp_tune_recommend("POW_1_EXP_THRESHOLD_nopow2", scores_nopow2, counts, CAND_MIN, CAND_MAX, saved_t);
        }
    }
    lmmp_tune_POW_1_EXP_THRESHOLD = saved_t;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
