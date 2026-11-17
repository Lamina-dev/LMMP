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

static double bench_mul_basecase(mp_size_t a, mp_size_t b, const bench_options *o) {
    if (a < b) {
        mp_size_t t = a;
        a = b;
        b = t;
    }
    mp_ptr pa = alloc_limbs(a), pb = alloc_limbs(b), d = alloc_limbs(a + b + 1);
    fill_random(pa, a, 0x0badcafef00dfaceull + a * 31 + b);
    fill_random(pb, b, 0x13579bdf2468ace0ull + b * 17 + a);
    double us;
    BENCH_MEASURE(us, o, lmmp_mul_basecase_(d, pa, a, pb, b); bench_sink += d[0];);
    lmmp_free(pa);
    lmmp_free(pb);
    lmmp_free(d);
    return us;
}

static void run_unbal(void) {
    report_ab("mul_basecase", 1000, 7, bench_mul_basecase(1000, 7, &bench_default), 1000 * 7 * 8.0);
    report_ab("mul_basecase", 10000, 70, bench_mul_basecase(10000, 70, &bench_default), 10000 * 70 * 8.0);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/mul_basecase]");
    const mp_size_t SZ[] = {50, 200, 1000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("mul_basecase", n, bench_mul_basecase(n, n, &bench_default), 0);
    }
    run_unbal();
    lmmp_global_deinit();
    return 0;
}
