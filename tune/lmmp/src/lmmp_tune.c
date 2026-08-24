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

/*
 * LMMP 阈值调优驱动。
 *
 * 用法:
 *   lmmp_tune [--quick] [--write] [--only mul22,mul33,...]
 *
 * 说明:
 *   - 本程序与静态调优核心 liblmmp_tune_core 一起构建（LMMP_TUNE_MODE=ON）。
 *   - 阈值通过 include/lmmp/impl/mparam.h 中的 extern 变量绑定，本文件定义并设置它们。
 *   - 测量采用“预热 + 多次采样取中位数”的方式，并限制单次采样时间，避免个别异常值影响。
 */

#include "lmmp/lmmp.h"
#include "lmmp/impl/ele_mul.h"
#include "lmmp/impl/mat22_mul.h"
#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_tune.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

/* ============================== 计时 ============================== */

static double g_tick_ns = 0.0;

static void timer_init(void) {
#ifdef _WIN32
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_tick_ns = 1e9 / (double)f.QuadPart;
#else
    (void)g_tick_ns;
#endif
}

static double now_ns(void) {
#ifdef _WIN32
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * g_tick_ns;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
#endif
}

typedef void (*tune_bench_fn)(void* ctx);

typedef struct {
    tune_bench_fn fn;
    unsigned loops;
} tune_sample;

static unsigned calibrate_loops(tune_bench_fn fn, void* ctx, double target_ms) {
    unsigned loops = 1;
    double t;
    do {
        double t0 = now_ns();
        for (unsigned i = 0; i < loops; ++i) fn(ctx);
        t = now_ns() - t0;
        if (t >= target_ms * 1e6) break;
        loops <<= 1;
    } while (loops < 65536);
    (void)t;
    return loops;
}

static double bench_ns_per_call(tune_bench_fn fn, void* ctx, unsigned samples, double target_ms) {
    unsigned loops = calibrate_loops(fn, ctx, target_ms);
    double* ts = (double*)malloc(sizeof(double) * samples);
    if (ts == NULL) return 1e300;
    for (unsigned s = 0; s < samples; ++s) {
        double t0 = now_ns();
        for (unsigned i = 0; i < loops; ++i) fn(ctx);
        ts[s] = (now_ns() - t0) / (double)loops;
    }
    /* 中位数，抗少量异常计时 */
    for (unsigned i = 0; i + 1 < samples; ++i) {
        for (unsigned j = i + 1; j < samples; ++j) {
            if (ts[i] > ts[j]) {
                double tmp = ts[i];
                ts[i] = ts[j];
                ts[j] = tmp;
            }
        }
    }
    double med = ts[samples / 2];
    free(ts);
    return med;
}

/* ============================== 基准上下文 ============================== */

static int g_quick = 0;
static int g_write = 0;

typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr d;
    mp_size_t n;
} mul_ctx;

static void mul_ctx_init(mul_ctx* c, mp_size_t n) {
    c->n = n;
    c->a = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)(2 * n + 1) * sizeof(mp_limb_t));
    for (mp_size_t i = 0; i < n; ++i) {
        c->a[i] = UINT64_C(0x9e3779b97f4a7c15) ^ (uint64_t)i * UINT64_C(0xbf58476d1ce4e5b9);
        c->b[i] = UINT64_C(0x94d049bb133111eb) ^ (uint64_t)i * UINT64_C(0xc2b2ae3d27d4eb4f);
    }
    c->a[n - 1] |= (uint64_t)1 << 63;
    c->b[n - 1] |= (uint64_t)1 << 63;
}

static void mul_ctx_free(mul_ctx* c) {
    lmmp_free(c->a);
    lmmp_free(c->b);
    lmmp_free(c->d);
}

static void bench_mul_call(void* v) {
    mul_ctx* c = (mul_ctx*)v;
    lmmp_mul_(c->d, c->a, c->n, c->b, c->n);
}

static double bench_mul_size(mp_size_t n) {
    mul_ctx c;
    mul_ctx_init(&c, n);
    double ns = bench_ns_per_call((tune_bench_fn)bench_mul_call, &c, 3, 5.0);
    mul_ctx_free(&c);
    return ns;
}

static double obj_mul(void) {
    static const mp_size_t sizes[] = {16, 24, 40, 80, 160, 320, 700, 1500};
    double sum = 0.0;
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        double ns = bench_mul_size(sizes[i]);
        if (ns <= 0.0) ns = 1e300;
        sum += ns;
    }
    return sum;
}

