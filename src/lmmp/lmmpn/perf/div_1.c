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

#include "limb_sizes.h"
#include "mparam.h"

static double bench_div_1(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1);
    fill_random(a, n, 0x0123456789abcdefull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_div_1_(q, a, n, x););
    lmmp_free(a);
    lmmp_free(q);
    return us;
}

static double bench_div_1_s(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1);
    fill_random(a, n, 0x0123456789abcdf1ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    double us;
    BENCH_MEASURE(us, o,
        mp_limb_t save = a[0];
        bench_sink += lmmp_div_1_s_(q, a, n, x);
        a[0] = save;
    );
    lmmp_free(a);
    lmmp_free(q);
    return us;
}

static double bench_mod_1(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n);
    fill_random(a, n, 0x3141592653589793ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_mod_1_(a, n, x););
    lmmp_free(a);
    return us;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/div_1]");
    run_limb_sizes("div_1", bench_div_1);
    run_limb_sizes("div_1_s", bench_div_1_s);
    run_limb_sizes("mod_1", bench_mod_1);
    lmmp_global_deinit();
    return 0;
}
