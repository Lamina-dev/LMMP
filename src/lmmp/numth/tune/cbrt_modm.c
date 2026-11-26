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

/* CBRT_NEWTON_MODM_THRESHOLD：lmmp_invcbrt_newton_ 每层残差链的阈值。层内先由
   cmod 门（mn+2T < 3nr+2 且 mn <= naz+3nr）决定整条链走梅森模乘还是精确截断，
   梅森链内 ir^2 与 ir^2*an 两个决策再受 T 与形状守卫约束，ir^2*an*ir 恒走梅森。
   阈值在每层内部读取且带形状守卫，经完整函数无法干净强制 ⟹ 分两级测量。

   A 层内直测（提炼分派）：按真实尺寸栈逐层构造操作数（ir 顶 limb 为 1、an 取
   被开方数最高 naz 个 limb），把该层的决策提炼成显式阈值的方案：0=精确截断链，
   1..4=梅森链 (p1,p2)=(ir^2 梅森?, ir^2*an 梅森?)，并复刻 lmmp_invcbrt_newton_
   的执行。候选 T 在一层内只会落到至多 5 种方案，每种可达方案与精确链基线配对
   测量一次；任意候选的 T 相关代价即各层方案耗时之和（dip 精确乘与线性步与 T
   无关）。每种方案先过残差恒等式：dp 与 an*ir^3 全积模 B^nres 或模 B^mn-1 一致。

   B 整函数配对 deployed 扫描：lmmp_invcbrt_newton_ 绑定候选 T 与绑定现值交替
   计时，读数是相对现值的比值（候选等于现值的一对即 A/A 噪声底）；每个候选的
   输出过逆立方根恒等式 a*ir^3 <= B^(3ns+na) < a*(ir+2)^3（ir ∈ {I-1, I}，
   I = floor(cbrt(B^(3ns+na)/a))）。推荐值取 B 在全体形状均稳定的候选上的
   平均比值 argmin，A 给出机理分解与预测作交叉诊断。梅森内部门保持默认。 */
enum {
    SHAPE_COUNT = 8,
    NR_MIN = 8,                              /* 只测 nr 不小于此值的层 */
    MAX_NS = 32768,
    MAX_NA = 768,
    MAX_LAYERS = 32,
    PLAN_COUNT = 5,                          /* 0=精确链；1..4=梅森链 (p1,p2) */
    CAND_STEP = 32,                          /* T 候选 0..1024 步 32 */
    CAND_GRID = 1024 / CAND_STEP + 1,
    CAND_MAX = CAND_GRID + 3,                /* 另加 1280、现值、NEVER */
    NEVER_T = 1 << 20,
    MAX_MN = 4 * (MAX_NA + MAX_NS + 16),     /* mersenne_next_size(nres) 的宽松上界 */
    MAX_N = 3 * MAX_NS + MAX_NA              /* 逆立方根恒等式的位幂指数 */
};
static const mp_size_t shape_na[SHAPE_COUNT] = {12, 12, 12, 48, 48, 192, 192, 768};
static const mp_size_t shape_ns[SHAPE_COUNT] = {512, 1024, 2048, 2048, 8192, 8192, 32768, 32768};

typedef struct {
    int cmod; /* 1=梅森链 */
    int p1, p2; /* 1=梅森 */
} layer_plan;

typedef struct {
    mp_ptr ir, an, t2, w1, dp, sq, wr, ref, fold;
    mp_size_t nr, naz, mn, nres;
    layer_plan plan;
} layer_context;

typedef struct {
    mp_ptr dstis, numa, sq, cube, cube1, ir1, p, q;
    mp_size_t na, ns, bind;
} whole_context;

