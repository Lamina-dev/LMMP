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

#include "../../../include/lmmp/impl/is_prime_table.h"
#include "../../../include/lmmp/impl/prime_table.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

/*
mont63 只能用于小于 2^63 的数，否则会溢出导致计算结果不正确
mont64 可以用于任意大小的数，但由于考虑了溢出的情况，所以速度理论上会慢一些（并未详细测试）
*/
#define MONT63_MAX ((ulong)(0x7fffffffffffffff))

static inline ulong mont64_reduce(u128 t, ulong m, ulong m_inv) {
    // sum = k*m + t < 2^129，第 129 位 c 由 128 位回绕捕获
    ulong k = _u128low(t) * m_inv;
    u128 km = (u128)k * m;
    u128 sum = km + t;
    uint c = sum < km;
    ulong hi = _u128high(sum);
    if (c) return hi - m;  // 2^128 + hi - m 的低 128 位高位即结果
    return hi >= m ? hi - m : hi;
}

static inline ulong mont_R2(ulong m) {
    mp_bitcnt_t shift = 0;
    clz_shl_u64(m, m, shift);
    mp_limb_t r[3] = {0, 0, 1ull << shift};
    mp_limb_t q[2];
    lmmp_div_1_s_(q, r, 3, m);
    return r[0] >> shift;
}

static inline ulong mont64_R2(ulong m) {
    return mont_R2(m);
}

static inline ulong to_mont64(ulong x, ulong R2, ulong m, ulong m_inv) {
    return mont64_reduce((u128)x * R2, m, m_inv);
}

static inline ulong from_mont64(ulong x, ulong m, ulong m_inv) {
    return mont64_reduce(x, m, m_inv);
}

static inline ulong mont64_mul(ulong a, ulong b, ulong m, ulong m_inv) {
    return mont64_reduce((u128)a * b, m, m_inv);
}

static inline ulong mont63_reduce(u128 t, ulong m, ulong m_inv) {
    // m < 2^63 时 k*m + t < 2^127，无 129 位溢出
    ulong k = _u128low(t) * m_inv;
    u128 sum = (u128)k * m + t;
    ulong hi = _u128high(sum);
    return hi >= m ? hi - m : hi;
}

static inline ulong mont63_R2(ulong m) {
    return mont_R2(m);
}

static inline ulong to_mont63(ulong x, ulong R2, ulong m, ulong m_inv) {
    return mont63_reduce((u128)x * R2, m, m_inv);
}

static inline ulong from_mont63(ulong x, ulong m, ulong m_inv) {
    return mont63_reduce(x, m, m_inv);
}

static inline ulong mont63_mul(ulong a, ulong b, ulong m, ulong m_inv) {
    return mont63_reduce((u128)a * b, m, m_inv);
}

uint lmmp_powmod_uint_odd_(uint base, ulong exp, uint mod) {
    lmmp_param_assert(mod > base);
    lmmp_param_assert(mod % 2 == 1);
    lmmp_param_assert(mod > 1);
    ulong dst = 1;
    ulong b = base;
    _udiv64_t binv = _udiv64_gen(mod);
    ulong q;
    while (1) {
        if (exp & 1) {
            dst *= b;
            q = _udiv64by64_q_preinv(dst, &binv);
            dst -= q * mod;
        }
        exp >>= 1;
        if (exp == 0)
            break;
        b *= b;
        q = _udiv64by64_q_preinv(b, &binv);
        b -= q * mod;
    }
    return dst;
}

