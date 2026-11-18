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

static double bench_not(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0xc1c2c3c4c5c6c7c8ull + n);
    double us;
    BENCH_MEASURE(us, o, lmmp_not_(d, a, n); bench_sink += d[0];);
    lmmp_free(a);
    lmmp_free(d);
    return us;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/not]");
    run_limb_sizes("not", bench_not);
    lmmp_global_deinit();
    return 0;
}
