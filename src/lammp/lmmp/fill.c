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

#include "../../../include/lammp/lmmp.h"


void lmmp_fill(mp_ptr dst, mp_size_t begin, mp_size_t end, mp_limb_t val) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(begin <= end);
    for (mp_size_t i = begin; i < end; i++) {
        dst[i] = val;
    }
}
