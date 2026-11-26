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

/* SQRT_NEWTON_MODM_THRESHOLD：lmmp_invsqrt_newton_ 每层残差链的两个决策（ir^2
   用梅森平方还是普通平方、ir^2*an 用梅森乘法还是普通乘法）共用的阈值。阈值在
   每层内部读取且带形状守卫（naz>=T、naz*8>=nsqr、mn<nsqr+naz），经完整函数无法
   干净强制，整函数耗时对它的依赖又只有百分之几 ⟹ 分两级测量。

   A 层内直测（提炼分派）：按真实尺寸栈逐层构造操作数（ir 顶 limb 为 1、an 取
   被开方数最高 naz 个 limb），把该层的两个决策提炼成显式阈值的方案
   (p1,p2)=(ir^2 梅森?, ir^2*an 梅森?)，复刻 lmmp_invsqrt_newton_ 的执行。候选 T
   在一层内只会落到至多 4 种方案，每种可达方案与全普通基线配对测量一次；任意
   候选的 T 相关代价即各层方案耗时之和（mul_n 与线性步与 T 无关）。每种方案先过
   残差恒等式：dp 与 ir^2*an 的全积模 B^nres 或模 B^mn-1 一致。

   B 整函数配对 deployed 扫描：lmmp_invsqrt_newton_ 绑定候选 T 与绑定现值交替
   计时，读数是相对现值的比值（候选等于现值的一对即 A/A 噪声底）；每个候选的
   输出过逆平方根恒等式 a*ir^2 <= B^(2ns+na) < a*(ir+2)^2（ir ∈ {I-1, I}，
   I = floor(sqrt(B^(2ns+na)/a))）。推荐值取 B 的平均比值
   argmin，A 给出机理分解与预测作交叉诊断。梅森内部门保持默认。 */
enum {
    SHAPE_COUNT = 8,
    NR_MIN = 8,                              /* 只测 nr 不小于此值的层 */
    MAX_NS = 32768,
    MAX_NA = 2048,
    MAX_LAYERS = 32,
    PLAN_COUNT = 4,                          /* (p1,p2) 四种方案，索引 0 为全普通基线 */
    CAND_STEP = 32,                          /* T 候选 0..1024 步 32 */
    CAND_GRID = 1024 / CAND_STEP + 1,
    CAND_MAX = CAND_GRID + 3,                /* 另加 1280、现值、NEVER */
    NEVER_T = 1 << 20,
    MAX_MN = 4 * (MAX_NA + MAX_NS / 2 + 4)   /* mersenne_next_size(nres) 的宽松上界 */
};
static const mp_size_t shape_na[SHAPE_COUNT] = {16, 16, 16, 128, 64, 512, 256, 2048};
static const mp_size_t shape_ns[SHAPE_COUNT] = {512, 1024, 2048, 2048, 8192, 8192, 32768, 32768};

typedef struct {
    int p1, p2; /* 1=梅森 */
} layer_plan;

typedef struct {
    mp_ptr ir, an, xp, dp, sq, ref, fold;
    mp_size_t nr, naz, mn, nres;
    layer_plan plan;
} layer_context;

typedef struct {
    mp_ptr dstis, numa, sq, p, u, v, q;
    mp_size_t na, ns, bind;
} whole_context;

/* 镜像 lmmp_invsqrt_newton_ 的两个决策（阈值 t 显式传入） */
static layer_plan plan_layer(mp_size_t nr, mp_size_t naz, mp_size_t t) {
    const mp_size_t mn = lmmp_mersenne_next_size_(naz + nr + 1);
    layer_plan plan;
    plan.p1 = !(2 * t + mn >= 2 * nr + 1);
    const mp_size_t nsqr = plan.p1 ? mn : 2 * nr + 1;
    plan.p2 = !(naz < t || naz * 8 < nsqr || mn >= nsqr + naz);
    return plan;
}

static int plan_index(layer_plan plan) {
    return (plan.p1 << 1) | plan.p2;
}