/* 镜像 lmmp_invcbrt_newton_ 的层内决策（阈值 t 显式传入） */
static layer_plan plan_layer(mp_size_t nr, mp_size_t naz, mp_size_t t) {
    const mp_size_t mn = lmmp_mersenne_next_size_(naz + 2 * nr + 2);
    layer_plan plan = {0, 0, 0};
    if (!(mn + 2 * t < 3 * nr + 2 && mn <= naz + 3 * nr))
        return plan;  // 精确截断链
    plan.cmod = 1;
    plan.p1 = !(2 * t + mn >= 2 * nr + 1);
    const mp_size_t nsqr = plan.p1 ? mn : 2 * nr + 2;
    plan.p2 = !(naz < t || naz * 8 < nsqr || mn >= nsqr + naz);
    return plan;
}

static int plan_index(layer_plan plan) {
    return plan.cmod ? 1 + ((plan.p1 << 1) | plan.p2) : 0;
}

static layer_plan plan_from_index(int idx) {
    layer_plan plan = {0, 0, 0};
    if (idx > 0) {
        plan.cmod = 1;
        plan.p1 = ((idx - 1) >> 1) & 1;
        plan.p2 = (idx - 1) & 1;
    }
    return plan;
}

/* 复刻 lmmp_invcbrt_newton_ 单层残差链：dp = an*ir^3 的模表示 */
static void run_layer(const layer_context* c) {
    const mp_size_t nr = c->nr, naz = c->naz, mn = c->mn, nres = c->nres;
    mp_size_t nsqr;
    if (!c->plan.cmod) {
        /* ---- 精确截断链：低 nres limb ---- */
        lmmp_sqr_(c->t2, c->ir, nr + 1);
        lmmp_mul_(c->w1, c->t2, 2 * nr + 2, c->an, naz);
        lmmp_mul_(c->dp, c->w1, nres, c->ir, nr + 1);
        return;
    }
    /* ---- 梅森链 ---- */
    if (!c->plan.p1) {
        lmmp_sqr_(c->t2, c->ir, nr + 1);
        nsqr = 2 * nr + 2;
    } else {
        lmmp_sqr_mersenne_(c->t2, mn, c->ir, nr + 1);
        nsqr = mn;
    }
    if (!c->plan.p2) {
        lmmp_mul_(c->w1, c->t2, nsqr, c->an, naz);
        if (nsqr + naz > mn) {
            if (lmmp_add_(c->w1, c->w1, mn, c->w1 + mn, nsqr + naz - mn))
                lmmp_inc(c->w1);
        }
    } else {
        lmmp_mul_mersenne_(c->w1, mn, c->t2, nsqr, c->an, naz);
    }
    lmmp_mul_mersenne_(c->dp, mn, c->w1, mn, c->ir, nr + 1);
}

static void bind_noop(void* opaque) {
    (void)opaque;
}

static void reset_layer(void* opaque) {
    const layer_context* c = opaque;
    lmmp_zero(c->t2, 2 * c->nr + 2 > c->mn ? 2 * c->nr + 2 : c->mn);
    lmmp_zero(c->w1, c->mn + c->naz);
    lmmp_zero(c->dp, c->plan.cmod ? c->mn : c->nres + c->nr + 1);
}

static mp_limb_t call_layer(void* opaque) {
    const layer_context* c = opaque;
    run_layer(c);
    return c->dp[0] ^ c->dp[c->nres - 1];
}

/* 残差恒等式：dp 与 an*ir^3 的全积在各自表示（模 B^nres 或模 B^mn-1）下一致 */
static int check_layer(layer_context* c) {
    const mp_size_t nr = c->nr, naz = c->naz, mn = c->mn, nres = c->nres, l = nres + nr + 1;
    lmmp_sqr_(c->sq, c->ir, nr + 1);
    lmmp_mul_(c->wr, c->sq, 2 * nr + 2, c->an, naz); /* an*ir^2，[0,nres) */
    lmmp_mul_(c->ref, c->wr, nres, c->ir, nr + 1);    /* an*ir^3，[0,l) */
    reset_layer(c);
    run_layer(c);
    if (!c->plan.cmod)
        return lmmp_cmp_(c->dp, c->ref, nres) == 0;
    lmmp_copy(c->fold, c->ref, mn);
    if (lmmp_add_(c->fold, c->fold, mn, c->ref + mn, l - mn))
        lmmp_inc(c->fold);
    return lmmp_cmp_(c->dp, c->fold, mn) == 0;
}

