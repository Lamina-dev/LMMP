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

static double bench_div_2(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), b = alloc_limbs(2), bw = alloc_limbs(2);
    fill_random(a, n, 0x2718281828459045ull + n);
    b[1] = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    b[0] = 0x3141592653589793ull;
    double us;
    BENCH_MEASURE(us, o,
        bw[0] = b[0];
        bw[1] = b[1];
        lmmp_div_2_(q, a, n, bw);
        bench_sink += q[0];
    );
    lmmp_free(a);
    lmmp_free(q);
    lmmp_free(b);
    lmmp_free(bw);
    return us;
}

static double bench_div_2_s(mp_size_t n, const bench_options *o) {
    if (n < 3) return 0.0;  /* 除数为两个 limb 时要求 n >= 3 */
    mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), b = alloc_limbs(2);
    fill_random(a, n, 0x2718281828459046ull + n);
    b[1] = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    b[0] = 0x3141592653589793ull;
    double us;
    BENCH_MEASURE(us, o,
        mp_limb_t s0 = a[0], s1 = a[1];
        bench_sink += lmmp_div_2_s_(q, a, n, b);
        a[0] = s0;
        a[1] = s1;
    );
    lmmp_free(a);
    lmmp_free(q);
    lmmp_free(b);
    return us;
}

static double bench_mod_2(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(2), bw = alloc_limbs(2);
    fill_random(a, n, 0x0badf00dcafefaceull + n);
    b[1] = 0x9e3779b97f4a7c15ull | LIMB_B_2;
    b[0] = 0x3141592653589793ull;
    double us;
    BENCH_MEASURE(us, o,
        bw[0] = b[0];
        bw[1] = b[1];
        lmmp_mod_2_(a, n, bw);
        bench_sink += bw[0];
    );
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(bw);
    return us;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/div_2]");
    run_limb_sizes("div_2", bench_div_2);
    run_limb_sizes("div_2_s", bench_div_2_s);
    run_limb_sizes("mod_2", bench_mod_2);
    lmmp_global_deinit();
    return 0;
}
