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

// 转换实体 lmmp_from_hex_ 在 ASM 构建下由 asm/{x64,arm64}/from_hex.S 同名替换，
// C 参考实现移至 generic/from_hex.c；本文件保留长度查询接口。

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