/* mullo */
typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr d;
    mp_size_t n;
} mullo_ctx;

static void mullo_ctx_init(mullo_ctx* c, mp_size_t n) {
    c->n = n;
    c->a = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    for (mp_size_t i = 0; i < n; ++i) {
        c->a[i] = UINT64_C(0xdeadbeefcafebabe) ^ (uint64_t)i * UINT64_C(0x100000001b3);
        c->b[i] = UINT64_C(0x8a5cd789635d2dff) ^ (uint64_t)i * UINT64_C(0x9e3779b97f4a7c15);
    }
    c->a[n - 1] |= (uint64_t)1 << 63;
    c->b[n - 1] |= (uint64_t)1 << 63;
}

static void mullo_ctx_free(mullo_ctx* c) {
    lmmp_free(c->a);
    lmmp_free(c->b);
    lmmp_free(c->d);
}

static void bench_mullo_call(void* v) {
    mullo_ctx* c = (mullo_ctx*)v;
    lmmp_mullo_(c->d, c->a, c->b, c->n);
}

static double obj_mullo(void) {
    static const mp_size_t sizes[] = {5, 10, 20, 40, 80, 160, 400};
    double sum = 0.0;
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        mullo_ctx c;
        mullo_ctx_init(&c, sizes[i]);
        double ns = bench_ns_per_call((tune_bench_fn)bench_mullo_call, &c, 3, 5.0);
        mullo_ctx_free(&c);
        if (ns <= 0.0) ns = 1e300;
        sum += ns;
    }
    return sum;
}

/* nPr */
typedef struct {
    mp_ptr dst;
    mp_size_t rn;
    mp_bitcnt_t bits;
    ulong n;
    ulong r;
} npr_ctx;

static void npr_ctx_init(npr_ctx* c, ulong n, ulong r) {
    c->n = n;
    c->r = r;
    c->bits = 0;
    c->rn = lmmp_nPr_size_(n, r, &c->bits);
    c->dst = (mp_ptr)lmmp_alloc((size_t)c->rn * sizeof(mp_limb_t));
}

static void npr_ctx_free(npr_ctx* c) {
    lmmp_free(c->dst);
}

static void bench_npr_call(void* v) {
    npr_ctx* c = (npr_ctx*)v;
    (void)lmmp_nPr_(c->dst, c->bits, c->rn, c->n, c->r);
}

static double bench_npr_pair(ulong n, ulong r, double target_ms) {
    npr_ctx c;
    npr_ctx_init(&c, n, r);
    double ns = bench_ns_per_call((tune_bench_fn)bench_npr_call, &c, 3, target_ms);
    npr_ctx_free(&c);
    return ns <= 0.0 ? 1e300 : ns;
}

/* ============================== nPr 2D 阈值：成本表法 ==============================
 * 分别用极端阈值强制走 product / factor 两条路径，每个 (n,r) 采样点只测两次；
 * 之后任意 (K,B) 的代价都可直接查表求和，无需重新计时，因此网格可以取得很密。 */

typedef struct {
    ulong n;
    ulong r;
    double product_ns;
    double factor_ns;
} npr_cache_entry;

#define NPR_CACHE_MAX 12

static npr_cache_entry g_npr_ushort_cache[NPR_CACHE_MAX];
static int g_npr_ushort_cache_n = 0;
static npr_cache_entry g_npr_uint_cache[NPR_CACHE_MAX];
static int g_npr_uint_cache_n = 0;

static void apply_npr_ushort(uint64_t k, uint64_t b);
static void apply_npr_uint(uint64_t k, uint64_t b);

