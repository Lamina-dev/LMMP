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

#include "lmmp/lmmpn.h"
#include "lmmp/mprand.h"
#include "lmmp/numth.h"
#include "lmmp_bench.hpp"

using namespace lmmp_bench;

namespace {

const int POOL = 1024;

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void fill_random(mp_ptr p, mp_size_t n, mp_limb_t seed) {
    lmmp_seed_random_(p, n, seed, 1);
    if (n > 0) p[n - 1] |= (mp_limb_t)1 << 63;
}

}  // namespace

/* cbrt_3 / cbrt_6 估计路径（log2/exp2 种子）与除法快速路径的性能基线 */

BENCH_CASE("numth/cbrt", cbrt_3) {
    mp_ptr a = alloc_limbs(3 * POOL);
    for (int i = 0; i < POOL; i++) {
        lmmp_seed_random_(a + 3 * i, 3, 0xC0FFEEull + i, 1);
        if (a[3 * i + 1] == 0 && a[3 * i + 2] == 0) a[3 * i + 1] = 1;
    }

    size_t idx = 0;
    auto m = measure([&] {
        mp_limb_t* p = a + 3 * (idx++ & (POOL - 1));
        lmmp_cbrt_3_(p[0], p[1], p[2]);
    });
    report("lmmp_cbrt_3_(3 limbs)", m);

    lmmp_free(a);
}

BENCH_CASE("numth/cbrt", cbrt_6_est_n4) {
    mp_ptr a = alloc_limbs(6 * POOL);
    mp_ptr r = alloc_limbs(2);
    for (int i = 0; i < POOL; i++) fill_random(a + 6 * i, 4, 0x1234578ull + i);

    size_t idx = 0;
    auto m = measure([&] { lmmp_cbrt_6_(r, a + 6 * (idx++ & (POOL - 1)), 4); });
    report("lmmp_cbrt_6_(n=4 est)", m);

    lmmp_free(a);
    lmmp_free(r);
}

BENCH_CASE("numth/cbrt", cbrt_6_est_n5) {
    mp_ptr a = alloc_limbs(6 * POOL);
    mp_ptr r = alloc_limbs(2);
    for (int i = 0; i < POOL; i++) fill_random(a + 6 * i, 5, 0x9ABCDEF1ull + i);

    size_t idx = 0;
    auto m = measure([&] { lmmp_cbrt_6_(r, a + 6 * (idx++ & (POOL - 1)), 5); });
    report("lmmp_cbrt_6_(n=5 est)", m);

    lmmp_free(a);
    lmmp_free(r);
}

// n=6 且最高 limb < CBRT_DIVIDE_MIN，强制走 log2/exp2 估计路径
BENCH_CASE("numth/cbrt", cbrt_6_est_n6) {
    mp_ptr a = alloc_limbs(6 * POOL);
    mp_ptr r = alloc_limbs(2);
    for (int i = 0; i < POOL; i++) {
        fill_random(a + 6 * i, 6, 0xFEDCBA98ull + i);
        mp_limb_t* p = a + 6 * i;
        p[5] &= 0x5FFFFFFFFFFFFFFFull;
        if (p[5] == 0) p[5] = 1;
    }

    size_t idx = 0;
    auto m = measure([&] { lmmp_cbrt_6_(r, a + 6 * (idx++ & (POOL - 1)), 6); });
    report("lmmp_cbrt_6_(n=6 est)", m);

    lmmp_free(a);
    lmmp_free(r);
}

// n=6 且最高 limb >= CBRT_DIVIDE_MIN，走 cbrt_3 种子 + 除法路径（对照）
BENCH_CASE("numth/cbrt", cbrt_6_fast_n6) {
    mp_ptr a = alloc_limbs(6 * POOL);
    mp_ptr r = alloc_limbs(2);
    for (int i = 0; i < POOL; i++) fill_random(a + 6 * i, 6, 0x13579BDFull + i);

    size_t idx = 0;
    auto m = measure([&] { lmmp_cbrt_6_(r, a + 6 * (idx++ & (POOL - 1)), 6); });
    report("lmmp_cbrt_6_(n=6 fast)", m);

    lmmp_free(a);
    lmmp_free(r);
}

/*
    非精确牛顿立方根（梅森变换加速，nf >> na 场景）与精确语义
    lmmp_cbrt_divide_ 的性能对照。

    输入取顶 limb ∈ [0xC000.., B) 的随机 a（divide 路径可直接归一化），
    radicand = a*B^(3nf) 共 3*ns limb（na 取 3 的倍数避免补位）。
    divide 计时已扣除 radicand 拷贝开销。
*/

