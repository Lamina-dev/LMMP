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

#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/log2_exp2.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/mul_hard.h"
#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"


// divide 路径（cbrt_divide_/cbrt6_fast_）的顶 limb 归一化下限：
// 保证 cbrt(A)^2 >= B^2/2，实际最小值约 0x5A827999FCEF3242，取整为 3B/8。
// newton 路径（invcbrt_newton_/cbrt_newton_）迭代收敛仅需顶 limb >= B/8
// （LIMB_B_8，层误差界按 B^nr/8 论证），其基例在 [B/8,3B/8) 自动回退
// 到 log2/exp2 估计路径
#define CBRT_DIVIDE_MIN (0x6000000000000000ull)

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

    mp_limb_t rem = lmmp_div_1_(x, x, 2, 3);
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
static void lmmp_cbrt6_est_(mp_ptr r, mp_srcptr numa, mp_size_t n) {
    mp_bitcnt_t hb = lmmp_limb_bits_(numa[n - 1]);
    uint64_t bits = LIMB_BITS * (n - 1) + hb;  // in [193,384]
    uint64_t s0 = bits - 129;                  // 最高 129bit 的起始位

    mp_limb_t x[3];
    log2_fixed_128(x, lmmp_shr64_(numa, n, s0 + 64), lmmp_shr64_(numa, n, s0));
    x[2] = bits - 1;

    mp_limb_t rem = lmmp_div_1_(x, x, 3, 3);
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
static void lmmp_cbrt6_fix_(mp_ptr r, mp_srcptr a, mp_ptr t) {
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

    与下方 lmmp_cbrt_divide_ 的推导同构（lo=hi=1 特例），即原 ns==2
    分支的去递归化移植：

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

/**
 * @param dst 结果 [dst,2]
 * @param numa 被开方数 [numa,6]（[numa,5] 存余数）
 * @param tp 临时区（至少 8 limb）
 * @warning numa[5]>=CBRT_DIVIDE_MIN, sep(dst,numa,tp)
*/
static void lmmp_cbrt6_fast_(mp_ptr restrict dst, mp_ptr restrict numa, mp_ptr restrict tp) {
    dst[1] = lmmp_cbrt_3_(numa[3], numa[4], numa[5]);
    lmmp_cube_3_(tp, dst[1]);
    lmmp_sub_n_(numa + 3, numa + 3, tp, 3);  // rk = Ah - Ahr^3

    lmmp_mullh_((dst + 1)[0], (dst + 1)[0], tp + 3);  // Ahr2 = [tp+3,2]
    /*
        A / (3*x^2) = A / 3 / x^2
        A % (3*x^2) = 3 * (A / 3 % x^2) + A % 3
    */
    mp_limb_t r = lmmp_div_1_(numa + 2, numa + 2, 4, 3);
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
        lmmp_cbrt6_fast_(dst, a, tp);
        return;
    }
    mp_limb_t t[10];
    lmmp_cbrt6_est_(dst, numa, na);
    lmmp_cbrt6_fix_(dst, a, t);
}

/*
        A     = Ah * B^(3*lo) + Al

        Ahr   = floor(Ah^(1/3))
        rk    = Ah - Ahr^3
        x_k   = Ahr * B^lo

        x_k+1 = (2*x_k + A / x_k^2 ) / 3
              = x_k + (A / x_k^2 - x_k) / 3
              = x_k + (A - x_k^3) / 3 * x_k^2
              = Ahr * B^lo + (rk * B^(3*lo) + Al) / 3 * x_k^2
              = Ahr * B^lo + Alr

        let  Alr = (rk * B^(3*lo) + Al) / 3 * x_k^2, R = (rk * B^(3*lo) + Al) mod 3 * x_k^2
        such that  (rk * B^(3*lo) + Al) = Alr * 3 * x_k^2 + R
                                          ┌───────────────────────────────────────────────────────────────────────┐
                                        = |Alr * 3 * Ahr^2*B^(2*lo) + R = R_correct + (Alr-1) * 3 * Ahr^2*B^(2*lo)|
                                          └───────────────────────────────────────────────────────────────┬───────┘
        r_k+1 = A - x_k+1^3                                                                               |
              = Ah*B^(3*lo) + Al - Ahr^3*B^(3*lo) - 3*Alr*Ahr^2*B^(2*lo) - 3*Ahr*Alr^2*B^lo - Alr^3       |
              = r_k * B^(3*lo) + Al - 3*Alr*Ahr^2*B^(2*lo) - 3*Ahr*Alr^2*B^lo - Alr^3                     |
              = R - 3*Ahr*Alr^2*B^lo - Alr^3                                                              |
                                                                                                          |
        Alr is either correct or 1 too big. We can prove this when hi >= lo+1.                            |
                                                                                                          |
                                                       ┌────────────────────────────────────────────────┐ |
        r_k+1 = R - 3*Ahr*(Alr-1)^2*B^lo - (Alr-1)^3 + | 3*Alr*Ahr^2*B^(2*lo) - 3*(Alr-1)*Ahr^2*B^(2*lo)├─┘
              = R - 3*Ahr*Alr^2*B^lo - Alr^3           └────────────────────────────────────────────────┘
(adjust)      + 3*Ahr^2*B^(2*lo) + 6*Ahr*Alr*B^lo - 3*Ahr*B^lo + 3*Alr^2 - 3*Alr + 1
*/

/*
    分割需满足 hi >= lo+1，理由：
    记 x = Ahr*B^lo，真值 S = x+t，r = A-S^3 <= 3*S^2+3*S，则 Alr 高估 2
    当且仅当 3*x*t^2 + t^3 + r >= 6*x^2，而 (t < B^lo)
        3*x*t^2 + t^3 + r < (9*Ahr+1)*B^(3*lo) + 3*Ahr^2*B^(2*lo) + 3*B^(2*lo) + 低阶
    由 CBRT_DIVIDE_MIN 有 Ahr >= 0.84*B^hi，故 hi >= lo+1 时
        3*Ahr^2 > (9*Ahr+1)*B^lo
    上式 < 6*x^2，即 Alr 至多高估 1，下方的单次修正才是充分的。

    ns=2 在函数入口直接走 lmmp_cbrt6_fast_（_6_ 的去递归化快速路径），
    其余 ns>=3 时 lo = (ns-1)/2 >= 1 且 hi = ns-lo >= lo+1 恒成立。
*/

void lmmp_cbrt_divide_(mp_ptr restrict dst, mp_ptr restrict numa, mp_size_t ns, mp_ptr restrict tp, int calr) {
    lmmp_param_assert(ns > 0);
    lmmp_param_assert(numa != NULL && dst != NULL && tp != NULL);
    lmmp_param_assert(numa[3 * ns - 1] >= CBRT_DIVIDE_MIN);
    if (ns == 1) {
        dst[0] = lmmp_cbrt_3_(numa[0], numa[1], numa[2]);
        if (calr) {
            lmmp_cube_3_(tp, dst[0]);
            lmmp_sub_n_(numa, numa, tp, 3);
        }
    } else if (ns == 2) {
        lmmp_cbrt6_fast_(dst, numa, tp);
    } else {
        mp_size_t lo = (ns - 1) / 2, hi = ns - lo;
#define Ahr     (dst + lo)             // [dst+lo,              hi]
#define rk      (numa + 3 * lo)        // [numa+3*lo,       2*hi+1]
#define R       (numa)                 // [numa,            2*ns+1]
#define Ahr2    (tp)                   // [tp,                2*hi]
#define Alr     (tp + 2 * hi)          // [tp + 2*hi,         lo+1]
#define Alr2    (tp + 2 * hi + lo)     // [tp + 2*hi+lo,      2*lo]
#define scratch (tp + 2 * hi + 3 * lo) // [tp + 2*hi+3*lo, hi+2*lo]

        lmmp_cbrt_divide_(Ahr, rk, hi, tp, 1);

        lmmp_sqr_(Ahr2, Ahr, hi);

        /*
            A / (3*x^2) = A / 3 / x^2
            A % (3*x^2) = 3 * (A / 3 % x^2) + A % 3
        */
        mp_limb_t r = lmmp_div_1_(rk - lo, rk - lo, hi + 1 + ns, 3);
        mp_limb_t qh = lmmp_div_s_(Alr, rk - lo, hi + 1 + ns, Ahr2, 2 * hi);
        lmmp_debug_assert(qh == 0);
        (rk - lo)[2 * hi] = lmmp_mul_1_(rk - lo, rk - lo, 2 * hi, 3);
        lmmp_inc_1(rk - lo, r);
        /*
            我们根据 cbrt(A/B^3) == floor(cbrt(A)/B) 可以知道，如果Alr正确结果
            必定被限制在B^lo以内，其至多高估1，因此Alr的最高位必定为0或1，而为1时，
            即代表此时结果已经高估。
        */
        mp_limb_t adj = Alr[lo];
        lmmp_debug_assert(adj == 0 || adj == 1);
        if (adj > 0) {
            // Alr[0] 仅可能为0，此时高估1
            lmmp_debug_assert(Alr[0] == 0);
            lmmp_fill_n(dst, lo, LIMB_MAX);
            if (calr == 0) return;
            /*
            x_k+1 = Ahr * B^lo + Alr
                  = Ahr * B^lo + B^lo - 1

            r_k+1 = R - 3*Ahr*(B^lo-1)^2*B^lo - (B^lo-1)^3 + 3*Alr*Ahr^2*B^(2*lo) - 3*(Alr-adj)*Ahr^2*B^(2*lo)
                  = R - 3*Ahr*B^(3*lo) + 6*Ahr*B^(2*lo) - 3*Ahr*B^lo + 3*adj*Ahr^2*B^(2*lo)
                    - B^(3*lo) + 3*B^(2*lo) - 3*B^lo + 1
            */

            // - 3*Ahr*B^(3*lo)
            mp_limb_t cy = lmmp_submul_1_(R + 3 * lo, Ahr, hi, 3);
            r = lmmp_sub_1_(R + 2 * lo + ns, R + 2 * lo + ns, ns + 1 - 2 * lo, cy);

            // + 6*Ahr*B^(2*lo)
            cy = lmmp_addmul_1_(R + 2 * lo, Ahr, hi, 6);
            r -= lmmp_add_1_(R + ns + lo, R + ns + lo, hi + 1, cy);

            // - 3*Ahr*B^lo
            cy = lmmp_submul_1_(R + lo, Ahr, hi, 3);
            r += lmmp_sub_1_(R + ns, R + ns, ns + 1, cy);

            // + 3*adj*Ahr^2*B^(2*lo)
            cy = lmmp_addmul_1_(R + 2 * lo, Ahr2, 2 * hi, 3);
            (R + 2 * ns)[0] += cy;
            r -= (R + 2 * ns)[0] < cy;

            // - B^(3*lo)
            r -= lmmp_sub_1_(R + 3 * lo, R + 3 * lo, 2 * ns + 1 - 3 * lo, 1);

            // + 3*B^(2*lo)
            r -= lmmp_add_1_(R + 2 * lo, R + 2 * lo, 2 * hi + 1, 3);

            // - 3*B^lo
            r += lmmp_sub_1_(R + lo, R + lo, 2 * ns + 1 - lo, 3);

            // + 1
            r -= lmmp_add_1_(R, R, 2 * ns + 1, 1);

            lmmp_debug_assert(r == 0);
        } else if (calr == 0) {
            /*
                calr=0 时无需维护余数，计算 Alr^3 与 3*Ahr*Alr^2*B^lo 再相减，
                仅仅是为了判定 Alr 是否高估 1（即 R 与 W = 3*Ahr*Alr^2*B^lo + Alr^3
                的大小），此判定无需完整平方与乘法：

                    Ahr = X*B^(hi-2) + Y,  X = [Ahr+hi-2,2]
                    Alr = H*B^(lo-2) + L,  H = [Alr+lo-2,2]

                    W = 3*Ahr*Alr^2*B^lo + Alr^3
                      = 3*X*H^2*B^(hi+3*lo-6) + E
                    E = 3*B^lo*(X*2*H*L*B^(hi+lo-4) + Y*H^2*B^(2*lo-4) + 低阶项) + Alr^3
                      < 10*B^(hi+3*lo-2) + B^(3*lo) <= 11*B^(hi+3*lo-2)    (hi>=2)

                即 W 在 B^(hi+3*lo-2) 尺度上的最高三个 limb 必落在
                [3*X*H^2 的最高三 limb, +11] 内，于是仅需比较 R 与 3*X*H^2
                的最高三个 limb（注意 adj=0 时 Alr < B^lo，W < 4*B^(hi+3*lo)）：
                    R 高于 B^(hi+3*lo) 的 limb 非零              =>  R > W
                    [R] > [3*X*H^2]+12（最高三 limb 意义下）      =>  R > W
                    [R] < [3*X*H^2]    （最高三 limb 意义下）     =>  R < W
                其余情况（最高三 limb 落入宽度 11 的窄带，如完全立方数
                等构造输入）回退到下方的精确路径，保证正确性。
            */
            if (lo >= 4) {
                mp_size_t i = 2 * ns;
                while (i > hi + 3 * lo && R[i] == 0) --i;
                if (i > hi + 3 * lo) {
                    // R >= B^(hi+3*lo+1) > W，Alr 未高估
                    lmmp_copy(dst, Alr, lo);
                    return;
                }
                lmmp_sqr_hard_2_(Alr2, Alr + lo - 2);                   // [Alr2,4] = H^2
                lmmp_mul_basecase_(scratch, Alr2, 4, Ahr + hi - 2, 2);  // [scratch,6] = X*H^2
                mp_limb_t qh2 = lmmp_mul_1_(scratch, scratch, 6, 3);    // [qh:scratch,6] = 3*X*H^2
                mp_limb_t qm = scratch[5], ql = scratch[4];
                mp_limb_t rh = R[hi + 3 * lo], rm = R[hi + 3 * lo - 1], rl = R[hi + 3 * lo - 2];
                // [th,tm,tl] = [3*X*H^2 的最高三 limb] + 12
                mp_limb_t tl = ql + 12, c1 = tl < 12;
                mp_limb_t tm = qm + c1, c2 = tm < c1;
                mp_limb_t th = qh2 + c2;
                if (rh > th || (rh == th && (rm > tm || (rm == tm && rl >= tl)))) {
                    // [R] > [3*X*H^2]+12
                    lmmp_copy(dst, Alr, lo);
                    return;
                }
                if (rh < qh2 || (rh == qh2 && (rm < qm || (rm == qm && rl < ql)))) {
                    // R < W，Alr 高估 1
                    lmmp_dec(Alr);
                    lmmp_copy(dst, Alr, lo);
                    return;
                }
            }
            // 窄带或小尺寸回退：精确判定
            lmmp_sqr_(Alr2, Alr, lo);
            lmmp_mul_(scratch, Alr2, 2 * lo, Alr, lo);
            r = lmmp_sub_(R, R, 2 * ns + 1, scratch, 3 * lo);

            if (2 * lo >= hi)
                lmmp_mul_(scratch, Alr2, 2 * lo, Ahr, hi);
            else
                lmmp_mul_(scratch, Ahr, hi, Alr2, 2 * lo);
            mp_limb_t b = lmmp_submul_1_(R + lo, scratch, 2 * lo + hi, 3);
            r += lmmp_sub_1_(R + 2 * lo + ns, R + 2 * lo + ns, ns + 1 - 2 * lo, b);
            if (r > 0)
                lmmp_dec(Alr);
            lmmp_copy(dst, Alr, lo);
        } else {
            lmmp_sqr_(Alr2, Alr, lo);
            lmmp_mul_(scratch, Alr2, 2 * lo, Alr, lo);
            r = lmmp_sub_(R, R, 2 * ns + 1, scratch, 3 * lo);

            if (2 * lo >= hi)
                lmmp_mul_(scratch, Alr2, 2 * lo, Ahr, hi);
            else
                lmmp_mul_(scratch, Ahr, hi, Alr2, 2 * lo);
            mp_limb_t b = lmmp_submul_1_(R + lo, scratch, 2 * lo + hi, 3);
            r += lmmp_sub_1_(R + 2 * lo + ns, R + 2 * lo + ns, ns + 1 - 2 * lo, b);

            if (r > 0) {
                // + 3*Alr^2
                mp_limb_t cy = lmmp_addmul_1_(R, Alr2, 2 * lo, 3);
                r -= lmmp_add_1_(R + 2 * lo, R + 2 * lo, 2 * hi + 1, cy);

                // - 3*Alr
                cy = lmmp_submul_1_(R, Alr, lo, 3);
                r += lmmp_sub_1_(R + lo, R + lo, 2 * ns + 1 - lo, cy);

                // + 6*Ahr*Alr*B^lo
                lmmp_mul_(scratch, Ahr, hi, Alr, lo);
                cy = lmmp_addmul_1_(R + lo, scratch, ns, 6);
                r -= lmmp_add_1_(R + ns + lo, R + ns + lo, ns + 1 - lo, cy);

                // + 3*Ahr^2*B^(2*lo)
                cy = lmmp_addmul_1_(R + 2 * lo, Ahr2, 2 * hi, 3);
                // r -= lmmp_add_1_(R + 2 * ns, R + 2 * ns, 1, cy);
                (R + 2 * ns)[0] += cy;
                r -= (R + 2 * ns)[0] < cy;

                // + 1
                r -= lmmp_add_1_(R, R, 2 * ns + 1, 1);

                // - 3*Ahr*B^lo
                cy = lmmp_submul_1_(R + lo, Ahr, hi, 3);
                r += lmmp_sub_1_(R + ns, R + ns, ns + 1, cy);

                lmmp_debug_assert(r == 0);
                lmmp_dec(Alr);
            }
            lmmp_copy(dst, Alr, lo);
        }
    }
#undef Ahr
#undef rk
#undef R
#undef Ahr2
#undef Alr
#undef Alr2
#undef scratch
}

static inline mp_size_t lmmp_cbrt_cls_shift_(mp_size_t bl, int top2) {
    mp_size_t r = bl % 3;
    if (r == 1) return -1;
    if (r == 2 && !top2) return -1;
    mp_size_t L = (r == 0) ? (bl + 63) / 64 : (bl + 64) / 64;
    L = (L + 2) / 3 * 3;
    return (r == 0 ? 64 * L : 63 + 64 * (L - 1)) - bl;
}

// a2 = [src,n] << s（s < 192：置零 w = s/64 个整 limb 后按 b = s%64 移位，
// 移位进位写入 a2[w+n]，缓冲至少 n+3 limb）。knorm 的 k=1/k^3 路径与
// snorm 的纯移位路径共用
static inline void lmmp_cbrt_shapply_(mp_srcptr src, mp_size_t n, mp_size_t s, mp_ptr a2) {
    mp_size_t w = s / LIMB_BITS, b = s % LIMB_BITS;
    lmmp_zero(a2, w);
    if (b)
        a2[w + n] = lmmp_shl_(a2 + w, src, n, b);
    else
        lmmp_copy(a2 + w, src, n);
}

/*
    knorm：精确路径（lmmp_cbrt_ 的 cbrt_divide_ 分支）专用归一化入口。
    divide 要求顶 limb >= 3B/8（CBRT_DIVIDE_MIN，保证 Alr 至多高估 1 的
    论证与 div_s 归一化），策略为优先移位、移位不可达时才乘小立方数：
    k=1 即纯 3 对齐移位路径（恒最先尝试），仅当移位无法到达 3B/8 时才
    以 k^3 乘子改变 bl 类别后重试。newton 路径不使用本函数（其收敛仅需
    B/8，移位恒可达，见 lmmp_cbrt_snorm_）。

    输出 a2 = numa*k^3 << s 写入调用方缓冲区（至少 na+5 limb），满足
    limbs(a2) ≡ 0 (mod 3) 且 a2 顶 limb >= 3B/8，同时返回：
        *kp  = 乘子 k（k=1 表示纯移位，无需乘子）
        *na2p = a2 的 limb 数
        返回值 = t = s/3（根的右移回退量，< 64）

    约 1/2 输入（bl ≡ 1 (mod 3)，或 bl ≡ 2 且顶 2bit 为 10）经 3 对齐移位
    无法到达 3B/8，此时乘小立方数 k^3 改变 bl 类别后重试。2 的幂乘子
    （k = 2,4,8,...）等价于 3 对齐移位（k^3 = 2^(3j)），不改变类别，故跳过。
    根的还原基于恒等式（K = k*2^t，x 为实值根）：

        floor(floor(x*K) / K) == floor(x)          （floor 复合不变性）

    即根结果 >>t 后一次 div_1(k) 即精确还原。

    候选筛选不计算完整乘积：记 q = 高128bit(numa)*k^3（152bit 小乘），
    X = numa*k^3 = q*B^sh + low（low < B^sh，sh = 64*(na-2)，na<=1 时 X=q），
    则 X 的类别完全由 q 决定且无歧义边界：
        q ∈ [2^(blq-1), 2^blq)  =>  X ∈ [2^(sh+blq-1), 2^(sh+blq))，bl(X) 精确
        3*2^(bl(X)-2) = qT*B^sh（qT = 3*2^(blq-2)）为阈值形式，
        X >= 3*2^(bl(X)-2)  <=>  q >= qT，low 不影响判定
    故 miss 的 k 仅花费 2 次 64x24bit 小乘即可排除，完整 mul_1 仅对最终
    命中的 k 计算一次。k 的类别近似均匀，每 k 命中概率约 1/2，搜索上限
    256 内未命中概率 < 2^-100（实际期望 2 次内命中）。
*/
static mp_size_t lmmp_cbrt_knorm_(mp_srcptr numa, mp_size_t na, mp_ptr a2, mp_size_t *na2p,
                                   mp_limb_t *kp) {
    lmmp_param_assert(numa != NULL && a2 != NULL && na > 0 && numa[na - 1] != 0);
    TEMP_DECL;
    mp_ptr xk = NULL;  // numa*k^3 暂存（仅命中 k>1 时分配一次，与 a2 分离）
    mp_limb_t hlo = na >= 2 ? numa[na - 2] : 0;
    for (mp_limb_t k = 1; k <= 256; ++k) {
        if (k > 1 && (k & (k - 1)) == 0) continue;  // 2 的幂不改变类别
        mp_limb_t k3 = k * k * k;  // <= 256^3 < 2^24

        // 预筛选：na>=2 时 q = 高128bit(numa)*k3（低位 low < B^(64*(na-2))）；
        // na==1 时 hi128 即 numa[0] 本身（无 B 因子），q = X 精确
        mp_limb_t t0[2], t1[2];
        lmmp_mullh_(hlo, k3, t0);
        lmmp_mullh_(numa[na - 1], k3, t1);
        mp_limb_t q0, q1, q2;
        mp_size_t sh;
        if (na == 1) {
            q0 = t1[0];
            q1 = t1[1];
            q2 = 0;
            sh = 0;
        } else {
            q0 = t0[0];
            q1 = t0[1] + t1[0];
            q2 = t1[1] + (q1 < t0[1]);
            sh = LIMB_BITS * (na - 2);
        }

        mp_size_t wq = q2 ? 2 : (q1 ? 1 : 0);
        mp_limb_t qtop = wq == 2 ? q2 : (wq == 1 ? q1 : q0);
        mp_size_t hb = lmmp_limb_bits_(qtop);
        mp_size_t bl = sh + LIMB_BITS * wq + hb;  // bl(numa*k^3)，精确
        int top2 = hb >= 2 ? qtop >= 3ULL << (hb - 2)
                           : (qtop & 1) && ((wq == 2 ? q1 : q0) >> (LIMB_BITS - 1));
        mp_size_t s = lmmp_cbrt_cls_shift_(bl, top2);
        if (s == (mp_size_t)-1) continue;  // mp_size_t 无符号，须按哨兵值比较

        // 命中：唯一一次完整 mul_1
        mp_srcptr ak;
        mp_size_t nak;
        if (k == 1) {
            ak = numa;
            nak = na;
        } else {
            if (!xk) xk = TALLOC_TYPE(na + 1, mp_limb_t);
            mp_limb_t cy = lmmp_mul_1_(xk, numa, na, k3);
            nak = na;
            if (cy) xk[na] = cy, ++nak;
            ak = xk;
        }

        // a2 = ak << s（k=1 时 ak == numa；k>1 时 ak == xk 与 a2 分离）；na2 <= w+nak+1
        mp_size_t na2 = (bl + s + LIMB_BITS - 1) / LIMB_BITS;
        lmmp_cbrt_shapply_(ak, nak, s, a2);
        lmmp_debug_assert(na2 % 3 == 0 && a2[na2 - 1] >= CBRT_DIVIDE_MIN);
        *kp = k;
        *na2p = na2;
        TEMP_FREE;
        return s / 3;
    }
    TEMP_FREE;
    lmmp_param_assert(0);  // 理论不可达（< 2^-100）
    return 0;
}

/*
    逆立方根牛顿迭代（结构与 lmmp_invsqrt_newton_ 同构，收敛率同为二次）：

        y' = y*(4 - a*y^3)/3

    尺寸栈 ns -> (ns>>1)+1 -> ... -> 3，自 nr=2 的基例起逐层将精度翻倍
    （下一层尺寸 na 满足 na ∈ {2*nr-2, 2*nr-1}，牛顿步一轮即可）。
    当前层近似记（an = a 的最高 naz = min(na,namax) 个 limb）

        ir = [dstis-nr, nr+1] = floor(B^(4*nr/3) / cbrt(ar)) - [0|1]
        ar = an 的最高 nr 个 limb

    层不变式仅要求 ar 顶 limb >= B/8（入口 B/8 归一化逐层保持：每层 ar
    均含 a 顶 limb）：由此 ir ∈ [B^nr, 2*B^nr)（dstis[nr]=1），下方
    d/xp/dip 的窗口宽度界均按此论证。

    残差与修正（对相对残差 e = 1 - an*ir^3/B^(naz+3*nr) 二次收敛 e' ~ 2*e^2）：

        d  = B^(naz+3*nr) - an*ir^3,     -16*B^(naz+2*nr) < d < 16*B^(naz+2*nr)

        x' = ir*B^(na-nr)
        t  = an*x'^3/B^(3*na+naz) = 1 - d/B^(naz+3*nr)
        i' = x'*(4-t)/3 = x' + x'*d/(3*B^(naz+3*nr))
                        = ir*B^(na-nr) + ir*d / (3*B^(naz+4*nr-na))

        xp  = |d| / (3*B^naz)          (取 2*nr+2 limb 窗口)
        dip = xp * ir                   (3*nr+3 limb 精确乘积)
        C   = dip * B^(na-4*nr)         (修正项, ~B^(na-nr))

        i' 的低位 [dstis-na, dstis-nr) <- dip[4*nr-na, 3*nr)，
        边界 limb dip[3*nr] 经 inc_1/dec_1 并入 [dstis-nr] 处。

    梅森变换（mod B^mn-1, mn = lmmp_fft_next_size_(nres), nres = naz+2*nr+2）：
        ir^3*an 高于 B^mn 的部分对 i' 无影响（|d| < 16*B^(naz+2*nr) << B^mn，
        残差信息全部保存在模的低 nres 个 limb 内），故 an*ir^3 的三次乘链
        （ir^2 -> *an -> *ir）均可用模乘截断计算。dp 折叠回 -d (mod B^mn-1)
        后，d 的符号由 dp[nres-1] 是否 > 15 判定：
            d > 0  =>  dp = M-d   (M = B^mn-1)，高位 limb 接近 B-1
            d <= 0 =>  dp = |d|   (< 16*B^(naz+2*nr)，高位 limb <= 15)
        d > 0 时 dp 按位取反即得 d 的窗口 limb（d <= 0 时窗口直接可得），
        配合 div_1 除 3 与边界 +/-1 修正吸收全部截断误差，保证 ir 绝不高估。

    与 sqrt 的差异：修正系数为 1/3 而非 1/2（除 3 不可移位实现，用 div_1_），
    残差链多一跳乘法（三次乘：ir^2、*an、*ir），xp 窗口宽度倍增为 2*nr+2，
    dip/dip 边界 limb 下移至 3*nr（C = dip*B^(na-4*nr)）。
*/

void lmmp_invcbrt_newton_(mp_ptr restrict dstis, mp_size_t ns, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(ns >= 3);
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa != NULL && dstis != NULL);
    lmmp_param_assert(numa[na - 1] >= LIMB_B_8);
    mp_size_t nr = ns, namax = na, mn;
    mp_size_t sizes[LIMB_BITS], *sizp = sizes;

    do {
        *sizp = nr;
        nr = (nr >> 1) + 1;
        ++sizp;
    } while (nr > 2);

    numa += na;
    dstis += ns;

    /*
        nr=2 基例（x = a 的最高 2 个 limb 组成的 2 limb 数, 契约保证 x >= B^2/8）：

            s  = floor(cbrt(x*B^4))              in [B^2/2, B^2)
            i2 = floor((B^4-1)/(s+1)),  i2^3*x ≈ B^8, i2 ∈ (B^2, 2*B^2]

        分母 +1 使基例系统性略微低估（i2 < cbrt(B^8/x) 恒成立），
        保证后续每层迭代恒有 ir <= 目标值。cbrt_6_ 按顶 limb 分流：
        >= 3B/8 直达除法快速路径，[B/8, 3B/8) 回退 log2/exp2 估计 +
        单调立方修正（仅 6 limb，代价可忽略）。
    */
    mp_limb_t numa2[6], sval[2];
    lmmp_zero(numa2, 4);
    numa2[4] = na > 1 ? numa[-2] : 0;
    numa2[5] = numa[-1];
    lmmp_cbrt_6_(sval, numa2, 6);

    if (sval[0] == LIMB_MAX && sval[1] == LIMB_MAX) {
        // s = B^2-1，s+1 = B^2 无法作为 div_s 的归一化除数，直接给出
        dstis[-2] = LIMB_MAX;
        dstis[-1] = LIMB_MAX;
        dstis[0] = 0;
    } else {
        mp_limb_t q[4] = {LIMB_MAX, LIMB_MAX, LIMB_MAX, LIMB_MAX};
        lmmp_inc(sval);  // s+1，最高 limb >= B/2，满足 div_s 归一化要求
        // 商 i2 ∈ (B^2, 2*B^2) 共 3 limb：[dstis-2, dstis-1] 为主干，
        // 返回的 qh 即最高 limb，写入 dstis[0]
        dstis[0] = lmmp_div_s_(dstis - 2, q, 4, sval, 2);
        lmmp_debug_assert(dstis[0] == 1);
    }

    TEMP_DECL;
    /*
        单层滚动区域布局（后续层相互复用已死区域）：
            R0 = [dp -> dip]  max(nres+nr+1, mn+2, 3*nr+4)
            R1 = [t2]         max(2*nr+2, mn)
            R2 = [w1 -> xp]   max(max(2*nr+2, mn)+naz+1, 2*nr+3)
        逐层求和的最大值由下方 alloc 公式覆盖（精确链最坏 ~7*nrmax+2*naz，
        梅森链最坏 ~9*nrmax+naz，均小于 2*(ns+namax+4)+6*nrmax+16）。
    */
    mp_size_t nrmax = (ns >> 1) + 2;
    mp_size_t alloc_size = 2 * (ns + namax + 4) + 6 * nrmax + 16;
    mp_ptr restrict tb = TALLOC_TYPE(alloc_size, mp_limb_t);

    do {
        na = *--sizp;

        // an = 0:[numa-naz,naz]
        // ir = 1:[dstis-nr,nr] = floor(B^(4*nr/3)/cbrt(ar)) - [0|1]
        //  d = B^(naz+3*nr)-an*ir*ir*ir
        //  -16*B^(naz+2*nr) < d < 16*B^(naz+2*nr)

        mp_size_t naz = LMMP_MIN(na, namax);
        mp_size_t nres = naz + 2 * nr + 2;
        mp_size_t nsqr;
        mn = lmmp_fft_next_size_(nres);

        // 梅森链需 mn 显著小于精确链的最大中间尺寸 3*nr+2（节省立方的完整
        // 展开），且模折叠加法偏移 naz+3*nr-mn >= 0
        int cmod = mn + 2 * CBRT_NEWTON_MODM_THRESHOLD < 3 * nr + 2 && mn <= naz + 3 * nr;

        mp_size_t r0 = LMMP_MAX(LMMP_MAX(nres + nr + 1, mn + 2), 3 * nr + 4);
        mp_size_t r1 = cmod ? LMMP_MAX(2 * nr + 2, mn) : 2 * nr + 2;
        mp_ptr dp = tb, t2 = tb + r0, w1 = tb + r0 + r1, xp = w1, dip = tb;

        if (cmod) {
            // ---- 梅森链：dp = an*ir^3 mod (B^mn-1)，再折叠为 -d (mod M) ----

            // ir^2
            if (2 * CBRT_NEWTON_MODM_THRESHOLD + mn >= 2 * nr + 1) {
                lmmp_sqr_(t2, dstis - nr, nr + 1);
                nsqr = 2 * nr + 2;
            } else {
                lmmp_sqr_mersenne_(t2, mn, dstis - nr, nr + 1);
                nsqr = mn;
            }

            // w1 = t2*an mod M
            if (naz < CBRT_NEWTON_MODM_THRESHOLD || naz * 8 < nsqr || mn >= nsqr + naz) {
                lmmp_mul_(w1, t2, nsqr, numa - naz, naz);
                if (nsqr + naz > mn) {
                    if (lmmp_add_(w1, w1, mn, w1 + mn, nsqr + naz - mn))
                        lmmp_inc(w1);
                }
            } else {
                lmmp_mul_mersenne_(w1, mn, t2, nsqr, numa - naz, naz);
            }

            // dp = w1*ir mod M,  naz+nr < mn <= naz+3*nr
            lmmp_mul_mersenne_(dp, mn, w1, mn, dstis - nr, nr + 1);
            //[dp,mn] -= B^(naz+3*nr) mod (B^mn-1)
            dp[mn] = 1;
            lmmp_dec(dp + naz + 3 * nr - mn);
            if (dp[mn] == 0)
                lmmp_dec(dp);
        } else {
            // ---- 精确截断链：dp = an*ir^3 mod B^nres ----
            // (X*Y) mod B^k = (X mod B^k)*(Y mod B^k) mod B^k，
            // t2 与 w1 均短于 B^nres（2*nr+2+naz = nres），无需预截断
            lmmp_sqr_(t2, dstis - nr, nr + 1);
            nsqr = 2 * nr + 2;
            lmmp_mul_(w1, t2, nsqr, numa - naz, naz);  // [w1, nres)
            lmmp_mul_(dp, w1, nres, dstis - nr, nr + 1);  // 低 nres limb 即 an*ir^3 mod B^nres
        }

        /*
            dp limb nres-1 = naz+2*nr+1 编码 d 的符号：
                d > 0  =>  dp = [B^nres|B^mn] - d，该 limb ∈ [B-16, B-1]
                d <= 0 =>  dp = |d| < 16*B^(naz+2*nr)，该 limb ∈ [0, 15]
        */
        mp_limb_t hi = dp[nres - 1];
        mp_limb_t rem;

        if (hi > 15) {
            // d > 0：ir 低估，修正相加。取反得 d 的窗口 limb 后除 3 向下取整
            if (!cmod)
                lmmp_dec(dp);  // 消除 B^nres-d 折叠借位，使取反恰为 d
            lmmp_not_(xp, dp + naz, 2 * nr + 2);
            rem = lmmp_div_1_(xp, xp, 2 * nr + 2, 3);
        } else {
            // d <= 0：ir 高估，修正相减。窗口直接可得，除 3 向上取整
            rem = lmmp_div_1_(xp, dp + naz, 2 * nr + 2, 3);
            if (rem != 0 || !lmmp_zero_q_(dp, naz))
                lmmp_inc(xp);
        }

        // dip = xp*ir,  C = dip*B^(na-4*nr)
        lmmp_mul_(dip, xp, 2 * nr + 2, dstis - nr, nr + 1);

        if (hi > 15) {
            // i' = ir*B^(na-nr) + C
            lmmp_copy(dstis - na, dip + 4 * nr - na, na - nr);
            lmmp_inc_1(dstis - nr, dip[3 * nr]);
        } else {
            // i' = ir*B^(na-nr) - C，以按位取反 + 借位减实现
            if (lmmp_zero_q_(dip, 4 * nr - na)) {
                // C 的取模窗口低位为 0 时的两补边界
                dip[3 * nr + 1] = 1;
                lmmp_dec(dip + 4 * nr - na);
            }
            lmmp_not_(dstis - na, dip + 4 * nr - na, na - nr);
            lmmp_dec_1(dstis - nr, dip[3 * nr] + 1);
        }

        nr = na;
    } while (sizp != sizes);
    TEMP_FREE;
}

