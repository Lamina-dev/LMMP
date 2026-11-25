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
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "impl/inlines.h"
#include "impl/longlong.h"
#include "mparam.h"
#include "impl/tmp_alloc.h"
#include "impl/mul_cache.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"


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

static inline mp_size_t lmmp_divexact_mu_window_(mp_size_t dn) {
    return dn >= DIVEXACT_MERSENNE_CACHE_THRESHOLD ? lmmp_fft_next_size_(dn + 1)
                                                   : lmmp_mersenne_next_size_(dn + 1);
}

static int lmmp_divexact_all_ones_(mp_srcptr p, mp_size_t n) {
    for (mp_size_t i = 0; i < n; ++i)
        if (p[i] != ~(mp_limb_t)0)
            return 0;
    return 1;
}

static void lmmp_divexact_mu_run_(
    mp_ptr              dst,
    mp_srcptr            np,
    mp_size_t            nn,
    mp_srcptr restrict   dp,
    mp_size_t            dn,
    mp_srcptr restrict dinv,
    mp_size_t            in,
    mp_ptr    restrict   tp
) {
    mp_size_t qn = nn - dn + 1;

    int ms_mode = dn >= DIVEXACT_MERSENNE_CACHE_THRESHOLD ? 2 : (dn >= DIVEXACT_MERSENNE_THRESHOLD ? 1 : 0);
    mp_size_t m = lmmp_divexact_mu_window_(dn);
    int use_lcache = in >= MULLO_DC_THRESHOLD;
    fft_gr_cache mctx;
    int mflag = 0;
    fft_mullo_cache lctx;
    int lflag = 0;

#define rp    (tp)           // [tp,        dn]    滑动余数窗
#define dpad  (tp + dn)      // [tp+dn,      m]    常驻 D'（mode 0 闲置）
#define work  (tp + dn + m)  // [tp+dn+m,   2m]    V/W'（mode 1/2）或全积（mode 0），亦作 mullo scratch

    /* D' = D·B^(m-in) mod B^m-1：[dp+in, dn-in] 落到 [dpad, dn-in]、[dp, in]
       落到 [dpad+m-in, in]，两段不交免进位 */
    lmmp_zero(dpad, m);
    lmmp_copy(dpad, dp + in, dn - in);
    lmmp_copy(dpad + m - in, dp, in);

    mp_limb_t cy = 0;
    mp_ptr qp = dst;

    lmmp_copy(rp, np, dn);
    np += dn;
    if (use_lcache) {
        lmmp_mullo_fft_cache_init_(qp, rp, dinv, in, work, &lctx);
        lflag = 1;
    } else {
        lmmp_mullo_n_(qp, rp, dinv, in, work);
    }
    qn -= in;

    while (qn > in) {
        /* [H, dn] = dp×qp div B^in：mode 1/2 截断窗恢复，mode 0 全积取高段 */
        mp_ptr H;
        if (ms_mode != 0) {
            lmmp_zero(work, m);
            lmmp_copy(work + m - in, rp, in); /* V = p_lo·B^(m-in) */
            if (ms_mode == 2) {
                if (mflag == 0) {
                    lmmp_mul_mersenne_cache_init_(work + m, m, qp, in, dpad, m, &mctx);
                    mflag = 1;
                } else {
                    lmmp_mul_mersenne_cache_(work + m, qp, &mctx);
                }
            } else {
                lmmp_mul_mersenne_(work + m, m, qp, in, dpad, m);
            }
            mp_ptr w = work + m; /* W' = D'×qp mod B^m-1，就地减 V 得 H */
            if (lmmp_sub_n_(w, w, work, m))
                lmmp_sub_1_(w, w, m, 1);
            if (lmmp_divexact_all_ones_(w, m))
                lmmp_zero(w, m);
            for (mp_size_t i = dn; i < m; ++i)
                lmmp_debug_assert(w[i] == 0); /* H < B^dn 的高段必为 0 */
            H = w;
        } else {
            lmmp_mul_(work, dp, dn, qp, in);
            H = work + in;
        }
        qp += in;
        if (dn != in) {
            cy += lmmp_sub_n_(rp, rp + in, H, dn - in);
            if (cy == 2) {
                lmmp_inc(H + dn - in);
                cy = 1;
            }
        }
        cy = lmmp_sub_nc_(rp + dn - in, np, H + dn - in, in, cy);
        np += in;
        if (use_lcache) {
            lmmp_mullo_fft_cache_(qp, rp, work, &lctx);
        } else {
            lmmp_mullo_n_(qp, rp, dinv, in, work);
        }
        qn -= in;
    }

    mp_ptr H;
    if (ms_mode != 0) {
        lmmp_zero(work, m);
        lmmp_copy(work + m - in, rp, in);
        if (ms_mode == 2) {
            if (mflag == 0) {
                lmmp_mul_mersenne_cache_init_(work + m, m, qp, in, dpad, m, &mctx);
                mflag = 1;
            } else {
                lmmp_mul_mersenne_cache_(work + m, qp, &mctx);
            }
        } else {
            lmmp_mul_mersenne_(work + m, m, qp, in, dpad, m);
        }
        mp_ptr w = work + m;
        if (lmmp_sub_n_(w, w, work, m))
            lmmp_sub_1_(w, w, m, 1);
        if (lmmp_divexact_all_ones_(w, m))
            lmmp_zero(w, m);
        for (mp_size_t i = dn; i < m; ++i)
            lmmp_debug_assert(w[i] == 0);
        H = w;
    } else {
        lmmp_mul_(work, dp, dn, qp, in);
        H = work + in;
    }
    qp += in;
    if (dn != in) {
        cy += lmmp_sub_n_(rp, rp + in, H, dn - in);
        if (cy == 2) {
            lmmp_inc(H + dn - in);
            cy = 1;
        }
    }
    lmmp_sub_nc_(rp + dn - in, np, H + dn - in, qn - (dn - in), cy);
    lmmp_mullo_n_(qp, rp, dinv, qn, work);

    if (mflag == 1)
        lmmp_fft_gr_cache_free_(&mctx);
    if (lflag == 1)
        lmmp_mullo_cache_free_(&lctx);

#undef rp
#undef dpad
#undef work
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

    mp_ptr restrict tp = TALLOC_TYPE(dn + 3 * lmmp_divexact_mu_window_(dn), mp_limb_t);
    if (dinv == NULL) {
        dinv = TALLOC_TYPE(in, mp_limb_t);
        lmmp_binvert_n_dc_(dinv, dp, in, tp);
    }

    lmmp_divexact_mu_run_(dst, np, nn, dp, dn, dinv, in, tp);

    TEMP_FREE;
}

