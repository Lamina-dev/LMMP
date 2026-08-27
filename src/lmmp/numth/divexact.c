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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mul_cache.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


void lmmp_divexact_1_(mp_ptr dst, mp_srcptr np, mp_size_t nn, mp_limb_t d, mp_limb_t dinv) {
    lmmp_param_assert(d % 2 == 1);
    lmmp_param_assert(dinv * d == 1);
    lmmp_param_assert(nn > 0);
    lmmp_debug_assert(dst != NULL && np != NULL);
    mp_limb_t c = 0;
    mp_limb_t l, s, lo, hi, q;
    mp_size_t i;
    for (i = 0; i < nn - 1; i++) {
        s = np[i];
        l = s - c;
        c = l > s;
        q = l * dinv;
        dst[i] = q;
        _umul64to128_(q, d, &lo, &hi);
        c += hi;
    }
    s = np[i];
    l = s - c;
    c = l > s;
    q = l * dinv;
    dst[i] = q;
    lmmp_debug_assert(c == 0);
}

void lmmp_divexact_2_(mp_ptr dst, mp_srcptr np, mp_size_t nn, mp_srcptr restrict dp, mp_srcptr restrict dinv) {
    lmmp_param_assert(dp[0] % 2 == 1);
    lmmp_param_assert(nn > 1);
    lmmp_debug_assert(dst != NULL && np != NULL);
    mp_limb_t c[2] = {0, 0};
    mp_limb_t l[2], s[2], q[2], t[4];
    mp_size_t i;
    mp_limb_t d0 = dp[0], d1 = dp[1];
    mp_limb_t ddinv0 = dinv[0], ddinv1 = dinv[1];

    if (nn % 2 == 0) {
        for (i = 0; i < nn - 2; i += 2) {
            s[0] = np[i];
            s[1] = np[i + 1];
            _u128sub(l, s, c);
            c[0] = _u128cmp(s, l) ? 1 : 0;
            c[1] = 0;
            _umul128to128_(l[1], l[0], ddinv1, ddinv0, q);
            dst[i] = q[0];
            dst[i + 1] = q[1];
            _umul128to256_(q[1], q[0], d1, d0, t);
            _u128add(c, c, t + 2);
        }
        s[0] = np[i];
        s[1] = np[i + 1];
        _u128sub(l, s, c);
        c[0] = _u128cmp(s, l);
        _umul128to128_(l[1], l[0], ddinv1, ddinv0, q);
        dst[i] = q[0];
        // 商最高 limb 为 0 时 c[0] 可能为 1；函数结果仍然正确。
    } else {
        i = 0;
        if (nn >= 5) {
            for (; i < nn - 4; i += 2) {
                s[0] = np[i];
                s[1] = np[i + 1];
                _u128sub(l, s, c);
                c[0] = _u128cmp(s, l);
                c[1] = 0;
                _umul128to128_(l[1], l[0], ddinv1, ddinv0, q);
                dst[i] = q[0];
                dst[i + 1] = q[1];
                _umul128to256_(q[1], q[0], d1, d0, t);
                _u128add(c, c, t + 2);
            }
        }
        s[0] = np[i];
        s[1] = np[i + 1];
        _u128sub(l, s, c);
        c[0] = _u128cmp(s, l);
        _umul128to128_(l[1], l[0], ddinv1, ddinv0, q);
        dst[i] = q[0];
        dst[i + 1] = q[1];
        // 商最高 limb 为 0 时 c[0] 可能为 1；函数结果仍然正确。
    }
}

static inline void lmmp_mullo_n_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict numa,
    mp_srcptr restrict numb,
    mp_size_t             n,
    mp_ptr    restrict   tp
) {
    if (n < MULLO_DC_THRESHOLD) {
        lmmp_mullo_dc_(dst, numa, numb, tp, n);
    } else {
        lmmp_mullo_fft_(dst, numa, numb, n, tp);
    }
}

#if 0
/**
 * @brief 已知乘积的低位，计算乘积的高位
 * @param dst 乘积的高位，长度为 n
 * @param lop 乘积的低位，长度为 n
 * @param numa 乘数a
 * @param numb 乘数b
 * @param n 乘数a和乘数b的 limb 长度
 * @param tp 临时空间（应需要2*n个limb）
 * @warning eqsep(dst,tp)
 */
