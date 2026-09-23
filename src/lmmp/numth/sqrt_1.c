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
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/numth.h"


ulong lmmp_sqrt_ulong_(ulong a) {
    ulong is;

    is = (ulong)sqrt((double)a);

    is -= (is * is > a);
    if (is == (1ULL << 32))
        is--;
    return is;
}

mp_limb_t lmmp_sqrt_1_(mp_ptr dstr, mp_limb_t x) {
    lmmp_param_assert(x >= LIMB_B_4);
    mp_limb_t s = lmmp_sqrt_ulong_(x);
    *dstr = x - s * s;
    return s;
}

mp_limb_t lmmp_sqrt_2_(mp_ptr dstr, mp_srcptr numa) {
    lmmp_param_assert(numa[1] >= LIMB_B_4);
    mp_limb_t rl, s, q, al, u;
    mp_slimb_t rh;

    s = lmmp_sqrt_1_(&rl, numa[1]);
    al = numa[0];

    //(r:alh)/2
    rl = rl << 31 | al >> 33;
    q = rl / s;
    q -= q >> 32;

    u = rl - s * q;
    s = s << 32 | q;
    rh = u >> 31;
    rl = (u << 33) | (al & (((mp_limb_t)1 << 33) - 1));

    q *= q;
    rh -= rl < q;
    rl -= q;
    if (rh < 0) {
        rl += s;
        rh += rl < s;
        --s;
        rl += s;
        rh += rl < s;
    }

    dstr[0] = rl;
    dstr[1] = rh;
    return s;
}

/*
    以 4 limb 为例，x = x3*B^3 + x2*B^2 + x1*B + x0（x3 >= B/4）：

        (s', rh) = sqrtrem(x3*B + x2)          2 limb 根，rh <= 2s'
        N        = rh*B + x1
        (Q2, u)  = divrem(N, s')               顶 limb 先归约至 s' 以下
        s_low    = Q2 >> 1                     s = s'*B + s_low
        r        = (N mod 2s')*B + x0 - s_low^2

    r < 0 时 s -= 1、r += 2s+1 一步修正（Karatsuba 平方根保证 Q2 至多
    高估 2，修正至多一步）；修正后 r = x - s^2 <= 2s 恒成立。
    3 limb 输入采用 beta = 2^32 的非均衡分割（s = s'*2^32 + q，商 33
    bit），其余同理。
*/

void lmmp_sqrt_3_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa) {
    lmmp_param_assert(numa != NULL && dsts != NULL);
    lmmp_param_assert(numa[2] >= LIMB_B_4);
    mp_limb_t a0 = numa[0];
    mp_limb_t rh[2], n1, n0, q, u, uc, s1, s0, r1, r0, p[2], neg;

    // s' = sqrt([a2,a1]) >= 2^63，rh = [a2,a1] - s'^2 <= 2s'
    mp_limb_t sh = lmmp_sqrt_2_(rh, numa + 1);

    // N = rh*2^32 + (a0>>32) < 2^34*s'，N/s' < 2^34，n1 = N>>64 < s'
    n1 = (rh[1] << 32) | (rh[0] >> 32);
    n0 = (rh[0] << 32) | (a0 >> 32);
    mp_limb_t qh = _udiv128by64to64_(n1, n0, sh, &u);
    // q = N div 2s'，(uc,u) = N mod 2s'
    q = qh >> 1;
    _add_ssaaaa(uc, u, 0, u, 0, sh & (-(qh & 1)));

    // s = s'*2^32 + q
    _add_ssaaaa(s1, s0, sh >> 32, sh << 32, 0, q);

    // r = u*2^32 + (a0 & M32) - q^2，|r| < 2^98（双 limb 有符号）
    lmmp_mullh_(q, q, p);
    _sub_ddmmss(r1, r0, (uc << 32) | (u >> 32), (u << 32) | (a0 & (((mp_limb_t)1 << 32) - 1)),
                p[1], p[0]);

    // r < 0（概率约 1/4）：s -= 1，r += 2s + 1，(s0<<1)|1 = 2*s0+1 无进位
    neg = -(r1 >> (LIMB_BITS - 1));
    _sub_ddmmss(s1, s0, s1, s0, 0, neg & 1);
    _add_ssaaaa(r1, r0, r1, r0, neg & ((s1 << 1) | (s0 >> (LIMB_BITS - 1))),
                neg & ((s0 << 1) | 1));

    dsts[0] = s0;
    dsts[1] = s1;
    if (dstr != NULL) {
        dstr[0] = r0;
        dstr[1] = r1;
        dstr[2] = 0;
    }
}

void lmmp_sqrt_4_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa) {
    lmmp_param_assert(numa != NULL && dsts != NULL);
    lmmp_param_assert(numa[3] >= LIMB_B_4);
    mp_limb_t x0 = numa[0], x1 = numa[1];
    mp_limb_t rh[2], qh, ql, u, uc, t, s1, s0, r2, r1, r0, p[2], b, neg;

    // s' = sqrt([x3,x2]) >= 2^63，rh = [x3,x2] - s'^2 <= 2s'
    mp_limb_t sh = lmmp_sqrt_2_(rh, numa + 2);

    // N = rh*B + x1 除以 s'：先把顶 limb 归约到 s' 以下（至多两次），商计入学 qh
    qh = rh[1];
    rh[0] -= sh & (-(mp_limb_t)(rh[1] != 0));
    t = (rh[0] >= sh);
    rh[0] -= sh & (-(mp_limb_t)t);
    qh += t;
    ql = _udiv128by64to64_(rh[0], x1, sh, &u);

    // N div s' = qh*B + ql = 2*s_low + (ql&1)，N mod 2s' = u + (ql&1)*s'
    s0 = (ql >> 1) | (qh << (LIMB_BITS - 1));
    qh >>= 1;
    _add_ssaaaa(uc, u, 0, u, 0, sh & (-(ql & 1)));

    // r = (uc,u)*B + x0 - s_low^2，s_low = qh*B + s0 <= B（qh=1 时 s0=0）
    lmmp_mullh_(s0, s0, p);
    _sub_ddmmss(r1, r0, u, x0, p[1], p[0]);
    b = (u < p[1]) | ((u == p[1]) & (x0 < p[0]));
    r2 = uc - qh - b;

    // s = s'*B + s_low；sh = B-1 且 s_low = B 时 s1 回绕为 0，r < 0 修正时解开
    s1 = sh + qh;
    neg = -(r2 >> (LIMB_BITS - 1));
    _sub_ddmmss(s1, s0, s1, s0, 0, neg & 1);
    // r += 2s + 1：低位 (s0<<1)|1 = 2*s0+1 无进位，进位链并入 r2
    {
        mp_limb_t al = neg & ((s0 << 1) | 1);
        mp_limb_t ah = neg & ((s1 << 1) | (s0 >> (LIMB_BITS - 1)));
        __uint128_t tc;

        tc = (__uint128_t)r0 + al;
        r0 = (mp_limb_t)tc;
        tc = (__uint128_t)r1 + ah + (mp_limb_t)(tc >> 64);
        r1 = (mp_limb_t)tc;
        r2 += (neg & (s1 >> (LIMB_BITS - 1))) + (mp_limb_t)(tc >> 64);
    }

    dsts[0] = s0;
    dsts[1] = s1;
    if (dstr != NULL) {
        dstr[0] = r0;
        dstr[1] = r1;
        dstr[2] = r2;
    }
}