/*
    snorm：newton 路径的移位归一化（无乘子版本，供 lmmp_cbrt_ 的 newton
    分支调用；lmmp_cbrt_newton_ 本体已改为输入契约，归一化与还原均由调用
    处完成）。invnewton 迭代的收敛条件实际仅需顶 limb >= B/8（层误差界按
    B^nr/8 论证，见 lmmp_invcbrt_newton_ 注释），而 B/8 恒可由 3 对齐移位
    到达，故 newton 路径不引入 knorm 的 k^3 乘子（省去乘子搜索、numa*k^3
    的 mul_1 遍历与还原时的 div_1(k) 遍历）。

    输出 a2 = numa << s 写入调用方缓冲区（至少 na+3 limb），满足
    limbs(a2) ≡ 0 (mod 3)（重建移位量为整 limb 的必要条件：B^(1/3) 无理，
    cbrt 无 sqrt 的半 limb 对齐技巧）且 a2 顶 limb >= B/8，即
    lmmp_cbrt_newton_ 的输入契约；返回
        *na2p = a2 的 limb 数（恒为 3*ceil(na/3)）
        返回值 = t = s/3（根的右移回退量，< 64）

    目标总位长 T = bl+s（bl = bitlen(numa)）的解析求解：s ≡ 0 (mod 3)
    保证根可经右移 t = s/3 bit 精确回退（floor(cbrt(X*2^(3t))) >> t ==
    floor(cbrt(X))，由 floor(2y)>>1 == floor(y) 对 t 归纳）。记
    L = limbs(a2) ≡ 0 (mod 3)，pos = T - 64*(L-1) 为顶 limb 有效位宽，
    由 64 ≡ 1 (mod 3) 有 T ≡ L-1+pos (mod 3)，结合 T ≡ bl (mod 3) 解出
    每 class（r = bl mod 3）唯一的 (pos, L) 组合：
        r=0: pos=64（顶 limb MSB=1 >= B/2）
        r=1: pos=62（顶 limb ∈ [B/8, B/4)）
        r=2: pos=63（顶 limb ∈ [B/4, B/2)）
    可达时最小 s 唯一（s 随 L 增大单调增）且 s <= 189，故 t <= 63。
    对照 divide 路径（knorm）：3B/8 仅 pos=63 且顶 2bit 为 11 或 pos=64
    可达，约 1/2 输入（r=1 恒不可达，r=2 且顶 2bit 为 10 不可达）必须乘
    小立方数 k^3 改类；B/8 三 class 全可达，移位永充分。
*/
static mp_size_t lmmp_cbrt_snorm_(mp_srcptr numa, mp_size_t na, mp_ptr a2, mp_size_t *na2p) {
    lmmp_param_assert(numa != NULL && a2 != NULL && na > 0 && numa[na - 1] != 0);
    mp_bitcnt_t hb = lmmp_limb_bits_(numa[na - 1]);
    mp_size_t bl = LIMB_BITS * (na - 1) + (mp_size_t)hb;
    mp_size_t r = bl % 3;
    // (pos, T mod 3) 对应关系：pos=64≡1 → T≡0；pos=62≡2 → T≡1；pos=63≡0 → T≡2
    static const mp_size_t POS_BY_CLASS[3] = {LIMB_BITS, LIMB_BITS - 2, LIMB_BITS - 1};
    mp_size_t pos = POS_BY_CLASS[r];

    // L = 最小的满足 T = 64*(L-1)+pos >= bl 的三倍数
    mp_size_t L = 1;
    if (bl > pos)
        L = (bl - pos + LIMB_BITS - 1) / LIMB_BITS + 1;
    L = (L + 2) / 3 * 3;

    mp_size_t s = LIMB_BITS * (L - 1) + pos - bl;  // <= 189, ≡ 0 (mod 3)
    lmmp_debug_assert(s >= 0 && s % 3 == 0 && s / 3 < LIMB_BITS);
    lmmp_cbrt_shapply_(numa, na, s, a2);
    lmmp_debug_assert(L % 3 == 0 && L <= na + 3 && a2[L - 1] >= LIMB_B_8);
    *na2p = L;
    return s / 3;
}

