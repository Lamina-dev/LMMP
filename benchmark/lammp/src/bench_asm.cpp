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

#include "lammp/lmmpn.h"
#include "lammp/impl/mparam.h"
#include "lammp_bench.hpp"

#include <cstring>

using namespace lammp_bench;

namespace {

volatile mp_limb_t g_sink = 0;

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void fill_random(mp_ptr p, mp_size_t n, mp_limb_t seed) {
    for (mp_size_t i = 0; i < n; ++i) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        p[i] = seed;
    }
    if (n > 0) p[n - 1] |= (mp_limb_t)1 << 63;
}

}  // namespace

BENCH_CASE("asm/addsub", add_n) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x123456789ull);
    fill_random(b, n, 0x987654321ull);
    auto m = measure([&] { g_sink += lmmp_add_n_(d, a, b, n); });
    report("lmmp_add_n_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/addsub", add_nc) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x123456789ull);
    fill_random(b, n, 0x987654321ull);
    auto m = measure([&] { g_sink += lmmp_add_nc_(d, a, b, n, 1); });
    report("lmmp_add_nc_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/addsub", sub_n) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x123456789ull);
    fill_random(b, n, 0x987654321ull);
    auto m = measure([&] { g_sink += lmmp_sub_n_(d, a, b, n); });
    report("lmmp_sub_n_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/addsub", sub_nc) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x123456789ull);
    fill_random(b, n, 0x987654321ull);
    auto m = measure([&] { g_sink += lmmp_sub_nc_(d, a, b, n, 1); });
    report("lmmp_sub_nc_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/mul1", mul_1) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x0a0b0c0d0e0f0102ull);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    auto m = measure([&] { g_sink += lmmp_mul_1_(d, a, n, x); });
    report("lmmp_mul_1_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(d);
}

BENCH_CASE("asm/mul1", addmul_1) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x1112131415161718ull);
    fill_random(d, n, 0x2122232425262728ull);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    auto m = measure([&] { g_sink += lmmp_addmul_1_(d, a, n, x); });
    report("lmmp_addmul_1_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(d);
}

BENCH_CASE("asm/mul1", submul_1) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x3132333435363738ull);
    fill_random(d, n, 0x4142434445464748ull);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    auto m = measure([&] { g_sink += lmmp_submul_1_(d, a, n, x); });
    report("lmmp_submul_1_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(d);
}

BENCH_CASE("asm/mul1", addshl1_n) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x5152535455565758ull);
    fill_random(b, n, 0x6162636465666768ull);
    auto m = measure([&] { g_sink += lmmp_addshl1_n_(d, a, b, n); });
    report("lmmp_addshl1_n_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/mul1", subshl1_n) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x7172737475767778ull);
    fill_random(b, n, 0x8182838485868788ull);
    auto m = measure([&] { g_sink += lmmp_subshl1_n_(d, a, b, n); });
    report("lmmp_subshl1_n_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/shift", shl) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x9192939495969798ull);
    auto m = measure([&] { g_sink += lmmp_shl_(d, a, n, 13); });
    report("lmmp_shl_(1000,13)", m, n * 8.0);
    lmmp_free(a); lmmp_free(d);
}

BENCH_CASE("asm/shift", shr) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xa1a2a3a4a5a6a7a8ull);
    auto m = measure([&] { g_sink += lmmp_shr_(d, a, n, 13); });
    report("lmmp_shr_(1000,13)", m, n * 8.0);
    lmmp_free(a); lmmp_free(d);
}

BENCH_CASE("asm/shift", shlnot) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xb1b2b3b4b5b6b7b8ull);
    auto m = measure([&] { g_sink += lmmp_shlnot_(d, a, n, 13); });
    report("lmmp_shlnot_(1000,13)", m, n * 8.0);
    lmmp_free(a); lmmp_free(d);
}

BENCH_CASE("asm/shift", not) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xc1c2c3c4c5c6c7c8ull);
    auto m = measure([&] { lmmp_not_(d, a, n); g_sink += d[0]; });
    report("lmmp_not_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(d);
}

BENCH_CASE("asm/shift", shr1add_n) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xd1d2d3d4d5d6d7d8ull);
    fill_random(b, n, 0xe1e2e3e4e5e6e7e8ull);
    auto m = measure([&] { g_sink += lmmp_shr1add_n_(d, a, b, n); });
    report("lmmp_shr1add_n_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/shift", shr1sub_n) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xf1f2f3f4f5f6f7f8ull);
    fill_random(b, n, 0x1112131415161718ull);
    auto m = measure([&] { g_sink += lmmp_shr1sub_n_(d, a, b, n); });
    report("lmmp_shr1sub_n_(1000)", m, n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/mulbase", mul_basecase) {
    const mp_size_t n = 200;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(2 * n + 1);
    fill_random(a, n, 0x0badcafef00dfaceull);
    fill_random(b, n, 0x13579bdf2468ace0ull);
    auto m = measure([&] { lmmp_mul_basecase_(d, a, n, b, n); g_sink += d[0]; });
    report("lmmp_mul_basecase_(200x200)", m, n * n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/mulbase", sqr_basecase) {
    const mp_size_t n = 200;
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(2 * n + 1);
    fill_random(a, n, 0x2468ace013579bdfull);
    auto m = measure([&] { lmmp_sqr_basecase_(d, a, n); g_sink += d[0]; });
    report("lmmp_sqr_basecase_(200)", m, n * n * 8.0);
    lmmp_free(a); lmmp_free(d);
}

BENCH_CASE("asm/mulbase", mullo_basecase) {
    const mp_size_t n = 200;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x0f0e0d0c0b0a0908ull);
    fill_random(b, n, 0x0102030405060708ull);
    auto m = measure([&] { lmmp_mullo_basecase_(d, a, b, n); g_sink += d[0]; });
    report("lmmp_mullo_basecase_(200)", m, n * n * 8.0);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

BENCH_CASE("asm/div", div_1) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1);
    fill_random(a, n, 0x0123456789abcdeful);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    auto m = measure([&] { g_sink += lmmp_div_1_(q, a, n, x); });
    report("lmmp_div_1_(1000)", m);
    lmmp_free(a); lmmp_free(q);
}

