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

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/mul_toom/toom]");
    run_toom();
    lmmp_global_deinit();
    return 0;
}
