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

#include "../../../include/lmmp/impl/base_table.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


mp_size_t lmmp_from_str_len_(const mp_byte_t* src, mp_size_t len, int base) {
    lmmp_param_assert(base >= 2 && base <= 256);
    if (src) {
        do {
            if (len == 0)
                return 1;
        } while (src[--len] == 0);
        ++len;
    }
    return lmmp_mulh_(len, lmmp_bases_table[base - 2].lg_base) + 1;
}

/**
 * @brief 将字符串转换为mp_limb_t数组
 * @param dst 输出数组
 * @param src 输入字符串
 * @param len 字符串长度
 * @param base 转换基数
 * @warning src[len-1]!=0, dst!=NULL, src!=NULL
 * @return 返回转换后的limb数量
 */
static mp_size_t lmmp_from_str_basecase_(mp_ptr dst, const mp_byte_t* src, mp_size_t len, int base) {
    lmmp_param_assert(src[len - 1] != 0);
    lmmp_param_assert(dst != NULL && src != NULL);
    mp_size_t digitspl = lmmp_bases_table[base - 2].digits_in_limb;
    mp_limb_t lbase = lmmp_bases_table[base - 2].large_base;
    mp_size_t limbs = 0, i = len;

    while (i) {
        mp_limb_t curlimb;
        if (i >= digitspl) {
            curlimb = src[--i];
            for (mp_size_t j = 1; j < digitspl; ++j) {
                curlimb = curlimb * base + src[--i];
            }
        } else {
            curlimb = src[--i];
            lbase = base;
            while (i) {
                curlimb = curlimb * base + src[--i];
                lbase *= base;
            }
        }

        if (limbs == 0) {
            dst[0] = curlimb;
            limbs = 1;
        } else {
            mp_limb_t cy = lmmp_mul_1_(dst, dst, limbs, lbase);
            cy += lmmp_add_1_(dst, dst, limbs, curlimb);
            if (cy) {
                dst[limbs] = cy;
                ++limbs;
            }
        }
    }

    return limbs;
}

/**
 * @brief 将字符串转换为mp_limb_t数组
 * @param dst 输出数组
 * @param src 输入字符串
 * @param len 字符串长度
 * @param pow 指数表
 * @param tp 临时数组
 * @warning src[len-1]!=0, sep(dst,tp), dst!=NULL, src!=NULL, tp!=NULL
 * @note 第一层调用时：nh>=2, [dst,2*N], [tp,limbs]
 *       后序递归时：N>=2, [dst,limbs+1], [tp,2*N-1]
 *       limbs为返回值，N = pow->np + pow->zeros
 * @return 返回转换后的limb数量
 */
static mp_size_t lmmp_from_str_divide_(
          mp_ptr       restrict dst,
    const mp_byte_t*            src,
          mp_size_t             len,
          mp_basepow_t*         pow,
          mp_ptr       restrict  tp
) {
    lmmp_param_assert(src[len - 1] != 0);
    lmmp_param_assert(dst != NULL && src != NULL && tp != NULL);
    mp_size_t limbs;

    if (lmmp_from_str_len_(0, len, pow->base) < FROM_STR_DIVIDE_THRESHOLD) {
        limbs = lmmp_from_str_basecase_(dst, src, len, pow->base);
    } else {
        mp_size_t pdigits = pow->digits;
        if (len <= pdigits) {
            limbs = lmmp_from_str_divide_(dst, src, len, pow - 1, tp);
        } else {
            mp_ptr p = pow->p;
            mp_size_t np = pow->np;
            mp_size_t zeros = pow->zeros;
            mp_ptr lp = tp, hp = tp + np + zeros - 2;  // overwrite 2 limbs
            mp_size_t nl = 0, nh;

            mp_size_t llen = pdigits;
            while (llen && src[llen - 1] == 0) --llen;
            if (llen)
                nl = lmmp_from_str_divide_(lp, src, llen, pow - 1, dst);

            mp_limb_t save0 = hp[0], save1 = hp[1];  // save 2 limbs
            nh = lmmp_from_str_divide_(hp, src + pdigits, len - pdigits, pow - 1, dst);
            if (nh >= np)
                lmmp_mul_(dst + zeros, hp, nh, p, np);
            else
                lmmp_mul_(dst + zeros, p, np, hp, nh);
            // restore 2 limbs
            hp[0] = save0;
            hp[1] = save1;

            if (zeros < nl) {
                lmmp_copy(dst, lp, zeros);
                mp_limb_t cy = lmmp_add_n_(dst + zeros, dst + zeros, lp + zeros, nl - zeros);
                // h*p+l<=(B^nh-1)*p+(p-1)<B^nh*p, limited by nh+np limbs,
                // so inc won't overflow
                if (cy)
                    lmmp_inc(dst + nl);
            } else {
                lmmp_copy(dst, lp, nl);
                if (zeros > nl)
                    lmmp_zero(dst + nl, zeros - nl);
            }

            limbs = nh + np + zeros;
            limbs -= dst[limbs - 1] == 0;
        }
    }

    return limbs;
}

