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
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


mp_size_t lmmp_from_decimal_len_(const char* src, mp_size_t len) {
    if (src) {
        // 忽略末尾的 '0'（值前导零，字符串低位在前）
        do {
            if (len == 0)
                return 0;
        } while (src[--len] == '0');
        ++len;
    }
    return lmmp_mulh_(len, lmmp_bases_table[DEC_BASE - 2].lg_base) + 1;
}

mp_size_t lmmp_from_decimal_(mp_ptr dst, const char* src, mp_size_t len) {
    lmmp_param_assert(dst != NULL && src != NULL);
    do {
        if (len == 0)
            return 0;
    } while (src[--len] == '0');
    ++len;

    if (lmmp_from_str_len_(0, len, DEC_BASE) < FROM_STR_BASEPOW_THRESHOLD)
        return lmmp_from_str_basecase_(dst, (const mp_byte_t*)src, len, DEC_BASE, '0');

    // 惰性获取全局幂表（置于 TEMP_B_DECL 之前，幂表持久堆分配与临时分配互不干扰）
    mp_size_t grps = (len - 1) / lmmp_bases_table[DEC_BASE - 2].digits_in_limb + 1;
    int ctop;
    mp_basepow_t* pows = lmmp_dec_pow_table_(grps, &ctop);

    mp_size_t limbs = lmmp_from_str_len_(0, len, DEC_BASE);
    TEMP_B_DECL;
    // 结果缓冲（多 1 limb）+ 各层非归一化幂值副本
    //（引擎乘法需要真值，而幂表存的是归一化值）
    mp_size_t alloc = limbs + 1;
    for (int i = 0; i <= ctop; ++i)
        alloc += pows[i].np + 1;
    mp_ptr tp = BALLOC_TYPE(alloc, mp_limb_t);

    mp_basepow_t upows[LIMB_BITS];
    mp_ptr cp = tp + limbs + 1;
    for (int i = 0; i <= ctop; ++i) {
        upows[i] = pows[i];
        lmmp_copy(cp, pows[i].p, pows[i].np);
        if (pows[i].norm_cnt)
            lmmp_shr_(cp, cp, pows[i].np, pows[i].norm_cnt);
        upows[i].p = cp;
        cp += pows[i].np + 1;
    }

    limbs = lmmp_from_str_divide_(tp, (const mp_byte_t*)src, len, upows + ctop, dst, '0');
    lmmp_copy(dst, tp, limbs);

    TEMP_B_FREE;
    return limbs;
}
