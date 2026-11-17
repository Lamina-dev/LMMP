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

static double bench_addshl1_n(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x5152535455565758ull + n);
    fill_random(b, n, 0x6162636465666768ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_addshl1_n_(d, a, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_subshl1_n(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n + 1);
    fill_random(a, n, 0x7172737475767778ull + n);
    fill_random(b, n, 0x8182838485868788ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_subshl1_n_(d, a, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_shr1add_n(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xd1d2d3d4d5d6d7d8ull + n);
    fill_random(b, n, 0xe1e2e3e4e5e6e7e8ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_shr1add_n_(d, a, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

static double bench_shr1sub_n(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xf1f2f3f4f5f6f7f8ull + n);
    fill_random(b, n, 0x1112131415162718ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_shr1sub_n_(d, a, b, n););
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/addsub_shift]");
    run_limb_sizes("addshl1_n", bench_addshl1_n);
    run_limb_sizes("subshl1_n", bench_subshl1_n);
    run_limb_sizes("shr1add_n", bench_shr1add_n);
    run_limb_sizes("shr1sub_n", bench_shr1sub_n);
    lmmp_global_deinit();
    return 0;
}