mp_size_t lmmp_from_str_(mp_ptr dst, const mp_byte_t* src, mp_size_t len, int base) {
    lmmp_param_assert(base >= 2 && base <= 256);
    lmmp_param_assert(dst != NULL && src != NULL);
    do {
        if (len == 0)
            return 0;
    } while (src[--len] == 0);
    ++len;

    mp_size_t limbs;
    if (LMMP_POW2_Q(base)) {
        mp_limb_t curlimb = 0;
        const mp_byte_t* srcend = src + len;
        int bitspd = lmmp_bases_table[base - 2].large_base;
        int bitpos = 0;
        limbs = 0;

        do {
            mp_limb_t curdigit = *src;
            curlimb |= curdigit << bitpos;
            bitpos += bitspd;
            if (bitpos >= LIMB_BITS) {
                dst[limbs] = curlimb;
                ++limbs;
                bitpos -= LIMB_BITS;
                curlimb = curdigit >> (bitspd - bitpos);
            }
        } while (++src != srcend);
        if (curlimb) {
            dst[limbs] = curlimb;
            ++limbs;
        }
    } else if (lmmp_from_str_len_(0, len, base) < FROM_STR_BASEPOW_THRESHOLD) {
        limbs = lmmp_from_str_basecase_(dst, src, len, base);
    } else {
        TEMP_DECL;
        mp_basepow_t powers[LIMB_BITS];
        mp_limb_t lbase = lmmp_bases_table[base - 2].large_base;
        mp_size_t digitspl = lmmp_bases_table[base - 2].digits_in_limb;
        mp_size_t bexp, lexp = (len - 1) / digitspl + 1;
        mp_size_t tzbit = lmmp_tailing_zeros_(lbase);
        // need 1 extra limb to store result
        mp_size_t alloc_size = lmmp_from_str_len_(0, len, base) + 1;
        mp_limb_t cy;
        mp_ptr tp;

        int cpow = lmmp_limb_bits_(lexp - 1);

        for (int i = cpow; i > 0; --i) {
            // we will calculate lbase^bexp
            bexp = ((lexp - 1) >> i) + 1;
            // we will calculate lbase^(bexp-1) first, and trim it s. t.
            // it contains at most 2 tailing 0 limb, then multiply it by lbase,
            // so we need npow limbs to store lbase^bexp
            mp_size_t npow = lmmp_from_str_len_(0, (bexp - 1) * digitspl + 1, base) + 1;

            if (tzbit) {
                mp_size_t tzlimb = tzbit * (bexp - 1) / LIMB_BITS;
                if (tzlimb >= 2)
                    npow -= tzlimb - 2;
            }

            // space needed for a trimmed npow-limb lbase^bexp
            alloc_size += npow;
        }

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
        for (int i = 2; i < cpow; ++i) {
            lmmp_sqr_(tp, p, np);
            np *= 2;
            np -= tp[np - 1] == 0;
            bexp = (lexp - 1) >> (cpow - i);
            if (bexp & 1) {
                cy = lmmp_mul_1_(tp, tp, np, lbase);
                tp[np] = cy;
                np += cy != 0;
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
        }

        limbs = lmmp_from_str_divide_(tp, src, len, powers + cpow - 1, dst);
        lmmp_copy(dst, tp, limbs);

        TEMP_FREE;
    }
    return limbs;
}
