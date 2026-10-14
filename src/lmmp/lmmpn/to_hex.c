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

// 转换实体 lmmp_to_hex_ 在 ASM 构建下由 asm/{x64,arm64}/to_hex.S 同名替换，
// C 参考实现移至 generic/to_hex.c；本文件保留长度查询接口。

mp_size_t lmmp_to_hex_len_(mp_srcptr numa, mp_size_t na) {
    if (numa) {
        do {
            if (na == 0)
                return 0;
        } while (numa[--na] == 0);
        // na 现为最高非零 limb 下标
        return (LIMB_BITS * na + lmmp_limb_bits_(numa[na]) + 3) >> 2;
    }
    return na << 4;
}