/*
    计算已归一化输入 x = cbrt(a*B^(3nf)) 的舍入近似 r。输入契约（由调用
    处保证，本函数不再内部归一化）：na ≡ 0 (mod 3) 且顶 limb >= B/8，
    即 snorm 的输出形（B/8 为 invcbrt_newton_ 的收敛下限；na 整 limb 对齐
    是重建移位量为整 limb 的必要条件——B^(1/3) 无理，cbrt 无 sqrt 的半
    limb 对齐技巧）。归一化与 >>s/3 还原均在 lmmp_cbrt_ 分发处完成。

        ic   = [ns+1] limb 逆立方根（多算 1 个 guard limb），ns = na/3+nf+1
        Q    = a*ic^2（ic 先平方再一次全乘，均为精确整数乘）
        root = round(Q / B^e0)，e0 = 2*ns + 2*(na/3) - nf

    记 It = cbrt(B^(3ns+na)/a)（实值），有 Q/B^e0 = x*(ic/It)^2，故

        r = round(x - eps),   eps = x*(1 - (ic/It)^2) >= 0

    ic ∈ {I-1, I}（I = floor(It)：实值截断 + 算法至多 1 ulp 低估）给出
    It-ic ∈ [0,2)，而 x/It = (a/B^na)^(2/3) / B（代入 ns = na/3+nf+1），
    因此
        0 <= eps = x*(It-ic)(It+ic)/It^2 < 4*x/It < 4/B

    （sqrt 对照：其 eps < 2^-63/2^-31 由 na 奇偶引入 sqrt(B) 半 limb 视窗；
    cbrt 的 B^(1/3) 无理使 na 必须整 limb 对齐，guard 恰为 1 limb）。
    eps 非负故 r 绝不超过 round(x)；仅当 frac(x) ∈ [1/2, 1/2+eps) 时得到
    floor(x)（eps < 2^-62，随机输入概率 < 2^-61）；完全立方输入（frac(x)=0，
    eps < 1/2）恒精确命中。又 x < B^((na+3nf)/3) = B^(ns-1)，结果与舍入
    进位均不出 ns limb，直接写入 [dsts,ns)。移位问题还原（floor 复合
    不变性）：r' = floor(r >> t) ∈ {round(x')-1, round(x')}（x' = x/2^t
    为原问题根），由分发处完成。
*/

