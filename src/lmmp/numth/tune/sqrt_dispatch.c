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

/* SQRT_INVNEWTON_K_THRESHOLD：lmmp_sqrt_ 根分派 !dstr && nf >= K*na 时走牛顿
   逆平方根（high），否则走分治除法（low）。分派只读一次、两后端内部都不再读
   该阈值 ⟹ 部署路径恒等于两条分支之一。本程序把分派的两个分支提炼进来直接
   调用（不经 lmmp_sqrt_ 入口）：high = lmmp_sqrt_newton_，low = 入口 else 分支
   的归一化 + lmmp_sqrt_divide_ + 还原移位（工作区在计时外预分配，仅少一次
   临时内存分配）；每个网格点另以 K=0 / K=巨大 绑定走完整入口，校验提炼分支
   与部署路径逐 limb 一致。网格 na 列 × 整数比例行 r=nf/na=1..R_MAX，逐点配对
   测量后按部署选择式 nf >= K*na ? high : low 对整数候选 K 评分。oracle：low
   结果过 floor 恒等式 s^2 <= numa*B^(2nf) < (s+1)^2，high 属于 {s, s+1}。
   SQRT_NEWTON_MODM_THRESHOLD 与梅森内部门保持默认。 */
enum {
    NA_COUNT = 20,
    R_MAX = 48,                             /* 比例行 r=1..48 */
    CANDIDATE_MAX = R_MAX,                  /* 候选整数 K=1..R_MAX */
    MAX_NA = 1024,
    MAX_NL = MAX_NA * (1 + 2 * R_MAX),      /* 99328 */
    MAX_NS = MAX_NL / 2 + 2,                /* nf+na/2+1 的上界 */
    NEVER_K = 1 << 20                       /* 绑定后 nf >= K*na 恒假 */
};
static const mp_size_t na_values[NA_COUNT] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32,
                                              48, 64, 96, 128, 192, 256, 384, 512, 768, 1024};

typedef struct {
    mp_ptr dst;
    mp_srcptr numa;
    mp_size_t na, nf, ns;
    mp_ptr work;
    int newton;
} arm_context;

typedef struct {
    mp_ptr dst_low, dst_high, dst_entry, numa, work;
    /* oracle 工作区 */
    mp_ptr sq, t, u, big, sa, sb;
} buffers;

/* lmmp_sqrt_ 的 else 分支（dstr=NULL） */
static void arm_divide(mp_ptr dsts, mp_srcptr numa, mp_size_t na, mp_size_t nf, mp_ptr work) {
    mp_size_t nl = na + 2 * nf;
    int nsh = lmmp_leading_zeros_(numa[na - 1]) / 2;
    mp_size_t ns = (nl + 1) / 2;
    mp_ptr numa2 = work, tp = work + 2 * ns;
    lmmp_zero(numa2, 2 * nf);
    lmmp_shl_(numa2 + 2 * ns - na, numa, na, nsh * 2);
    if (nl & 1) {
        numa2[2 * nf] = 0;
        nsh += LIMB_BITS / 2;
    } else {
        dsts[ns] = 0;
    }
    lmmp_sqrt_divide_(dsts, numa2, ns, tp, 0);
    if (nsh)
        lmmp_shr_(dsts, dsts, ns, nsh);
}

static void bind_never(void* opaque) {
    (void)opaque;
    lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD = NEVER_K;
}

static void bind_always(void* opaque) {
    (void)opaque;
    lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD = 0;
}

static void reset_output(void* opaque) {
    arm_context* context = opaque;
    lmmp_zero(context->dst, context->ns);
}

static mp_limb_t call_arm(void* opaque) {
    arm_context* context = opaque;
    if (context->newton)
        lmmp_sqrt_newton_(context->dst, context->numa, context->na, context->nf);
    else
        arm_divide(context->dst, context->numa, context->na, context->nf, context->work);
    return context->dst[0] ^ context->dst[context->ns - 1];
}

