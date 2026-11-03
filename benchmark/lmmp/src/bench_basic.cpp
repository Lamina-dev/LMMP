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
#include "lmmp/numth.h"
#include "lmmp_bench.hpp"
#include "lmmp_bench_lib.hpp"

namespace lmmp_bench {

/*
    高层运算基准：乘/平方/除法/开方/gcd/阶乘/斐波那契，长度入参，
    随机归一化输入，返回微秒/op。
*/

double bench_mul(mp_size_t a, mp_size_t b, const BenchOpts& o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x1111222233334444ull + a * 31 + b);
    fill_random(pb, b, 0x5555666677778888ull + b * 17 + a);
    double us = measure([&] { lmmp_mul_(d, pa, a, pb, b); }, o);
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

double bench_mul_n(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(2 * n + 2);
    fill_random(a, n, 0x120e3489bceull + n);
    fill_random(b, n, 0x92cab543221ull + n * 3);
    double us = measure([&] { lmmp_mul_n_(d, a, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_sqr(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(2 * n + 2);
    fill_random(a, n, 0x0badf00dcafefaceull + n);
    double us = measure([&] { lmmp_sqr_(d, a, n); }, o);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

double bench_mul_toom22(mp_size_t a, mp_size_t b, const BenchOpts& o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x120e3489bc1ull + a * 31 + b);
    fill_random(pb, b, 0x92cab5432211ull + b * 17 + a);
    double us = measure([&] { lmmp_mul_toom22_(d, pa, a, pb, b); }, o);
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

double bench_mul_toom33(mp_size_t a, mp_size_t b, const BenchOpts& o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x120e3489bc2ull + a * 31 + b);
    fill_random(pb, b, 0x92cab5432212ull + b * 17 + a);
    double us = measure([&] { lmmp_mul_toom33_(d, pa, a, pb, b); }, o);
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

double bench_mul_toom44(mp_size_t a, mp_size_t b, const BenchOpts& o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x120e3489bc3ull + a * 31 + b);
    fill_random(pb, b, 0x92cab5432213ull + b * 17 + a);
    double us = measure([&] { lmmp_mul_toom44_(d, pa, a, pb, b); }, o);
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

double bench_mul_fft(mp_size_t a, mp_size_t b, const BenchOpts& o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x120e3489bc4ull + a * 31 + b);
    fill_random(pb, b, 0x92cab5432214ull + b * 17 + a);
    double us = measure([&] { lmmp_mul_fft_(d, pa, a, pb, b); }, o);
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

double bench_mul_mersenne_fft(mp_size_t n, const BenchOpts& o) {
    mp_size_t rn = lmmp_fft_next_size_(n);
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(rn + 2);
    fill_random(a, n, 0x120e3489bc5ull + n);
    fill_random(b, n, 0x92cab5432215ull + n * 3);
    double us = measure([&] { lmmp_mul_mersenne_fft_(d, rn, a, n, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_mul_mersenne_dc(mp_size_t n, const BenchOpts& o) {
    mp_size_t rn = lmmp_mersenne_next_size_(n);
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(rn + 2);
    fill_random(a, n, 0x120e3489bc6ull + n);
    fill_random(b, n, 0x92cab5432216ull + n * 3);
    double us = measure([&] { lmmp_mul_mersenne_dc_(d, rn, a, n, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_mullo(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x120e3489bc7ull + n);
    fill_random(b, n, 0x92cab5432217ull + n * 3);
    double us = measure([&] { lmmp_mullo_(d, a, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_mullo_dc(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n), tp = alloc_limbs(2 * n);
    fill_random(a, n, 0x120e3489bc8ull + n);
    fill_random(b, n, 0x92cab5432218ull + n * 3);
    double us = measure([&] { lmmp_mullo_dc_(d, a, b, tp, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    lmmp_free(tp);
    return us;
}

double bench_mullo_fft(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n), tp = alloc_limbs(2 * n);
    fill_random(a, n, 0x120e3489bc9ull + n);
    fill_random(b, n, 0x92cab5432219ull + n * 3);
    double us = measure([&] { lmmp_mullo_fft_(d, a, b, n, tp); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    lmmp_free(tp);
    return us;
}

double bench_div(mp_size_t a, mp_size_t b, const BenchOpts& o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr numa = alloc_limbs(a), numb = alloc_limbs(b);
    mp_ptr q = alloc_limbs(a - b + 1), r = alloc_limbs(b);
    fill_random(numa, a, 0x0123456789abcdefull + a * 31 + b);
    fill_random(numb, b, 0xfedcba9876543210ull + b * 17 + a);
    double us = measure([&] { lmmp_div_(q, r, numa, a, numb, b); }, o);
    lmmp_free(numa);
    lmmp_free(numb);
    lmmp_free(q);
    lmmp_free(r);
    return us;
}

double bench_sqrt(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n / 2 + 1), b = alloc_limbs(n);
    fill_random(b, n, 0x92cab5432211ull + n);
    double us = measure([&] { lmmp_sqrt_(a, NULL, b, n, 0); }, o);
    lmmp_free(a);
    lmmp_free(b);
    return us;
}

double bench_gcd_lehmer(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x120bce34ba89eull + n);
    fill_random(b, n, 0x2cab5423322191ull + n * 3);
    a[n - 1] |= (mp_limb_t)1 << 63;
    b[n - 1] >>= 1;
    if (b[n - 1] == 0) b[n - 1] = 1;
    a[0] |= 1;
    b[0] |= 1; /* 契约：输入须归一化（顶 limb 非零）且 a>=b */
    double us = measure([&] { sink += (mp_limb_t)lmmp_gcd_lehmer_(d, a, n, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_gcd_hgcd(mp_size_t n, const BenchOpts& o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x120e3489bce1ull + n);
    fill_random(b, n, 0x92cab5432211ull + n * 3);
    a[n - 1] |= (mp_limb_t)1 << 63;
    b[n - 1] >>= 1;
    if (b[n - 1] == 0) b[n - 1] = 1;
    a[0] |= 1;
    b[0] |= 1; /* 契约：输入须归一化且 un>=vn */
    double us = measure([&] { sink += (mp_limb_t)lmmp_gcd_hgcd_(d, a, n, b, n); }, o);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

double bench_fib(ulong n, const BenchOpts& o) {
    mp_size_t rn = lmmp_fibonacci_size_(n);
    mp_ptr a = alloc_limbs(rn + 2);
    double us = measure([&] { lmmp_fibonacci_(a, rn, n); sink += a[0]; }, o);
    lmmp_free(a);
    return us;
}

double bench_factorial(uint n, const BenchOpts& o) {
    mp_bitcnt_t bits = 0;
    mp_size_t need = lmmp_factorial_size_(n, &bits);
    mp_ptr d = alloc_limbs(need + 2);
    double us = measure([&] { lmmp_factorial_(d, bits, need, n); sink += d[0]; }, o);
    lmmp_free(d);
    return us;
}

}  // namespace lmmp_bench

/* ---- 扫描用例 ---- */

using namespace lmmp_bench;

BENCH_CASE("lmmpn/mul", mul) {
    const mp_size_t SZ[] = {10, 50, 100, 500, 1000, 5000};
    for (mp_size_t n : SZ) report_n("mul", n, bench_mul(n, n), n * n * 8.0);
}

BENCH_CASE("lmmpn/mul", mul_n) {
    const mp_size_t SZ[] = {10, 50, 100, 500, 1000, 5000};
    for (mp_size_t n : SZ) report_n("mul_n", n, bench_mul_n(n), n * n * 8.0);
}

BENCH_CASE("lmmpn/mul", unbal) {
    report_ab("mul", 1000, 7, bench_mul(1000, 7), 1000 * 7 * 8.0);
    report_ab("mul", 5000, 100, bench_mul(5000, 100), 5000 * 100 * 8.0);
    report_ab("mul", 10000, 3000, bench_mul(10000, 3000), 10000 * 3000 * 8.0);
}

BENCH_CASE("lmmpn/sqr", sqr) {
    const mp_size_t SZ[] = {10, 50, 100, 500, 1000, 5000};
    for (mp_size_t n : SZ) report_n("sqr", n, bench_sqr(n), n * n * 8.0);
}

BENCH_CASE("lmmpn/mul_toom", toom) {
    const mp_size_t T22[] = {20, 50, 200};
    const mp_size_t T33[] = {30, 100, 500};
    const mp_size_t T44[] = {50, 200, 1000};
    for (mp_size_t n : T22) report_n("mul_toom22", n, bench_mul_toom22(n, n), n * n * 8.0);
    for (mp_size_t n : T33) report_n("mul_toom33", n, bench_mul_toom33(n, n), n * n * 8.0);
    for (mp_size_t n : T44) report_n("mul_toom44", n, bench_mul_toom44(n, n), n * n * 8.0);
}

BENCH_CASE("lmmpn/mul_fft", fft) {
    const mp_size_t SZ[] = {100, 500, 2000, 5000};
    for (mp_size_t n : SZ) report_n("mul_fft", n, bench_mul_fft(n, n), n * n * 8.0);
}

BENCH_CASE("lmmpn/mul_mersenne", mersenne) {
    const mp_size_t SZ[] = {100, 500, 2000};
    for (mp_size_t n : SZ) report_n("mul_mersenne_fft", n, bench_mul_mersenne_fft(n), n * n * 8.0);
    for (mp_size_t n : SZ) report_n("mul_mersenne_dc", n, bench_mul_mersenne_dc(n), n * n * 8.0);
}

BENCH_CASE("lmmpn/mullo", mullo) {
    const mp_size_t BC[] = {20, 100};
    const mp_size_t SZ[] = {50, 500, 2000};
    for (mp_size_t n : BC) report_n("mullo_basecase", n, bench_mullo_basecase(n), n * n * 8.0);
    for (mp_size_t n : SZ) report_n("mullo", n, bench_mullo(n), n * n * 8.0);
    for (mp_size_t n : SZ) report_n("mullo_dc", n, bench_mullo_dc(n), n * n * 8.0);
    for (mp_size_t n : SZ) report_n("mullo_fft", n, bench_mullo_fft(n), n * n * 8.0);
}

BENCH_CASE("lmmpn/div", div) {
    report_ab("div", 2000, 1000, bench_div(2000, 1000));
    report_ab("div", 1000, 7, bench_div(1000, 7));
    report_ab("div", 10000, 700, bench_div(10000, 700));
}

BENCH_CASE("numth/sqrt", sqrt) {
    const mp_size_t SZ[] = {5000, 20000, 50000};
    for (mp_size_t n : SZ) report_n("sqrt", n, bench_sqrt(n), n * 8.0);
}

BENCH_CASE("numth/gcd", lehmer) {
    const mp_size_t SZ[] = {200, 1000, 2000};
    for (mp_size_t n : SZ) report_n("gcd_lehmer", n, bench_gcd_lehmer(n), n * 8.0);
}

BENCH_CASE("numth/gcd", hgcd) {
    const mp_size_t SZ[] = {64, 1000, 5000, 20000};
    for (mp_size_t n : SZ) report_n("gcd_hgcd", n, bench_gcd_hgcd(n), n * 8.0);
}

BENCH_CASE("numth/fib", fib) {
    const ulong SZ[] = {100000, 1000000, 10000000, 50000000};
    for (ulong n : SZ) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "fibonacci(%llu)", (unsigned long long)n);
        report(buf, bench_fib(n));
    }
}

BENCH_CASE("numth/factorial", factorial) {
    const uint SZ[] = {100, 1000, 10000};
    for (uint n : SZ) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "factorial(%u)", n);
        report(buf, bench_factorial(n));
    }
}