/* 复刻 lmmp_invsqrt_newton_ 单层的 ir^2 与 ir^2*an 两步 */
static void run_layer(const layer_context* c) {
    const mp_size_t nr = c->nr, naz = c->naz, mn = c->mn;
    mp_ptr xp = c->xp, dp = c->dp;
    mp_size_t nsqr;
    int cmod;
    if (!c->plan.p1) {
        cmod = 0;
        lmmp_sqr_(xp, c->ir, nr + 1);
        nsqr = 2 * nr + 1;
    } else {
        cmod = 1;
        lmmp_sqr_mersenne_(xp, mn, c->ir, nr + 1);
        nsqr = mn;
    }
    if (!c->plan.p2) {
        if (cmod == 0)
            nsqr = LMMP_MIN(nsqr, c->nres);
        lmmp_mul_(dp, xp, nsqr, c->an, naz);
        if (cmod == 1) {
            if (lmmp_add_(dp, dp, mn, dp + mn, naz))
                lmmp_inc(dp);
        }
    } else {
        if (nsqr > mn) {  // cmod==0
            if (lmmp_add_(xp, xp, mn, xp + mn, nsqr - mn))
                lmmp_inc(xp);
            nsqr = mn;
        }
        lmmp_mul_mersenne_(dp, mn, xp, nsqr, c->an, naz);
    }
}

static void bind_noop(void* opaque) {
    (void)opaque;
}

static void reset_layer(void* opaque) {
    const layer_context* c = opaque;
    lmmp_zero(c->xp, 2 * c->nr + 1 > c->mn ? 2 * c->nr + 1 : c->mn);
    lmmp_zero(c->dp, c->mn);
}

static mp_limb_t call_layer(void* opaque) {
    const layer_context* c = opaque;
    run_layer(c);
    return c->dp[0] ^ c->dp[c->mn - 1];
}

/* 残差恒等式：dp 与 ir^2*an 的全积在各自表示（模 B^nres 或模 B^mn-1）下一致 */
static int check_layer(layer_context* c) {
    const mp_size_t nr = c->nr, naz = c->naz, mn = c->mn, l = 2 * nr + 1 + naz;
    lmmp_sqr_(c->sq, c->ir, nr + 1);
    lmmp_mul_(c->ref, c->sq, 2 * nr + 1, c->an, naz);
    reset_layer(c);
    run_layer(c);
    if (!(c->plan.p1 || c->plan.p2))
        return lmmp_cmp_(c->dp, c->ref, c->nres) == 0;
    lmmp_copy(c->fold, c->ref, mn);
    if (lmmp_add_(c->fold, c->fold, mn, c->ref + mn, l - mn))
        lmmp_inc(c->fold);
    return lmmp_cmp_(c->dp, c->fold, mn) == 0;
}

static void bind_whole(void* opaque) {
    whole_context* c = opaque;
    lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD = c->bind;
}

static void reset_whole(void* opaque) {
    whole_context* c = opaque;
    lmmp_zero(c->dstis, c->ns + 1);
}

static mp_limb_t call_whole(void* opaque) {
    whole_context* c = opaque;
    lmmp_invsqrt_newton_(c->dstis, c->ns, c->numa, c->na);
    return c->dstis[0] ^ c->dstis[c->ns];
}

/* [x,len] 与 K=B^big 的精确比较：返回 -1/0/1（x<K / x==K / x>K）。
   K 仅 [big] 位为 1、其余为 0，故高于 big 的 limb 非零即 x>K，
   [big] 位 >1 亦 x>K，==1 时由低位 limb 是否全零定相等或大于 */
static int cmp_pow2_(mp_srcptr x, mp_size_t len, mp_size_t big) {
    for (mp_size_t i = big + 1; i < len; ++i)
        if (x[i] != 0)
            return 1;
    if (x[big] > 1)
        return 1;
    if (x[big] == 1) {
        for (mp_size_t i = 0; i < big; ++i)
            if (x[i] != 0)
                return 1;
        return 0;
    }
    return -1;
}

/* 逆平方根恒等式：a*ir^2 <= K=B^(2ns+na) < a*(ir+2)^2，按完整 limb 值比较，
   接受 ir ∈ {I-1, I}（I = floor(sqrt(K/a))），拒绝 I+1（下界失败）与 I-2（上界失败）。
   I=floor(sqrt(K/a)) 保证 a*(I+1)^2 > K 恒严格成立，故 ir=I-1 时 a*(ir+2)^2=a*(I+1)^2 > K，
   ir=I 时 a*(ir+2)^2=a*(I+2)^2 > K，上界 ir+2 对两个契约值都通过 */
