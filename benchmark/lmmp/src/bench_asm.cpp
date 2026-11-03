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
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "lmmp/lmmp.h"
#include "lmmp/lmmpn.h"
#include "lmmp/mprand.h"
#include "lmmp/impl/mparam.h"
#include "lmmp_bench.hpp"
#include "lmmp_bench_lib.hpp"

namespace lmmp_bench {

/*
    低层原语基准：全部以长度 n（limbs）为入参，随机归一化输入，
    返回微秒/op。
*/

double bench_add_n(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x123456789ull + n);
    fill_random(b, n, 0x987654321ull + n);
    double us = measure([&] { sink += lmmp_add_n_(d, a, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_add_nc(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x12345678aull + n);
    fill_random(b, n, 0x987654322ull + n);
    double us = measure([&] { sink += lmmp_add_nc_(d, a, b, n, 1); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_sub_n(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x12345678bull + n);
    fill_random(b, n, 0x876543210ull + n);
    double us = measure([&] { sink += lmmp_sub_n_(d, a, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_sub_nc(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x12345678cull + n);
    fill_random(b, n, 0x765432101ull + n);
    double us = measure([&] { sink += lmmp_sub_nc_(d, a, b, n, 1); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_mul_1(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x0a0b0c0d0e0f0102ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us = measure([&] { sink += lmmp_mul_1_(d, a, n, x); }, o);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

double bench_addmul_1(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x1112131415161718ull + n);
    fill_random(d, n, 0x2122232425262728ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us = measure([&] { sink += lmmp_addmul_1_(d, a, n, x); }, o);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

double bench_submul_1(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x3132333435363738ull + n);
    fill_random(d, n, 0x4142434445464748ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us = measure([&] { sink += lmmp_submul_1_(d, a, n, x); }, o);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

double bench_addshl1_n(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x5152535455565758ull + n);
    fill_random(b, n, 0x6162636465666768ull + n);
    double us = measure([&] { sink += lmmp_addshl1_n_(d, a, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_subshl1_n(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x7172737475767778ull + n);
    fill_random(b, n, 0x8182838485868788ull + n);
    double us = measure([&] { sink += lmmp_subshl1_n_(d, a, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_shl(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x9192939495969798ull + n);
    double us = measure([&] { sink += lmmp_shl_(d, a, n, 13); }, o);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

double bench_shr(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xa1a2a3a4a5a6a7a8ull + n);
    double us = measure([&] { sink += lmmp_shr_(d, a, n, 13); }, o);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

double bench_shlnot(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xb1b2b3b4b5b6b7b8ull + n);
    double us = measure([&] { sink += lmmp_shlnot_(d, a, n, 13); }, o);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

double bench_not(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xc1c2c3c4c5c6c7c8ull + n);
    double us = measure([&] { lmmp_not_(d, a, n); sink += d[0]; }, o);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

double bench_shr1add_n(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xd1d2d3d4d5d6d7d8ull + n);
    fill_random(b, n, 0xe1e2e3e4e5e6e7e8ull + n);
    double us = measure([&] { sink += lmmp_shr1add_n_(d, a, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_shr1sub_n(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xf1f2f3f4f5f6f7f8ull + n);
    fill_random(b, n, 0x1112131415162718ull + n);
    double us = measure([&] { sink += lmmp_shr1sub_n_(d, a, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_mul_basecase(mp_size_t a, mp_size_t b, const BenchOpts& o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 1);
    fill_random(pa, a, 0x0badcafef00dfaceull + a * 31 + b);
    fill_random(pb, b, 0x13579bdf2468ace0ull + b * 17 + a);
    double us = measure([&] { lmmp_mul_basecase_(d, pa, a, pb, b); sink += d[0]; }, o);
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

double bench_sqr_basecase(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(2 * n + 1);
    fill_random(a, n, 0x2468ace013579bdfull + n);
    double us = measure([&] { lmmp_sqr_basecase_(d, a, n); sink += d[0]; }, o);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

double bench_mullo_basecase(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x0f0e0d0c0b0a0908ull + n);
    fill_random(b, n, 0x0102030405060708ull + n);
    double us = measure([&] { lmmp_mullo_basecase_(d, a, b, n); sink += d[0]; }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_div_1(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1);
    fill_random(a, n, 0x0123456789abcdefull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us = measure([&] { sink += lmmp_div_1_(q, a, n, x); }, o);
    lmmp_free(a);
    lmmp_free(q);
    return us;
}

double bench_div_1_s(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1);
    fill_random(a, n, 0x0123456789abcdf1ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    double us = measure(
        [&] {
            mp_limb_t save = a[0];
            sink += lmmp_div_1_s_(q, a, n, x);
            a[0] = save;
        },
        o);
    lmmp_free(a);
    lmmp_free(q);
    return us;
}

double bench_mod_1(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n);
    fill_random(a, n, 0x3141592653589793ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us = measure([&] { sink += lmmp_mod_1_(a, n, x); }, o);
    lmmp_free(a);
    return us;
}

double bench_div_2(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), b = alloc_limbs(2), bw = alloc_limbs(2);
    fill_random(a, n, 0x2718281828459045ull + n);
    b[1] = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    b[0] = 0x3141592653589793ull;
    double us = measure(
        [&] {
            bw[0] = b[0];
            bw[1] = b[1];
            lmmp_div_2_(q, a, n, bw);
            sink += q[0];
        },
        o);
    lmmp_free(a);
    lmmp_free(q);
    lmmp_free(b);
    lmmp_free(bw);
    return us;
}

double bench_div_2_s(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), b = alloc_limbs(2);
    fill_random(a, n, 0x2718281828459046ull + n);
    b[1] = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    b[0] = 0x3141592653589793ull;
    double us = measure(
        [&] {
            mp_limb_t s0 = a[0], s1 = a[1];
            sink += lmmp_div_2_s_(q, a, n, b);
            a[0] = s0;
            a[1] = s1;
        },
        o);
    lmmp_free(a);
    lmmp_free(q);
    lmmp_free(b);
    return us;
}

double bench_mod_2(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(2), bw = alloc_limbs(2);
    fill_random(a, n, 0x0badf00dcafefaceull + n);
    b[1] = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    b[0] = 0x3141592653589793ull;
    double us = measure(
        [&] {
            bw[0] = b[0];
            bw[1] = b[1];
            lmmp_mod_2_(a, n, bw);
            sink += bw[0];
        },
        o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(bw);
    return us;
}

double bench_div_3_2(const BenchOpts& o) {
    mp_ptr np = alloc_limbs(3), npw = alloc_limbs(3), dp = alloc_limbs(2);
    np[0] = 0x1111111122222222ull;
    np[1] = 0x3333333344444444ull;
    np[2] = 0x5555555566666666ull;
    dp[0] = 0x7777777788888888ull;
    dp[1] = 0x99999999aaaaaaaall | LIMB_B_2;
    mp_limb_t inv21 = lmmp_inv_2_1_(dp[1], dp[0]);
    double us = measure(
        [&] {
            npw[0] = np[0];
            npw[1] = np[1];
            npw[2] = np[2];
            sink += lmmp_div_3_2_(npw, dp, inv21);
        },
        o);
    lmmp_free(np);
    lmmp_free(npw);
    lmmp_free(dp);
    return us;
}

double bench_inv_1(const BenchOpts& o) {
    mp_limb_t x = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    return measure([&] { sink += lmmp_inv_1_(x); }, o);
}

double bench_inv_2_1(const BenchOpts& o) {
    mp_limb_t xh = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    mp_limb_t xl = 0x3141592653589793ull;
    return measure([&] { sink += lmmp_inv_2_1_(xh, xl); }, o);
}

}  // namespace lmmp_bench

/* ---- 扫描用例：每族原语扫长度表，逐行报告 ---- */

using namespace lmmp_bench;

namespace {

/* 单长度原语表：统一签名 double(mp_size_t, const BenchOpts&) */
struct Len1Op {
    const char* name;
    double (*fn)(mp_size_t, const BenchOpts&);
};

void run_len1(const Len1Op* ops, size_t cnt, const mp_size_t* sizes, size_t ncnt) {
    for (size_t i = 0; i < cnt; ++i)
        for (size_t j = 0; j < ncnt; ++j) {
            mp_size_t n = sizes[j];
            report_n(ops[i].name, n, ops[i].fn(n, {}), n * 8.0);
        }
}

}  // namespace

BENCH_CASE("asm/addsub", addsub) {
    const Len1Op OPS[] = {
        {"add_n", bench_add_n},     {"add_nc", bench_add_nc},     {"sub_n", bench_sub_n},
        {"sub_nc", bench_sub_nc},   {"addshl1_n", bench_addshl1_n}, {"subshl1_n", bench_subshl1_n},
        {"shr1add_n", bench_shr1add_n}, {"shr1sub_n", bench_shr1sub_n},
    };
    const mp_size_t SZ[] = {100, 1000, 10000};
    run_len1(OPS, sizeof(OPS) / sizeof(OPS[0]), SZ, sizeof(SZ) / sizeof(SZ[0]));
}

BENCH_CASE("asm/shift", shift) {
    const Len1Op OPS[] = {
        {"shl", bench_shl}, {"shr", bench_shr}, {"shlnot", bench_shlnot}, {"not", bench_not},
    };
    const mp_size_t SZ[] = {100, 1000, 10000};
    run_len1(OPS, sizeof(OPS) / sizeof(OPS[0]), SZ, sizeof(SZ) / sizeof(SZ[0]));
}

BENCH_CASE("asm/mul1", mul1) {
    const Len1Op OPS[] = {
        {"mul_1", bench_mul_1}, {"addmul_1", bench_addmul_1}, {"submul_1", bench_submul_1},
    };
    const mp_size_t SZ[] = {100, 1000, 10000};
    run_len1(OPS, sizeof(OPS) / sizeof(OPS[0]), SZ, sizeof(SZ) / sizeof(SZ[0]));
}

BENCH_CASE("asm/mulbase", mulbase) {
    const mp_size_t SZ[] = {50, 200, 1000};
    for (mp_size_t n : SZ) report_n("mul_basecase", n, bench_mul_basecase(n, n), n * n * 8.0);
    for (mp_size_t n : SZ) report_n("sqr_basecase", n, bench_sqr_basecase(n), n * n * 8.0);
    const mp_size_t LO[] = {50, 200};
    for (mp_size_t n : LO) report_n("mullo_basecase", n, bench_mullo_basecase(n), n * n * 8.0);
}

BENCH_CASE("asm/mulbase", unbal) {
    report_ab("mul_basecase", 1000, 7, bench_mul_basecase(1000, 7), 1000 * 7 * 8.0);
    report_ab("mul_basecase", 10000, 70, bench_mul_basecase(10000, 70), 10000 * 70 * 8.0);
}

BENCH_CASE("asm/div", div) {
    const Len1Op OPS[] = {
        {"div_1", bench_div_1}, {"div_1_s", bench_div_1_s}, {"mod_1", bench_mod_1},
        {"div_2", bench_div_2}, {"div_2_s", bench_div_2_s}, {"mod_2", bench_mod_2},
    };
    const mp_size_t SZ[] = {100, 1000, 10000};
    run_len1(OPS, sizeof(OPS) / sizeof(OPS[0]), SZ, sizeof(SZ) / sizeof(SZ[0]));
}

BENCH_CASE("asm/inv", inv) {
    report("inv_1", bench_inv_1());
    report("inv_2_1", bench_inv_2_1());
    report("div_3_2", bench_div_3_2());
}