static void npr_cache_fill(int is_uint) {
    npr_cache_entry* cache = is_uint ? g_npr_uint_cache : g_npr_ushort_cache;
    int* pn = is_uint ? &g_npr_uint_cache_n : &g_npr_ushort_cache_n;

    static const ulong ushort_ns[] = {60000, 60000, 65535, 65535, 50000, 50000};
    static const ulong ushort_rs[] = { 4000, 30000, 10000, 32768,  5000, 25000};
    static const ulong uint_ns[] = {200000, 200000, 200000, 100000, 100000, 300000};
    static const ulong uint_rs[] = {  5000,  50000,  20000,  10000,  50000,   1000};

    const ulong* ns = is_uint ? uint_ns : ushort_ns;
    const ulong* rs = is_uint ? uint_rs : ushort_rs;
    int count = is_uint ? 6 : 6;
    double target_ms = g_quick ? 2.0 : 6.0;

    /* 强制 product：K=0,B=0 时 n+B > r*K 恒成立（n>0）。 */
    if (is_uint) apply_npr_uint(0, 0);
    else         apply_npr_ushort(0, 0);
    for (int i = 0; i < count; ++i) {
        cache[i].n = ns[i];
        cache[i].r = rs[i];
        cache[i].product_ns = bench_npr_pair(ns[i], rs[i], target_ms);
    }

    /* 强制 factor：K 取足够大，使 n+B <= r*K 对全部采样点成立。 */
    if (is_uint) apply_npr_uint(10000000, 0);
    else         apply_npr_ushort(1000000, 0);
    for (int i = 0; i < count; ++i) {
        cache[i].factor_ns = bench_npr_pair(ns[i], rs[i], target_ms);
    }

    *pn = count;
}

static double npr_cache_eval(int is_uint, uint64_t K, uint64_t B) {
    const npr_cache_entry* cache = is_uint ? g_npr_uint_cache : g_npr_ushort_cache;
    int count = is_uint ? g_npr_uint_cache_n : g_npr_ushort_cache_n;
    double sum = 0.0;
    for (int i = 0; i < count; ++i) {
        uint64_t left = cache[i].n + B;
        uint64_t right = cache[i].r * K;
        sum += (left > right) ? cache[i].product_ns : cache[i].factor_ns;
    }
    return sum;
}

/* nCr */
typedef struct {
    mp_ptr dst;
    mp_size_t rn;
    mp_bitcnt_t bits;
    uint n;
    uint r;
} ncr_ctx;

static void ncr_ctx_init(ncr_ctx* c, uint n, uint r) {
    c->n = n;
    c->r = r;
    c->bits = 0;
    c->rn = lmmp_nCr_size_(n, r, &c->bits);
    c->dst = (mp_ptr)lmmp_alloc((size_t)c->rn * sizeof(mp_limb_t));
}

static void ncr_ctx_free(ncr_ctx* c) {
    lmmp_free(c->dst);
}

static void bench_ncr_call(void* v) {
    ncr_ctx* c = (ncr_ctx*)v;
    (void)lmmp_nCr_(c->dst, c->bits, c->rn, c->n, c->r);
}

static double obj_ncr(void) {
    double sum = 0.0;
    static const uint ns[] = {2000, 3000, 5000};
    static const uint rs[] = {1000, 1500, 2500};
    for (unsigned i = 0; i < 3; ++i) {
        ncr_ctx c;
        ncr_ctx_init(&c, ns[i], rs[i]);
        double x = bench_ns_per_call((tune_bench_fn)bench_ncr_call, &c, 3, g_quick ? 2.0 : 5.0);
        ncr_ctx_free(&c);
        sum += x <= 0.0 ? 1e300 : x;
    }
    return sum;
}

/* pow_1 */
typedef struct {
    mp_ptr dst;
    mp_size_t rn;
    mp_limb_t base;
    ulong exp;
} pow1_ctx;

static void pow1_ctx_init(pow1_ctx* c, mp_limb_t base, ulong exp) {
    c->base = base;
    c->exp = exp;
    c->rn = lmmp_pow_1_size_(base, exp);
    c->dst = (mp_ptr)lmmp_alloc((size_t)c->rn * sizeof(mp_limb_t));
}

static void pow1_ctx_free(pow1_ctx* c) {
    lmmp_free(c->dst);
}

static void bench_pow1_call(void* v) {
    pow1_ctx* c = (pow1_ctx*)v;
    (void)lmmp_pow_1_(c->dst, c->rn, c->base, c->exp);
}

static double obj_pow1(void) {
    double sum = 0.0;
    for (ulong e = 1; e <= 40; ++e) {
        pow1_ctx c;
        pow1_ctx_init(&c, 3, e);
        double ns = bench_ns_per_call((tune_bench_fn)bench_pow1_call, &c, 3, g_quick ? 1.0 : 3.0);
        pow1_ctx_free(&c);
        sum += ns <= 0.0 ? 1e300 : ns;
    }
    return sum;
}

/* elem_mul */
typedef struct {
    mp_ptr dst;
    mp_ptr tp;
    ulong* limbs;
    mp_size_t n;
} elem_ctx;