ulong lmmp_powmod_ulong_odd_(ulong base, ulong exp, ulong mod) {
    lmmp_param_assert(mod > base);
    lmmp_param_assert(mod % 2 == 1);
    lmmp_param_assert(mod > 1);
    if (mod <= MP_UINT_MAX)
        return lmmp_powmod_uint_odd_(base, exp, mod);
    else if (mod <= MONT63_MAX) {
        ulong R2 = mont63_R2(mod);
        ulong m_inv = lmmp_binvert_ulong_(mod);
        m_inv = -m_inv;
        ulong dst = to_mont63(1, R2, mod, m_inv);
        base = to_mont63(base, R2, mod, m_inv);
        while (1) {
            if (exp & 1)
                dst = mont63_mul(dst, base, mod, m_inv);
            exp >>= 1;
            if (exp == 0)
                break;
            base = mont63_mul(base, base, mod, m_inv);
        }
        return from_mont63(dst, mod, m_inv);
    } else {
        ulong R2 = mont64_R2(mod);
        ulong m_inv = lmmp_binvert_ulong_(mod);
        m_inv = -m_inv;
        ulong dst = to_mont64(1, R2, mod, m_inv);
        base = to_mont64(base, R2, mod, m_inv);
        while (1) {
            if (exp & 1)
                dst = mont64_mul(dst, base, mod, m_inv);
            exp >>= 1;
            if (exp == 0)
                break;
            base = mont64_mul(base, base, mod, m_inv);
        }
        return from_mont64(dst, mod, m_inv);
    }
}

static inline int miller_rabin_32(ulong a, ulong t, ulong u, uint m, _udiv64_t* binv) {
    ulong v = 1;
    ulong base = a;
    ulong q;
    while (1) {
        if (u & 1) {
            v *= base;
            q = _udiv64by64_q_preinv(v, binv);
            v -= q * m;
        }
        u >>= 1;
        if (u == 0)
            break;
        base *= base;
        q = _udiv64by64_q_preinv(base, binv);
        base -= q * m;
    }

    if (v == 1 || v == m - 1)
        return 1;
    for (ulong j = 1; j < t; ++j) {
        v *= v;
        q = _udiv64by64_q_preinv(v, binv);
        v -= q * m;
        if (v == m - 1)
            return 1;
        if (v == 1)
            return 0;
    }
    return 0;
}

/*
    强伪素数第二阶段检查：v = b^d（蒙域）已算出。
    one/m_1 为蒙域的 1 与 m-1。
    与旧实现一致：平方链中先查 m_1，再查 one（提前判定合数）。
*/
static inline int sprp_check63(ulong v, ulong t, ulong m, ulong m_inv, ulong one, ulong m_1) {
    if (v == one || v == m_1) return 1;
    for (t--; t > 0; t--) {
        v = mont63_mul(v, v, m, m_inv);
        if (v == m_1) return 1;
        if (v == one) return 0;
    }
    return 0;
}

static inline int sprp_check64(ulong v, ulong t, ulong m, ulong m_inv, ulong one, ulong m_1) {
    if (v == one || v == m_1) return 1;
    for (t--; t > 0; t--) {
        v = mont64_mul(v, v, m, m_inv);
        if (v == m_1) return 1;
        if (v == one) return 0;
    }
    return 0;
}

/*
    基底 2 专用幂（蒙域，LSB 链）：基底以蒙域值起步后，平方链保持
   base_j = 2^(2^j)*B 不变量，结果恒在蒙域；平方链与 v 乘链相互独立，
   乱序执行可重叠（实测优于 MSB 加倍链——加倍条件减位于串行依赖上）。
*/
static inline ulong powmod2_63(ulong d, ulong R2, ulong m, ulong m_inv) {
    ulong v = to_mont63(1, R2, m, m_inv);
    ulong base = to_mont63(2, R2, m, m_inv);
    while (1) {
        if (d & 1) v = mont63_mul(v, base, m, m_inv);
        d >>= 1;
        if (d == 0) break;
        base = mont63_mul(base, base, m, m_inv);
    }
    return v;
}

static inline ulong powmod2_64(ulong d, ulong R2, ulong m, ulong m_inv) {
    ulong v = to_mont64(1, R2, m, m_inv);
    ulong base = to_mont64(2, R2, m, m_inv);
    while (1) {
        if (d & 1) v = mont64_mul(v, base, m, m_inv);
        d >>= 1;
        if (d == 0) break;
        base = mont64_mul(base, base, m, m_inv);
    }
    return v;
}

/*******************************************************************************
 * from http://probableprime.org/download/example-primality.c
 * Deterministic Miller-Rabin tests for 64-bit.
 * Hashed 2-bases for n < 684630005672341 (slightly more than 2^49)
 * Hashed 3-bases for n < 2^64
 *
 * Based on Steve Worley's 2^32 example:
 *    http://www.mersenneforum.org/showthread.php?t=12209
 * With a 3-base encoding idea from Bradley Berg.
 *
 * Copyright 2014, Dana Jacobsen <dana@acm.org>
 *******************************************************************************/