static void bench_cbrt_newton_vs_divide(mp_size_t na, mp_size_t nf) {
    mp_size_t ns = (na + 3 * nf) / 3;
    mp_ptr a = alloc_limbs(na);
    lmmp_seed_random_(a, na, 0xC0FFEEull + na * 31 + nf, 1);
    a[na - 1] |= (mp_limb_t)3 << 62;

    mp_ptr radicand = alloc_limbs(3 * ns);
    mp_ptr radbuf = alloc_limbs(3 * ns);
    mp_ptr dst = alloc_limbs(ns + 8);
    mp_ptr tp = alloc_limbs(4 * ns + 8);
    lmmp_zero(radicand, 3 * nf);
    lmmp_copy(radicand + 3 * nf, a, na);

    auto m = measure([&] { lmmp_cbrt_newton_(dst, a, na, nf); });
    report("lmmp_cbrt_newton_(round)", m);

    // cbrt_divide_ 会覆写 radicand，故每轮重拷，并扣除拷贝开销
    auto md = measure([&] {
        lmmp_copy(radbuf, radicand, 3 * ns);
        lmmp_cbrt_divide_(dst, radbuf, ns, tp, 0);
    });
    auto mc = measure([&] { lmmp_copy(radbuf, radicand, 3 * ns); });
    double tdiv = md.ns_per_op - mc.ns_per_op;
    if (tdiv < md.ns_per_op * 0.5) tdiv = md.ns_per_op;  // 拷贝占比异常时放弃校正
    std::printf("  %-32s %12.2f ns/op\n", "lmmp_cbrt_divide_(exact)", tdiv);

    lmmp_free(a);
    lmmp_free(radicand);
    lmmp_free(radbuf);
    lmmp_free(dst);
    lmmp_free(tp);
}

BENCH_CASE("numth/cbrt", cbrt_newton_nf2000_na3) { bench_cbrt_newton_vs_divide(3, 2000); }

BENCH_CASE("numth/cbrt", cbrt_newton_nf8000_na3) { bench_cbrt_newton_vs_divide(3, 8000); }

BENCH_CASE("numth/cbrt", cbrt_newton_nf32000_na3) { bench_cbrt_newton_vs_divide(3, 32000); }

BENCH_CASE("numth/cbrt", cbrt_newton_nf8000_na24) { bench_cbrt_newton_vs_divide(24, 8000); }

BENCH_CASE("numth/cbrt", cbrt_newton_nf32000_na48) { bench_cbrt_newton_vs_divide(48, 32000); }

/*
    lmmp_cbrt_ 精确路径（nf=0 带余数）：class-1（3 对齐移位直达 divide）
    与 class-U（bl≡1 mod 3，k^3 乘子归一化后 divide）对照。两条路径仅
    差一次 mul_1/div_1，预期耗时一致；对照旧实现（class-U 走牛顿 B^(3T)
    垫高 + 立方修正循环）为数量级差异。
*/

static void bench_cbrt_exact_rem(mp_size_t na, int cls) {
    mp_size_t ns = (na + 2) / 3;
    mp_ptr a = alloc_limbs(na);
    lmmp_seed_random_(a, na, 0xBEEFull + na * 13 + cls, 1);
    if (cls == 0) {
        a[na - 1] |= (mp_limb_t)3 << 62;  // 顶 2bit=11，大概率 3 对齐可达
    } else {
        // 强制 bl ≡ 1 (mod 3)：3 对齐移位恒不可达 3B/8，必走 k^3 乘子
        int hb = 2 - (int)(na - 1) % 3;
        while (hb < 2) hb += 3;
        a[na - 1] = ((mp_limb_t)1 << (hb - 1)) | (a[na - 1] & (((mp_limb_t)1 << (hb - 1)) - 1));
    }
    mp_ptr dsts = alloc_limbs(ns + 4);
    mp_ptr dstr = alloc_limbs(2 * na / 3 + 8);
    auto m = measure([&] { lmmp_cbrt_(dsts, dstr, a, na, 0); });
    report(cls ? "lmmp_cbrt_(exact_rem, class-U k^3)" : "lmmp_cbrt_(exact_rem, class-1 shift)", m);
    lmmp_free(a);
    lmmp_free(dsts);
    lmmp_free(dstr);
}

BENCH_CASE("numth/cbrt", cbrt_exact_rem_na1000_c1) { bench_cbrt_exact_rem(1000, 0); }

BENCH_CASE("numth/cbrt", cbrt_exact_rem_na1000_cU) { bench_cbrt_exact_rem(1000, 1); }

BENCH_CASE("numth/cbrt", cbrt_exact_rem_na3000_c1) { bench_cbrt_exact_rem(3000, 0); }

BENCH_CASE("numth/cbrt", cbrt_exact_rem_na3000_cU) { bench_cbrt_exact_rem(3000, 1); }