static void elem_ctx_init(elem_ctx* c, mp_size_t n) {
    c->n = n;
    c->dst = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->tp = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->limbs = (ulong*)lmmp_alloc((size_t)n * sizeof(ulong));
    for (mp_size_t i = 0; i < n; ++i)
        c->limbs[i] = (ulong)(UINT64_C(0x0000000100000001) ^ (uint64_t)i * UINT64_C(0x9e3779b97f4a7c15)) | 1;
}

static void elem_ctx_free(elem_ctx* c) {
    lmmp_free(c->dst);
    lmmp_free(c->tp);
    lmmp_free(c->limbs);
}

static void bench_elem_call(void* v) {
    elem_ctx* c = (elem_ctx*)v;
    (void)lmmp_elem_mul_ulong_(c->dst, c->limbs, c->n, c->tp);
}

static double obj_elem(void) {
    static const mp_size_t sizes[] = {10, 25, 50, 100, 250};
    double sum = 0.0;
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        elem_ctx c;
        elem_ctx_init(&c, sizes[i]);
        double ns = bench_ns_per_call((tune_bench_fn)bench_elem_call, &c, 3, g_quick ? 1.0 : 3.0);
        elem_ctx_free(&c);
        sum += ns <= 0.0 ? 1e300 : ns;
    }
    return sum;
}

/* mat22 */
typedef struct {
    lmmp_mat22_t a;
    lmmp_mat22_t b;
    lmmp_mat22_t d;
    mp_ptr va[4];
    mp_ptr vb[4];
    mp_ptr vd[4];
    mp_size_t n;
} mat22_ctx;

static void mat22_ctx_init(mat22_ctx* c, mp_size_t n) {
    c->n = n;
    for (int i = 0; i < 4; ++i) {
        c->va[i] = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
        c->vb[i] = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
        c->vd[i] = (mp_ptr)lmmp_alloc((size_t)(2 * n + 4) * sizeof(mp_limb_t));
        for (mp_size_t j = 0; j < n; ++j) {
            c->va[i][j] = UINT64_C(0x0123456789abcdef) ^ (uint64_t)(i + 1) * (uint64_t)j * UINT64_C(0x9e3779b97f4a7c15);
            c->vb[i][j] = UINT64_C(0xfedcba9876543210) ^ (uint64_t)(i + 3) * (uint64_t)j * UINT64_C(0xc2b2ae3d27d4eb4f);
        }
        c->va[i][n - 1] |= (uint64_t)1 << 63;
        c->vb[i][n - 1] |= (uint64_t)1 << 63;
    }
    c->a.a00 = c->va[0]; c->a.a01 = c->va[1]; c->a.a10 = c->va[2]; c->a.a11 = c->va[3];
    c->a.n00 = (mp_ssize_t)n; c->a.n01 = (mp_ssize_t)n; c->a.n10 = (mp_ssize_t)n; c->a.n11 = (mp_ssize_t)n;
    c->b.a00 = c->vb[0]; c->b.a01 = c->vb[1]; c->b.a10 = c->vb[2]; c->b.a11 = c->vb[3];
    c->b.n00 = (mp_ssize_t)n; c->b.n01 = (mp_ssize_t)n; c->b.n10 = (mp_ssize_t)n; c->b.n11 = (mp_ssize_t)n;
    c->d.a00 = c->vd[0]; c->d.a01 = c->vd[1]; c->d.a10 = c->vd[2]; c->d.a11 = c->vd[3];
    c->d.n00 = c->d.n01 = c->d.n10 = c->d.n11 = 0;
}

static void mat22_ctx_free(mat22_ctx* c) {
    for (int i = 0; i < 4; ++i) {
        lmmp_free(c->va[i]);
        lmmp_free(c->vb[i]);
        lmmp_free(c->vd[i]);
    }
}

static void bench_mat22_mul_call(void* v) {
    mat22_ctx* c = (mat22_ctx*)v;
    mp_size_t tn = 0, maxa = 0;
    int choose = lmmp_mat22_mul_size_(&c->d, &c->a, &c->b, &tn, &maxa);
    lmmp_mat22_mul_(&c->d, &c->a, &c->b, choose, tn, maxa);
}

static void bench_mat22_sqr_call(void* v) {
    mat22_ctx* c = (mat22_ctx*)v;
    mp_size_t tn = 0;
    /* 与源码一致：平方只根据元素长度选择算法 */
    int choose = (c->n < MAT22_SQR_STRASSEN_THRESHOLD) ? 0 : 1;
    lmmp_mat22_sqr_(&c->d, &c->a, choose, tn > 0 ? tn : 2 * c->n + 4);
}

