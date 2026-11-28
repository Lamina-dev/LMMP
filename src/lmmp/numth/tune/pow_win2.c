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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "mparam.h"
#include "support/perf.h"
#include "tune/common.h"
#include "tune/params.h"

/* win2 幂的双阈值判别条件（根层，n>=2、exp>2）。win2 只对奇指数可达，且需同时
   满足形状与两个阈值：
     (exp & 1) && exp > POW_WIN2_EXP_THRESHOLD && n > POW_WIN2_N_THRESHOLD
              && 2*popcnt(exp) >= bits(exp)
   满足则走 lmmp_pow_win2_；奇指数但不满足上式走 lmmp_pow_basecase_；偶指数走
   "奇部 basecase + 平方链"，与两阈值无关（恒不可达 win2）。
   两个阈值都只在 lmmp_pow_ 根层各读一次，win2_/basecase_ 内部用 sqr_/mul_ 的
   独立阈值，不引用本二者，所以强制绑定对任意 (n,exp) 干净，sub_n=0、无 deployed
   扫描；但两阈值互相决定对方是否可达，必须联合测（单轴固定对方默认值会在对方
   阈值之外整段不可达）。

   每点配对（仅在 win2 可达的奇指数点测量）：
     low  = basecase，绑 POW_WIN2_EXP_THRESHOLD=exp（exp>exp 恒假）；
     high = win2，绑 POW_WIN2_EXP_THRESHOLD=exp-1、POW_WIN2_N_THRESHOLD=n-1
            （exp>exp-1 且 n>n-1 恒真，形状已满足）。
   偶指数与"奇指数但 popcnt 不足"的点部署恒走非 win2，两阈值无部署差异，直接跳过。
   两路都经完整入口 lmmp_pow_，对 base 只读、dst 分离，无需副本池。
   oracle=两路结果逐 limb 全等。

   评分（dispatch 族口径）：稳定点上"候选 (te,tn) 所选路径耗时 ÷ 较快路径耗时 - 1"
   的均值，二维 argmin 为推荐值；候选选择 win2 当且仅当 exp>te && n>tn（测量点已
   保证 exp 奇且 popcnt 满足）。不稳定点不计分。 */

enum {
    N_COUNT = 11,
    MAX_N = 1024,
    MAX_EXP = 256,
    /* 结果 limb 上界：bits(base) <= 64n，bits(result) <= exp*64n，limb <= exp*n，+2 富余 */
    MAX_RN = MAX_N * MAX_EXP + 2,
    TN_COUNT = 12,
    TIME_BUDGET_MS = 60000 /* 单程序时间预算（用户裁决：约 1 分钟，加硬超时） */
};

static const mp_size_t n_grid[N_COUNT] = {2, 8, 32, 64, 128, 256, 384, 512, 640, 768, 1024};
/* 全部为奇指数（win2 仅奇指数可达）。含 2^k-1 满 popcnt 族与一般 popcnt 族，
   二者都满足 2*popcnt>=bits，覆盖不同窗口密度。 */
static const ulong exp_grid[] = {3, 7,  15, 31, 63, 95,  127, 159, 191, 223, 255,
                                 5, 13, 29, 53, 93, 125, 141, 173, 205, 237, 253};
enum { EXP_COUNT = ARRAY_SIZE(exp_grid), TE_COUNT = EXP_COUNT + 1 };
/* tn 候选逐一跨过 n 网格；384 与 512 之间无网格 n 的取值不单列。 */
static const double tn_cand[TN_COUNT] = {1, 2, 8, 32, 64, 128, 256, 384, 512, 640, 768, 1024};

/* win2 是否可达的指数形状判据（与 lmmp_pow_ 根层逐字一致：奇指数且 popcnt 条件） */
static int shape_ok(ulong exp) {
    return (exp & 1) && (2 * lmmp_limb_popcnt_(exp) >= lmmp_limb_bits_(exp));
}

typedef struct {
    mp_ptr dst;
    mp_srcptr base;
    mp_size_t n;
    ulong exp;
    mp_size_t cap;
    mp_size_t bind_te;
    mp_size_t bind_tn;
} measure_context;

static void bind_threshold(void* opaque) {
    measure_context* context = opaque;
    lmmp_tune_POW_WIN2_EXP_THRESHOLD = context->bind_te;
    lmmp_tune_POW_WIN2_N_THRESHOLD = context->bind_tn;
}

static void reset_output(void* opaque) {
    measure_context* context = opaque;
    lmmp_zero(context->dst, context->cap);
}

