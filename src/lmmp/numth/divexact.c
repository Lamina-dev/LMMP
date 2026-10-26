/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/mul_cache.h"
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
    lmmp_param_assert(nn > 1);
    lmmp_param_assert(dst != NULL && np != NULL);
    lmmp_param_assert(dp != NULL && dinv != NULL);
    lmmp_param_assert(dp[0] % 2 == 1);
    u128 c = 0;
    u128 l, s, q;
    mp_limb_t t[4];
    mp_size_t i;
    mp_limb_t d0 = dp[0], d1 = dp[1];
    mp_limb_t ddinv0 = dinv[0], ddinv1 = dinv[1];

    if (nn % 2 == 0) {
        for (i = 0; i < nn - 2; i += 2) {
            s = _u128load(np + i);
            l = s - c;
            c = (u128)(s < l);  // 借位（原 c 非 0 时成立）
            q = _umul128to128_(_u128high(l), _u128low(l), ddinv1, ddinv0);
            dst[i] = _u128low(q);
            dst[i + 1] = _u128high(q);
            _umul128to256_(_u128high(q), _u128low(q), d1, d0, t);
            c += _u128load(t + 2);
        }
        s = _u128load(np + i);
        l = s - c;
        c = (u128)(s < l);
        q = _umul128to128_(_u128high(l), _u128low(l), ddinv1, ddinv0);
        dst[i] = _u128low(q);
        // 商最高 limb 为 0 时 c 可能为 1；函数结果仍然正确。
    } else {
        i = 0;
        if (nn >= 5) {
            for (; i < nn - 4; i += 2) {
                s = _u128load(np + i);
                l = s - c;
                c = (u128)(s < l);
                q = _umul128to128_(_u128high(l), _u128low(l), ddinv1, ddinv0);
                dst[i] = _u128low(q);
                dst[i + 1] = _u128high(q);
                _umul128to256_(_u128high(q), _u128low(q), d1, d0, t);
                c += _u128load(t + 2);
            }
        }
        s = _u128load(np + i);
        l = s - c;
        c = (u128)(s < l);
        q = _umul128to128_(_u128high(l), _u128low(l), ddinv1, ddinv0);
        dst[i] = _u128low(q);
        dst[i + 1] = _u128high(q);
        // 商最高 limb 为 0 时 c 可能为 1；函数结果仍然正确。
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

static void lmmp_divexact_mu_run_(
    mp_ptr               dst,
    mp_srcptr             np,
    mp_size_t             nn,
    mp_srcptr restrict    dp,
    mp_size_t             dn,
    mp_srcptr restrict  dinv,
    mp_size_t             in,
    mp_ptr    restrict    tp /* [0,dn)=rp 滚动余数, [dn,dn+2*(dn+in))=积/mullo scratch */
) {
    mp_size_t qn = nn - dn + 1;
    mp_size_t pn = dn + in;

    /* 主积三档：朴素 | 梅森分发（不缓存）| 梅森缓存（仅 fft） */
    int ms_mode = dn >= DIVEXACT_MERSENNE_CACHE_THRESHOLD ? 2 : (dn >= DIVEXACT_MERSENNE_THRESHOLD ? 1 : 0);
    /* 缓存档取 fft 可行尺寸（缓存仅 fft），朴素/分发档取梅森细粒度（低 padding） */
    mp_size_t m = ms_mode == 2 ? lmmp_fft_next_size_(pn) : (ms_mode == 1 ? lmmp_mersenne_next_size_(pn) : 0);
    int use_lcache = in >= MULLO_DC_THRESHOLD;
    fft_gr_cache mctx;
    int mflag = 0;
    fft_mullo_cache lctx;
    int lflag = 0;

#define rp   (tp)
#define prod (tp + dn)

    mp_limb_t cy = 0;
    mp_ptr qp = dst;

    lmmp_copy(rp, np, dn);
    np += dn;
    if (use_lcache) {
        lmmp_mullo_fft_cache_init_(qp, rp, dinv, in, prod, &lctx);
        lflag = 1;
    } else {
        lmmp_mullo_n_(qp, rp, dinv, in, prod);
    }
    qn -= in;

    while (qn > in) {
        /* prod[0..pn) = dp × qp 块 */
        if (ms_mode == 2) {
            if (mflag == 0) {
                lmmp_mul_mersenne_cache_init_(prod, m, qp, in, dp, dn, &mctx);
                mflag = 1;
            } else {
                lmmp_mul_mersenne_cache_(prod, qp, &mctx);
            }
        } else if (ms_mode == 1) {
            lmmp_mul_mersenne_(prod, m, qp, in, dp, dn);
        } else {
            lmmp_mul_(prod, dp, dn, qp, in);
        }
        qp += in;
        if (dn != in) {
            cy += lmmp_sub_n_(rp, rp + in, prod + in, dn - in);
            if (cy == 2) {
                lmmp_inc(prod + dn);
                cy = 1;
            }
        }
        cy = lmmp_sub_nc_(rp + dn - in, np, prod + dn, in, cy);
        np += in;
        if (use_lcache) {
            lmmp_mullo_fft_cache_(qp, rp, prod, &lctx);
        } else {
            lmmp_mullo_n_(qp, rp, dinv, in, prod);
        }
        qn -= in;
    }

    /* 末块：余 qn<=in 位商；np 只剩 qn-(dn-in) limb 消费额度 */
    if (ms_mode == 2) {
        if (mflag == 0) {
            lmmp_mul_mersenne_cache_init_(prod, m, qp, in, dp, dn, &mctx);
            mflag = 1;
        } else {
            lmmp_mul_mersenne_cache_(prod, qp, &mctx);
        }
    } else if (ms_mode == 1) {
        lmmp_mul_mersenne_(prod, m, qp, in, dp, dn);
    } else {
        lmmp_mul_(prod, dp, dn, qp, in);
    }
    qp += in;
    if (dn != in) {
        cy += lmmp_sub_n_(rp, rp + in, prod + in, dn - in);
        if (cy == 2) {
            lmmp_inc(prod + dn);
            cy = 1;
        }
    }
    lmmp_sub_nc_(rp + dn - in, np, prod + dn, qn - (dn - in), cy);
    lmmp_mullo_n_(qp, rp, dinv, qn, prod);

    if (mflag == 1)
        lmmp_fft_gr_cache_free_(&mctx);
    if (lflag == 1)
        lmmp_mullo_cache_free_(&lctx);

#undef rp
#undef prod
}

void lmmp_divexact_unbalance_(
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
    mp_size_t qn = nn - dn + 1;

    if (qn <= dn) {
        mp_ptr restrict l = TALLOC_TYPE(qn, mp_limb_t);
        mp_ptr restrict t2 = TALLOC_TYPE(3 * qn, mp_limb_t);
        if (dinv == NULL) {
            dinv = TALLOC_TYPE(qn, mp_limb_t);
            lmmp_binvert_n_dc_(dinv, dp, qn, t2);
        }
        lmmp_copy(l, np, qn);
        lmmp_mullo_n_(dst, l, dinv, qn, t2);
        TEMP_FREE;
        return;
    }
    mp_size_t b = (qn - 1) / dn + 1;
    mp_size_t in = (qn - 1) / b + 1;
    mp_size_t pn = dn + in;

    mp_ptr restrict tp = TALLOC_TYPE(dn + 2 * pn, mp_limb_t);
    if (dinv == NULL) {
        dinv = TALLOC_TYPE(in, mp_limb_t);
        lmmp_binvert_n_dc_(dinv, dp, in, tp);
    }

    lmmp_divexact_mu_run_(dst, np, nn, dp, dn, dinv, in, tp);

    TEMP_FREE;
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
        // 末轮hi必定为 0
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
            lmmp_divexact_unbalance_(dst, np, nn, dp, dn, NULL);
        }
    }
}
