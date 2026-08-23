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

#include "../../../include/lmmp/impl/base_table.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


mp_size_t lmmp_to_str_len_(mp_srcptr numa, mp_size_t na, int base) {
    lmmp_param_assert(base >= 2 && base <= 256);
    int mslbits = 0;
    if (numa) {
        do {
            if (na == 0)
                return 1;
        } while (numa[--na] == 0);
        mslbits = lmmp_limb_bits_(numa[na]);
    }
    return lmmp_mulh_(na * LIMB_BITS + mslbits, lmmp_bases_table[base - 2].inv_lg_base) + 1;
}

/**
 * @brief 将mp_limb_t数组转换为字符串
 * @param dst 输出字符串
 * @param numa 输入数组
 * @param na 输入数组长度
 * @param base 转换基数
 * @warning numa[na-1]!=0
 * @return 返回转换后的字符串长度
 */
static mp_size_t lmmp_to_str_basecase_(mp_byte_t* dst, mp_srcptr numa, mp_size_t na, int base) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa[na - 1]!= 0);
    int i;
    int digitspl = lmmp_bases_table[base - 2].digits_in_limb;
    mp_limb_t lbase = lmmp_bases_table[base - 2].large_base;
    mp_size_t n = 0;
    mp_limb_t frac;
    mp_limb_t tp[1 + TO_STR_BASEPOW_THRESHOLD];
    lmmp_copy(tp + 1, numa, na);

    do {
        tp[0] = 0;
        lmmp_div_1_(tp, tp, na + 1, lbase);
        frac = tp[0] + 1;
        na -= tp[na] == 0;
        if (na == 0)
            break;
        i = digitspl;
        do {
            dst[--i] = lmmp_mulh_(frac, base);
            frac *= base;
        } while (i);
        dst += digitspl;
        n += digitspl;
    } while (1);

    mp_byte_t msbyte;
    i = digitspl;
    while (i && (msbyte = lmmp_mulh_(frac, base)) == 0) {
        --i;
        frac *= base;
    }
    n += i;
    while (i) {
        dst[--i] = msbyte;
        frac *= base;
        msbyte = lmmp_mulh_(frac, base);
    }
    return n;
}

/**
 * @brief 将mp_limb_t数组转换为字符串
 * @param dst 输出字符串
 * @param numa 输入数组
 * @param na 输入数组长度
 * @param pow 指数表
 * @param tpq 临时数组
 * @warning numa[na-1]!=0, sep(dst,tpq)
 * @return 返回转换后的字符串长度
 */
static mp_size_t lmmp_to_str_divide_(
    mp_byte_t*              dst, 
    mp_ptr        restrict numa, 
    mp_size_t                na,
    mp_basepow_t*           pow,
    mp_ptr        restrict  tpq
) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa[na - 1] != 0);
    lmmp_param_assert(pow != NULL);
    mp_size_t digits;
    if (na < TO_STR_DIVIDE_THRESHOLD) {
        digits = lmmp_to_str_basecase_(dst, numa, na, pow->base);
    } else {
        mp_ptr p = pow->p, invp = pow->invp;
        mp_size_t np = pow->np, ni = pow->ni;
        mp_size_t zeros = pow->zeros;
        mp_size_t pdigits = pow->digits;
        int cnt = pow->norm_cnt;
        int adjust = 0;

        // may adjust na s.t. qh=0
        if (na >= np + zeros) {
            mp_limb_t ah = 0, al = numa[na - 1] << cnt;
            if (cnt) {
                ah = numa[na - 1] >> (LIMB_BITS - cnt);
                if (na > zeros + 1)
                    al |= numa[na - 2] >> (LIMB_BITS - cnt);
            }
            adjust = (ah || al >= p[np - 1]);
        }

        // if numa<p
        if (na + adjust <= np + zeros) {
            // skip this power
            digits = lmmp_to_str_divide_(dst, numa, na, pow - 1, tpq);
        } else {
            numa[na] = 0;
            na += adjust;

            numa += zeros;
            na -= zeros;

            mp_size_t nq = na - np, nr = np + zeros;
            mp_ptr q = tpq, r = numa - zeros;

            if (cnt)
                lmmp_shl_(numa, numa, na, cnt);
            if (invp)
                lmmp_div_mulinv_(q, numa, na, p, np, invp, ni);
            else
                lmmp_div_s_(q, numa, na, p, np);
            if (cnt)
                lmmp_shr_(numa, numa, np, cnt);

            mp_size_t digitsh = 0, digitsl = 0;

            while (nq && q[nq - 1] == 0) --nq;
            if (nq)
                digitsh = lmmp_to_str_divide_(dst + pdigits, q, nq, pow - 1, tpq + nq + 1);

            while (nr && r[nr - 1] == 0) --nr;
            if (nr)
                digitsl = lmmp_to_str_divide_(dst, r, nr, pow - 1, tpq);

            if (digitsh) {
                while (digitsl != pdigits) {
                    dst[digitsl] = 0;
                    ++digitsl;
                }
            }

            digits = digitsl + digitsh;
        }
    }
    return digits;
}

