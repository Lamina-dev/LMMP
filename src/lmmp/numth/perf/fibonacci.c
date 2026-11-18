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

static double bench_fib(ulong n, const bench_options *o) {
    mp_size_t rn = lmmp_fibonacci_size_(n);
    mp_ptr a = alloc_limbs(rn + 2);
    double us;
    BENCH_MEASURE(us, o, lmmp_fibonacci_(a, rn, n); bench_sink += a[0];);
    lmmp_free(a);
    return us;
}

static void run_fib(void) {
    const ulong SZ[] = {100000, 1000000, 10000000, 50000000};
    for (size_t i = 0; i < ARRAY_SIZE(SZ); ++i) {
        ulong n = SZ[i];
        char buf[64];
        snprintf(buf, sizeof(buf), "fibonacci(%llu)", (unsigned long long)n);
        report(buf, bench_fib(n, &bench_default), 0);
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/fib/fib]");
    run_fib();
    lmmp_global_deinit();
    return 0;
}
