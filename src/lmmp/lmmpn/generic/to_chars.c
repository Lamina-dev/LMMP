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

#include "../../../../include/lmmp/lmmpn.h"


void lmmp_dec_to_chars_(mp_byte_t* dst, mp_size_t n) {
    for (mp_size_t i = 0; i < n; i++)
        dst[i] = (mp_byte_t)(dst[i] + (mp_byte_t)'0');
}

void lmmp_b36_to_chars_(mp_byte_t* dst, mp_size_t n, bool upper) {
    // x>=10 时 c = x + '0' + adj, adj = 'a'-'0'-10 = 39 (小写) / 'A'-'0'-10 = 7 (大写)
    // -(x>9) 生成全0/全1掩码，与 adj 按位与实现无分支选择
    const int adj = upper ? 7 : 39;
    for (mp_size_t i = 0; i < n; i++) {
        int x = dst[i];
        dst[i] = (mp_byte_t)(x + '0' + (-(x > 9) & adj));
    }
}
