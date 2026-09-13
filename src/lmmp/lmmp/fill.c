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

#include "../../../include/lmmp/lmmp.h"


void lmmp_fill(mp_ptr begin, mp_ptr end, mp_limb_t val) {
    lmmp_param_assert(begin != NULL && end != NULL);
    for (mp_ptr p = begin; p < end; p++) {
        *p = val;
    }
}

void lmmp_fill_n(mp_ptr dst, mp_size_t len, mp_limb_t val) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(len > 0);
    for (mp_size_t i = 0; i < len; i++) {
        dst[i] = val;
    }
}