static int check_invsqrt(const whole_context* c) {
    const mp_size_t na = c->na, ns = c->ns, big = 2 * ns + na;
    lmmp_sqr_(c->sq, c->dstis, ns + 1);                   /* ir^2，[0,2ns+2) */
    lmmp_mul_(c->p, c->sq, 2 * ns + 2, c->numa, na);      /* a*ir^2，[0,big+2) */
    if (cmp_pow2_(c->p, big + 2, big) > 0)
        return 0;  /* a*ir^2 > K */
    /* ir2 = ir+2，经精确平方与乘法得 q = a*(ir+2)^2 */
    lmmp_copy(c->v, c->dstis, ns + 1);
    lmmp_add_1_(c->v, c->v, ns + 1, 1);
    c->v[ns + 1] = lmmp_add_1_(c->v, c->v, ns + 1, 1);
    lmmp_sqr_(c->u, c->v, ns + 2);                        /* (ir+2)^2，[0,2ns+4) */
    if (na >= 2 * ns + 4)
        lmmp_mul_(c->q, c->numa, na, c->u, 2 * ns + 4);
    else
        lmmp_mul_(c->q, c->u, 2 * ns + 4, c->numa, na);   /* [0,big+4) */
    return cmp_pow2_(c->q, big + 4, big) > 0;  /* 须 a*(ir+2)^2 > K */
}

