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
#include "../../../include/lmmp/impl/log2_exp2.h"
#include "../../../include/lmmp/impl/mul_hard.h"
#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"


#define CBRT_DIVIDE_MIN (0x6000000000000000ull)

static inline mp_limb_t lmmp_div3_(mp_ptr q, mp_srcptr a, mp_size_t n) {
    const mp_limb_t dn = 0xC000000000000000ull;  // 3<<62
    const mp_limb_t di = 0x5555555555555555ull;  // lmmp_inv_1_(3<<62)
    mp_limb_t ah, al, qq, r;
    ah = a[n - 1] >> 2;
    al = (a[n - 1] << 62) | (a[n - 2] >> 2);
    _udiv_qrnnd_preinv(qq, r, ah, al, dn, di);
    q[n - 1] = qq;
    for (mp_size_t i = n - 2; i > 0; i--) {
        ah = r;
        al = (a[i] << 62) | (a[i - 1] >> 2);
        _udiv_qrnnd_preinv(qq, r, ah, al, dn, di);
        q[i] = qq;
    }
    ah = r;
    al = a[0] << 62;
    _udiv_qrnnd_preinv(qq, r, ah, al, dn, di);
    q[0] = qq;
    return r >> 62;
}

static inline void lmmp_cube_3_(mp_ptr restrict dst, mp_limb_t a) {
    mp_limb_t t[2];
    lmmp_mullh_(a, a, t);
    lmmp_mullh_(t[0], a, dst);
    lmmp_mullh_(t[1], a, t);
    dst[1] += t[0];
    dst[2] = t[1] + (dst[1] < t[0] ? 1 : 0);
}

mp_limb_t lmmp_cbrt_3_(mp_limb_t a0, mp_limb_t a1, mp_limb_t a2) {
    lmmp_param_assert(a1 > 0 || a2 > 0);
    mp_limb_t x[2];
    /* exact high 65 bits */
    mp_limb_t a_hi;
    mp_bitcnt_t bits;
    if (a2 == 0) {
        mp_bitcnt_t a1_bits = lmmp_limb_bits_(a1);
        bits = LIMB_BITS + a1_bits;
        a1_bits--;
        if (a1_bits == 0)
            a_hi = a0;
        else
            a_hi = (a1 << (LIMB_BITS - a1_bits)) | (a0 >> a1_bits);
    } else {
        mp_bitcnt_t a2_bits = lmmp_limb_bits_(a2);
        bits = LIMB_BITS * 2 + a2_bits;
        a2_bits--;
        if (a2_bits == 0)
            a_hi = a1;
        else
            a_hi = (a2 << (LIMB_BITS - a2_bits)) | (a1 >> a2_bits);
    }
    lmmp_debug_assert(bits >= 65);

    x[1] = bits - 1;
    x[0] = log2_fixed_64(a_hi);

    mp_limb_t rem = lmmp_div3_(x, x, 2);
    if (2 * rem >= 3) // round
        lmmp_inc(x);

    mp_bitcnt_t shift = x[1];
    x[0] = exp2_fixed_64(x[0]);

    lmmp_debug_assert(shift <= 64);
    mp_limb_t r;
    if (shift == 64)
        r = LIMB_MAX;
    else
        r = (x[0] >> (64 - shift)) | (1ULL << shift);

    // log2/exp2 固定精度近似存在 +-2 ulp 的误差，用单调立方比较修正
    // 到精确 floor：先降后升，两个循环均在真值处终止。
    mp_limb_t t[3], a[3] = {a0, a1, a2};
    lmmp_cube_3_(t, r);
    while (lmmp_cmp_(t, a, 3) > 0) {
        --r;
        lmmp_cube_3_(t, r);
    }
    while (r < LIMB_MAX) {
        lmmp_cube_3_(t, r + 1);
        if (lmmp_cmp_(t, a, 3) > 0) break;
        ++r;
    }
    return r;
}

/*
    cbrt_6 所需的辅助：移位取位、立方与修正。

        bits = bitlen(A) in [193,384]
        y    = log2(A) = (bits-1) + log2(1+f)，f 取 A 的最高 129bit 中
               去掉前导 1 后的 128bit 小数（log2_fixed_128）
        r    = floor(2^(y/3)) = 2^s + (exp2_fixed_128(y/3 的小数) >> (128-s))

    估计误差主要来自 log2/exp2 各 +-2 ulp（128bit）与除 3 舍入，理论
    上界约 +-8，典型 <= 2；随后以单调立方比较修正到精确 floor。
*/

