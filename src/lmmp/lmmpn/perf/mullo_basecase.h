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

#ifndef LMMP_PERF_MULLO_BASECASE_H
#define LMMP_PERF_MULLO_BASECASE_H

#include "support/perf.h"

static inline double bench_mullo_basecase(mp_size_t n, const bench_options *o) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n);
    fill_random(a, n, 0x0f0e0d0c0b0a0908ull + n);
    fill_random(b, n, 0x0102030405060708ull + n);
    double us;
    BENCH_MEASURE(us, o, lmmp_mullo_basecase_(d, a, b, n); bench_sink += d[0];);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    return us;
}

#endif
