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

#include <math.h>

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


/**
 * @brief 将x转换为不小于x的double（提取高53位）
 * @param x 输入
 * @return double值d，满足 d >= x 且 d/x <= 1 + 2^-52
 * @note 仅当x超出53位有效数字时，舍弃低位并对高位进位。
 *       pow的位数估计中，exp可达2^64，超出双精度有效数字，
 *       提取高位既保留全部有效数字，又保证转换不会引入低估。
 */
static inline double ulong_upper_d(uint64_t x) {
    int bits = lmmp_limb_bits_(x);
    if (bits <= 53) {
        return (double)x;  // 本身可精确表示
    }
    int sh = bits - 53;
    uint64_t h = x >> sh;
    h += (x & (((uint64_t)1 << sh) - 1)) != 0;  // 舍弃非0低位时进位
    return ldexp((double)h, sh);
}

/**
 * @brief 计算floor(r)+1并转为uint64
 * @param r 位数估计值（非负）
 * @return floor(r)+1，即以bit计的长度；超出uint64范围时钳制为MP_ULONG_MAX（此规模的缓冲区不可能分配成功）
 * @note 位数 = floor(log2(x))+1。当估计值恰为整数（如底数是2的幂）时，
 *       ceil会少计1bit，故统一使用floor+1
 */
static inline uint64_t floor1_u64(double r) {
    if (r >= 0x1p63) {
        return MP_ULONG_MAX;
    }
    return (uint64_t)floor(r) + 1;
}

mp_size_t lmmp_pow_1_size_(mp_limb_t base, ulong exp) {
    lmmp_param_assert(base >= 1);
    lmmp_param_assert(exp > 0);
    if (base == 1) {
        return 1;
    } else if (exp <= 2) {
        return 3;
    } else {
        /*
        base = mant * 2^E，E = bits(base)-1，mant ∈ [1,2]。
        base^exp 的位数为 exp*log2(mant) + exp*E：
        整数部分 exp*E 走整数运算；log2 仅作用于 [1,2] 区间
        （ulp为2^-52，完整保留53位有效数字），exp 提取高53位并
        向上舍入为double，保证转换不引入低估。
        */
        int bits = lmmp_limb_bits_(base);
        double mant;
        if (bits <= 53) {
            mant = ldexp((double)base, 1 - bits);
        } else {
            // 高53位无条件进位：base < h*2^(bits-53)
            uint64_t h = (base >> (bits - 53)) + 1;
            mant = ldexp((double)h, -52);  // in (1,2]
        }
        mp_size_t rn = floor1_u64(ulong_upper_d(exp) * log2(mant));
        if (rn == MP_ULONG_MAX) {
            return rn;  // 钳制值，此规模的缓冲区不可能分配成功
        }
        rn += exp * (mp_bitcnt_t)(bits - 1);
        return (rn + LIMB_BITS - 1) / LIMB_BITS + 2;
    }
}

mp_size_t lmmp_pow_size_(mp_srcptr base, mp_size_t n, ulong exp) {
    lmmp_param_assert(n > 0);
    lmmp_param_assert(base[n - 1] != 0);
    if (n == 1) {
        return lmmp_pow_1_size_(base[0], exp);
    }
    if (exp == 1) {
        return n;
    } else if (exp == 2) {
        return n * 2;
    } else {
        /*
        直接提取base的最高53位为h（无条件进位，base < h*2^sh，
        sh为被舍弃的低位位数），mant = h*2^-52 ∈ (1,2]，
        E = bits(base)-1 = sh+52 为log2的整数部分。
        base^exp 的位数为 exp*log2(mant) + exp*E：
        整数部分 exp*E 走整数运算；log2 仅作用于 (1,2] 区间，
        完整保留53位有效数字；exp 提取高53位并向上舍入为double。
        */
        int t = lmmp_limb_bits_(base[n - 1]);
        uint64_t h;
        if (t >= 53) {
            h = (base[n - 1] >> (t - 53)) + 1;
        } else {
            // n >= 2 必然成立（单limb已在前面处理）
            h = ((base[n - 1] << (53 - t)) | (base[n - 2] >> (LIMB_BITS - 53 + t))) + 1;
        }
        mp_bitcnt_t e_int = (n - 1) * LIMB_BITS + t - 1;

        mp_size_t rn = floor1_u64(ulong_upper_d(exp) * log2(ldexp((double)h, -52)));
        if (rn == MP_ULONG_MAX) {
            return rn;  // 钳制值，此规模的缓冲区不可能分配成功
        }
        rn += exp * e_int;
        rn = (rn + LIMB_BITS - 1) / LIMB_BITS;
        return rn + 2;
    }
}