// (p >> s) 的低 64bit，要求 0 <= s < 64*n
static inline mp_limb_t lmmp_shr64_(mp_srcptr p, mp_size_t n, uint64_t s) {
    mp_size_t w = (mp_size_t)(s >> 6);
    uint64_t b = s & 63;
    mp_limb_t lo = p[w];
    mp_limb_t hi = (w + 1 < n) ? p[w + 1] : 0;
    return b ? ((lo >> b) | (hi << (64 - b))) : lo;
}

/**
 * @brief 计算 [t,6]=[r,2]^3
 * @note [t+6,4]=[r,2]^2
 */
static inline void lmmp_cube_6_(mp_ptr restrict t, mp_srcptr restrict r) {
    lmmp_sqr_hard_2_(t + 6, r);
    lmmp_mul_basecase_(t, t + 6, 4, r, 2);
}

// [r,2] = floor(cbrt([numa,n])) 的估计值，3 < n <= 6，numa[n-1] != 0
static void lmmp_cbrt_6_est_(mp_ptr r, mp_srcptr numa, mp_size_t n) {
    mp_bitcnt_t hb = lmmp_limb_bits_(numa[n - 1]);
    uint64_t bits = LIMB_BITS * (n - 1) + hb;  // in [193,384]
    uint64_t s0 = bits - 129;                  // 最高 129bit 的起始位

    mp_limb_t x[3];
    log2_fixed_128(x, lmmp_shr64_(numa, n, s0 + 64), lmmp_shr64_(numa, n, s0));
    x[2] = bits - 1;

    mp_limb_t rem = lmmp_div3_(x, x, 3);
    if (2 * rem >= 3) // round
        lmmp_inc(x);

    uint64_t s = x[2];  // in [64,128]
    mp_limb_t e[2];
    exp2_fixed_128(e, x[1], x[0]);

    if (s >= 128) {
        // 舍入进位到 2^128，钳位后交给修正循环
        r[0] = r[1] = LIMB_MAX;
        return;
    }
    // r = 2^s + (e >> (128-s))，总 < 2^(s+1) <= 2^128 不溢出
    mp_limb_t d = 128 - s;  // in [1,64]
    if (d == 64) {
        r[0] = e[1];
        r[1] = 1;
    } else {
        r[0] = (e[1] << (64 - d)) | (e[0] >> d);
        r[1] = (e[1] >> d) | (1ULL << (s - 64));
    }
}

/*
    基于估计值 [r,2] 做单调立方比较修正至精确 floor(cbrt([a,6]))，
    返回时 t[0..5] = r^3（供余数计算复用）。

    下降循环至多降到 r = B（A > B^3 保证 r >= B），lmmp_dec 不会越过
    region 下溢；上升循环在 r = B^2-1 处封顶（A < B^6）。
*/
static void lmmp_cbrt_6_fix_(mp_ptr r, mp_srcptr a, mp_ptr t) {
    mp_limb_t u[2], ut[10];
    lmmp_cube_6_(t, r);
    while (lmmp_cmp_(t, a, 6) > 0) {
        lmmp_dec(r);
        lmmp_cube_6_(t, r);
    }
    if (r[0] == LIMB_MAX && r[1] == LIMB_MAX) return;
    u[0] = r[0] + 1;
    u[1] = r[1] + (u[0] == 0);
    lmmp_cube_6_(ut, u);
    while (lmmp_cmp_(ut, a, 6) <= 0) {
        r[0] = u[0];
        r[1] = u[1];
        lmmp_copy(t, ut, 6);
        if (u[0] == LIMB_MAX && u[1] == LIMB_MAX) return;  // A < B^6，不会到达
        u[0] += 1;
        u[1] += (u[0] == 0);
        lmmp_cube_6_(ut, u);
    }
}

