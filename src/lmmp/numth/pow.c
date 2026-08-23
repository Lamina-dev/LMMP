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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/lglg.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


mp_size_t lmmp_pow_1_size_(mp_limb_t base, ulong exp) {
    lmmp_param_assert(base >= 1);
    lmmp_param_assert(exp > 0);
    if (base == 1) {
        return 1;
    } else if (exp <= 2) {
        return 3;
    } else if (exp <= MP_UINT_MAX) {
        /*
        base = b * 2^base_tz
        */
        slong base_tz = lmmp_limb_bits_(base);
        uint32_t b;
        if (base_tz < 32) {
            b = base << (32 - base_tz);
        } else {
            b = base >> (base_tz - 32);
        }
        base_tz = base_tz - 32;
        base_tz *= exp;
        mp_size_t rn = xlog2n_ceil(exp, b);
        rn += base_tz;
        rn = (rn + LIMB_BITS - 1) / LIMB_BITS;
        return rn + 2;
    } else {
        /*
        base = b * 2^base_tz
        */
        slong base_tz = lmmp_limb_bits_(base);
        uint32_t b;
        if (base_tz < 32) {
            b = base << (32 - base_tz);
        } else {
            b = base >> (base_tz - 32);
        }
        base_tz = base_tz - 32;
        base_tz *= exp;

        mp_size_t rn;
        /*
        exp = exp' * 2^bits
        exp*log2(base) = exp*log2(b*2^base_tz)
                        = exp*log2(b) + exp*base_tz
                        = exp'*log2(b)*2^bits + exp*base_tz
        */
        mp_bitcnt_t bits = lmmp_limb_bits_(exp);
        bits -= 32;
        exp >>= bits;
        exp++;
        rn = xlog2n_ceil(exp, b) << bits;
        rn += base_tz;
        rn = (rn + LIMB_BITS - 1) / LIMB_BITS;
        return rn + 2;
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
        base = b * 2^base_tz
        */
        mp_bitcnt_t base_tz = lmmp_limb_bits_(base[n - 1]);
        uint32_t b;
        if (base_tz < 32) {
            b = base[n - 1] << (32 - base_tz);
            b |= (base[n - 2] >> (LIMB_BITS - 32 + base_tz));
            base_tz = (n - 2) * LIMB_BITS + LIMB_BITS - 32 + base_tz;
        } else if (base_tz == 32) {
            b = base[n - 1];
            base_tz = (n - 1) * LIMB_BITS;
        } else {
            b = base[n - 1] >> (base_tz - 32);
            base_tz = (n - 1) * LIMB_BITS + base_tz - 32;
        }

        mp_size_t rn;
        if (exp <= MP_UINT_MAX) {
            rn = exp * base_tz;
            rn += xlog2n_ceil(exp, b);
        } else {
            /*
            exp = exp' * 2^bits
            exp*log2(base) = exp*log2(b*2^base_tz)
                           = exp*log2(b) + exp*base_tz
                           = exp'*log2(b)*2^bits + exp*base_tz
            */
            mp_bitcnt_t bits = lmmp_limb_bits_(exp);
            rn = exp * base_tz;
            bits -= 32;
            exp >>= bits;
            exp++;
            rn += xlog2n_ceil(exp, b) << bits;
        }
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
                for (mp_size_t i = 1; i < exp; ++i) {
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