BENCH_CASE("asm/div", div_1_s) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1);
    fill_random(a, n, 0x0123456789abcdeful);
    mp_limb_t x = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    auto m = measure([&] {
        mp_limb_t save = a[0];
        g_sink += lmmp_div_1_s_(q, a, n, x);
        a[0] = save;
    });
    report("lmmp_div_1_s_(1000)", m);
    lmmp_free(a); lmmp_free(q);
}

BENCH_CASE("asm/div", mod_1) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n);
    fill_random(a, n, 0x3141592653589793ull);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    auto m = measure([&] { g_sink += lmmp_mod_1_(a, n, x); });
    report("lmmp_mod_1_(1000)", m);
    lmmp_free(a);
}

BENCH_CASE("asm/div", div_2) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), b = alloc_limbs(2), bw = alloc_limbs(2);
    fill_random(a, n, 0x2718281828459045ull);
    b[1] = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    b[0] = 0x3141592653589793ull;
    auto m = measure([&] { bw[0] = b[0]; bw[1] = b[1]; lmmp_div_2_(q, a, n, bw); g_sink += q[0]; });
    report("lmmp_div_2_(1000)", m);
    lmmp_free(a); lmmp_free(q); lmmp_free(b); lmmp_free(bw);
}

BENCH_CASE("asm/div", div_2_s) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), b = alloc_limbs(2);
    fill_random(a, n, 0x2718281828459045ull);
    b[1] = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    b[0] = 0x3141592653589793ull;
    auto m = measure([&] {
        mp_limb_t s0 = a[0], s1 = a[1];
        g_sink += lmmp_div_2_s_(q, a, n, b);
        a[0] = s0; a[1] = s1;
    });
    report("lmmp_div_2_s_(1000)", m);
    lmmp_free(a); lmmp_free(q); lmmp_free(b);
}

BENCH_CASE("asm/div", mod_2) {
    const mp_size_t n = 1000;
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(2), bw = alloc_limbs(2);
    fill_random(a, n, 0x0badf00dcafefaceull);
    b[1] = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    b[0] = 0x3141592653589793ull;
    auto m = measure([&] { bw[0] = b[0]; bw[1] = b[1]; lmmp_mod_2_(a, n, bw); g_sink += bw[0]; });
    report("lmmp_mod_2_(1000)", m);
    lmmp_free(a); lmmp_free(b); lmmp_free(bw);
}

BENCH_CASE("asm/div", div_3_2) {
    const int iters = 10000;
    mp_ptr np = alloc_limbs(3);
    mp_ptr npw = alloc_limbs(3);
    mp_ptr dp = alloc_limbs(2);
    np[0] = 0x1111111122222222ull;
    np[1] = 0x3333333344444444ull;
    np[2] = 0x5555555566666666ull;
    dp[0] = 0x7777777788888888ull;
    dp[1] = 0x99999999aaaaaaaall | LIMB_B_2;
    mp_limb_t inv21 = lmmp_inv_2_1_(dp[1], dp[0]);
    auto m = measure([&] {
        for (int i = 0; i < iters; ++i) {
            npw[0] = np[0]; npw[1] = np[1]; npw[2] = np[2];
            g_sink += lmmp_div_3_2_(npw, dp, inv21);
        }
    });
    report("lmmp_div_3_2_(x10000)", m);
    lmmp_free(np); lmmp_free(npw); lmmp_free(dp);
}

BENCH_CASE("asm/inv", inv_1) {
    const int iters = 10000;
    mp_limb_t x = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    auto m = measure([&] {
        for (int i = 0; i < iters; ++i) g_sink += lmmp_inv_1_(x);
    });
    report("lmmp_inv_1_(x10000)", m);
}

BENCH_CASE("asm/inv", inv_2_1) {
    const int iters = 10000;
    mp_limb_t xh = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    mp_limb_t xl = 0x3141592653589793ull;
    auto m = measure([&] {
        for (int i = 0; i < iters; ++i) g_sink += lmmp_inv_2_1_(xh, xl);
    });
    report("lmmp_inv_2_1_(x10000)", m);
}