/*
    cbrt_6 的快速除法路径：cbrt_3 种子 + 一次除法 + 修正循环。

        A   = Ah*B^3 + Al，Ahr = floor(cbrt(Ah))（cbrt_3 直接给出）
        x   = Ahr*B + Alr，Alr = [(rk*B^3+Al)/3] / Ahr^2 由 div_s 给出，
        N   = 3*x^2*Alr + R（R 为 div_s 重建余数）

    Alr 至多高估 2（含 Alr=B、B+1 情形），修正循环：

        N = 3*x^2*q + R（q 为除法商，R 为重建余数）
        A < (x+u)^3  <=>  rsav < W(u)
    其中 rsav = 3*x^2*(q-u) + R 随 u 的递减同步累加，
    W(u) = 3*Ahr*u^2*B + u^3。循环终止于 u = t，此时
        R' = rsav - W(t) = A - (x+t)^3
    恰为余数。由 Alr 高估至多 2 知循环至多约 3 轮。
*/

void lmmp_cbrt_6_fast_(mp_ptr restrict dst, mp_ptr restrict numa, mp_ptr restrict tp) {
    lmmp_param_assert(numa != NULL && dst != NULL && tp != NULL);
    lmmp_param_assert(numa[5] >= CBRT_DIVIDE_MIN);

    dst[1] = lmmp_cbrt_3_(numa[3], numa[4], numa[5]);
    lmmp_cube_3_(tp, dst[1]);
    lmmp_sub_n_(numa + 3, numa + 3, tp, 3);  // rk = Ah - Ahr^3

    lmmp_mullh_((dst + 1)[0], (dst + 1)[0], tp + 3);  // Ahr2 = [tp+3,2]
    /*
        A / (3*x^2) = A / 3 / x^2
        A % (3*x^2) = 3 * (A / 3 % x^2) + A % 3
    */
    mp_limb_t r = lmmp_div3_(numa + 2, numa + 2, 4);
    mp_limb_t qh = lmmp_div_s_(tp + 5, numa + 2, 4, tp + 3, 2);  // Alr = [tp+5,3)
    lmmp_debug_assert(qh == 0);
    numa[4] = lmmp_mul_1_(numa + 2, numa + 2, 2, 3);
    lmmp_inc_1(numa + 2, r);

    mp_limb_t rsav[6], w[6], u2[4], u3[5], x3sq[6];
    rsav[5] = 0;
    lmmp_copy(rsav, numa, 5);
    x3sq[0] = 0;
    x3sq[1] = 0;
    x3sq[5] = 0;
    lmmp_mullh_((dst + 1)[0], (dst + 1)[0], u2); // u2 = Ahr^2
    x3sq[4] = lmmp_mul_1_(x3sq + 2, u2, 2, 3);   // 3*x^2 = 3*Ahr^2*B^2 占 [2,5)
    for (;;) {
        w[0] = 0;
        lmmp_sqr_hard_2_(u2, tp + 5);                   // u^2
        w[4] = lmmp_mul_1_(w + 1, u2, 3, (dst + 1)[0]); // Ahr*u^2 占 [1,5)
        w[5] = lmmp_mul_1_(w + 1, w + 1, 4, 3);         // 3*Ahr*u^2*B
        lmmp_mul_basecase_(u3, u2, 3, tp + 5, 2);       // u^3
        w[5] += lmmp_add_n_(w, w, u3, 5);               // W < 4*B^5，w[5] <= 4 不溢出
        if (lmmp_sub_(numa, rsav, 6, w, 6) == 0)
            break;
        lmmp_dec(tp + 5);
        mp_limb_t ca = lmmp_add_n_(rsav, rsav, x3sq, 6);  // rsav < 12*B^4，无进位
        lmmp_debug_assert(ca == 0);
    }
    dst[0] = tp[5];
}

void lmmp_cbrt_6_(mp_ptr dst, mp_srcptr numa, mp_size_t na) {
    lmmp_param_assert(na > 3 && na <= 6);
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_param_assert(numa[na - 1] != 0);
    mp_limb_t a[6] = {0, 0, 0, 0, 0, 0};
    for (mp_size_t i = 0; i < na; i++) a[i] = numa[i];
    if (na == 6 && a[5] >= CBRT_DIVIDE_MIN) {
        // cbrt_3 种子 + 除法修正：比 log2/exp2 估计更快（条件与
        // cbrt_divide_ 相同，保证 div_s 的 MSB 归一化要求）
        mp_limb_t tp[10];
        lmmp_cbrt_6_fast_(dst, a, tp);
        return;
    }
    mp_limb_t t[10];
    lmmp_cbrt_6_est_(dst, numa, na);
    lmmp_cbrt_6_fix_(dst, a, t);
}