static void print_info(void) {
    printf("program=tune_numth_sqrt_modm\n");
    printf("parameter=SQRT_NEWTON_MODM_THRESHOLD (per-layer mersenne switch in lmmp_invsqrt_newton_)\n");
    printf("consumer=lmmp_invsqrt_newton_ residual chain: ir^2 (plain/mersenne square), ir^2*an (plain/mersenne mul)\n");
    printf("binding_mode=runtime-branch\n");
    printf("default=%llu\n", (unsigned long long)SQRT_NEWTON_MODM_THRESHOLD);
    printf("current=%llu\n", (unsigned long long)lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD);
    printf("stageA=per-layer plans (p1,p2) extracted from lmmp_invsqrt_newton_, each reachable plan paired vs all-plain\n");
    printf("stageB=whole lmmp_invsqrt_newton_, candidate T paired vs current T\n");
    printf("input=shapes (na,ns): (16,512) (16,1024) (16,2048) (128,2048) (64,8192) (512,8192) (256,32768) (2048,32768); "
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
        lmmp_tune_usage("tune_numth_sqrt_modm");
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
        lmmp_tune_print_current("SQRT_NEWTON_MODM_THRESHOLD", (unsigned long long)SQRT_NEWTON_MODM_THRESHOLD,
                                (unsigned long long)lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD);
        lmmp_global_deinit();
        return EXIT_SUCCESS;
    }

    /* 候选表：0..1024 步 32，并入 1280、NEVER 与现值，升序 */
    mp_size_t cand[CAND_MAX];
    int cand_count = 0, cur_index = -1;
    const mp_size_t saved_t = lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD;
    for (int i = 0; i < CAND_GRID; ++i, ++cand_count)
        cand[cand_count] = (mp_size_t)i * CAND_STEP;
    const mp_size_t extra[3] = {1280, NEVER_T, saved_t};
    for (int e = 0; e < 3; ++e) {
        int dup = 0;
        for (int i = 0; i < cand_count; ++i)
            if (cand[i] == extra[e])
                dup = 1;
        if (dup)
            continue;  /* 已在表中：不移动元素、不增计数 */
        int at = cand_count;
        while (at > 0 && cand[at - 1] > extra[e]) {
            cand[at] = cand[at - 1];
            --at;
        }
        cand[at] = extra[e];
        ++cand_count;
    }
    for (int t = 0; t < cand_count; ++t)
        if (cand[t] == saved_t)
            cur_index = t;

    /* 段长：numa、ir [nr+1]、an [naz]、xp/dp/fold [mn]、sq [max(2nr+2, 2ns+2)]、ref [2nr+1+naz]、
       dstis [ns+1]、p [big+2]、u [(ir+2)^2，2ns+4]、v [ir+2，ns+2]、q [big+4] */
    mp_ptr storage = alloc_limbs(MAX_NA + 4 + (MAX_NS / 2 + 4) + (MAX_NA + 4) + 3 * (MAX_MN + 8) +
                                 (2 * MAX_NS + 8) + (2 * MAX_NS + MAX_NA + 8) + (MAX_NS + 4) +
                                 4 * (2 * MAX_NS + MAX_NA + 8));
    if (storage == NULL) {
        fprintf(stderr, "cannot allocate tune buffers\n");
        lmmp_global_deinit();
        return EXIT_FAILURE;
    }
    mp_ptr numa = storage, ir = numa + MAX_NA + 4, an = ir + MAX_NS / 2 + 4;
    mp_ptr xp = an + MAX_NA + 4, dp = xp + MAX_MN + 8, fold = dp + MAX_MN + 8;
    mp_ptr sq = fold + MAX_MN + 8, ref = sq + 2 * MAX_NS + 8;
    mp_ptr dstis = ref + 2 * MAX_NS + MAX_NA + 8, p = dstis + MAX_NS + 4, u = p + 2 * MAX_NS + MAX_NA + 8;
    mp_ptr v = u + 2 * MAX_NS + MAX_NA + 8, q = v + 2 * MAX_NS + MAX_NA + 8;

    /* stage_a[s][t]：形状 s 在候选 t 下的 T 相关层代价和；ratio_b[s][t]：整函数候选/现值 */
    double stage_a[SHAPE_COUNT][CAND_MAX], ratio_b[SHAPE_COUNT][CAND_MAX];
    double whole_cur_us[SHAPE_COUNT];
    unsigned stable_points = 0, unstable_points = 0;
    int result = EXIT_SUCCESS;

    printf("# program=tune_numth_sqrt_modm\n");
    printf("# parameter=SQRT_NEWTON_MODM_THRESHOLD\n");
    printf("# binding_mode=runtime-branch\n");
    printf("# default=%llu seed=%llu samples=%u min_ms=%.3f mad_limit=%.6f\n",
           (unsigned long long)SQRT_NEWTON_MODM_THRESHOLD, options.seed, options.samples, options.min_ms,
           options.mad_limit);
    printf("# stageA: # layer shape na ns nr naz mn base_us p10_us p01_us p11_us (p1=ir^2 mersenne, p2=ir^2*an mersenne)\n");
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
        }
        whole_cur_us[s] = 0.0;

        /* ---- stage A ---- */
        for (int i = count - 1; i >= 0 && result == EXIT_SUCCESS; --i) {
            const mp_size_t nr = i == count - 1 ? 2 : sizes[i + 1], na_layer = sizes[i];
            if (nr < NR_MIN)
                continue;
            const mp_size_t naz = na_layer < na ? na_layer : na, nres = naz + nr + 1;
            const mp_size_t mn = lmmp_mersenne_next_size_(nres);
            layer_context ctx = {.ir = ir,
                                 .an = naz < na ? numa + na - naz : numa,
                                 .xp = xp,
                                 .dp = dp,
                                 .sq = sq,
                                 .ref = ref,
                                 .fold = fold,
                                 .nr = nr,
                                 .naz = naz,
                                 .mn = mn,
                                 .nres = nres,
                                 .plan = {0, 0}};
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
                continue;  /* 该层全部候选同走全普通，与 T 无关 */

            double plan_ratio[PLAN_COUNT] = {0}, base_sum = 0.0;
            int base_pairs = 0;
            layer_context base_ctx = ctx;
            for (int k = 0; k < plans && result == EXIT_SUCCESS; ++k) {
                layer_context plan_ctx = ctx;
                plan_ctx.plan.p1 = (order[k] >> 1) & 1;
                plan_ctx.plan.p2 = order[k] & 1;
                if (!check_layer(&plan_ctx)) {
                    fprintf(stderr, "layer residue mismatch at shape=%d nr=%llu naz=%llu plan=(%d,%d)\n", s,
                            (unsigned long long)nr, (unsigned long long)naz, plan_ctx.plan.p1, plan_ctx.plan.p2);
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
            printf("# layer shape=%d na=%llu ns=%llu nr=%llu naz=%llu mn=%llu base_us=%.6f", s, (unsigned long long)na,
                   (unsigned long long)ns, (unsigned long long)nr, (unsigned long long)naz, (unsigned long long)mn,
                   base_us);
            for (int idx = 1; idx < PLAN_COUNT; ++idx)
                if (seen[idx])
                    printf(" p%d%d_us=%.6f", (idx >> 1) & 1, idx & 1, plan_us[idx]);
            printf("\n");
            for (int t = 0; t < cand_count; ++t)
                stage_a[s][t] += plan_us[plan_of[t]];
        }
        if (result != EXIT_SUCCESS)
            break;

        /* ---- stage B ---- */
        whole_context w = {
            .dstis = dstis, .numa = numa, .sq = sq, .p = p, .u = u, .v = v, .q = q, .na = na, .ns = ns, .bind = 0};
        for (int t = 0; t < cand_count && result == EXIT_SUCCESS; ++t) {
            w.bind = cand[t];
            bind_whole(&w);
            reset_whole(&w);
            lmmp_invsqrt_newton_(dstis, ns, numa, na);
            const int ok = check_invsqrt(&w);
            lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD = saved_t;
            if (!ok) {
                fprintf(stderr, "invsqrt identity mismatch at shape=%d T=%llu\n", s, (unsigned long long)cand[t]);
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
            lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD = saved_t;
            if (stable)
                ++stable_points;
            else
                ++unstable_points;
            ratio_b[s][t] = c_us / cur_us;
            whole_cur_us[s] = cur_us;
            printf("B %d %llu %llu %llu %.6f %.6f %.6f %.6f %.6f %u %u\n", s, (unsigned long long)na,
                   (unsigned long long)ns, (unsigned long long)cand[t], cur_us, cur_mad, c_us, c_mad, c_us / cur_us,
                   retries, stable);
        }
    }

    if (result == EXIT_SUCCESS) {
        /* 汇总：B 的平均比值（相对现值的 deployed 超出）与 A 的平均预测比值 */
        double mean_ratio[CAND_MAX], mean_pred[CAND_MAX];
        int best = 0, best_pred = 0;
        for (int t = 0; t < cand_count; ++t) {
            mean_ratio[t] = 0.0;
            mean_pred[t] = 0.0;
            for (int s = 0; s < SHAPE_COUNT; ++s) {
                mean_ratio[t] += ratio_b[s][t] / SHAPE_COUNT;
                mean_pred[t] += stage_a[s][t] / stage_a[s][cur_index] / SHAPE_COUNT;
            }
            if (mean_ratio[t] < mean_ratio[best])
                best = t;
            if (mean_pred[t] < mean_pred[best_pred])
                best_pred = t;
        }
        if (unstable_points > 0)
            printf("# note: %u unstable measurement(s) retained in scoring\n", unstable_points);
        printf("# aa_noise_floor: shape na ns cur_us ratio_at_current\n");
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
        const double flat_limit = mean_ratio[best] + 0.005;
        int flat_low = best, flat_high = best;
        while (flat_low > 0 && mean_ratio[flat_low - 1] <= flat_limit)
            --flat_low;
        while (flat_high + 1 < cand_count && mean_ratio[flat_high + 1] <= flat_limit)
            ++flat_high;
        for (int t = 0; t < cand_count; ++t)
            printf("# badness candidate=%llu deployed_ratio=%.6f predicted_rel=%.6f\n", (unsigned long long)cand[t],
                   mean_ratio[t], mean_pred[t]);
        printf("# recommended=%llu deployed_ratio=%.6f\n", (unsigned long long)cand[best], mean_ratio[best]);
        printf("# flat_within_0.5pct=%llu..%llu\n", (unsigned long long)cand[flat_low], (unsigned long long)cand[flat_high]);
        printf("# predicted_best=%llu predicted_rel=%.6f\n", (unsigned long long)cand[best_pred], mean_pred[best_pred]);
        printf("# current=%llu deployed_ratio=%.6f predicted_rel=%.6f\n", (unsigned long long)saved_t,
               mean_ratio[cur_index], mean_pred[cur_index]);
        printf("# never=%llu deployed_ratio=%.6f\n", (unsigned long long)NEVER_T,
               mean_ratio[cand_count - 1]);
        printf("# stable=%u unstable=%u\n", stable_points, unstable_points);
        printf("recommended SQRT_NEWTON_MODM_THRESHOLD = %llu\n", (unsigned long long)cand[best]);
    }
    lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD = saved_t;
    lmmp_free(storage);
    lmmp_global_deinit();
    return result;
}
