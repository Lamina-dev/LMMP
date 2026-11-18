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
#include "lmmp/numth.h"

static double bench_factorial(uint n, const bench_options *o) {
    mp_bitcnt_t bits = 0;
    mp_size_t need = lmmp_factorial_size_(n, &bits);
    mp_ptr d = alloc_limbs(need + 2);
    double us;
    BENCH_MEASURE(us, o, lmmp_factorial_(d, bits, need, n); bench_sink += d[0];);
    lmmp_free(d);
    return us;
}

static void run_factorial(void) {
    const uint SZ[] = {100, 1000, 10000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        uint n = SZ[i];
        char buf[64];
        snprintf(buf, sizeof(buf), "factorial(%u)", n);
        report(buf, bench_factorial(n, &bench_default), 0);
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/factorial/factorial]");
    run_factorial();
    lmmp_global_deinit();
    return 0;
}
