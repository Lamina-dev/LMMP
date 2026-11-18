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

#ifndef LMMP_PERF_LIMB_SIZES_H
#define LMMP_PERF_LIMB_SIZES_H

#include "support/perf.h"
#include "lmmp/lmmpn.h"

static inline void run_limb_sizes(const char* name, double (*bench)(mp_size_t, const bench_options*)) {
    const mp_size_t sizes[] = {100, 1000, 10000};
    for (size_t i = 0; i < ARRAY_SIZE(sizes); ++i) {
        mp_size_t n = sizes[i];
        report_n(name, n, bench(n, &bench_default), n * 8.0);
    }
}

#endif