bool lmmp_is_prime_uint_(uint n) {
    if (n == 2)
        return true;
    if (n % 2 == 0 || n <= 1)
        return false;
    int judge = lmmp_is_prime_table_(n);
    if (judge == 0) {
        return false;
    } else if (judge == 1) {
        return true;
    }

    if (trial_div35711(n))
        return false;

    ushort bases[2];
    bases[0] = 2;
    bases[1] = dj_base49[((0x3AC69A35UL * n) & 0xFFFFFFFFUL) >> 21] + 3;
    if (n % bases[0] == 0)
        return false;
    if (n % bases[1] == 0)
        return false;

    ulong u = n - 1, t = 0;
    while (u % 2 == 0) u /= 2, ++t;

    _udiv64_t binv = _udiv64_gen(n);
    if (miller_rabin_32(bases[0], t, u, n, &binv))
        if (miller_rabin_32(bases[1], t, u, n, &binv))
            return true;
        else
            return false;
    else
        return false;
}

static void powmod_win3_63(ulong *pv, ulong bm, ulong d, int ebits, ulong m, ulong m_inv) {
    ulong w[8], v = bm;
    int bit = ebits - 2;
    w[1] = bm;
    w[2] = mont63_mul(bm, bm, m, m_inv);
    w[3] = mont63_mul(w[2], bm, m, m_inv);
    w[4] = mont63_mul(w[2], w[2], m, m_inv);
    w[5] = mont63_mul(w[4], bm, m, m_inv);
    w[6] = mont63_mul(w[3], w[3], m, m_inv);
    w[7] = mont63_mul(w[6], bm, m, m_inv);
    while (bit >= 2) {
        v = mont63_mul(v, v, m, m_inv);
        v = mont63_mul(v, v, m, m_inv);
        v = mont63_mul(v, v, m, m_inv);
        {
            uint idx = (uint)(d >> (bit - 2)) & 7;
            if (idx) v = mont63_mul(v, w[idx], m, m_inv);
        }
        bit -= 3;
    }
    for (; bit >= 0; bit--) {
        v = mont63_mul(v, v, m, m_inv);
        if ((d >> bit) & 1) v = mont63_mul(v, bm, m, m_inv);
    }
    *pv = v;
}

static void powmod_win3_64(ulong *pv, ulong bm, ulong d, int ebits, ulong m, ulong m_inv) {
    ulong w[8], v = bm;
    int bit = ebits - 2;
    w[1] = bm;
    w[2] = mont64_mul(bm, bm, m, m_inv);
    w[3] = mont64_mul(w[2], bm, m, m_inv);
    w[4] = mont64_mul(w[2], w[2], m, m_inv);
    w[5] = mont64_mul(w[4], bm, m, m_inv);
    w[6] = mont64_mul(w[3], w[3], m, m_inv);
    w[7] = mont64_mul(w[6], bm, m, m_inv);
    while (bit >= 2) {
        v = mont64_mul(v, v, m, m_inv);
        v = mont64_mul(v, v, m, m_inv);
        v = mont64_mul(v, v, m, m_inv);
        {
            uint idx = (uint)(d >> (bit - 2)) & 7;
            if (idx) v = mont64_mul(v, w[idx], m, m_inv);
        }
        bit -= 3;
    }
    for (; bit >= 0; bit--) {
        v = mont64_mul(v, v, m, m_inv);
        if ((d >> bit) & 1) v = mont64_mul(v, bm, m, m_inv);
    }
    *pv = v;
}

