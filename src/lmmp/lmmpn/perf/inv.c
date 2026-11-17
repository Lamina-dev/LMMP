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
#include "mparam.h"

static double bench_inv_1(const bench_options *o) {
    mp_limb_t x = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_inv_1_(x););
    return us;
}

static double bench_inv_2_1(const bench_options *o) {
    mp_limb_t xh = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    mp_limb_t xl = 0x3141592653589793ull;
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_inv_2_1_(xh, xl););
    return us;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/inv]");
    report("inv_1", bench_inv_1(&bench_default), 0);
    report("inv_2_1", bench_inv_2_1(&bench_default), 0);
    lmmp_global_deinit();
    return 0;
}
