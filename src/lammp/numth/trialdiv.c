/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lammp/numth.h"
#include "../../../include/lammp/lmmpn.h"
#include "../../../include/lammp/impl/prime_table.h"
#include "../../../include/lammp/impl/tmp_alloc.h"
#include "../../../include/lammp/impl/ele_mul.h"

#define MAX_T 0xffffffffffffull

static ushortp lmmp_trialdiv_short_(mp_srcptr restrict np, mp_size_t nn, ushort N, ushort* rn) {
    if (np == NULL || nn == 0) {
        *rn = 0;
        return NULL;
    }
    ulong t = 1;
    ushort idx[20];
    uint idx_cnt = 0;
    ushort retn_max = 10;
    ushortp ret = ALLOC_TYPE(retn_max, ushort);
    ushort retn = 0;
    if (!(np[0] & 1)) {
        ret[retn++] = 2;
    }
    for (mp_size_t i = 1;; i++) {
        ushort p = prime_short_table[i];
        if (p > N || i >= PRIME_SHORT_TABLE_SIZE - 1) break;
        t *= p;
        idx[idx_cnt++] = p;
        if (t > MAX_T || idx_cnt == 20) {
            mp_limb_t r = lmmp_mod_1_(np, nn, t);
            for (uint j = 0; j < idx_cnt; j++) {
                if (r == 0 || r % idx[j] == 0) {
                    ret[retn++] = idx[j];
                    if (retn == retn_max) {
                        retn_max = retn_max * 12 / 10;
                        ret = REALLOC_TYPE(ret, retn_max, ushort);
                    }
                }
            }
            idx_cnt = 0;
            t = 1;
        }
    }
    if (idx_cnt > 0) {
        mp_limb_t r = lmmp_mod_1_(np, nn, t);
        for (uint j = 0; j < idx_cnt; j++) {
            if (r == 0 || r % idx[j] == 0) {
                ret[retn++] = idx[j];
                if (retn == retn_max) {
                    retn_max += 10;
                    ret = REALLOC_TYPE(ret, retn_max, ushort);
                }
            }
        }
    }
    if (retn == 0) {
        lmmp_free(ret);
        *rn = 0;
        return NULL;
    } else {
        *rn = retn;
        return ret;
    }
}

ushortp lmmp_trialdiv_(mp_srcptr restrict np, mp_size_t nn, ushort N, ushort* rn) {
    ushort primen = lmmp_prime_cnt16_(N);
    if (primen > 4 * nn) {
        return lmmp_trialdiv_short_(np, nn, N, rn);
    } else {
    /*
    if [np,nn] is too large, we calculate the product of primes up to N,
    and divide [np,nn] by the product to get remainder.
    */
        TEMP_S_DECL;
        ulongp restrict pp = SALLOC_TYPE(primen / 4 + 1, ulong);
        ulong t = 1;
        mp_size_t pn = 0;
        for (ushort i = 0; i < primen; i++) {
            t *= prime_short_table[i];
            if (t > MAX_T) {
                pp[pn++] = t;
                t = 1;
            }
        }
        if (t > 1) {
            pp[pn++] = t;
        }
        mp_ptr restrict prod = SALLOC_TYPE(pn * 2, mp_limb_t);
        pn = lmmp_elem_mul_ulong_(prod, pp, pn, prod + pn);

        lmmp_div_(NULL, prod, np, nn, prod, pn);
        while (pn > 0 && prod[pn - 1] == 0) --pn;
        ushortp restrict ret;
        if (pn == 0) {
            ret = ALLOC_TYPE(primen, ushort);
            for (ushort i = 0; i < primen; i++) {
                ret[i] = prime_short_table[i];
            }
            *rn = primen;
        } else {
            ret = lmmp_trialdiv_short_(prod, pn, N, rn);
        } 
        TEMP_S_FREE;
        return ret;
    }
}