static inline void lmmp_mulhi_n_(
    mp_ptr              dst,
    mp_srcptr restrict  lop,
    mp_srcptr restrict numa,
    mp_srcptr restrict numb,
    mp_size_t             n,
    mp_ptr               tp
) {
    if (n < MULHI_MERSENNE_THRESHOLD) {
        lmmp_mul_n_(tp, numa, numb, n);
        lmmp_copy(dst, tp + n, n);
    } else {
        mp_limb_t c;
        mp_size_t m = lmmp_fft_next_size_((n * 2 + 1) >> 1);
        lmmp_debug_assert(n * 2 > m && m >= n);
        lmmp_mul_mersenne_(tp, m, numa, n, numb, n);
        c = lmmp_sub_(tp, tp, m, lop, n);
        if (c != 0)
            lmmp_dec(tp);
        if (m == n) {
            lmmp_copy(dst, tp, n);
        } else {
            mp_size_t fn = m - n;
            mp_size_t sn = n - fn;
            lmmp_copy(dst, tp + n, fn);
            lmmp_copy(dst + fn, tp, sn);
        }
    }
}
#endif // 0

void lmmp_divexact_unbalanced_(
    mp_ptr              dst,
    mp_srcptr            np,
    mp_size_t            nn,
    mp_srcptr restrict   dp,
    mp_size_t            dn,
    mp_ptr    restrict dinv
) {
    lmmp_param_assert(dst != NULL && np != NULL && dp != NULL);
    lmmp_param_assert(dn > 0);
    lmmp_param_assert(nn >= dn);
    lmmp_param_assert((dp[0] & 1) != 0);

    TEMP_DECL;
    fft_gr_cache mersenne_ctx;
    fft_cache fft_ctx;
    int mersenne_flag = 0, fft_flag = 0;
    mp_ptr restrict tp = TALLOC_TYPE(4 * dn, mp_limb_t);
    if (dinv == NULL) {
        dinv = TALLOC_TYPE(dn, mp_limb_t);
        lmmp_binvert_n_dc_(dinv, dp, dn, tp);
    }

#define c       (tp)          // [tp, dn]
#define l       (tp + dn)     // [tp + dn, dn]
#define scratch (tp + 2 * dn) // [tp + 2 * dn, 2*dn]

    mp_size_t i = 0, qn = nn - dn + 1;
    mp_limb_t ca;
    lmmp_zero(c, dn);
    if (qn > dn) {
        for (; i < qn - dn; i += dn) {
            lmmp_sub_n_(l, np + i, c, dn);
            ca = lmmp_cmp_(l, np + i, dn) == 1 ? 1 : 0;

            // lmmp_mullo_n_(dst + i, l, dinv, dn, scratch);
            if (dn < MULLO_DC_THRESHOLD)
                lmmp_mullo_dc_(dst + i, l, dinv, scratch, dn);
            else {
                if (fft_flag == 0) {
                    mp_size_t hn = lmmp_fft_next_size_((dn * 2 + 1) >> 1);
                    lmmp_mul_fft_cache_init_(scratch, hn, l, dn, dinv, dn, &fft_ctx);
                    fft_flag = 1;
                } else {
                    lmmp_mul_fft_cache_(scratch, l, &fft_ctx);
                }
                lmmp_copy(dst + i, scratch, dn);
            }

            // lmmp_mulhi_n_(scratch + dn, l, dst + i, dp, dn, scratch);
            if (dn < MULHI_MERSENNE_THRESHOLD) {
                lmmp_mul_n_(scratch, dst + i, dp, dn);
            } else {
                mp_limb_t cy;
                mp_size_t m = lmmp_fft_next_size_((dn * 2 + 1) >> 1);
                if (mersenne_flag == 0) {
                    lmmp_mul_mersenne_cache_init_(scratch, m, dst + i, dn, dp, dn, &mersenne_ctx);
                    mersenne_flag = 1;
                } else {
                    lmmp_mul_mersenne_cache_(scratch, dst + i, &mersenne_ctx);
                }
                cy = lmmp_sub_(scratch, scratch, m, l, dn);
                if (cy != 0)
                    lmmp_dec(scratch);
                if (m == dn) {
                    lmmp_copy(scratch + dn, scratch, dn);
                } else {
                    mp_size_t fn = m - dn;
                    mp_size_t sn = dn - fn;
                    //lmmp_copy(scratch + dn, scratch + dn, fn);
                    lmmp_copy(scratch + dn + fn, scratch, sn);
                }
            }

            if (ca) {
                lmmp_add_1_(c, scratch + dn, dn, 1);
            } else {
                lmmp_copy(c, scratch + dn, dn);
            }
        }
    }
    lmmp_sub_n_(l, np + i, c, dn);
    lmmp_mullo_n_(dst + i, l, dinv, qn - i, scratch);

    if (mersenne_flag == 1)
        lmmp_fft_gr_cache_free_(&mersenne_ctx);
    if (fft_flag == 1)
        lmmp_fft_cache_free_(&fft_ctx);

    TEMP_FREE;
#undef c
#undef l
#undef scratch
}

