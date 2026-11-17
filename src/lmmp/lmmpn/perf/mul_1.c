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

static double bench_mul_1(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x0a0b0c0d0e0f0102ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_mul_1_(d, a, n, x););
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

static double bench_addmul_1(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x1112131415161718ull + n);
    fill_random(d, n, 0x2122232425262728ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_addmul_1_(d, a, n, x););
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

static double bench_submul_1(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x3132333435363738ull + n);
    fill_random(d, n, 0x4142434445464748ull + n);
    mp_limb_t x = 0x9e3779b97f4a7c15ull;
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_submul_1_(d, a, n, x););
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/mul_1]");
    run_limb_sizes("mul_1", bench_mul_1);
    run_limb_sizes("addmul_1", bench_addmul_1);
    run_limb_sizes("submul_1", bench_submul_1);
    lmmp_global_deinit();
    return 0;
}