mp_size_t lmmp_to_str_(mp_byte_t* dst, mp_srcptr numa, mp_size_t na, int base) {
    lmmp_param_assert(base >= 2 && base <= 256);
    do {
        if (na == 0)
            return 0;
    } while (numa[--na] == 0);
    ++na;

    mp_size_t digits;
    if (LMMP_POW2_Q(base)) {
        mp_limb_t curlimb = numa[na - 1];
        int cnt = lmmp_bases_table[base - 2].large_base;
        int bitsh = lmmp_limb_bits_(curlimb);
        int mask = (1 << cnt) - 1;
        mp_size_t bits = bitsh + LIMB_BITS * (na - 1);
        digits = (bits - 1) / cnt + 1;
        dst += digits;
        int bitpos = digits * cnt - LIMB_BITS * (na - 1);

        do {
            while ((bitpos -= cnt) >= 0) {
                *--dst = curlimb >> bitpos & mask;
            }
            if (--na == 0)
                break;
            mp_limb_t prevlimb = curlimb << -bitpos;
            curlimb = numa[na - 1];
            bitpos += LIMB_BITS;
            *--dst = (prevlimb | curlimb >> bitpos) & mask;
        } while (1);
    } else if (na < TO_STR_BASEPOW_THRESHOLD) {
        digits = lmmp_to_str_basecase_(dst, numa, na, base);
    } else {
        TEMP_DECL;
        mp_basepow_t powers[LIMB_BITS];
        mp_size_t exps[LIMB_BITS];
        mp_limb_t lbase = lmmp_bases_table[base - 2].large_base;
        mp_size_t digitspl = lmmp_bases_table[base - 2].digits_in_limb;
        mp_size_t bexp = (lmmp_to_str_len_(numa, na, base) - 1) / digitspl + 1;
        mp_size_t tzbit = lmmp_tailing_zeros_(lbase);
        // numa 的拷贝空间，多一个 limb 预留规整化移位所需
        mp_size_t alloc_size = na + 1;
        mp_limb_t cy;
        mp_ptr tp;

        int cpow = 0;

        do {
            bexp = (bexp + 1) >> 1;
            exps[cpow] = bexp;
            ++cpow;

            // we will calculate lbase^(bexp-1) first, and trim it s. t.
            // it contains at most 2 tailing 0 limb, then multiply it by lbase,
            // so we need npow limbs to store lbase^bexp
            mp_size_t npow = lmmp_from_str_len_(0, (bexp - 1) * digitspl + 1, base) + 1;

            // space needed for quotients in recursive calls,
            // quotients are smaller than lbase^bexp
            alloc_size += npow + 1;

            if (tzbit) {
                mp_size_t tzlimb = tzbit * (bexp - 1) / LIMB_BITS;
                if (tzlimb >= 2)
                    npow -= tzlimb - 2;
            }

            // space needed for a trimmed npow-limb lbase^bexp and its inverse
            alloc_size += npow * 2;
        } while (bexp > 1);

        tp = BALLOC_TYPE(alloc_size, mp_limb_t);

        for (int i = 0; i < 2; ++i) {
            tp[0] = lbase;
            powers[i].p = tp;
            powers[i].np = 1;
            tp += i + 1;
            powers[i].zeros = 0;
            powers[i].digits = digitspl * (i + 1);
            powers[i].base = base;
        }

        mp_ptr p = powers[1].p;
        mp_size_t zeros = 0, np = 1;
        bexp = 1;
        for (int i = 2; i < cpow; ++i) {
            lmmp_sqr_(tp, p, np);
            bexp *= 2;
            np *= 2;
            np -= tp[np - 1] == 0;
            if (bexp + 1 < exps[cpow - 1 - i]) {
                cy = lmmp_mul_1_(tp, tp, np, lbase);
                tp[np] = cy;
                np += cy != 0;
                ++bexp;
            }
            zeros *= 2;
            while (tp[0] == 0) {
                // at most 2 tailing 0 limb here
                ++zeros;
                ++tp;
                --np;
            }
            p = tp;
            powers[i].p = p;
            powers[i].np = np;
            powers[i].zeros = zeros;
            powers[i].digits = digitspl * (bexp + 1);
            powers[i].base = base;
            tp += np + 1;
        }

        for (int i = 1; i < cpow; ++i) {
            p = powers[i].p;
            np = powers[i].np;
            cy = lmmp_mul_1_(p, p, np, lbase);
            p[np] = cy;
            np += cy != 0;
            if (p[0] == 0) {
                ++powers[i].zeros;
                ++p;
                --np;
            }

            powers[i].p = p;
            powers[i].np = np;

            // Note: all powers except powers[0] are normalized
            // ASSERT: powers[0] will be never used in lmmp_to_str_divide_
            // i.e. TO_STR_DIVIDE_THRESHOLD >= 3
            int cnt = lmmp_leading_zeros_(p[np - 1]);
            if ((powers[i].norm_cnt = cnt))
                lmmp_shl_(p, p, np, cnt);

            if (np < DIV_MULINV_L_THRESHOLD) {
                // use divs, no need to compute inv
                powers[i].invp = 0;
                powers[i].ni = 0;
            } else {
                // pre-compute inverse
                mp_size_t ni = lmmp_div_inv_size_(np + powers[i].zeros, np);
                lmmp_inv_prediv_(tp, p, np, ni);
                powers[i].invp = tp;
                powers[i].ni = ni;
                tp += ni;
            }
        }

        lmmp_copy(tp, numa, na);
        digits = lmmp_to_str_divide_(dst, tp, na, powers + cpow - 1, tp + na + 1);

        TEMP_FREE;
    }

    return digits;
}
