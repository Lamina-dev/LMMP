/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/impl/prime_table.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"

#define MAX_T 0xffffffffffffull


bool lmmp_trialdiv_(mp_srcptr restrict np, mp_size_t nn, ushort N) {
    ushort primen = lmmp_prime_cnt16_(N);
    mp_limb_t r;
    mp_limb_t t = 1; // 分块素数积，每块取到 ≥MAX_T 为止
    ushort p;
    ushort parr[32];
    ushort parrn = 0;
    for (ushort i = 0; i < primen; i++) {
        p = prime_short_table[i];
        parr[parrn++] = p;
        t *= p;
        if (t > MAX_T) {
            r = lmmp_mod_1_(np, nn, t);
            for (ushort j = 0; j < parrn; j++) {
                if (r == 0 || r % parr[j] == 0) {
                    return true;
                }
            }
            parrn = 0;
            t = 1;
        }
    }
    if (parrn > 0) {
        r = lmmp_mod_1_(np, nn, t);
        for (ushort j = 0; j < parrn; j++) {
            if (r == 0 || r % parr[j] == 0) {
                return true;
            }
        }
    }
    return false;
}
