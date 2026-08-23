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

#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/impl/inlines.h"


mp_bitcnt_t lmmp_extract_bits_(mp_srcptr restrict num, mp_size_t n, mp_limb_t* restrict ext, int bits) {
    lmmp_param_assert(bits <= LIMB_BITS && bits > 0);
    lmmp_param_assert(n > 0);
    if (n == 1) {
        int lb = lmmp_limb_bits_(num[0]);
        if (lb <= bits) {
            *ext = num[0];
            return 0;
        } else {
            *ext = num[0] >> (lb - bits);
            return lb - bits;
        }
    } else {
        int lb = lmmp_limb_bits_(num[n - 1]);
        if (lb < bits) {
            *ext = num[n - 1] << (bits - lb);
            *ext |= num[n - 2] >> (LIMB_BITS - bits + lb);
            return LIMB_BITS * (n - 1) - (bits - lb);
        } else if (lb == bits) {
            *ext = num[n - 1];
            return LIMB_BITS * (n - 1);
        } else {
            *ext = num[n - 1] >> (lb - bits);
            return LIMB_BITS * (n - 1) + (lb - bits);
        }
    }
}