static double obj_mat22(int square) {
    static const mp_size_t sizes[] = {20, 40, 60, 80, 120, 200};
    double sum = 0.0;
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        mat22_ctx c;
        mat22_ctx_init(&c, sizes[i]);
        double ns = bench_ns_per_call(square ? (tune_bench_fn)bench_mat22_sqr_call : (tune_bench_fn)bench_mat22_mul_call,
                                      &c, 3, g_quick ? 1.0 : 3.0);
        mat22_ctx_free(&c);
        sum += ns <= 0.0 ? 1e300 : ns;
    }
    return sum;
}

static double obj_mat22_mul_wrap(void) { return obj_mat22(0); }
static double obj_mat22_sqr_wrap(void) { return obj_mat22(1); }

/* ============================== 搜索 ============================== */

typedef double (*obj_fn)(void);

static uint64_t search_1d(const char* name, uint64_t lo, uint64_t hi, uint64_t hint,
                          void (*apply)(uint64_t), obj_fn obj) {
    uint64_t best = hint;
    apply(best);
    double best_ns = obj();
    printf("  %-34s start=%llu %.3f ms\n", name, (unsigned long long)best, best_ns * 1e-6);

    uint64_t cur_lo = lo, cur_hi = hi;
    int steps = g_quick ? 6 : 10;
    for (int round = 0; round < 3; ++round) {
        uint64_t prev = best;
        for (int i = 0; i <= steps; ++i) {
            uint64_t v;
            if (cur_hi - cur_lo <= (uint64_t)steps) {
                v = cur_lo + (uint64_t)i;
                if (v > cur_hi) v = cur_hi;
            } else {
                v = cur_lo + (cur_hi - cur_lo) * (uint64_t)i / (uint64_t)steps;
            }
            apply(v);
            double ns = obj();
            printf("    %-30s v=%llu %.3f ms\n", "", (unsigned long long)v, ns * 1e-6);
            if (ns < best_ns) {
                best_ns = ns;
                best = v;
            }
        }
        if (best == prev && best_ns <= 1e300) {
            /* 小范围精细扫描已稳定 */
            if (cur_hi - cur_lo <= 2) break;
            cur_lo = best > cur_lo + 1 ? best - 1 : cur_lo;
            cur_hi = best + 1 < cur_hi ? best + 1 : cur_hi;
            steps = g_quick ? 3 : 5;
        } else {
            uint64_t span = cur_hi - cur_lo;
            uint64_t nl = (best > cur_lo + span / 4) ? best - span / 4 : cur_lo;
            uint64_t nh = (best + span / 4 < cur_hi) ? best + span / 4 : cur_hi;
            if (nh <= nl) { nl = cur_lo; nh = cur_hi; }
            cur_lo = nl;
            cur_hi = nh;
            steps = g_quick ? 4 : 7;
        }
    }
    apply(best);
    printf("  %-34s -> %llu (%.3f ms)\n", name, (unsigned long long)best, best_ns * 1e-6);
    return best;
}

typedef struct {
    uint64_t K;
    uint64_t B;
} pair_t;

