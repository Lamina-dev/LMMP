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

#include "../../../include/lmmp/impl/str_conv.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/lmmpn.h"


// ASCII -> nibble 值表，非法字符映射为 0（契约要求输入全为合法十六进制数字）
static const mp_byte_t lmmp_hex_val_tab[256] = {
    ['0'] = 0,  ['1'] = 1,  ['2'] = 2,  ['3'] = 3,  ['4'] = 4,
    ['5'] = 5,  ['6'] = 6,  ['7'] = 7,  ['8'] = 8,  ['9'] = 9,
    ['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15,
    ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
};

mp_size_t lmmp_from_hex_len_(const char* src, mp_size_t len) {
    if (src) {
        // 忽略末尾的 '0'（值前导零，字符串低位在前）
        do {
            if (len == 0)
                return 0;
        } while (src[--len] == '0');
        ++len;
    }
    return (len + 15) >> 4;
}

mp_size_t lmmp_from_hex_(mp_ptr dst, const char* src, mp_size_t len) {
    lmmp_param_assert(dst != NULL && src != NULL);
    do {
        if (len == 0)
            return 0;
    } while (src[--len] == '0');
    ++len;

    mp_size_t nfull = len >> 4;
    const char* sp = src;
    for (mp_size_t i = 0; i < nfull; ++i, sp += 16) {
        mp_limb_t l = 0;
        // 组内自高向低读，使得 sp[j] 落在第 j 个 nibble
        for (int j = 15; j >= 0; --j)
            l = (l << 4) | lmmp_hex_val_tab[(mp_byte_t)sp[j]];
        dst[i] = l;
    }

    int r = (int)(len & 15);
    if (r) {
        mp_limb_t l = 0;
        for (int j = r - 1; j >= 0; --j)
            l = (l << 4) | lmmp_hex_val_tab[(mp_byte_t)sp[j]];
        dst[nfull] = l;
    }

    return nfull + (r != 0);
}
