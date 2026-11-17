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

#include "support/perf.h"
#include "lmmp/lmmpn.h"
#include "lmmp/mprand.h"
#include "lmmp/numth.h"
#include "mullo_basecase.h"

/*
    高层运算基准：乘/平方/除法/开方/gcd/阶乘/斐波那契，长度入参，
    随机归一化输入，返回微秒/op。
*/

static double bench_mul(mp_size_t a, mp_size_t b, const bench_options *o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x1111222233334444ull + a * 31 + b);
    fill_random(pb, b, 0x5555666677778888ull + b * 17 + a);
    double us;
    BENCH_MEASURE(us, o, lmmp_mul_(d, pa, a, pb, b););
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

static double bench_mul_n(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(2 * n + 2);
    fill_random(a, n, 0x120e3489bceull + n);
    fill_random(b, n, 0x92cab543221ull + n * 3);
    double us;
    BENCH_MEASURE(us, o, lmmp_mul_n_(d, a, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_sqr(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(2 * n + 2);
    fill_random(a, n, 0x0badf00dcafefaceull + n);
    double us;
    BENCH_MEASURE(us, o, lmmp_sqr_(d, a, n););
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

static double bench_mul_toom22(mp_size_t a, mp_size_t b, const bench_options *o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x120e3489bc1ull + a * 31 + b);
    fill_random(pb, b, 0x92cab5432211ull + b * 17 + a);
    double us;
    BENCH_MEASURE(us, o, lmmp_mul_toom22_(d, pa, a, pb, b););
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

static double bench_mul_toom33(mp_size_t a, mp_size_t b, const bench_options *o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x120e3489bc2ull + a * 31 + b);
    fill_random(pb, b, 0x92cab5432212ull + b * 17 + a);
    double us;
    BENCH_MEASURE(us, o, lmmp_mul_toom33_(d, pa, a, pb, b););
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

static double bench_mul_toom44(mp_size_t a, mp_size_t b, const bench_options *o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x120e3489bc3ull + a * 31 + b);
    fill_random(pb, b, 0x92cab5432213ull + b * 17 + a);
    double us;
    BENCH_MEASURE(us, o, lmmp_mul_toom44_(d, pa, a, pb, b););
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

static double bench_mul_fft(mp_size_t a, mp_size_t b, const bench_options *o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 2);
    fill_random(pa, a, 0x120e3489bc4ull + a * 31 + b);
    fill_random(pb, b, 0x92cab5432214ull + b * 17 + a);
    double us;
    BENCH_MEASURE(us, o, lmmp_mul_fft_(d, pa, a, pb, b););
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

static double bench_mul_mersenne_fft(mp_size_t n, const bench_options *o) {
    mp_size_t rn = lmmp_fft_next_size_(n);
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(rn + 2);
    fill_random(a, n, 0x120e3489bc5ull + n);
    fill_random(b, n, 0x92cab5432215ull + n * 3);
    double us;
    BENCH_MEASURE(us, o, lmmp_mul_mersenne_fft_(d, rn, a, n, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_mul_mersenne_dc(mp_size_t n, const bench_options *o) {
    mp_size_t rn = lmmp_mersenne_next_size_(n);
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(rn + 2);
    fill_random(a, n, 0x120e3489bc6ull + n);
    fill_random(b, n, 0x92cab5432216ull + n * 3);
    double us;
    BENCH_MEASURE(us, o, lmmp_mul_mersenne_dc_(d, rn, a, n, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_mullo(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x120e3489bc7ull + n);
    fill_random(b, n, 0x92cab5432217ull + n * 3);
    double us;
    BENCH_MEASURE(us, o, lmmp_mullo_(d, a, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_mullo_dc(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n), tp = alloc_limbs(2 * n);
    fill_random(a, n, 0x120e3489bc8ull + n);
    fill_random(b, n, 0x92cab5432218ull + n * 3);
    double us;
    BENCH_MEASURE(us, o, lmmp_mullo_dc_(d, a, b, tp, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    lmmp_free(tp);
    return us;
}

static double bench_mullo_fft(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n), tp = alloc_limbs(2 * n);
    fill_random(a, n, 0x120e3489bc9ull + n);
    fill_random(b, n, 0x92cab5432219ull + n * 3);
    double us;
    BENCH_MEASURE(us, o, lmmp_mullo_fft_(d, a, b, n, tp););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    lmmp_free(tp);
    return us;
}

static double bench_div(mp_size_t a, mp_size_t b, const bench_options *o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr numa = alloc_limbs(a), numb = alloc_limbs(b);
    mp_ptr q = alloc_limbs(a - b + 1), r = alloc_limbs(b);
    fill_random(numa, a, 0x0123456789abcdefull + a * 31 + b);
    fill_random(numb, b, 0xfedcba9876543210ull + b * 17 + a);
    double us;
    BENCH_MEASURE(us, o, lmmp_div_(q, r, numa, a, numb, b););
    lmmp_free(numa);
    lmmp_free(numb);
    lmmp_free(q);
    lmmp_free(r);
    return us;
}

static void run_mul(void) {
    const mp_size_t SZ[] = {10, 50, 100, 500, 1000, 5000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("mul", n, bench_mul(n, n, &bench_default), 0);
    }
}

static void run_mul_n(void) {
    const mp_size_t SZ[] = {10, 50, 100, 500, 1000, 5000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("mul_n", n, bench_mul_n(n, &bench_default), 0);
    }
}

static void run_unbal(void) {
    report_ab("mul", 1000, 7, bench_mul(1000, 7, &bench_default), 1000 * 7 * 8.0);
    report_ab("mul", 5000, 100, bench_mul(5000, 100, &bench_default), 5000 * 100 * 8.0);
    report_ab("mul", 10000, 3000, bench_mul(10000, 3000, &bench_default), 10000 * 3000 * 8.0);
}

static void run_sqr(void) {
    const mp_size_t SZ[] = {10, 50, 100, 500, 1000, 5000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("sqr", n, bench_sqr(n, &bench_default), 0);
    }
}

static void run_toom(void) {
    const mp_size_t T22[] = {20, 50, 200};
    const mp_size_t T33[] = {30, 100, 500};
    const mp_size_t T44[] = {50, 200, 1000};
    for (size_t i = 0; i < ARRAY_SIZE(T22); ++i) {
        mp_size_t n = T22[i];
        report_n("mul_toom22", n, bench_mul_toom22(n, n, &bench_default), n * n * 8.0);
    }
    for (size_t i = 0; i < ARRAY_SIZE(T33); ++i) {
        mp_size_t n = T33[i];
        report_n("mul_toom33", n, bench_mul_toom33(n, n, &bench_default), n * n * 8.0);
    }
    for (size_t i = 0; i < ARRAY_SIZE(T44); ++i) {
        mp_size_t n = T44[i];
        report_n("mul_toom44", n, bench_mul_toom44(n, n, &bench_default), n * n * 8.0);
    }
}

static void run_fft(void) {
    const mp_size_t SZ[] = {100, 500, 2000, 5000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("mul_fft", n, bench_mul_fft(n, n, &bench_default), n * n * 8.0);
    }
}

static void run_mersenne(void) {
    const mp_size_t SZ[] = {100, 500, 2000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("mul_mersenne_fft", n, bench_mul_mersenne_fft(n, &bench_default), n * n * 8.0);
    }
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("mul_mersenne_dc", n, bench_mul_mersenne_dc(n, &bench_default), n * n * 8.0);
    }
}

static void run_mullo(void) {
    const mp_size_t BC[] = {20, 100};
    const mp_size_t SZ[] = {50, 500, 2000};
    for (size_t i = 0; i < ARRAY_SIZE(BC); ++i) {
        mp_size_t n = BC[i];
        report_n("mullo_basecase", n, bench_mullo_basecase(n, &bench_default), n * n * 8.0);
    }
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("mullo", n, bench_mullo(n, &bench_default), n * n * 8.0);
    }
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("mullo_dc", n, bench_mullo_dc(n, &bench_default), n * n * 8.0);
    }
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("mullo_fft", n, bench_mullo_fft(n, &bench_default), n * n * 8.0);
    }
}

static void run_div(void) {
    report_ab("div", 2000, 1000, bench_div(2000, 1000, &bench_default), 0);
    report_ab("div", 1000, 7, bench_div(1000, 7, &bench_default), 0);
    report_ab("div", 10000, 700, bench_div(10000, 700, &bench_default), 0);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/mul/mul]");
    run_mul();
    puts("[lmmpn/mul/mul_n]");
    run_mul_n();
    puts("[lmmpn/mul/unbal]");
    run_unbal();
    puts("[lmmpn/sqr/sqr]");
    run_sqr();
    puts("[lmmpn/mul_toom/toom]");
    run_toom();
    puts("[lmmpn/mul_fft/fft]");
    run_fft();
    puts("[lmmpn/mul_mersenne/mersenne]");
    run_mersenne();
    puts("[lmmpn/mullo/mullo]");
    run_mullo();
    puts("[lmmpn/div/div]");
    run_div();
    lmmp_global_deinit();
    return 0;
}