void lmmp_divexact_basecase_(mp_ptr dst, mp_ptr np, mp_size_t nn, mp_srcptr restrict dp, mp_size_t dn) {
    lmmp_param_assert(dst != NULL && np != NULL && dp != NULL);
    lmmp_param_assert(dn > 0);
    lmmp_param_assert(nn >= dn);
    lmmp_param_assert((dp[0] & 1) != 0);

    const mp_size_t qn = nn - dn + 1;

    if (dn == 1) {
        lmmp_divexact_1_(dst, np, nn, dp[0], lmmp_binvert_ulong_(dp[0]));
        if (dst != np) {
            lmmp_zero(np, nn);
        }
        return;
    }

    /* 有效宽度：商 Q = N*D^(-1) mod B^qn 只需 d 的低 min(dn, qn) limb */
    const mp_size_t D = dn < qn ? dn : qn;

    const mp_limb_t ninv1 = 0 - lmmp_binvert_ulong_(dp[0]);
    const mp_limb_t e = _umul64to64hi_(dp[0], ninv1) + 1;
    const mp_limb_t ninv2 = ninv1 * e + dp[1] * ninv1 * ninv1;
    lmmp_debug_assert(dp[0] * ninv1 == ~(mp_limb_t)0);

    /* 商流式取负：Q_true = -Q mod B^qn = ~Q + 1，negc 为借位链 */
    mp_limb_t negc = 1;
#define LMMP_DIVX_PUT(k, qv)           \
    do {                               \
        mp_limb_t _q_ = (qv);          \
        dst[k] = ~_q_ + negc;          \
        negc &= (mp_limb_t)(_q_ == 0); \
    } while (0)

    mp_ptr up = np;
    mp_size_t i = 0;
    mp_limb_t cy = 0;

    for (; qn - i >= D + 2; i += 2, up += 2) {
        mp_limb_t q[2], qh;
        _umul64to128_(up[0], ninv1, &q[0], &qh);
        q[1] = qh + up[0] * ninv2 + up[1] * ninv1;
        const mp_limb_t sv = up[D];
        const mp_limb_t chi = lmmp_addmul_2_(up, dp, D, q);
        mp_limb_t s = sv + up[D];
        mp_limb_t c = s < sv;
        s += cy;
        c += s < cy;
        up[D] = s;
        mp_limb_t t = up[D + 1] + chi;
        cy = t < chi;
        t += c;
        cy += t < c;
        up[D + 1] = t;
        lmmp_debug_assert(up[0] == 0 && up[1] == 0);
        LMMP_DIVX_PUT(i, q[0]);
        LMMP_DIVX_PUT(i + 1, q[1]);
    }

    if (qn - i == D + 1) {
        const mp_limb_t q = up[0] * ninv1;
        const mp_limb_t hi = lmmp_addmul_1_(up, dp, D, q);
        up[D] += hi + cy;
        lmmp_debug_assert(up[0] == 0);
        LMMP_DIVX_PUT(i, q);
        i++;
        up++;
    }

    for (mp_size_t w = qn - i; w >= 2; w -= 2, i += 2, up += 2) {
        mp_limb_t q[2], qh;
        _umul64to128_(up[0], ninv1, &q[0], &qh);
        q[1] = qh + up[0] * ninv2 + up[1] * ninv1;
        lmmp_addmul_2_(up, dp, w, q);
        lmmp_debug_assert(up[0] == 0 && up[1] == 0);
        LMMP_DIVX_PUT(i, q[0]);
        LMMP_DIVX_PUT(i + 1, q[1]);
    }
    if (i < qn) {
        const mp_limb_t q = up[0] * ninv1;
        up[0] = 0;
        LMMP_DIVX_PUT(i, q);
    }
#undef LMMP_DIVX_PUT

    lmmp_zero(np + qn, nn - qn);
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
        if (dn < DIVEXACT_BASECASE_THRESHOLD) {
            TEMP_DECL;
            mp_ptr restrict tp = TALLOC_TYPE(nn, mp_limb_t);
            lmmp_copy(tp, np, nn);
            lmmp_divexact_basecase_(dst, tp, nn, dp, dn);
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