static void powmod2_win3_63(ulong *pv1, ulong *pv2, ulong bm1, ulong bm2, ulong d, int ebits,
                            ulong m, ulong m_inv) {
    ulong w1[8], w2[8], v1 = bm1, v2 = bm2;
    int bit = ebits - 2;
    w1[1] = bm1;
    w1[2] = mont63_mul(bm1, bm1, m, m_inv);
    w1[3] = mont63_mul(w1[2], bm1, m, m_inv);
    w1[4] = mont63_mul(w1[2], w1[2], m, m_inv);
    w1[5] = mont63_mul(w1[4], bm1, m, m_inv);
    w1[6] = mont63_mul(w1[3], w1[3], m, m_inv);
    w1[7] = mont63_mul(w1[6], bm1, m, m_inv);
    w2[1] = bm2;
    w2[2] = mont63_mul(bm2, bm2, m, m_inv);
    w2[3] = mont63_mul(w2[2], bm2, m, m_inv);
    w2[4] = mont63_mul(w2[2], w2[2], m, m_inv);
    w2[5] = mont63_mul(w2[4], bm2, m, m_inv);
    w2[6] = mont63_mul(w2[3], w2[3], m, m_inv);
    w2[7] = mont63_mul(w2[6], bm2, m, m_inv);
    while (bit >= 2) {
        v1 = mont63_mul(v1, v1, m, m_inv);
        v2 = mont63_mul(v2, v2, m, m_inv);
        v1 = mont63_mul(v1, v1, m, m_inv);
        v2 = mont63_mul(v2, v2, m, m_inv);
        v1 = mont63_mul(v1, v1, m, m_inv);
        v2 = mont63_mul(v2, v2, m, m_inv);
        {
            uint idx = (uint)(d >> (bit - 2)) & 7;
            if (idx) {
                v1 = mont63_mul(v1, w1[idx], m, m_inv);
                v2 = mont63_mul(v2, w2[idx], m, m_inv);
            }
        }
        bit -= 3;
    }
    for (; bit >= 0; bit--) {
        v1 = mont63_mul(v1, v1, m, m_inv);
        v2 = mont63_mul(v2, v2, m, m_inv);
        if ((d >> bit) & 1) {
            v1 = mont63_mul(v1, bm1, m, m_inv);
            v2 = mont63_mul(v2, bm2, m, m_inv);
        }
    }
    *pv1 = v1;
    *pv2 = v2;
}

static void powmod2_win3_64(ulong *pv1, ulong *pv2, ulong bm1, ulong bm2, ulong d, int ebits,
                            ulong m, ulong m_inv) {
    ulong w1[8], w2[8], v1 = bm1, v2 = bm2;
    int bit = ebits - 2;
    w1[1] = bm1;
    w1[2] = mont64_mul(bm1, bm1, m, m_inv);
    w1[3] = mont64_mul(w1[2], bm1, m, m_inv);
    w1[4] = mont64_mul(w1[2], w1[2], m, m_inv);
    w1[5] = mont64_mul(w1[4], bm1, m, m_inv);
    w1[6] = mont64_mul(w1[3], w1[3], m, m_inv);
    w1[7] = mont64_mul(w1[6], bm1, m, m_inv);
    w2[1] = bm2;
    w2[2] = mont64_mul(bm2, bm2, m, m_inv);
    w2[3] = mont64_mul(w2[2], bm2, m, m_inv);
    w2[4] = mont64_mul(w2[2], w2[2], m, m_inv);
    w2[5] = mont64_mul(w2[4], bm2, m, m_inv);
    w2[6] = mont64_mul(w2[3], w2[3], m, m_inv);
    w2[7] = mont64_mul(w2[6], bm2, m, m_inv);
    while (bit >= 2) {
        v1 = mont64_mul(v1, v1, m, m_inv);
        v2 = mont64_mul(v2, v2, m, m_inv);
        v1 = mont64_mul(v1, v1, m, m_inv);
        v2 = mont64_mul(v2, v2, m, m_inv);
        v1 = mont64_mul(v1, v1, m, m_inv);
        v2 = mont64_mul(v2, v2, m, m_inv);
        {
            uint idx = (uint)(d >> (bit - 2)) & 7;
            if (idx) {
                v1 = mont64_mul(v1, w1[idx], m, m_inv);
                v2 = mont64_mul(v2, w2[idx], m, m_inv);
            }
        }
        bit -= 3;
    }
    for (; bit >= 0; bit--) {
        v1 = mont64_mul(v1, v1, m, m_inv);
        v2 = mont64_mul(v2, v2, m, m_inv);
        if ((d >> bit) & 1) {
            v1 = mont64_mul(v1, bm1, m, m_inv);
            v2 = mont64_mul(v2, bm2, m, m_inv);
        }
    }
    *pv1 = v1;
    *pv2 = v2;
}

