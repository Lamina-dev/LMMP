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

static double bench_powlo(mp_size_t n, const bench_options *o) {
    mp_ptr b = alloc_limbs(n), e = alloc_limbs(n), dst = alloc_limbs(n);
    lmmp_seed_random_(b, n, 0x51ce77ab0bull + n, 1);
    b[0] |= 1;
    lmmp_seed_random_(e, n, 0x25c0ff340bull + n, 1);
    e[n - 1] |= (mp_limb_t)1 << 63;
    double us;
    BENCH_MEASURE(us, o, lmmp_powlo_(dst, b, n, e, n); bench_sink += dst[0];);
    lmmp_free(b);
    lmmp_free(e);
    lmmp_free(dst);
    return us;
}

static const mp_size_t POWLO_SZ[] = {1, 2, 8, 64, 200, 400, 1000};

static void run_powlo(void) {
    for (size_t i = 0; i < ARRAY_SIZE(POWLO_SZ); ++i) {
        mp_size_t n = POWLO_SZ[i];
        report_n("powlo e=n", n, bench_powlo(n, &bench_default), 0);
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[numth/powlo/powlo]");
    run_powlo();
    lmmp_global_deinit();
    return 0;
}
