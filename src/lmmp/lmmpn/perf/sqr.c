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

static double bench_sqr(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(2 * n + 2);
    fill_random(a, n, 0x0badf00dcafefaceull + n);
    double us;
    BENCH_MEASURE(us, o, lmmp_sqr_(d, a, n););
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

static void run_sqr(void) {
    const mp_size_t SZ[] = {10, 50, 100, 500, 1000, 5000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("sqr", n, bench_sqr(n, &bench_default), 0);
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/sqr/sqr]");
    run_sqr();
    lmmp_global_deinit();
    return 0;
}