bool lmmp_is_prime_notrial_(ulong n) {
    lmmp_param_assert(n > 2);
    if (n < 684630005672341) {
        ushort bases[2];
        bases[0] = 2;
        bases[1] = dj_base49[((0x3AC69A35UL * n) & 0xFFFFFFFFUL) >> 21] + 3;
        if (n % bases[0] == 0)
            return false;
        if (n % bases[1] == 0)
            return false;

        ulong m_inv = -lmmp_binvert_ulong_(n);
        ulong R2 = mont63_R2(n);
        ulong one = to_mont63(1, R2, n, m_inv);
        ulong m_1 = to_mont63(n - 1, R2, n, m_inv);

        ulong d;
        ulong t;
        ctz_shr_u64(d, n - 1, t);
        int eb = lmmp_limb_bits_(d);

        ulong v2 = powmod2_63(d, R2, n, m_inv);
        if (!sprp_check63(v2, t, n, m_inv, one, m_1)) return false;

        ulong v;
        powmod_win3_63(&v, to_mont63(bases[1], R2, n, m_inv), d, eb, n, m_inv);
        return sprp_check63(v, t, n, m_inv, one, m_1);
    } else {
        ushort bases[3];
        ulong bbmask = dj_base64[((0x3AC69A35UL * n) & 0xFFFFFFFFUL) >> 18];
        bases[0] = 2;
        bases[1] = (bbmask & 0x8000) ? 26460 : 9375;
        bases[2] = (bbmask & 0x7FFF) + 3;

        if (n % bases[0] == 0)
            return false;
        if (n % bases[1] == 0)
            return false;
        if (n % bases[2] == 0)
            return n == bases[2];

        ulong one = 1;
        ulong m_1 = n - 1;
        ulong m_inv = -lmmp_binvert_ulong_(n);

        ulong d;
        ulong t;
        ctz_shr_u64(d, n - 1, t);
        int eb = lmmp_limb_bits_(d);

        if (n <= MONT63_MAX) {
            ulong R2 = mont63_R2(n);
            one = to_mont63(one, R2, n, m_inv);
            m_1 = to_mont63(m_1, R2, n, m_inv);

            ulong v2 = powmod2_63(d, R2, n, m_inv);
            if (!sprp_check63(v2, t, n, m_inv, one, m_1)) return false;

            /* 双基底交错 + 3 位窗口 */
            ulong bm1 = to_mont63(bases[1], R2, n, m_inv);
            ulong bm2 = to_mont63(bases[2], R2, n, m_inv);
            ulong v1, v2_;
            powmod2_win3_63(&v1, &v2_, bm1, bm2, d, eb, n, m_inv);
            if (!sprp_check63(v1, t, n, m_inv, one, m_1)) return false;
            return sprp_check63(v2_, t, n, m_inv, one, m_1);
        } else {
            ulong R2 = mont64_R2(n);
            one = to_mont64(one, R2, n, m_inv);
            m_1 = to_mont64(m_1, R2, n, m_inv);

            ulong v2 = powmod2_64(d, R2, n, m_inv);
            if (!sprp_check64(v2, t, n, m_inv, one, m_1)) return false;

            /* 双基底交错 + 3 位窗口 */
            ulong bm1 = to_mont64(bases[1], R2, n, m_inv);
            ulong bm2 = to_mont64(bases[2], R2, n, m_inv);
            ulong v1, v2_;
            powmod2_win3_64(&v1, &v2_, bm1, bm2, d, eb, n, m_inv);
            if (!sprp_check64(v1, t, n, m_inv, one, m_1)) return false;
            return sprp_check64(v2_, t, n, m_inv, one, m_1);
        }
    }
}

