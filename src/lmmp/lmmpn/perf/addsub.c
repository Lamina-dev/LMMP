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

static double bench_add_n(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x123456789ull + n);
    fill_random(b, n, 0x987654321ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_add_n_(d, a, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_add_nc(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x12345678aull + n);
    fill_random(b, n, 0x987654322ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_add_nc_(d, a, b, n, 1););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_sub_n(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x12345678bull + n);
    fill_random(b, n, 0x876543210ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_sub_n_(d, a, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_sub_nc(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x12345678cull + n);
    fill_random(b, n, 0x765432101ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_sub_nc_(d, a, b, n, 1););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/addsub]");
    run_limb_sizes("add_n", bench_add_n);
    run_limb_sizes("add_nc", bench_add_nc);
    run_limb_sizes("sub_n", bench_sub_n);
    run_limb_sizes("sub_nc", bench_sub_nc);
    lmmp_global_deinit();
    return 0;
}
