/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/* 阈值调优：BINOMIAL_DIV_K/B_THRESHOLD（div路径与factor路径的分界斜率） */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/impl/prime_table.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

typedef struct {
    mp_ptr dst;
    mp_size_t rn;
    uint n;
    uint r;
} ncr_div_ctx;

static double bench_ncr(void* v) {
    ncr_div_ctx* c = (ncr_div_ctx*)v;
    (void)lmmp_odd_nCr_uint_(c->dst, c->rn, c->n, c->r);
    return 0.0;
}

/* 强制div路径：K=1,B=0 时 1*nPr_n > 0 恒成立 */
static double bench_ncr_div(void* v) {
    lmmp_tune_BINOMIAL_DIV_K_THRESHOLD = 1;
    lmmp_tune_BINOMIAL_DIV_B_THRESHOLD = 0;
    return bench_ncr(v);
}

/* 强制factor路径：K=0,B=1 时 0*nPr_n > 1*fac_n 恒不成立（fac_n>0） */
static double bench_ncr_factor(void* v) {
    lmmp_tune_BINOMIAL_DIV_K_THRESHOLD = 0;
    lmmp_tune_BINOMIAL_DIV_B_THRESHOLD = 1;
    return bench_ncr(v);
}

static void* make_ctx(uint n, uint r) {
    mp_bitcnt_t bits = 0;
    const mp_size_t rn = lmmp_nCr_size_(n, r, &bits);
    ncr_div_ctx* c = (ncr_div_ctx*)lmmp_alloc(sizeof(ncr_div_ctx));
    if (c == NULL)
        return NULL;
    c->n = n;
    c->r = r;
    c->rn = rn;
    c->dst = (mp_ptr)lmmp_alloc((size_t)(rn + 2) * sizeof(mp_limb_t));
    if (c->dst == NULL) {
        lmmp_free(c);
        return NULL;
    }
    return c;
}

static void free_ctx(void* v) {
    ncr_div_ctx* c = (ncr_div_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->dst);
        lmmp_free(c);
    }
}

int tune_run_binomial_div(void) {
#define N_N 9
#define N_RP 9
    static const uint ns[N_N] = {70000, 100000, 150000, 200000, 300000, 400000, 600000, 750000, 1000000};
    /* r = n * rp / 1000，即r占n的千分比 */
    static const uint rp[N_RP] = {5, 10, 20, 50, 100, 150, 200, 250, 300};
    tune_ratio_point_t points[N_N * N_RP];
    size_t npoints = 0;

    const uint64_t old_k = lmmp_tune_BINOMIAL_DIV_K_THRESHOLD;
    const uint64_t old_b = lmmp_tune_BINOMIAL_DIV_B_THRESHOLD;
    /* 样本点必须越过basecase阈值才能进入div/factor分支；binomial_rn 模块可能
       已把运行时阈值改为调优值，这里以当前运行时值为过滤下限。 */
    const uint64_t rn_min = lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD;

    /* 先把质数表一次性扩容到最大n，使factor路径的测量与原调优假设一致：
       忽略质数表初始化开销，瓶颈集中在质数表的遍历。 */
    lmmp_prime_int_table_init_(ns[N_N - 1]);

    printf("  measuring div/factor pairs (n, r fractions in bp of 1/1000)...\n");
    for (size_t i = 0; i < N_N; ++i) {
        for (size_t j = 0; j < N_RP; ++j) {
            const uint n = ns[i];
            /* 注意乘法需在64位下进行，n*rp会超出uint范围 */
            const uint r = (uint)((ulong)n * rp[j] / 1000);
            if (r < 25 || 2 * r > n)
                continue;
            mp_bitcnt_t bits = 0;
            const mp_size_t rn = lmmp_nCr_size_(n, r, &bits);
            if (rn < (mp_size_t)rn_min)
                continue;
            tune_measure_t md, mf;
            void* cd = make_ctx(n, r);
            void* cf = make_ctx(n, r);
            if (cd == NULL || cf == NULL) {
                free_ctx(cd);
                free_ctx(cf);
                continue;
            }

            tune_measure_pair(bench_ncr_div, cd, bench_ncr_factor, cf,
                              g_tune.samples, g_tune.target_ms, &md, &mf);
            const uint64_t npr_n = lmmp_nPr_size_(n, r, &bits);
            const uint64_t fac_n = lmmp_factorial_size_(r, &bits);
            printf("    n=%-8u r=%-8u nPr_n=%-7llu fac_n=%-7llu ratio=%7.4f "
                   "div=%12.1f ns factor=%12.1f ns\n",
                   n, r, (unsigned long long)npr_n, (unsigned long long)fac_n,
                   (double)npr_n / (double)fac_n, md.median_ns, mf.median_ns);
            points[npoints].npr_n = npr_n;
            points[npoints].fac_n = fac_n;
            points[npoints].div_ns = md.median_ns;
            points[npoints].factor_ns = mf.median_ns;
            ++npoints;

            free_ctx(cd);
            free_ctx(cf);
        }
    }

    const tune_line_choice_t choice = tune_choose_2d_ratio(
        points, npoints, 1, 512, 1, 8192, old_k, old_b, 1024);
    printf("  -> K=%llu B=%llu (B/K=%.4f) badness=%.6f\n",
           (unsigned long long)choice.k, (unsigned long long)choice.b,
           (double)choice.b / (double)choice.k, choice.badness);

    lmmp_tune_BINOMIAL_DIV_K_THRESHOLD = choice.k;
    lmmp_tune_BINOMIAL_DIV_B_THRESHOLD = choice.b;
    tune_record_add("BINOMIAL_DIV_K_THRESHOLD", old_k, choice.k,
                    choice.badness, choice.badness);
    tune_record_add("BINOMIAL_DIV_B_THRESHOLD", old_b, choice.b,
                    choice.badness, choice.badness);
    return 0;
#undef N_N
#undef N_RP
}