/* floor 恒等式：s^2 <= numa*B^(2nf) < (s+1)^2（s 取归一化长度） */
static int check_floor(const buffers* b, mp_srcptr s, mp_size_t ns, mp_srcptr numa, mp_size_t na, mp_size_t nf) {
    mp_size_t rn = ns;
    while (rn > 0 && s[rn - 1] == 0) --rn;
    if (rn == 0)
        return 0;
    mp_size_t nl = na + 2 * nf;
    mp_size_t l = 2 * rn + 2 > nl ? 2 * rn + 2 : nl;
    lmmp_sqr_(b->sq, s, rn);
    lmmp_zero(b->t, 2 * rn + 2);
    lmmp_copy(b->t, b->sq, 2 * rn);
    b->u[rn] = lmmp_shl_(b->u, s, rn, 1);
    lmmp_add_(b->t, b->t, 2 * rn, b->u, rn + 1);
    lmmp_add_1_(b->t, b->t, 2 * rn + 2, 1);
    lmmp_zero(b->big, l);
    lmmp_copy(b->big + 2 * nf, numa, na);
    lmmp_zero(b->sa, l);
    lmmp_zero(b->sb, l);
    lmmp_copy(b->sa, b->sq, 2 * rn);
    lmmp_copy(b->sb, b->big, l);
    if (lmmp_cmp_(b->sa, b->sb, l) > 0)
        return 0;
    lmmp_zero(b->sa, l);
    lmmp_zero(b->sb, l);
    lmmp_copy(b->sa, b->big, l);
    lmmp_copy(b->sb, b->t, 2 * rn + 2);
    return lmmp_cmp_(b->sa, b->sb, l) < 0;
}

/* high ∈ {low, low+1}（逐 limb 比较同长度 ns） */
static int check_rounded(const buffers* b, mp_srcptr high, mp_srcptr low, mp_size_t ns) {
    if (lmmp_cmp_(high, low, ns) == 0)
        return 1;
    lmmp_copy(b->t, low, ns);
    lmmp_add_1_(b->t, b->t, ns, 1);
    return lmmp_cmp_(high, b->t, ns) == 0;
}

