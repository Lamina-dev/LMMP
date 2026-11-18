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
#include "mparam.h"

static double bench_div_3_2(const bench_options *o) {
    mp_ptr np = alloc_limbs(3), npw = alloc_limbs(3), dp = alloc_limbs(2);
    np[0] = 0x1111111122222222ull;
    np[1] = 0x3333333344444444ull;
    np[2] = 0x5555555566666666ull;
    dp[0] = 0x7777777788888888ull;
    dp[1] = 0x99999999aaaaaaaall | LIMB_B_2;
    mp_limb_t inv21 = lmmp_inv_2_1_(dp[1], dp[0]);
    double us;
    BENCH_MEASURE(us, o,
        npw[0] = np[0];
        npw[1] = np[1];
        npw[2] = np[2];
        bench_sink += lmmp_div_3_2_(npw, dp, inv21);
    );
    lmmp_free(np);
    lmmp_free(npw);
    lmmp_free(dp);
    return us;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    puts("[lmmpn/div_3_2]");
    report("div_3_2", bench_div_3_2(&bench_default), 0);
    lmmp_global_deinit();
    return 0;
}