/* 在已缓存的 nPr 成本表上做细网格二维搜索。 */
static pair_t search_2d_table(const char* name, int is_uint,
                              uint64_t k_lo, uint64_t k_hi, uint64_t k_hint,
                              uint64_t b_lo, uint64_t b_hi, uint64_t b_hint) {
    pair_t best = {k_hint, b_hint};
    double best_ns = npr_cache_eval(is_uint, best.K, best.B);
    printf("  %-34s start=(K=%llu,B=%llu) %.3f ms\n", name,
           (unsigned long long)best.K, (unsigned long long)best.B, best_ns * 1e-6);

    int k_steps = g_quick ? 20 : 40;
    int b_steps = g_quick ? 20 : 40;
    int rounds = g_quick ? 2 : 3;

    for (int round = 0; round < rounds; ++round) {
        for (int i = 0; i <= k_steps; ++i) {
            uint64_t k = k_lo + (k_hi - k_lo) * (uint64_t)i / (uint64_t)k_steps;
            for (int j = 0; j <= b_steps; ++j) {
                double lr = log((double)(b_lo > 0 ? b_lo : 1));
                double hr = log((double)(b_hi > 0 ? b_hi : 1));
                double lv = lr + (hr - lr) * (double)j / (double)b_steps;
                uint64_t b = (uint64_t)(exp(lv) + 0.5);
                if (b < b_lo) b = b_lo;
                if (b > b_hi) b = b_hi;
                double ns = npr_cache_eval(is_uint, k, b);
                if (ns < best_ns) {
                    best_ns = ns;
                    best.K = k;
                    best.B = b;
                }
            }
        }
        printf("    %-30s round=%d best=(%llu,%llu) %.3f ms\n", "", round,
               (unsigned long long)best.K, (unsigned long long)best.B, best_ns * 1e-6);
        /* 在最优值附近细化。 */
        uint64_t k_span = (k_hi - k_lo) / 4 + 1;
        uint64_t k_lo_new = best.K > k_span ? best.K - k_span : k_lo;
        uint64_t k_hi_new = best.K + k_span < k_hi ? best.K + k_span : k_hi;
        double best_lr = log((double)(best.B > 0 ? best.B : 1));
        double lr = log((double)(b_lo > 0 ? b_lo : 1));
        double hr = log((double)(b_hi > 0 ? b_hi : 1));
        double half = (hr - lr) / 8.0;
        uint64_t b_lo_new = (uint64_t)(exp(best_lr - half) + 0.5);
        uint64_t b_hi_new = (uint64_t)(exp(best_lr + half) + 0.5);
        if (k_hi_new <= k_lo_new) { k_lo_new = k_lo; k_hi_new = k_hi; }
        if (b_hi_new <= b_lo_new) { b_lo_new = b_lo; b_hi_new = b_hi; }
        k_lo = k_lo_new; k_hi = k_hi_new; b_lo = b_lo_new; b_hi = b_hi_new;
    }

    if (is_uint) apply_npr_uint(best.K, best.B);
    else         apply_npr_ushort(best.K, best.B);
    best_ns = npr_cache_eval(is_uint, best.K, best.B);
    printf("  %-34s -> (K=%llu,B=%llu) (%.3f ms)\n", name,
           (unsigned long long)best.K, (unsigned long long)best.B, best_ns * 1e-6);
    return best;
}

/* ============================== 阈值应用与结果输出 ============================== */

typedef struct {
    const char* name;
    uint64_t value;
    uint64_t old_value;
} tuned_t;

#define MAX_TUNED 12
static tuned_t g_tuned[MAX_TUNED];
static int g_tuned_n = 0;

static void add_tuned(const char* name, uint64_t old_value, uint64_t new_value) {
    if (g_tuned_n < MAX_TUNED) {
        g_tuned[g_tuned_n].name = name;
        g_tuned[g_tuned_n].old_value = old_value;
        g_tuned[g_tuned_n].value = new_value;
        ++g_tuned_n;
    }
}

static void apply_mul22(uint64_t v) { lmmp_tune_MUL_TOOM22_THRESHOLD = v; }
static void apply_mul33(uint64_t v) { lmmp_tune_MUL_TOOM33_THRESHOLD = v; }
static void apply_mul44(uint64_t v) { lmmp_tune_MUL_TOOM44_THRESHOLD = v; }
static void apply_mullo_base(uint64_t v) { lmmp_tune_MULLO_BASECASE_THRESHOLD = v; }
static void apply_binom(uint64_t v) { lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD = v; }
static void apply_pow1(uint64_t v) { lmmp_tune_POW_1_EXP_THRESHOLD = v; }
static void apply_elem(uint64_t v) { lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD = v; }
static void apply_mat22_mul(uint64_t v) { lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD = v; }
static void apply_mat22_sqr(uint64_t v) { lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD = v; }

static void apply_npr_ushort(uint64_t k, uint64_t b) {
    lmmp_tune_PERMUTATION_USHORT_K_THRESHOLD = k;
    lmmp_tune_PERMUTATION_USHORT_B_THRESHOLD = b;
}

static void apply_npr_uint(uint64_t k, uint64_t b) {
    lmmp_tune_PERMUTATION_UINT_K_THRESHOLD = k;
    lmmp_tune_PERMUTATION_UINT_B_THRESHOLD = b;
}

static int want(const char* only, const char* name) {
    if (only == NULL || only[0] == '\0') return 1;
    const char* p = only;
    while (p != NULL && *p != '\0') {
        const char* q = strchr(p, ',');
        size_t len = q ? (size_t)(q - p) : strlen(p);
        if (strncmp(p, name, len) == 0 && name[len] == '\0') return 1;
        p = q ? q + 1 : NULL;
    }
    return 0;
}