void lmmp_divexact_basecase_(mp_ptr dst, mp_ptr np, mp_size_t nn, mp_srcptr restrict dp, mp_size_t dn, mp_limb_t dinv) {
    lmmp_param_assert(dst != NULL && np != NULL && dp != NULL);
    lmmp_param_assert(dn > 0);
    lmmp_param_assert(nn >= dn);
    lmmp_param_assert((dp[0] & 1) != 0);
    lmmp_param_assert((dp[0] * dinv) == 1);
    mp_size_t i;
    mp_limb_t q;
    mp_limb_t hi;
    mp_size_t qn = nn - dn + 1;
    for (i = 0; i < qn; i++) {
        q = dinv * np[i];
        hi = lmmp_submul_1_(np + i, dp, dn, q);
        lmmp_debug_assert(np[i] == 0);
#if LMMP_DEBUG_ASSERT_CHECK == 1
        if (hi && i + dn < nn) {
            lmmp_dec_1(np + i + dn, hi);
        } else {
            lmmp_debug_assert(hi == 0);
        }
#else
        if (hi) {
            lmmp_dec_1(np + i + dn, hi);
        }
#endif
        dst[i] = q;
    }
}

void lmmp_divexact_divide_(mp_ptr restrict dst, mp_srcptr restrict np, mp_size_t nn, mp_srcptr restrict dp, mp_size_t dn) {
    lmmp_param_assert(dst != NULL && np != NULL && dp != NULL);
    lmmp_param_assert(dn > 0);
    lmmp_param_assert(nn >= dn);
    lmmp_param_assert((dp[0] & 1) != 0);

    mp_size_t qn = nn - dn + 1;
    lmmp_param_assert(qn <= dn);
    mp_size_t rn = qn - qn / 2;
    qn = qn / 2;

    TEMP_DECL;
    mp_ptr restrict dinv = TALLOC_TYPE(rn, mp_limb_t);
    mp_ptr restrict tp = TALLOC_TYPE(3 * rn, mp_limb_t);
    lmmp_binvert_n_dc_(dinv, dp, rn, tp);
    lmmp_mullo_n_(dst, np, dinv, rn, tp);
    if (qn == 0) {
        TEMP_FREE;
        return;
    }

    lmmp_debug_assert(3 * rn >= qn + rn);
    lmmp_mul_(tp, dp, qn + rn, dst, rn);

    // 即使存在错位，高位的np也一定可以将其约减去，因此无需进行任何处理
    lmmp_sub_n_(tp, np + rn, tp + rn, qn);
    lmmp_mullo_n_(dst + rn, tp, dinv, qn, tp + rn);
    TEMP_FREE;
}

void lmmp_divexact_(mp_ptr dst, mp_srcptr np, mp_size_t nn, mp_srcptr restrict dp, mp_size_t dn) {
    lmmp_param_assert(dst != NULL && np != NULL && dp != NULL);
    lmmp_param_assert(dn > 0);
    lmmp_param_assert(nn >= dn);
    lmmp_param_assert((dp[0] & 1) != 0);

    if (dn == 1) {
        mp_limb_t dinv = lmmp_binvert_ulong_(dp[0]);
        lmmp_divexact_1_(dst, np, nn, dp[0], dinv);
    } else if (dn == 2) {
        mp_limb_t dinv[2];
        lmmp_binvert_2_(dinv, dp);
        lmmp_divexact_2_(dst, np, nn, dp, dinv);
    } else {
        mp_size_t qn = nn - dn + 1;
        if (nn < DIVEXACT_NN_THRESHOLD && dn < DIVEXACT_BASECASE_THRESHOLD) {
            mp_limb_t dinv = lmmp_binvert_ulong_(dp[0]);
            TEMP_DECL;
            mp_ptr restrict tp = TALLOC_TYPE(nn, mp_limb_t);
            lmmp_copy(tp, np, nn);
            lmmp_divexact_basecase_(dst, tp, nn, dp, dn, dinv);
            TEMP_FREE;
        } else if (qn < dn) {
            TEMP_DECL;
            mp_ptr restrict tp;
            if (dst == np) {
                tp = TALLOC_TYPE(nn, mp_limb_t);
                lmmp_copy(tp, np, nn);
                np = tp;
            }
            lmmp_divexact_divide_(dst, np, nn, dp, dn);
            TEMP_FREE;
        } else {
            lmmp_divexact_unbalanced_(dst, np, nn, dp, dn, NULL);
        }
    }
}