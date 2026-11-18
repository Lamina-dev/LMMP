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

static double bench_mullo_basecase(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x0f0e0d0c0b0a0908ull + n);
    fill_random(b, n, 0x0102030405060708ull + n);
    double us;
    BENCH_MEASURE(us, o, lmmp_mullo_basecase_(d, a, b, n); bench_sink += d[0];);
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

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/mullo/mullo]");
    run_mullo();
    puts("[lmmpn/mullo/basecase]");
    const mp_size_t LO[] = {50, 200};
    for (size_t i = 0; i < ARRAY_SIZE(LO); ++i) {
        mp_size_t n = LO[i];
        report_n("mullo_basecase", n, bench_mullo_basecase(n, &bench_default), 0);
    }
    lmmp_global_deinit();
    return 0;
}