static void bind_whole(void* opaque) {
    whole_context* c = opaque;
    lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD = c->bind;
}

static void reset_whole(void* opaque) {
    whole_context* c = opaque;
    lmmp_zero(c->dstis, c->ns + 1);
}

static mp_limb_t call_whole(void* opaque) {
    whole_context* c = opaque;
    lmmp_invcbrt_newton_(c->dstis, c->ns, c->numa, c->na);
    return c->dstis[0] ^ c->dstis[c->ns];
}

/* x^3：[sq,2len] = x^2，[cube,3len] = x^2*x */
static void cube_of(mp_ptr cube, mp_ptr sq, const mp_ptr x, mp_size_t len) {
    lmmp_sqr_(sq, x, len);
    lmmp_mul_(cube, sq, 2 * len, x, len);
}

/* 逆立方根恒等式：a*ir^3 <= K=B^(3ns+na) < a*(ir+2)^3，接受 ir ∈ {I-1, I}，
   拒绝 I+1（下界失败）与 I-2（上界失败） */
static int check_invcbrt(const whole_context* c) {
    const mp_size_t na = c->na, ns = c->ns, big = 3 * ns + na;
    cube_of(c->cube, c->sq, c->dstis, ns + 1);
    lmmp_mul_(c->p, c->cube, 3 * ns + 3, c->numa, na); /* a*ir^3，[0,big+3) */
    for (mp_size_t i = big; i < big + 3; ++i)
        if (c->p[i] != 0)
            return 0;
    lmmp_copy(c->ir1, c->dstis, ns + 1);
    lmmp_add_1_(c->ir1, c->ir1, ns + 1, 1);
    c->ir1[ns + 1] = lmmp_add_1_(c->ir1, c->ir1, ns + 1, 1); /* ir+2 */
    cube_of(c->cube1, c->sq, c->ir1, ns + 2);
    lmmp_mul_(c->q, c->cube1, 3 * ns + 6, c->numa, na); /* a*(ir+2)^3，[0,big+6) */
    for (mp_size_t i = big; i < big + 6; ++i)
        if (c->q[i] != 0)
            return 1;
    return 0;
}

