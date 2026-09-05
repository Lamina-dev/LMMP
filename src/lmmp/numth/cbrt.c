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
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/log2_exp2.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"


// 此阈值是为了保证 cbrt(A)^2 >= B^2/2
// 实际值大约 0x5A827999FCEF3242
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
    lmmp_param_assert(a1 > 0);
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
    lmmp_sqr_basecase_(t + 6, r, 2);
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

    mp_limb_t rsav[7], w[7], u2[4], u3[5], x3sq[7];
    lmmp_zero(rsav + 5, 2);
    lmmp_copy(rsav, numa, 5);
    lmmp_zero(x3sq, 7);
    lmmp_sqr_basecase_(u2, dst + 1, 1);         // u2 = Ahr^2
    x3sq[4] = lmmp_mul_1_(x3sq + 2, u2, 2, 3);  // 3*x^2 = 3*Ahr^2*B^2 占 [2,5)
    for (;;) {
        // [w,6] = W(u) = 3*Ahr*u^2*B + u^3
        lmmp_zero(w, 7);
        lmmp_sqr_basecase_(u2, tp + 5, 2);             // u^2
        lmmp_mul_basecase_(w + 1, u2, 3, dst + 1, 1);  // Ahr*u^2 占 [1,5)
        w[5] = lmmp_mul_1_(w + 1, w + 1, 4, 3);        // 3*Ahr*u^2*B
        lmmp_mul_basecase_(u3, u2, 3, tp + 5, 2);      // u^3
        w[5] += lmmp_add_n_(w, w, u3, 5);              // W < 4*B^5，w[5] <= 4 不溢出
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
    if (ns == 2) {
        lmmp_cbrt6_fast_(dst, numa, tp);
        return;
    }
    if (ns == 1) {
        dst[0] = lmmp_cbrt_3_(numa[0], numa[1], numa[2]);
        if (calr) {
            lmmp_cube_3_(tp, dst[0]);
            lmmp_sub_n_(numa, numa, tp, 3);
        }
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
            lmmp_fill(dst, 0, lo, LIMB_MAX);
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
                lmmp_sqr_basecase_(Alr2, Alr + lo - 2, 2);              // [Alr2,4] = H^2
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

#if 0
/*

    B^(3*ns) // [numa,3*ns]^(2/3)

    A     = Ah * B^(3*lo) + Al

    Ahr   = B^(nf+3*hi) / Ah^(2/3)
    x_k   = Ahr * B^lo

    x_k+1 = x_k + x_k/3 - A^2 * x_k^4 / 3 / B^(9*na+3*nf)

*/

#define INVCBRT_MIN 0xa000000000000000ull

void lmmp_invcbrt_newton_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t nf) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa != NULL && dst != NULL);
    lmmp_param_assert(numa[3 * na - 1] >= INVCBRT_MIN);

    mp_size_t ns = na + nf;
    if (ns == 1) {
        mp_limb_t a_sqr[6], a_sqrcbrt[2], tp[9];
        lmmp_sqr_basecase_(a_sqr, numa, 3);
        lmmp_cbrt_divide_(a_sqrcbrt, a_sqr, 2, tp, 0);
        lmmp_zero(tp, 3);
        tp[3] = 1;

        lmmp_div_2_s_(dst, tp, 4, a_sqrcbrt);
    } else {

    }
}

#endif
