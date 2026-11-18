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

static void run_div(void) {
    report_ab("div", 2000, 1000, bench_div(2000, 1000, &bench_default), 0);
    report_ab("div", 1000, 7, bench_div(1000, 7, &bench_default), 0);
    report_ab("div", 10000, 700, bench_div(10000, 700, &bench_default), 0);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/div/div]");
    run_div();
    lmmp_global_deinit();
    return 0;
}