void lmmp_cbrt_newton_(mp_ptr dsts, mp_srcptr numa, mp_size_t na, mp_size_t nf) {
    lmmp_param_assert(na > 0 && na % 3 == 0);
    lmmp_param_assert(nf >= 2);
    lmmp_param_assert(3 * nf >= 2 * na + 3);
    lmmp_param_assert(numa != NULL && dsts != NULL);
    lmmp_param_assert(numa[na - 1] >= LIMB_B_8);

    TEMP_DECL;
    mp_size_t ns = na / 3 + nf + 1;
    mp_ptr ic = TALLOC_TYPE(ns + 1, mp_limb_t);
    mp_ptr q1 = TALLOC_TYPE(2 * ns + 2, mp_limb_t);
    mp_ptr q = TALLOC_TYPE(na + 2 * ns + 3, mp_limb_t);

    lmmp_invcbrt_newton_(ic, ns, numa, na);

    // Q = a*ic^2：先平方再单乘（对比 ic*a 后再乘 ic 的两次同规模乘法，
    // 平方以约半成本替代其一；2*ns+2 >= na 满足 mul 的长操作数在前）
    lmmp_sqr_(q1, ic, ns + 1);
    lmmp_mul_(q, q1, 2 * ns + 2, numa, na);

    // dsts = round(Q/2^e0)，舍入位为 q[e0-1] 的最高位（结果 <= B^(ns-1)，
    // 进位不会越出 ns limb，无需中间缓冲）
    mp_size_t e0 = 2 * ns + 2 * (na / 3) - nf;
    lmmp_copy(dsts, q + e0, ns);
    if (q[e0 - 1] >> (LIMB_BITS - 1))
        lmmp_inc(dsts);
    TEMP_FREE;
}