static void print_info(void) {
    printf("program=tune_numth_sqrt_dispatch\n");
    printf("parameter=SQRT_INVNEWTON_K_THRESHOLD (integer slope, nf >= K*na selects newton)\n");
    printf("consumer=lmmp_sqrt_ root dispatch (dstr==NULL): lmmp_sqrt_newton_ vs lmmp_sqrt_divide_ arm\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)SQRT_INVNEWTON_K_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD);
    printf("input=na {1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024} x integer ratio r=nf/na 1..%d\n",
           R_MAX);
    printf("arms=extracted dispatch branches timed directly (work buffer preallocated); entry cross-check via K=0/K=never\n");
    printf("scoring=mean relative excess over per-point best arm (cost/min(low,high)-1)\n");
    printf("candidate_range=1..%d step=1\n", R_MAX);
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_sqrt_dispatch");
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
        lmmp_tune_print_current("SQRT_INVNEWTON_K_THRESHOLD", (unsigned long long)SQRT_INVNEWTON_K_THRESHOLD,
                                (unsigned long long)lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    /* 工作区总长：numa + 三个结果 + divide 分支 (2ns + 3ns/2 + 1) + oracle */
    const mp_size_t l_oracle = 2 * MAX_NS + 2;
    mp_ptr storage = alloc_limbs(MAX_NA + 3 * (MAX_NS + 2) + (7 * MAX_NS / 2 + 2) + 2 * (2 * MAX_NS + 4) +
                                 (MAX_NS + 4) + 3 * l_oracle);
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    buffers b;
    b.numa = storage;
    b.dst_low = storage + MAX_NA;
    b.dst_high = b.dst_low + MAX_NS + 2;
    b.dst_entry = b.dst_high + MAX_NS + 2;
    b.work = b.dst_entry + MAX_NS + 2;
    b.sq = b.work + 7 * MAX_NS / 2 + 2;
    b.t = b.sq + 2 * MAX_NS + 4;
    b.u = b.t + 2 * MAX_NS + 4;
    b.big = b.u + MAX_NS + 4;
    b.sa = b.big + l_oracle;
    b.sb = b.sa + l_oracle;

    double excess[CANDIDATE_MAX + 1] = {0}, abs_sum[CANDIDATE_MAX + 1] = {0};
    unsigned long counts[CANDIDATE_MAX + 1] = {0};
    const mp_size_t saved_k = lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD;
    unsigned stable_points = 0, unstable_points = 0;
    double column_ratio[NA_COUNT][R_MAX + 1] = {{0}};  /* 逐列各行 high/low；0 表示该行未测 */
    int column_rows[NA_COUNT] = {0};

    printf("# program=tune_numth_sqrt_dispatch\n");
    printf("# parameter=SQRT_INVNEWTON_K_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)SQRT_INVNEWTON_K_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# low=divide arm high=newton arm (extracted from lmmp_sqrt_, timed directly)\n");
    lmmp_tune_print_env_comment();
    printf("# na nf r divide_us divide_mad newton_us newton_mad ratio retries stable\n");

    int result = EXIT_SUCCESS;
    for (int na_index = 0; na_index < NA_COUNT && result == EXIT_SUCCESS; ++na_index) {
        const mp_size_t na = na_values[na_index];
        for (mp_size_t r = 1; r <= R_MAX && result == EXIT_SUCCESS; ++r) {
            const mp_size_t nf = r * na, nl = na + 2 * nf, ns = nf + na / 2 + 1;
            if (nl <= 4 || nf < 2)
                continue;  /* 入口基例或 newton 契约外，任意 K 同路 */
            fill_random(b.numa, na, (mp_limb_t)(options.seed + 8 * nf + na));
            /* 三种顶 limb 形态覆盖归一化移位 nsh=0/1/16 */
            if ((na_index + r) % 3 == 1)
                b.numa[na - 1] >>= 2;
            else if ((na_index + r) % 3 == 2)
                b.numa[na - 1] >>= 32;

            /* 提炼分支各跑一次 + oracle + 完整入口对照 */
            arm_context low_ctx = {.dst = b.dst_low, .numa = b.numa, .na = na, .nf = nf, .ns = ns, .work = b.work,
                                   .newton = 0};
            arm_context high_ctx = low_ctx;
            high_ctx.dst = b.dst_high;
            high_ctx.newton = 1;
            lmmp_zero(b.dst_low, ns);
            lmmp_zero(b.dst_high, ns);
            arm_divide(b.dst_low, b.numa, na, nf, b.work);
            lmmp_sqrt_newton_(b.dst_high, b.numa, na, nf);
            bind_never(NULL);
            lmmp_zero(b.dst_entry, ns);
            lmmp_sqrt_(b.dst_entry, NULL, b.numa, na, nf);
            int ok = lmmp_cmp_(b.dst_entry, b.dst_low, ns) == 0;
            bind_always(NULL);
            lmmp_zero(b.dst_entry, ns);
            lmmp_sqrt_(b.dst_entry, NULL, b.numa, na, nf);
            ok = ok && lmmp_cmp_(b.dst_entry, b.dst_high, ns) == 0;
            ok = ok && check_floor(&b, b.dst_low, ns, b.numa, na, nf) && check_rounded(&b, b.dst_high, b.dst_low, ns);
            lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD = saved_k;
            if (!ok) {
                fprintf(stderr, "sqrt dispatch arm mismatch at na=%llu nf=%llu\n", (unsigned long long)na,
                        (unsigned long long)nf);
                result = EXIT_FAILURE;
                break;
            }

            lmmp_tune_bench low_bench = {
                .bind = bind_never, .reset = reset_output, .call = call_arm, .context = &low_ctx};
            lmmp_tune_bench high_bench = {
                .bind = bind_always, .reset = reset_output, .call = call_arm, .context = &high_ctx};
            double low_us, low_mad, high_us, high_mad;
            unsigned retries;
            bool stable;
            if (!lmmp_tune_measure_pair(&low_bench, &high_bench, &options, &low_us, &low_mad, &high_us, &high_mad,
                                        &retries, &stable)) {
                fprintf(stderr, "measurement failed at na=%llu nf=%llu\n", (unsigned long long)na,
                        (unsigned long long)nf);
                result = EXIT_FAILURE;
                break;
            }
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD = saved_k;
            printf("%llu %llu %llu %.6f %.6f %.6f %.6f %.6f %u %u\n", (unsigned long long)na,
                   (unsigned long long)nf, (unsigned long long)r, low_us, low_mad, high_us, high_mad, high_us / low_us,
                   retries, stable);
            column_ratio[na_index][r] = high_us / low_us;
            column_rows[na_index] = (int)r;

            /* 部署评分：nf >= K*na（整数 r 即 r >= K）走 newton */
            const double best = low_us < high_us ? low_us : high_us;
            for (int k = 1; k <= CANDIDATE_MAX; ++k) {
                const double cost = nf >= (mp_size_t)k * na ? high_us : low_us;
                excess[k] += cost / best - 1.0;
                abs_sum[k] += cost;
                ++counts[k];
            }
        }
    }

    if (result == EXIT_SUCCESS) {
        if (stable_points == 0) {
            printf("# no stable measurement points; recommendation withheld\n");
        } else {
            if (unstable_points > 0)
                printf("# note: %u unstable point(s) retained in scoring\n", unstable_points);
            /* 逐列交叉：首个 newton 严格更快的 r，与其后恒不慢的最小 r */
            for (int na_index = 0; na_index < NA_COUNT; ++na_index) {
                int first_win = 0, stable_from = 0;
                for (int r = 1; r <= column_rows[na_index]; ++r) {
                    if (column_ratio[na_index][r] == 0.0)
                        continue;
                    if (first_win == 0 && column_ratio[na_index][r] < 1.0)
                        first_win = r;
                }
                for (int r = column_rows[na_index]; r >= 1; --r) {
                    if (column_ratio[na_index][r] == 0.0)
                        continue;
                    if (column_ratio[na_index][r] <= 1.0)
                        stable_from = r;  /* 自顶向下连续不慢段的下端 */
                    else
                        break;
                }
                printf("# crossover na=%llu first_win_r=%d stable_from_r=%d ratio_at_rmax=%.4f\n",
                       (unsigned long long)na_values[na_index], first_win, stable_from,
                       column_ratio[na_index][column_rows[na_index]]);
            }
            /* 汇总：相对超出（相对逐点最优分支），并列出平坦带 */
            int best = 1;
            for (int k = 2; k <= CANDIDATE_MAX; ++k)
                if (counts[k] && excess[k] / counts[k] < excess[best] / counts[best])
                    best = k;
            const double best_excess = excess[best] / counts[best];
            int flat_low = best, flat_high = best;
            while (flat_low > 1 && excess[flat_low - 1] / counts[flat_low - 1] <= best_excess + 0.01)
                --flat_low;
            while (flat_high < CANDIDATE_MAX && excess[flat_high + 1] / counts[flat_high + 1] <= best_excess + 0.01)
                ++flat_high;
            for (int k = 1; k <= CANDIDATE_MAX; ++k)
                printf("# badness candidate=%d excess=%.6f avg_us=%.6f\n", k, excess[k] / counts[k],
                       abs_sum[k] / counts[k]);
            const mp_size_t current = saved_k;
            printf("# recommended=%d recommended_excess=%.6f\n", best, best_excess);
            printf("# flat_within_1pct_excess=%d..%d\n", flat_low, flat_high);
            if (current >= 1 && current <= CANDIDATE_MAX && counts[current])
                printf("# current=%llu current_excess=%.6f current_avg_us=%.6f\n", (unsigned long long)current,
                       excess[current] / counts[current], abs_sum[current] / counts[current]);
            else
                printf("# current=%llu off-grid\n", (unsigned long long)current);
            printf("# points=%lu stable=%u unstable=%u\n", counts[best], stable_points, unstable_points);
            printf("recommended SQRT_INVNEWTON_K_THRESHOLD = %d\n", best);
        }
    }
    lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD = saved_k;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
