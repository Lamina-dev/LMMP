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

#include "prime_pool.h"

static double bench_is_strong_lucas(mp_size_t n, PrimePool kind, const bench_options *o) {
    int cnt = pool_cnt(n);
    mp_ptr a = pool_n(n, cnt, 0x51ce77ac02ull + n, kind);
    size_t idx = 0;
    double us;
    BENCH_MEASURE(us, o, bench_sink += (mp_limb_t)lmmp_is_strong_lucas_(a + n * (idx++ % cnt), n););
    lmmp_free(a);
    return us;
}

static void run_strong_lucas(void) {
    const mp_size_t SZ[] = {3, 8, 16, 32, 64, 200};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        mp_size_t n = SZ[i];
        report_n("strong_lucas coprime", n, bench_is_strong_lucas(n, PRIME_POOL_COPRIME, &bench_default), 0);
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/prime/strong_lucas]");
    run_strong_lucas();
    lmmp_global_deinit();
    return 0;
}