static void print_info(void) {
    printf("program=tune_numth_cbrt_modm\n");
    printf("parameter=CBRT_NEWTON_MODM_THRESHOLD (per-layer mersenne switch in lmmp_invcbrt_newton_)\n");
    printf("consumer=lmmp_invcbrt_newton_ residual chain: cmod gate, ir^2, ir^2*an (plain/mersenne), ir^2*an*ir\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)CBRT_NEWTON_MODM_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD);
    printf("stageA=per-layer plans extracted from lmmp_invcbrt_newton_ (exact chain, mersenne chain (p1,p2)), each paired vs exact\n");
    printf("stageB=whole lmmp_invcbrt_newton_, candidate T paired vs current T\n");
    printf("input=shapes (na,ns): (12,512) (12,1024) (12,2048) (48,2048) (48,8192) (192,8192) (192,32768) (768,32768); "
           "layers nr>=%d\n",
           NR_MIN);
    printf("candidate_range=0..1024 step %d, plus 1280, current and never (T=%d)\n", CAND_STEP, NEVER_T);
    lmmp_tune_print_env();
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    if (!lmmp_tune_parse(argc, argv, &options))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage("tune_numth_cbrt_modm");
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
        lmmp_tune_print_current("CBRT_NEWTON_MODM_THRESHOLD", (unsigned long long)CBRT_NEWTON_MODM_THRESHOLD,
                                (unsigned long long)lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    /* 候选表：0..1024 步 32，并入 1280、NEVER 与现值，升序 */
    mp_size_t cand[CAND_MAX];
    int cand_count = 0, cur_index = -1;
    const mp_size_t saved_t = lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD;
    for (int i = 0; i < CAND_GRID; ++i, ++cand_count)
        cand[cand_count] = (mp_size_t)i * CAND_STEP;
    const mp_size_t extra[3] = {1280, NEVER_T, saved_t};
    for (int e = 0; e < 3; ++e) {
        int at = cand_count;
        while (at > 0 && cand[at - 1] > extra[e]) {
            cand[at] = cand[at - 1];
            --at;
        }
        if (at > 0 && cand[at - 1] == extra[e])
            continue;  /* 已在表中 */
        cand[at] = extra[e];
        ++cand_count;
    }
    for (int t = 0; t < cand_count; ++t)
        if (cand[t] == saved_t)
            cur_index = t;

    /* 段长：numa、ir [nr+1]、an [naz]、t2/fold [mn]、w1 [mn+naz]、dp/ref [mn（梅森链输出
       宽度，不小于精确链的 nres+nr+1）]、sq [max(2nr+2,2ns+4)]、w [nres]、dstis [ns+1]、
       cube/cube1 [3ns+6]、ir1 [ns+2]、p [big+3]、q [big+6] */
    _Static_assert(MAX_MN + 8 > MAX_NA + 3 * (MAX_NS / 2 + 1) + 8, "dp segment must cover the exact chain");
    mp_ptr storage = alloc_limbs(MAX_NA + 4 + (MAX_NS / 2 + 4) + (MAX_NA + 4) + 4 * (MAX_MN + 8) +
                                 (MAX_MN + MAX_NA + 8) + (2 * MAX_NS + 8) + (MAX_NA + MAX_NS + 8) + (MAX_NS + 4) +
                                 2 * (3 * MAX_NS + 16) + (MAX_NS + 4) + (MAX_N + 8) + (MAX_N + 12));
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage, ir = numa + MAX_NA + 4, an = ir + MAX_NS / 2 + 4;
    mp_ptr t2 = an + MAX_NA + 4, fold = t2 + MAX_MN + 8, w1 = fold + MAX_MN + 8;
    mp_ptr dp = w1 + MAX_MN + MAX_NA + 8, ref = dp + MAX_MN + 8;
    mp_ptr sq = ref + MAX_MN + 8, wr = sq + 2 * MAX_NS + 8;
    mp_ptr dstis = wr + MAX_NA + MAX_NS + 8, cube = dstis + MAX_NS + 4;
    mp_ptr cube1 = cube + 3 * MAX_NS + 16, ir1 = cube1 + 3 * MAX_NS + 16;
    mp_ptr p = ir1 + MAX_NS + 4, q = p + MAX_N + 8;

    /* stage_a[s][t]：形状 s 在候选 t 下的 T 相关层代价和；ratio_b[s][t]：整函数候选/现值 */
    double stage_a[SHAPE_COUNT][CAND_MAX], ratio_b[SHAPE_COUNT][CAND_MAX];
    double whole_cur_us[SHAPE_COUNT];
    int whole_stable[SHAPE_COUNT][CAND_MAX];
    unsigned stable_points = 0, unstable_points = 0;
    int result = EXIT_SUCCESS;

    printf("# program=tune_numth_cbrt_modm\n");
    printf("# parameter=CBRT_NEWTON_MODM_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)CBRT_NEWTON_MODM_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# stageA: # layer shape na ns nr naz mn exact_us m00_us m01_us m10_us m11_us (mersenne chain; p1=ir^2 mersenne, "
           "p2=ir^2*an mersenne)\n");
    printf("# stageB: B shape na ns T cur_us cur_mad cand_us cand_mad ratio retries stable\n");
    lmmp_tune_print_env_comment();

    for (int s = 0; s < SHAPE_COUNT && result == EXIT_SUCCESS; ++s) {
        const mp_size_t na = shape_na[s], ns = shape_ns[s];
        fill_random(numa, na, (mp_limb_t)(options.seed + 97 * s + na + ns));
        /* 尺寸栈：sizes[0]=ns ... 末项=3；层 i 的当前尺寸 nr 与下一层尺寸 sizes[i] */
        mp_size_t sizes[MAX_LAYERS];
        int count = 0;
        for (mp_size_t n = ns; n > 2 && count < MAX_LAYERS; n = (n >> 1) + 1) sizes[count++] = n;
        for (int t = 0; t < cand_count; ++t) {
            stage_a[s][t] = 0.0;
            ratio_b[s][t] = 1.0;
            whole_stable[s][t] = 0;
        }
        whole_cur_us[s] = 0.0;

        /* ---- stage A ---- */
        for (int i = count - 1; i >= 0 && result == EXIT_SUCCESS; --i) {
            const mp_size_t nr = i == count - 1 ? 2 : sizes[i + 1], na_layer = sizes[i];
            if (nr < NR_MIN)
                continue;
            const mp_size_t naz = na_layer < na ? na_layer : na, nres = naz + 2 * nr + 2;
            const mp_size_t mn = lmmp_mersenne_next_size_(nres);
            layer_context ctx = {.ir = ir,
                                 .an = naz < na ? numa + na - naz : numa,
                                 .t2 = t2,
                                 .w1 = w1,
                                 .dp = dp,
                                 .sq = sq,
                                 .wr = wr,
                                 .ref = ref,
                                 .fold = fold,
                                 .nr = nr,
                                 .naz = naz,
                                 .mn = mn,
                                 .nres = nres,
                                 .plan = {0, 0, 0}};
            /* ir 顶 limb 为 1、低位随机；an 即 ctx.an（numa 最高 naz 个 limb） */
            fill_random(ir, nr, (mp_limb_t)(options.seed + 31 * s + nr + naz));
            ir[nr] = 1;

            int plan_of[CAND_MAX], seen[PLAN_COUNT] = {0}, order[PLAN_COUNT], plans = 0;
            for (int t = 0; t < cand_count; ++t) {
                plan_of[t] = plan_index(plan_layer(nr, naz, cand[t]));
                if (!seen[plan_of[t]]) {
                    seen[plan_of[t]] = 1;
                    order[plans++] = plan_of[t];
                }
            }
            if (plans == 1)
                continue;  /* 该层全部候选同走精确链，与 T 无关 */

            double plan_ratio[PLAN_COUNT] = {0}, base_sum = 0.0;
            int base_pairs = 0;
            layer_context base_ctx = ctx;
            for (int k = 0; k < plans && result == EXIT_SUCCESS; ++k) {
                layer_context plan_ctx = ctx;
                plan_ctx.plan = plan_from_index(order[k]);
                if (!check_layer(&plan_ctx)) {
                    fprintf(stderr, "layer residue mismatch at shape=%d nr=%llu naz=%llu plan=%d\n", s,
                            (unsigned long long)nr, (unsigned long long)naz, order[k]);
                    result = EXIT_FAILURE;
                    break;
                }
                if (order[k] == 0)
                    continue;
                lmmp_tune_bench base_bench = {
                    .bind = bind_noop, .reset = reset_layer, .call = call_layer, .context = &base_ctx};
                lmmp_tune_bench plan_bench = {
                    .bind = bind_noop, .reset = reset_layer, .call = call_layer, .context = &plan_ctx};
                double base_us, base_mad, us, mad;
                unsigned retries;
                bool stable;
                if (!lmmp_tune_measure_pair(&base_bench, &plan_bench, &options, &base_us, &base_mad, &us, &mad,
                                            &retries, &stable)) {
                    fprintf(stderr, "layer measurement failed at shape=%d nr=%llu\n", s, (unsigned long long)nr);
                    result = EXIT_FAILURE;
                    break;
                }
                if (stable)
                    ++stable_points;
                else
                    ++unstable_points;
                plan_ratio[order[k]] = us / base_us;
                base_sum += base_us;
                ++base_pairs;
            }
            if (result != EXIT_SUCCESS)
                break;
            /* 基线绝对耗时取各配对基线的均值，方案耗时 = 基线均值 × 配对比值 */
            const double base_us = base_sum / base_pairs;
            double plan_us[PLAN_COUNT];
            plan_us[0] = base_us;
            for (int idx = 1; idx < PLAN_COUNT; ++idx)
                plan_us[idx] = seen[idx] ? base_us * plan_ratio[idx] : 0.0;
            printf("# layer shape=%d na=%llu ns=%llu nr=%llu naz=%llu mn=%llu exact_us=%.6f", s, (unsigned long long)na,
                   (unsigned long long)ns, (unsigned long long)nr, (unsigned long long)naz, (unsigned long long)mn,
                   base_us);
            for (int idx = 1; idx < PLAN_COUNT; ++idx)
                if (seen[idx])
                    printf(" m%d%d_us=%.6f", (idx - 1) >> 1, (idx - 1) & 1, plan_us[idx]);
            printf("\n");
            for (int t = 0; t < cand_count; ++t)
                stage_a[s][t] += plan_us[plan_of[t]];
        }
        if (result != EXIT_SUCCESS)
            break;

        /* ---- stage B ---- */
        whole_context w = {.dstis = dstis,
                           .numa = numa,
                           .sq = sq,
                           .cube = cube,
                           .cube1 = cube1,
                           .ir1 = ir1,
                           .p = p,
                           .q = q,
                           .na = na,
                           .ns = ns,
                           .bind = 0};
        for (int t = 0; t < cand_count && result == EXIT_SUCCESS; ++t) {
            w.bind = cand[t];
            bind_whole(&w);
            reset_whole(&w);
            lmmp_invcbrt_newton_(dstis, ns, numa, na);
            const int ok = check_invcbrt(&w);
            lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD = saved_t;
            if (!ok) {
                fprintf(stderr, "invcbrt identity mismatch at shape=%d T=%llu\n", s, (unsigned long long)cand[t]);
                result = EXIT_FAILURE;
                break;
            }
            whole_context cur_ctx = w, cand_ctx = w;
            cur_ctx.bind = saved_t;
            lmmp_tune_bench cur_bench = {
                .bind = bind_whole, .reset = reset_whole, .call = call_whole, .context = &cur_ctx};
            lmmp_tune_bench cand_bench = {
                .bind = bind_whole, .reset = reset_whole, .call = call_whole, .context = &cand_ctx};
            double cur_us, cur_mad, c_us, c_mad;
            unsigned retries;
            bool stable;
            if (!lmmp_tune_measure_pair(&cur_bench, &cand_bench, &options, &cur_us, &cur_mad, &c_us, &c_mad, &retries,
                                        &stable)) {
                fprintf(stderr, "whole measurement failed at shape=%d T=%llu\n", s, (unsigned long long)cand[t]);
                result = EXIT_FAILURE;
                break;
            }
            lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD = saved_t;
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            ratio_b[s][t] = c_us / cur_us;
            whole_stable[s][t] = stable;
            if (t == cur_index)
                whole_cur_us[s] = cur_us;  /* A/A 行的 cur_us 与 aa_ratio 取同一次配对 */
            printf("B %d %llu %llu %llu %.6f %.6f %.6f %.6f %.6f %u %u\n", s, (unsigned long long)na,
                   (unsigned long long)ns, (unsigned long long)cand[t], cur_us, cur_mad, c_us, c_mad, c_us / cur_us,
                   retries, stable);
        }
    }

    if (result == EXIT_SUCCESS) {
        /* 汇总：B 的平均比值（相对现值的 deployed 比值）与 A 的平均预测比值。
           不稳定读数保留打印作诊断，但不入候选比较：仅当所有形状对该候选都有
           稳定读数时才参与推荐，mean_ratio 只对这些共同形状取平均 */
        double mean_ratio[CAND_MAX], mean_pred[CAND_MAX];
        int comparable[CAND_MAX];
        int best = -1, best_pred = 0;
        for (int t = 0; t < cand_count; ++t) {
            mean_ratio[t] = 0.0;
            mean_pred[t] = 0.0;
            comparable[t] = 1;
            for (int s = 0; s < SHAPE_COUNT; ++s) {
                if (!whole_stable[s][t])
                    comparable[t] = 0;
                mean_ratio[t] += ratio_b[s][t] / SHAPE_COUNT;
                mean_pred[t] += stage_a[s][t] / stage_a[s][cur_index] / SHAPE_COUNT;
            }
            if (comparable[t] && (best < 0 || mean_ratio[t] < mean_ratio[best]))
                best = t;
            if (mean_pred[t] < mean_pred[best_pred])
                best_pred = t;
        }
        if (unstable_points > 0)
            printf("# note: %u unstable measurement(s) kept for diagnostics, excluded from recommendation\n",
                   unstable_points);
        printf("# aa_noise_floor: shape na ns cur_us ratio_at_current best_T best_ratio predicted_rel_at_best\n");
        for (int s = 0; s < SHAPE_COUNT; ++s) {
            int bt = 0;
            for (int t = 1; t < cand_count; ++t)
                if (ratio_b[s][t] < ratio_b[s][bt])
                    bt = t;
            printf("# aa shape=%d na=%llu ns=%llu cur_us=%.6f aa_ratio=%.6f best_T=%llu best_ratio=%.6f "
                   "pred_rel_at_best=%.6f\n",
                   s, (unsigned long long)shape_na[s], (unsigned long long)shape_ns[s], whole_cur_us[s],
                   ratio_b[s][cur_index], (unsigned long long)cand[bt], ratio_b[s][bt],
                   stage_a[s][bt] / stage_a[s][cur_index]);
        }
        for (int t = 0; t < cand_count; ++t)
            printf("# badness candidate=%llu deployed_ratio=%.6f predicted_rel=%.6f comparable=%d\n",
                   (unsigned long long)cand[t], mean_ratio[t], mean_pred[t], comparable[t]);
        printf("# predicted_best=%llu predicted_rel=%.6f\n", (unsigned long long)cand[best_pred],
               mean_pred[best_pred]);
        printf("# stable=%u unstable=%u\n", stable_points, unstable_points);
        if (best < 0) {
            printf("# no candidate with stable readings on all shapes; recommendation withheld\n");
        } else {
            const double flat_limit = mean_ratio[best] + 0.005;
            int flat_low = best, flat_high = best;
            while (flat_low > 0 && comparable[flat_low - 1] && mean_ratio[flat_low - 1] <= flat_limit)
                --flat_low;
            while (flat_high + 1 < cand_count && comparable[flat_high + 1] &&
                   mean_ratio[flat_high + 1] <= flat_limit)
                ++flat_high;
            printf("# recommended=%llu deployed_ratio=%.6f\n", (unsigned long long)cand[best], mean_ratio[best]);
            printf("# flat_within_0.5pct=%llu..%llu\n", (unsigned long long)cand[flat_low],
                   (unsigned long long)cand[flat_high]);
            printf("# current=%llu deployed_ratio=%.6f predicted_rel=%.6f\n", (unsigned long long)saved_t,
                   mean_ratio[cur_index], mean_pred[cur_index]);
            printf("# never=%llu deployed_ratio=%.6f\n", (unsigned long long)NEVER_T, mean_ratio[cand_count - 1]);
            printf("recommended CBRT_NEWTON_MODM_THRESHOLD = %llu\n", (unsigned long long)cand[best]);
        }
    }
    lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD = saved_t;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