bool lmmp_is_prime_ulong_(ulong n) {
    if (n == 2)
        return true;
    if (n % 2 == 0 || n <= 1)
        return false;
    if (n <= MP_UINT_MAX) {
        int judge = lmmp_is_prime_table_(n);
        if (judge == 0) {
            return false;
        } else if (judge == 1) {
            return true;
        }
    }
    if (trial_div35711(n))
        return false;
    return lmmp_is_prime_notrial_(n);
}

static inline bool trial_div13(ulong n) {
    const _udiv64_t div13 = {.magic = 4256940940086819604ull, .more = 3};
    ulong q = _udiv64by64_q_preinv(n, &div13);
    n -= q * 13;
    return n == 0;
}

static inline bool trial_div17(ulong n) {
    const _udiv64_t div17 = {.magic = 16276538888567251426ull, .more = 4};
    ulong q = _udiv64by64_q_preinv(n, &div17);
    n -= q * 17;
    return n == 0;
}

static inline bool trial_div19(ulong n) {
    const _udiv64_t div19 = {.magic = 12621456471485482685ull, .more = 4};
    ulong q = _udiv64by64_q_preinv(n, &div19);
    n -= q * 19;
    return n == 0;
}

static inline bool trial_div23(ulong n) {
    const _udiv64_t div23 = {.magic = 7218291159277650633ull, .more = 4};
    ulong q = _udiv64by64_q_preinv(n, &div23);
    n -= q * 23;
    return n == 0;
}

static inline bool trial_div29(ulong n) {
    const _udiv64_t div29 = {.magic = 1908283869694091547ull, .more = 4};
    ulong q = _udiv64by64_q_preinv(n, &div29);
    n -= q * 29;
    return n == 0;
}

static inline bool trial_div31(ulong n) {
    const _udiv64_t div31 = {.magic = 595056260442243601ull, .more = 4};
    ulong q = _udiv64by64_q_preinv(n, &div31);
    n -= q * 31;
    return n == 0;
}

static inline bool trial_div37(ulong n) {
    const _udiv64_t div37 = {.magic = 13461137567301564693ull, .more = 5};
    ulong q = _udiv64by64_q_preinv(n, &div37);
    n -= q * 37;
    return n == 0;
}

static inline bool trial_div41(ulong n) {
    const _udiv64_t div41 = {.magic = 10348173504763894809ull, .more = 5};
    ulong q = _udiv64by64_q_preinv(n, &div41);
    n -= q * 41;
    return n == 0;
}

// 64位内最大的质数
#define ULONG_PRIME_MAX 0xFFFFFFFFFFFFFFC5ull
#define ULONG_PRIME_MIN 2

ulong lmmp_next_prime_ulong_(ulong n) {
    if (n < prime_short_table[PRIME_SHORT_TABLE_SIZE - 1]) {
        ushort idx = lmmp_prime_cnt16_(n);
        return prime_short_table[idx];
    } else if (n >= ULONG_PRIME_MAX) {
        return MP_ULONG_MAX;
    } else {
        n += (n % 2 == 0) ? 1 : 2;
        while (1) {
            if (trial_div35711(n)
             || trial_div13(n)
             || trial_div17(n)
             || trial_div19(n)
             || trial_div23(n)
             || trial_div29(n)
             || trial_div31(n)
             || trial_div37(n)
             || trial_div41(n)) {
                n += 2;
            } else {
                if (lmmp_is_prime_notrial_(n)) {
                    return n;
                } else {
                    n += 2;
                }
            }
        }
    }
}

ulong lmmp_prev_prime_ulong_(ulong n) {
    if (n < ULONG_PRIME_MIN) {
        return 0;
    } else if (n < PRIME_SHORT_TABLE_N) {
        ushort idx = lmmp_prime_cnt16_(n);
        return prime_short_table[idx - 1];
    } else {
        n -= (n % 2 == 0) ? 1 : 0;
        while (1) {
            if (trial_div35711(n)
             || trial_div13(n)
             || trial_div17(n)
             || trial_div19(n)
             || trial_div23(n)
             || trial_div29(n)
             || trial_div31(n)
             || trial_div37(n)
             || trial_div41(n)) {
                n -= 2;
            } else {
                if (lmmp_is_prime_notrial_(n)) {
                    return n;
                } else {
                    n -= 2;
                }
            }
        }
    }
}