static mp_limb_t call_entry(void* opaque) {
    measure_context* context = opaque;
    mp_size_t rn = lmmp_pow_(context->dst, context->cap, context->base, context->n, context->exp);
    return context->dst[0] ^ context->dst[rn - 1] ^ context->base[0];
}

static mp_size_t run_forced(measure_context* context, mp_size_t te, mp_size_t tn) {
    context->bind_te = te;
    context->bind_tn = tn;
    bind_threshold(context);
    lmmp_zero(context->dst, context->cap);
    return lmmp_pow_(context->dst, context->cap, context->base, context->n, context->exp);
}

static void print_info(const double* te_cand, unsigned te_count) {
    printf("program=tune_numth_pow_win2\n");
    printf("parameters=POW_WIN2_EXP_THRESHOLD, POW_WIN2_N_THRESHOLD (joint root gate, n>=2)\n");
    printf("consumer=lmmp_pow_ root dispatch only (win2 vs basecase, odd exp)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default_exp=%llu default_n=%llu\n", (unsigned long long)POW_WIN2_EXP_THRESHOLD,
           (unsigned long long)POW_WIN2_N_THRESHOLD);
    printf("current_exp=%llu current_n=%llu\n", (unsigned long long)lmmp_tune_POW_WIN2_EXP_THRESHOLD,
           (unsigned long long)lmmp_tune_POW_WIN2_N_THRESHOLD);
    printf("candidates_exp=");
    for (unsigned i = 0; i < te_count; ++i)
        printf(i ? ",%.0f" : "%.0f", te_cand[i]);
    printf("\ncandidates_n=");
    for (int i = 0; i < TN_COUNT; ++i)
        printf(i ? ",%.0f" : "%.0f", tn_cand[i]);
    printf("\ninput=n x exp grid (odd exp only), shape-filtered (exp odd and 2*popcnt(exp)>=bits(exp))\n");
    printf("forced_root_equivalence=all (gates read once at root, win2/basecase use independent thresholds)\n");
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    /* 时间预算：大点单次调用即超过块长（iters=1），单点成本≈调用次数×单次耗时。
       308 点网格在 5 样本下约 61 s 超预算，按框架先例降为 3 样本×2ms（约 35-40 s），
       读数靠多轮合并。60 s 硬超时只作失败安全网：触发即报错退出，不出部分推荐。 */
    options.samples = 3;
    options.min_ms = 2.0;
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_pow_win2");
        return EXIT_SUCCESS;
    }

    /* exp > te 只在阈值到达一个可测指数时改变，从 1 开始覆盖每个区间。 */
    double te_cand[TE_COUNT] = {1};
    unsigned te_count = 1;
    for (int i = 0; i < EXP_COUNT; ++i) {
        if (shape_ok(exp_grid[i]) && !lmmp_tune_add_double(te_cand, &te_count, TE_COUNT, (double)exp_grid[i]))
            return EXIT_FAILURE;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    setvbuf(stdout, NULL, _IOLBF, 4096);
#if defined(LMMP_WINDOWS)
    SetProcessAffinityMask(GetCurrentProcess(), (DWORD_PTR)0xFFF);
#endif
    if (options.action == LMMP_TUNE_INFO) {
        print_info(te_cand, te_count);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_CURRENT) {
        lmmp_tune_print_current("POW_WIN2_EXP_THRESHOLD", (unsigned long long)POW_WIN2_EXP_THRESHOLD,
                                (unsigned long long)lmmp_tune_POW_WIN2_EXP_THRESHOLD);
        lmmp_tune_print_current("POW_WIN2_N_THRESHOLD", (unsigned long long)POW_WIN2_N_THRESHOLD,
                                (unsigned long long)lmmp_tune_POW_WIN2_N_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    mp_ptr storage = alloc_limbs(2 * MAX_RN + MAX_N + 4);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr dst_low = storage;
    mp_ptr dst_high = storage + MAX_RN;
    mp_ptr base_buf = storage + 2 * MAX_RN;
    static double scores[TE_COUNT * TN_COUNT];
    static unsigned long counts[TE_COUNT * TN_COUNT];
    const mp_size_t saved_te = lmmp_tune_POW_WIN2_EXP_THRESHOLD;
    const mp_size_t saved_tn = lmmp_tune_POW_WIN2_N_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0, skipped_shape = 0;

    printf("# program=tune_numth_pow_win2\n");
    printf("# parameters=POW_WIN2_EXP_THRESHOLD,POW_WIN2_N_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default_exp=%llu default_n=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)POW_WIN2_EXP_THRESHOLD, (unsigned long long)POW_WIN2_N_THRESHOLD, options.seed,
           options.samples, options.min_ms, options.mad_limit);
    printf("# low=basecase forced (POW_WIN2_EXP_THRESHOLD=exp); high=win2 forced (EXP=exp-1,N=n-1)\n");
    lmmp_tune_print_env_comment();
    printf("# n exp low_us low_mad high_us high_mad ratio retries stable\n");

    int result = EXIT_SUCCESS;
    const double deadline = bench_clock() + TIME_BUDGET_MS * 1e-3;
    for (int ni = 0; ni < N_COUNT && result == EXIT_SUCCESS; ++ni) {
        const mp_size_t n = n_grid[ni];
        fill_random(base_buf, n, (mp_limb_t)(options.seed + 7 * n));
        for (int ei = 0; ei < EXP_COUNT && result == EXIT_SUCCESS; ++ei) {
            if (bench_clock() > deadline) {
                fprintf(stderr, "time budget exceeded (%d ms); grid/samples must be reduced\n", TIME_BUDGET_MS);
                result = EXIT_FAILURE;
                break;
            }
            const ulong exp = exp_grid[ei];
            if (!shape_ok(exp)) {
                ++skipped_shape;
                continue;
            }
            const mp_size_t cap = lmmp_pow_size_(base_buf, n, exp);
            if (cap > MAX_RN) {
                fprintf(stderr, "cap overflow at n=%llu exp=%llu\n", (unsigned long long)n, (unsigned long long)exp);
                result = EXIT_FAILURE;
                break;
            }
            measure_context low_context = {
                .dst = dst_low, .base = base_buf, .n = n, .exp = exp, .cap = cap, .bind_te = exp, .bind_tn = n - 1};
            measure_context high_context = {
                .dst = dst_high, .base = base_buf, .n = n, .exp = exp, .cap = cap, .bind_te = exp - 1, .bind_tn = n - 1};

            mp_size_t rl = run_forced(&low_context, exp, n - 1);
            mp_size_t rh = run_forced(&high_context, exp - 1, n - 1);
            if (rl != rh || lmmp_cmp_(dst_low, dst_high, rl) != 0) {
                fprintf(stderr, "win2 result mismatch at n=%llu exp=%llu\n", (unsigned long long)n,
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
                fprintf(stderr, "measurement failed at n=%llu exp=%llu\n", (unsigned long long)n,
                        (unsigned long long)exp);
                result = EXIT_FAILURE;
                break;
            }
            printf("%llu %llu %.6f %.6f %.6f %.6f %.4f %u %u\n", (unsigned long long)n, (unsigned long long)exp, low_us,
                   low_mad, high_us, high_mad, high_us / low_us, retries, stable);
            if (!stable) {
                ++unstable_points;
                continue; /* 不稳定点不计分 */
            }
            ++stable_points;

            const double faster = low_us < high_us ? low_us : high_us;
            const double excess_low = low_us / faster - 1.0;
            const double excess_high = high_us / faster - 1.0;
            for (unsigned x = 0; x < te_count; ++x) {
                for (int y = 0; y < TN_COUNT; ++y) {
                    const int use_win2 = (exp > (ulong)te_cand[x]) && (n > (mp_size_t)tn_cand[y]);
                    scores[x * TN_COUNT + y] += use_win2 ? excess_high : excess_low;
                    ++counts[x * TN_COUNT + y];
                }
            }
        }
    }

    if (result == EXIT_SUCCESS && bench_clock() > deadline) {
        fprintf(stderr, "time budget exceeded (%d ms); grid/samples must be reduced\n", TIME_BUDGET_MS);
        result = EXIT_FAILURE;
    }
    if (result == EXIT_SUCCESS) {
        printf("# shape_skipped=%u stable=%u unstable=%u\n", skipped_shape, stable_points, unstable_points);
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            lmmp_tune_recommend_pair("POW_WIN2_EXP_THRESHOLD", "POW_WIN2_N_THRESHOLD", scores, counts, te_cand,
                                     te_count, tn_cand, TN_COUNT, (double)saved_te, (double)saved_tn);
        }
    }
    lmmp_tune_POW_WIN2_EXP_THRESHOLD = saved_te;
    lmmp_tune_POW_WIN2_N_THRESHOLD = saved_tn;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
