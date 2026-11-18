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

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/mul/mul]");
    run_mul();
    puts("[lmmpn/mul/mul_n]");
    run_mul_n();
    puts("[lmmpn/mul/unbal]");
    run_unbal();
    lmmp_global_deinit();
    return 0;
}