/* ============================== mparam.h 写回 ============================== */

static void write_mparam(void) {
    const char* path = LMMP_SOURCE_DIR "/include/lmmp/impl/mparam.h";
    FILE* f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "Warning: cannot open %s for writing back.\n", path);
        return;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc((size_t)sz + 1);
    if (buf == NULL) { fclose(f); return; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    buf[rd] = '\0';
    fclose(f);

    for (int i = 0; i < g_tuned_n; ++i) {
        char needle[96];
        snprintf(needle, sizeof(needle), "#define %s ", g_tuned[i].name);
        char* pos = strstr(buf, needle);
        if (pos != NULL) {
            char* line_end = strchr(pos, '\n');
            char* insert = (char*)malloc(256);
            if (insert == NULL) continue;
            snprintf(insert, 256, "#define %s %llu", g_tuned[i].name,
                     (unsigned long long)g_tuned[i].value);
            size_t old_line = line_end ? (size_t)(line_end - pos) : strlen(pos);
            size_t new_len = strlen(insert);
            size_t tail = strlen(pos);
            if (new_len != old_line) {
                memmove(pos + new_len, pos + old_line, tail - old_line + 1);
            }
            memcpy(pos, insert, new_len);
            free(insert);
        }
    }
    f = fopen(path, "w");
    if (f == NULL) {
        fprintf(stderr, "Warning: cannot write %s\n", path);
    } else {
        fwrite(buf, 1, strlen(buf), f);
        fclose(f);
        printf("Updated %s\n", path);
    }
    free(buf);
}

/* ============================== main ============================== */