mp_size_t lmmp_pow_(mp_ptr restrict dst, mp_size_t rn, mp_srcptr restrict base, mp_size_t n, ulong exp) {
    lmmp_param_assert(n > 0);
    lmmp_param_assert(exp > 0);
    lmmp_param_assert(base[n - 1] != 0);
    if (exp == 1) {
        lmmp_copy(dst, base, n);
        return n;
    } else if (exp == 2) {
        lmmp_sqr_(dst, base, n);
        rn = n << 1;
        rn -= (dst[rn - 1] == 0);
        return rn;
    } else {
        mp_size_t base_tz = 0;
        while (*base == 0) {
            ++base_tz;
            ++base;
            --n;
        }
        base_tz *= exp;
        lmmp_zero(dst, base_tz);
        dst += base_tz;
        if (n == 1) {
            if (exp <= POW_1_EXP_THRESHOLD) {
                dst[0] = base[0];
                rn = 1;
                for (ulong i = 1; i < exp; ++i) {
                    dst[rn] = lmmp_mul_1_(dst, dst, rn, base[0]);
                    ++rn;
                    rn -= (dst[rn - 1] == 0);
                }
                return rn + base_tz;
            } else {
                return lmmp_pow_1_(dst, rn, base[0], exp) + base_tz;
            }
        } else { /* n > 2 */
            if (exp > POW_WIN2_EXP_THRESHOLD && n > POW_WIN2_N_THRESHOLD) {
                if ((exp % 4 == 3) || (2 * lmmp_limb_popcnt_(exp) >= (lmmp_limb_bits_(exp)))) {
                    return lmmp_pow_win2_(dst, rn, base, n, exp) + base_tz;
                }
            }
            if (exp & 1) {
                return lmmp_pow_basecase_(dst, rn, base, n, exp) + base_tz;
            }

            int tz = lmmp_tailing_zeros_(exp);
            TEMP_DECL;
            mp_ptr restrict sq = TALLOC_TYPE((rn + 2) >> 1, mp_limb_t);
            exp >>= tz;

            if (tz & 1) {
                if (exp == 1) {
                    lmmp_copy(sq, base, n);
                    rn = n;
                } else {
                    mp_size_t rn1 = lmmp_pow_size_(base, n, exp);
                    rn = lmmp_pow_basecase_(sq, rn1, base, n, exp);
                }
                int i = 2;
                for (; i <= tz; i += 2) {
                    lmmp_sqr_(dst, sq, rn);
                    rn <<= 1;
                    rn -= (dst[rn - 1] == 0);
                    lmmp_sqr_(sq, dst, rn);
                    rn <<= 1;
                    rn -= (sq[rn - 1] == 0);
                }
                lmmp_sqr_(dst, sq, rn);
                rn <<= 1;
                rn -= (dst[rn - 1] == 0);
            } else {
                if (exp == 1) {
                    lmmp_copy(dst, base, n);
                    rn = n;
                } else {
                    mp_size_t rn1 = lmmp_pow_size_(base, n, exp);
                    rn = lmmp_pow_basecase_(dst, rn1, base, n, exp);
                }
                int i = 2;
                for (; i <= tz; i += 2) {
                    lmmp_sqr_(sq, dst, rn);
                    rn <<= 1;
                    rn -= (sq[rn - 1] == 0);
                    lmmp_sqr_(dst, sq, rn);
                    rn <<= 1;
                    rn -= (dst[rn - 1] == 0);
                }
            }
            TEMP_FREE;
            return rn + base_tz;
        }
    }
}
