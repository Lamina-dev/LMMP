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

static double bench_shl(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x9192939495969798ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_shl_(d, a, n, 13););
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

static double bench_shr(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xa1a2a3a4a5a6a7a8ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_shr_(d, a, n, 13););
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

static double bench_shlnot(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xb1b2b3b4b5b6b7b8ull + n);
    double us;
    BENCH_MEASURE(us, o, bench_sink += lmmp_shlnot_(d, a, n, 13););
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/shift]");
    run_limb_sizes("shl", bench_shl);
    run_limb_sizes("shr", bench_shr);
    run_limb_sizes("shlnot", bench_shlnot);
    lmmp_global_deinit();
    return 0;
}