int main(int argc, char** argv) {
    const char* only = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--quick") == 0) {
            g_quick = 1;
        } else if (strcmp(argv[i], "--write") == 0) {
            g_write = 1;
        } else if (strcmp(argv[i], "--only") == 0 && i + 1 < argc) {
            only = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: lmmp_tune [--quick] [--write] [--only name1,name2,...]\n");
            return 0;
        }
    }

    timer_init();
    lmmp_global_init();
    printf("LMMP tune driver\n");
    printf("mode: %s, write: %s\n", g_quick ? "quick" : "normal", g_write ? "yes" : "no");

    if (want(only, "mul22")) {
        printf("\n[Tune] MUL_TOOM22_THRESHOLD\n");
        uint64_t old = lmmp_tune_MUL_TOOM22_THRESHOLD;
        uint64_t v = search_1d("MUL_TOOM22_THRESHOLD", 5, 60, old,
                               apply_mul22, obj_mul);
        add_tuned("MUL_TOOM22_THRESHOLD", old, v);
        lmmp_tune_MUL_TOOM22_THRESHOLD = v;
    }
    if (want(only, "mul33")) {
        printf("\n[Tune] MUL_TOOM33_THRESHOLD\n");
        uint64_t old = lmmp_tune_MUL_TOOM33_THRESHOLD;
        uint64_t v = search_1d("MUL_TOOM33_THRESHOLD", 30, 220, old,
                               apply_mul33, obj_mul);
        add_tuned("MUL_TOOM33_THRESHOLD", old, v);
        lmmp_tune_MUL_TOOM33_THRESHOLD = v;
    }
    if (want(only, "mul44")) {
        printf("\n[Tune] MUL_TOOM44_THRESHOLD\n");
        uint64_t old = lmmp_tune_MUL_TOOM44_THRESHOLD;
        uint64_t v = search_1d("MUL_TOOM44_THRESHOLD", 200, 1200, old,
                               apply_mul44, obj_mul);
        add_tuned("MUL_TOOM44_THRESHOLD", old, v);
        lmmp_tune_MUL_TOOM44_THRESHOLD = v;
    }
    if (want(only, "mullo")) {
        printf("\n[Tune] MULLO_BASECASE_THRESHOLD\n");
        uint64_t old = lmmp_tune_MULLO_BASECASE_THRESHOLD;
        uint64_t v = search_1d("MULLO_BASECASE_THRESHOLD", 3, 100, old,
                               apply_mullo_base, obj_mullo);
        add_tuned("MULLO_BASECASE_THRESHOLD", old, v);
        lmmp_tune_MULLO_BASECASE_THRESHOLD = v;
    }
    if (want(only, "npr_ushort")) {
        printf("\n[Tune] PERMUTATION_USHORT (K,B)\n");
        uint64_t old_k = lmmp_tune_PERMUTATION_USHORT_K_THRESHOLD;
        uint64_t old_b = lmmp_tune_PERMUTATION_USHORT_B_THRESHOLD;
        npr_cache_fill(0);
        pair_t p = search_2d_table("PERMUTATION_USHORT", 0, 2, 64, old_k,
                                   512, 262144, old_b);
        add_tuned("PERMUTATION_USHORT_K_THRESHOLD", old_k, p.K);
        add_tuned("PERMUTATION_USHORT_B_THRESHOLD", old_b, p.B);
        lmmp_tune_PERMUTATION_USHORT_K_THRESHOLD = p.K;
        lmmp_tune_PERMUTATION_USHORT_B_THRESHOLD = p.B;
    }
    if (want(only, "npr_uint")) {
        printf("\n[Tune] PERMUTATION_UINT (K,B)\n");
        uint64_t old_k = lmmp_tune_PERMUTATION_UINT_K_THRESHOLD;
        uint64_t old_b = lmmp_tune_PERMUTATION_UINT_B_THRESHOLD;
        npr_cache_fill(1);
        pair_t p = search_2d_table("PERMUTATION_UINT", 1, 16, 512, old_k,
                                   65536, 16777216, old_b);
        add_tuned("PERMUTATION_UINT_K_THRESHOLD", old_k, p.K);
        add_tuned("PERMUTATION_UINT_B_THRESHOLD", old_b, p.B);
        lmmp_tune_PERMUTATION_UINT_K_THRESHOLD = p.K;
        lmmp_tune_PERMUTATION_UINT_B_THRESHOLD = p.B;
    }
    if (want(only, "ncr")) {
        printf("\n[Tune] BINOMIAL_RN_BASECASE_THRESHOLD\n");
        uint64_t old = lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD;
        uint64_t v = search_1d("BINOMIAL_RN_BASECASE_THRESHOLD", 10, 120, old,
                               apply_binom, obj_ncr);
        add_tuned("BINOMIAL_RN_BASECASE_THRESHOLD", old, v);
        lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD = v;
    }
    if (want(only, "pow1")) {
        printf("\n[Tune] POW_1_EXP_THRESHOLD\n");
        uint64_t old = lmmp_tune_POW_1_EXP_THRESHOLD;
        uint64_t v = search_1d("POW_1_EXP_THRESHOLD", 1, 50, old,
                               apply_pow1, obj_pow1);
        add_tuned("POW_1_EXP_THRESHOLD", old, v);
        lmmp_tune_POW_1_EXP_THRESHOLD = v;
    }
    if (want(only, "elem")) {
        printf("\n[Tune] ELEM_MUL_BASECASE_THRESHOLD\n");
        uint64_t old = lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD;
        uint64_t v = search_1d("ELEM_MUL_BASECASE_THRESHOLD", 5, 100, old,
                               apply_elem, obj_elem);
        add_tuned("ELEM_MUL_BASECASE_THRESHOLD", old, v);
        lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD = v;
    }
    if (want(only, "mat22_mul")) {
        printf("\n[Tune] MAT22_MUL_STRASSEN_THRESHOLD\n");
        uint64_t old = lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD;
        uint64_t v = search_1d("MAT22_MUL_STRASSEN_THRESHOLD", 20, 160, old,
                               apply_mat22_mul, obj_mat22_mul_wrap);
        add_tuned("MAT22_MUL_STRASSEN_THRESHOLD", old, v);
        lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD = v;
    }
    if (want(only, "mat22_sqr")) {
        printf("\n[Tune] MAT22_SQR_STRASSEN_THRESHOLD\n");
        uint64_t old = lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD;
        uint64_t v = search_1d("MAT22_SQR_STRASSEN_THRESHOLD", 20, 160, old,
                               apply_mat22_sqr, obj_mat22_sqr_wrap);
        add_tuned("MAT22_SQR_STRASSEN_THRESHOLD", old, v);
        lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD = v;
    }

    printf("\n==================== Tune summary ====================\n");
    for (int i = 0; i < g_tuned_n; ++i) {
        printf("%-38s %llu -> %llu\n", g_tuned[i].name,
               (unsigned long long)g_tuned[i].old_value,
               (unsigned long long)g_tuned[i].value);
    }
    if (g_write && g_tuned_n > 0) {
        write_mparam();
    } else if (g_tuned_n > 0) {
        printf("\nUse --write to write these values back into include/lmmp/impl/mparam.h.\n");
    }

    lmmp_global_deinit();
    return 0;
}
