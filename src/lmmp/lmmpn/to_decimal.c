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
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


mp_size_t lmmp_to_decimal_len_(mp_srcptr numa, mp_size_t na) {
    int mslbits = 0;
    if (numa) {
        do {
            if (na == 0)
                return 1;
        } while (numa[--na] == 0);
        mslbits = lmmp_limb_bits_(numa[na]);
    }
    return lmmp_mulh_(na * LIMB_BITS + mslbits, lmmp_bases_table[DEC_BASE - 2].inv_lg_base) + 1;
}

typedef struct {
    mp_basepow_t pow[LIMB_BITS];
    int top;  // 已构建的最高层下标，-1 表示未初始化
} dec_pow_ctx_t;

static LMMP_THREAD_LOCAL dec_pow_ctx_t dec_pow_ctx_g = {
    .top = -1,
};

// 初始化第 0 层：10^19 = lmmp_bases_table[DEC_BASE-2].large_base
static void dec_pow_init_(void) {
    mp_basepow_t* p0 = &dec_pow_ctx_g.pow[0];
    p0->p = (mp_ptr)lmmp_alloc(sizeof(mp_limb_t));
    p0->p[0] = lmmp_bases_table[DEC_BASE - 2].large_base;
    p0->np = 1;
    p0->zeros = 0;
    p0->digits = lmmp_bases_table[DEC_BASE - 2].digits_in_limb;
    int cnt = lmmp_leading_zeros_(p0->p[0]);
    if ((p0->norm_cnt = cnt))
        lmmp_shl_(p0->p, p0->p, 1, cnt);
    p0->invp = NULL;
    p0->ni = 0;
    p0->base = DEC_BASE;
    dec_pow_ctx_g.top = 0;
}

/*
 * 由第 k-1 层平方构建第 k 层：pow[k] = pow[k-1]^2 = 10^(19*2^k)。
 * 设第 k-1 层真值 T = v * B^zeros（v 为剥离尾部零 limb 后的奇 limb 数），
 * v 的尾零 bit 数 = 19*2^(k-1) mod 64 < 64，故 v^2 的尾零 limb 至多 1 个，
 * 剥离后新 zeros = 2*zeros + 剥离数 = floor(19*2^k / 64)。
 */
static void dec_pow_extend_(int k) {
    mp_basepow_t* pre = &dec_pow_ctx_g.pow[k - 1];
    mp_size_t np = pre->np;
    TEMP_B_DECL;
    // buf 布局：[v: np（非归一化真值）| sq: 2np（平方结果）]
    mp_ptr buf = BALLOC_TYPE(3 * np + 2, mp_limb_t);
    mp_ptr v = buf, sq = buf + np;

    lmmp_copy(v, pre->p, np);
    if (pre->norm_cnt)
        lmmp_shr_(v, v, np, pre->norm_cnt);
    lmmp_sqr_(sq, v, np);

    mp_size_t nq = 2 * np;
    nq -= sq[nq - 1] == 0;
    mp_size_t zeros = 2 * pre->zeros;
    while (sq[0] == 0) {
        ++zeros;
        ++sq;
        --nq;
    }

    mp_basepow_t* cur = &dec_pow_ctx_g.pow[k];
    cur->p = (mp_ptr)lmmp_alloc((size_t)nq * sizeof(mp_limb_t));
    lmmp_copy(cur->p, sq, nq);
    int cnt = lmmp_leading_zeros_(cur->p[nq - 1]);
    if ((cur->norm_cnt = cnt))
        lmmp_shl_(cur->p, cur->p, nq, cnt);
    cur->np = nq;
    cur->zeros = zeros;
    cur->digits = (mp_size_t)lmmp_bases_table[DEC_BASE - 2].digits_in_limb << k;
    cur->base = DEC_BASE;
    if (nq < DIV_MULINV_L_THRESHOLD) {
        // 规模较小，使用基础除法，无需预计算逆元
        cur->invp = NULL;
        cur->ni = 0;
    } else {
        mp_size_t ni = lmmp_div_inv_size_(nq + zeros, nq);
        cur->invp = (mp_ptr)lmmp_alloc((size_t)ni * sizeof(mp_limb_t));
        lmmp_inv_prediv_(cur->invp, cur->p, nq, ni);
        cur->ni = ni;
    }

    TEMP_B_FREE;
}

mp_basepow_t* lmmp_dec_pow_table_(mp_size_t grps, int* ctop) {
    lmmp_param_assert(grps > 1 && ctop != NULL);
    if (dec_pow_ctx_g.top < 0)
        dec_pow_init_();
    // 最高有效层：2^need <= grps，即 need = floor(log2(grps))
    int need = lmmp_limb_bits_((mp_limb_t)grps) - 1;
    while (dec_pow_ctx_g.top < need) {
        ++dec_pow_ctx_g.top;
        dec_pow_extend_(dec_pow_ctx_g.top);
    }
    *ctop = need;
    return dec_pow_ctx_g.pow;
}

void lmmp_dec_pow_table_free_(void) {
    if (dec_pow_ctx_g.top < 0)
        return;
    for (int i = 0; i <= dec_pow_ctx_g.top; ++i) {
        lmmp_free(dec_pow_ctx_g.pow[i].p);
        if (dec_pow_ctx_g.pow[i].invp)
            lmmp_free(dec_pow_ctx_g.pow[i].invp);
    }
    dec_pow_ctx_g.top = -1;
}

mp_size_t lmmp_to_decimal_(char* dst, mp_srcptr numa, mp_size_t na) {
    lmmp_param_assert(dst != NULL && numa != NULL);
    do {
        if (na == 0)
            return 0;
    } while (numa[--na] == 0);
    ++na;

    if (na < TO_STR_BASEPOW_THRESHOLD)
        return lmmp_to_str_basecase_((mp_byte_t*)dst, numa, na, DEC_BASE, '0');

    // 惰性获取全局幂表
    mp_size_t digits = lmmp_to_str_len_(numa, na, DEC_BASE);
    mp_size_t grps = (digits - 1) / lmmp_bases_table[DEC_BASE - 2].digits_in_limb + 1;
    int ctop;
    mp_basepow_t* pows = lmmp_dec_pow_table_(grps, &ctop);

    TEMP_B_DECL;
    // numa 拷贝空间（多 1 limb 预留规整移位）+ 各层商空间
    mp_size_t alloc = na + 1;
    for (int i = 1; i <= ctop; ++i)
        alloc += pows[i].np + pows[i].zeros + 1;
    mp_ptr tp = BALLOC_TYPE(alloc, mp_limb_t);

    lmmp_copy(tp, numa, na);
    digits = lmmp_to_str_divide_((mp_byte_t*)dst, tp, na, pows + ctop, tp + na + 1, '0');

    TEMP_B_FREE;
    return digits;
}
