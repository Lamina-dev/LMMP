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

static double bench_sqrt(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n / 2 + 1), b = alloc_limbs(n);
    fill_random(b, n, 0x92cab5432211ull + n);
    double us;
    BENCH_MEASURE(us, o, lmmp_sqrt_(a, NULL, b, n, 0););
    lmmp_free(a);
    lmmp_free(b);
    return us;
}

static double bench_gcd_lehmer(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x120bce34ba89eull + n);
    fill_random(b, n, 0x2cab5423322191ull + n * 3);
    a[n - 1] |= (mp_limb_t)1 << 63;
    b[n - 1] >>= 1;
    if (b[n - 1] == 0) b[n - 1] = 1;
    a[0] |= 1;
    b[0] |= 1; /* 契约：输入须归一化（顶 limb 非零）且 a>=b */
    double us;
    BENCH_MEASURE(us, o, bench_sink += (mp_limb_t)lmmp_gcd_lehmer_(d, a, n, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_gcd_hgcd(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x120e3489bce1ull + n);
    fill_random(b, n, 0x92cab5432211ull + n * 3);
    a[n - 1] |= (mp_limb_t)1 << 63;
    b[n - 1] >>= 1;
    if (b[n - 1] == 0) b[n - 1] = 1;
    a[0] |= 1;
    b[0] |= 1; /* 契约：输入须归一化且 un>=vn */
    double us;
    BENCH_MEASURE(us, o, bench_sink += (mp_limb_t)lmmp_gcd_hgcd_(d, a, n, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_fib(ulong n, const bench_options *o) {
    mp_size_t rn = lmmp_fibonacci_size_(n);
    mp_ptr a = alloc_limbs(rn + 2);
    double us;
    BENCH_MEASURE(us, o, lmmp_fibonacci_(a, rn, n); bench_sink += a[0];);
    lmmp_free(a);
    return us;
}

static double bench_factorial(uint n, const bench_options *o) {
    mp_bitcnt_t bits = 0;
    mp_size_t need = lmmp_factorial_size_(n, &bits);
    mp_ptr d = alloc_limbs(need + 2);
    double us;
    BENCH_MEASURE(us, o, lmmp_factorial_(d, bits, need, n); bench_sink += d[0];);
    lmmp_free(d);
    return us;
}

/* ---- 扫描用例 ---- */

static void run_sqrt(void) {
    const mp_size_t SZ[] = {5000, 20000, 50000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("sqrt", n, bench_sqrt(n, &bench_default), n * 8.0);
    }
}

static void run_lehmer(void) {
    const mp_size_t SZ[] = {200, 1000, 2000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("gcd_lehmer", n, bench_gcd_lehmer(n, &bench_default), n * 8.0);
    }
}

static void run_hgcd(void) {
    const mp_size_t SZ[] = {64, 1000, 5000, 20000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("gcd_hgcd", n, bench_gcd_hgcd(n, &bench_default), n * 8.0);
    }
}

static void run_fib(void) {
    const ulong SZ[] = {100000, 1000000, 10000000, 50000000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        ulong n = SZ[i];
        char buf[64];
        snprintf(buf, sizeof(buf), "fibonacci(%llu)", (unsigned long long)n);
        report(buf, bench_fib(n, &bench_default), 0);
    }
}

static void run_factorial(void) {
    const uint SZ[] = {100, 1000, 10000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        uint n = SZ[i];
        char buf[64];
        snprintf(buf, sizeof(buf), "factorial(%u)", n);
        report(buf, bench_factorial(n, &bench_default), 0);
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/sqrt/sqrt]");
    run_sqrt();
    puts("[numth/gcd/lehmer]");
    run_lehmer();
    puts("[numth/gcd/hgcd]");
    run_hgcd();
    puts("[numth/fib/fib]");
    run_fib();
    puts("[numth/factorial/factorial]");
    run_factorial();
    lmmp_global_deinit();
    return 0;
}