void lmmp_cbrt_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa, mp_size_t na, mp_size_t nf) {
    lmmp_debug_assert(na > 0);
    lmmp_debug_assert(numa[na - 1] > 0);
    mp_size_t nl = na + 3 * nf;

    if (nl <= 6) {
        // 小输入：任意归一化均可由基例函数族直接处理
        mp_limb_t xb[6] = {0, 0, 0, 0, 0, 0}, r[2] = {0, 0};
        for (mp_size_t i = 0; i < na; ++i) xb[3 * nf + i] = numa[i];

        if (nl <= 2) {
            r[0] = nl == 1 ? lmmp_cbrt_ulong_(xb[0]) : lmmp_cbrt_3_(xb[0], xb[1], 0);
        } else if (nl == 3) {
            r[0] = lmmp_cbrt_3_(xb[0], xb[1], xb[2]);
        } else {
            lmmp_cbrt_6_(r, xb, nl);
        }
        mp_size_t ns = (nl + 2) / 3;
        dsts[0] = r[0];
        if (ns > 1) dsts[1] = r[1];

        if (dstr) {
            // [dstr,2*ns+1] = X - r^3：立方由基例原语直接给出，
            // rem < 3r^2+3r+1 < B^(2*ns+1)，低段直接减无借位
            mp_limb_t t[10];
            if (ns == 1) {
                lmmp_cube_3_(t, r[0]);
                lmmp_sub_n_(dstr, xb, t, 3);
            } else {
                lmmp_cube_6_(t, r);
                lmmp_sub_n_(dstr, xb, t, 5);
            }
        }
        return;
    }

    /*
        与 lmmp_sqrt_ 的分发同构：大 nf 无余数走 lmmp_cbrt_newton_ 的
        [floor|round] 近似；其余（含全部 dstr != NULL 与精确 floor 语义）
        统一走 knorm 归一化 + lmmp_cbrt_divide_ 精确求根，>>t 与 div_1(k)
        的 floor 复合精确还原，无需任何修正循环。

        cbrt_newton_ 为输入契约版（na ≡ 0 (mod 3)，顶 limb >= B/8），
        归一化与还原在本分发处完成：snorm 纯移位归一化（与 knorm 的 k=1
        移位路径共用 shapply，B/8 即满足迭代收敛）+ 契约调用 + 结果 >>t
        的 floor 复合还原。na2 = 3*ceil(na/3) 使 na2/3+nf+1 恰等于
        (na+3nf+2)/3+1，>>t 后的写入长度与语义和内部归一化旧版一致。
    */
    if (!dstr && nf >= CBRT_INVNEWTON_K_THRESHOLD * na && nf >= CBRT_INVNEWTON_NF_MIN) {
        TEMP_DECL;
        mp_ptr a2 = TALLOC_TYPE(na + 3, mp_limb_t);
        mp_size_t na2;
        mp_size_t t = lmmp_cbrt_snorm_(numa, na, a2, &na2);
        lmmp_debug_assert(na2 / 3 + nf + 1 == (na + 3 * nf + 2) / 3 + 1);
        lmmp_cbrt_newton_(dsts, a2, na2, nf);
        if (t)
            lmmp_shr_(dsts, dsts, na2 / 3 + nf + 1, t);
        TEMP_FREE;
        return;
    }

    TEMP_DECL;
    mp_ptr a2 = TALLOC_TYPE(na + 5, mp_limb_t);
    mp_size_t na2;
    mp_limb_t k;
    mp_size_t t = lmmp_cbrt_knorm_(numa, na, a2, &na2, &k);

    mp_size_t ns2 = na2 / 3 + nf;    // 移位后问题根长（radicand = a2*B^(3nf)）
    mp_size_t ns0 = (nl + 2) / 3;    // 原问题根长
    mp_size_t nR = ns0 + 1;          // 根工作长度（含可能的最高零 limb）
    mp_ptr tp = TALLOC_TYPE(4 * ns2, mp_limb_t);
    // nf==0 时被开方数即 a2 本身，rad 直接复用（knorm 后 a2 不再读取）；
    // nf>0 时 rad = a2*B^(3nf) 须另建（低位补零后拷贝）。两种形下缓冲均
    // 覆盖 divide 的 [rad,3ns2) 与余数修正的 [rad,2ns2+3)
    mp_ptr rad;
    if (nf) {
        rad = TALLOC_TYPE(3 * ns2, mp_limb_t);
        lmmp_zero(rad, 3 * nf);
        lmmp_copy(rad + 3 * nf, a2, na2);
    } else {
        rad = a2;
    }

    // divide 写满 [dsts,ns2)（根顶 limb 恒非零），仅补零可能的最高空 limb
    lmmp_zero(dsts + ns2, nR - ns2);
    /*
        dstr 路径基于恒等式（K = k*2^t，radicand = X*K^3）：

            S = floor(K*cbrt(X)) = K*r + e,   e = S mod K < K < 2^72
            R = radicand - S^3  （divide 以 calr=1 维护）
            K^3*rem = radicand - (S-e)^3 = R + 3*S^2*e - 3*S*e^2 + e^3

        即 rem = (R + 3*S^2*e - 3*S*e^2 + e^3) / K^3，各除法/移位均精确整除。
        注意 K^3 | R 单独不成立（e ≠ 0 一般），e 的修正项不可省略。e 的至多
        2 个 limb 由根还原步骤的返回值免费给出：
            e_lo = S mod 2^t   （shr_ 移出低位）
            e_hi = (S>>t) mod k（div_1 余数）
        3*S^2*e 等项均为 e <= 2 limb 的 O(n) 小乘，唯一超线性操作是 S 的
        平方，免去 r^3 的 sqr + mul 两次全尺寸乘法。k==1 且 t==0 时 e=0
        且 radicand == X，R 即真余数，走直接下拷快路径。
    */
    int remnorm = dstr != NULL && k == 1 && t == 0;
    lmmp_cbrt_divide_(dsts, rad, ns2, tp, dstr != NULL);

    mp_ptr Sp = NULL;
    if (dstr && !remnorm) {
        // S 暂存：随后的还原会原地改写 dsts（>>t 与 div_1(k)），而修正项
        // 仍需还原前的 S，故先复制（e 依赖还原返回值，无法重排避免）
        Sp = TALLOC_TYPE(ns2, mp_limb_t);
        lmmp_copy(Sp, dsts, ns2);
    }
    mp_limb_t elo = 0;
    if (t) {
        mp_limb_t sc = lmmp_shr_(dsts, dsts, nR, t);
        if (Sp) elo = sc >> (LIMB_BITS - t);
    }
    mp_limb_t ehi = k > 1 ? lmmp_div_1_(dsts, dsts, nR, k) : 0;

    if (dstr) {
        // remnorm 时 na ≡ 0 (mod 3) 且 hb 满偏，ns2 == ns0，R 区域长度恰合
        if (remnorm) {
            lmmp_copy(dstr, rad, 2 * ns0 + 1);
            TEMP_FREE;
            return;
        }
        // e = ehi*2^t + elo（ehi < k <= 256, elo < 2^t，e < K < 2^72）
        mp_limb_t e0, e1;
        if (t == 0) {
            e0 = ehi;
            e1 = 0;
        } else {
            e0 = elo | (ehi << t);
            e1 = ehi >> (LIMB_BITS - t);
        }

        mp_size_t L = 2 * ns2 + 3;
        mp_ptr N = rad;
        lmmp_zero(N + 2 * ns2 + 1, L - (2 * ns2 + 1));

        if (e0 | e1) {
            mp_ptr s2v = TALLOC_TYPE(2 * ns2, mp_limb_t);
            mp_ptr tb = TALLOC_TYPE(2 * ns2 + 2, mp_limb_t);
            // N += 3*S^2*e，pe = [tb, 2*ns2+2) = S^2*e
            lmmp_sqr_(s2v, Sp, ns2);
            mp_limb_t cy = lmmp_mul_1_(tb, s2v, 2 * ns2, e0);
            tb[2 * ns2] = cy;
            tb[2 * ns2 + 1] = 0;
            if (e1) {
                // addmul 窗口顶为 limb 2*ns2，返回进位归属 limb 2*ns2+1
                tb[2 * ns2 + 1] = lmmp_addmul_1_(tb + 1, s2v, 2 * ns2, e1);
            }
            cy = lmmp_addmul_1_(N, tb, 2 * ns2 + 2, 3);
            if (cy)
                cy = lmmp_add_1_(N + 2 * ns2 + 2, N + 2 * ns2 + 2, L - 2 * ns2 - 2, cy);
            lmmp_debug_assert(cy == 0);

            mp_limb_t eb[2] = {e0, e1}, e2b[4], e3b[6];
            lmmp_sqr_hard_2_(e2b, eb);
            lmmp_mul_basecase_(e3b, e2b, 4, eb, 2);
            cy = lmmp_add_(N, N, L, e3b, 6);
            lmmp_debug_assert(cy == 0);

            // N -= 3*S*e^2（e^2 至多 3 limb <= ns2）；低窗内可能需向高位借位
            mp_size_t ne2 = 4;
            while (ne2 > 1 && e2b[ne2 - 1] == 0) --ne2;
            lmmp_mul_(tb, Sp, ns2, e2b, ne2);
            mp_limb_t bo = lmmp_submul_1_(N, tb, ns2 + ne2, 3);
            if (bo)
                bo = lmmp_sub_1_(N + ns2 + ne2, N + ns2 + ne2, L - ns2 - ne2, bo);
            lmmp_debug_assert(bo == 0);  // N = K^3*rem >= 0
        }

        // rem = (N/k^3) >> (3t)
        if (k > 1)
            lmmp_div_1_(N, N, L, k * k * k);
        mp_size_t w = 3 * t / LIMB_BITS, b = 3 * t % LIMB_BITS;
        mp_ptr rp = N + w;
        if (b)
            lmmp_shr_(rp, rp, L - w, b);
        // w <= 2 且 ns2 >= ns0 保证 L - w >= 2*ns0 + 1，拷贝即写满余数区
        lmmp_debug_assert(L - w >= 2 * ns0 + 1);
        lmmp_copy(dstr, rp, 2 * ns0 + 1);
        TEMP_FREE;
        return;
    }
    TEMP_FREE;
}