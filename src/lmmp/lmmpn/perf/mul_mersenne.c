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

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/mul_mersenne/mersenne]");
    run_mersenne();
    lmmp_global_deinit();
    return 0;
}
