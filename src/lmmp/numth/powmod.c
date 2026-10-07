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

/*
    奇模数模幂：Montgomery 归约（REDC）+ 滑动窗口梯子。

    Montgomery 域核心（三层 REDC 分派、蒙域上下文与进蒙域 redcify）的
    单一定义见 include/lmmp/impl/powmod.h，由本文件与 is_prime_n.c 共用。
    本文件提供：
      - 1/2 limb 特化 lmmp_powmod_1_ / lmmp_powmod_2_（u128 标量蒙域梯子）；
      - 蒙域梯子 lmmp_powmod_odd_mont_（滑动窗口，结果留在蒙域：同模数批量
        幂/素性检验多轮经同一上下文复用 ninv 与 FFT 变换缓存，并免去出蒙域
        REDC——蒙域内 ±1 探测等价性见 impl/powmod.h 头注）；
      - 公开入口 lmmp_powmod_odd_（= 上下文 init + 蒙域梯子 + 出蒙域 REDC）；
      - 任意模数 lmmp_powmod_（m 偶时 2-adic 分解 + CRT 合成）。
      - 单步公开 REDC lmmp_redc_（tp 只读契约：basecase 层先复制再归约）。
*/

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/mul_cache.h"
#include "../../../include/lmmp/impl/powmod.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


#define getbit(p, bi) ((p[(bi - 1) / LIMB_BITS] >> (bi - 1) % LIMB_BITS) & 1)

static inline mp_limb_t getbits(const mp_limb_t* p, mp_bitcnt_t bi, mp_bitcnt_t nbits) {
    mp_bitcnt_t nbits_in_r;
    mp_limb_t r;
    mp_size_t i;

    if (bi <= nbits) {
        return p[0] & (((mp_limb_t)1 << bi) - 1);
    } else {
        bi -= nbits;                     /* bit index of low bit to extract */
        i = bi / LIMB_BITS;              /* word index of low bit to extract */
        bi %= LIMB_BITS;                 /* bit index in low word */
        r = p[i] >> bi;                  /* extract (low) bits */
        nbits_in_r = LIMB_BITS - bi;     /* number of bits now in r */
        if (nbits_in_r < nbits)          /* did we get enough bits? */
            r += p[i + 1] << nbits_in_r; /* prepend bits from higher word */
        return r & (((mp_limb_t)1 << nbits) - 1);
    }
}

static inline mp_bitcnt_t count_bits(mp_srcptr p, mp_size_t n) {
    return (n - 1) * LIMB_BITS + lmmp_limb_bits_(p[n - 1]);
}

/*
    1/2 limb Montgomery 内联（惯例与正确性论证见 is_prime_ulong.c 的
    mont64_reduce 与 is_prime_2.c 的 mul_128/sqr_128/mont2_redc，本文件
    按需复制为 static）。全程 u128 标量表达：和的进位由回绕比较自然
    产生，编译器融合为 add/adc 链；x64 下全积/平方借用 longlong.h
    调优的 mulx/adcx 内联，其余平台为纯 C（mul/umulh 链）。
*/

/* 1 limb REDC：r = (T + k*m)/B mod m，k = T_lo*ninv mod B，ninv = -m^(-1) mod B。
   T < m*B 时结果 < 2m；k*m + T < 2^129，第 129 位 c 由 128 位回绕捕获，
   回绕进位下 hi - m 即规范结果 */
static inline mp_limb_t powmod1_redc_(u128 t, mp_limb_t m, mp_limb_t ninv) {
    mp_limb_t k = _u128low(t) * ninv;
    u128 km = (u128)k * m;
    u128 sum = km + t;
    uint c = sum < km;
    mp_limb_t hi = _u128high(sum);
    if (c)
        return hi - m;
    return hi >= m ? hi - m : hi;
}

/* 2 limb 全积 a*b -> (hi, lo) */
static inline void mul_128(u128 a, u128 b, u128* hi, u128* lo) {
#if defined(LMMP_ASM_X64) && (defined(__GNUC__) || defined(__clang__))
    ulong u[4];
    _umul128to256_(_u128high(a), _u128low(a), _u128high(b), _u128low(b), u);
    *lo = ((u128)u[1] << 64) | u[0];
    *hi = ((u128)u[3] << 64) | u[2];
#else
    ulong a0 = _u128low(a), a1 = _u128high(a);
    ulong b0 = _u128low(b), b1 = _u128high(b);
    u128 p00 = (u128)a0 * b0;
    u128 p01 = (u128)a0 * b1;
    u128 p10 = (u128)a1 * b0;
    u128 p11 = (u128)a1 * b1;
    u128 mid = p01 + p10;
    u128 cm = mid < p01; /* 129 位交叉和进位，属 hi 的第 64 位 */
    u128 l = (mid << 64) + p00;
    u128 cl = l < p00; /* 进位属 hi 的第 0 位 */
    *lo = l;
    *hi = p11 + (mid >> 64) + cl + (cm << 64);
#endif
}

/* 2 limb 平方 a^2 -> (hi, lo)：交叉积单次乘法后整体加倍（3 乘法平方） */
static inline void sqr_128(u128 a, u128* hi, u128* lo) {
#if defined(LMMP_ASM_X64) && (defined(__GNUC__) || defined(__clang__))
    ulong u[4];
    _usqr128to256_(_u128high(a), _u128low(a), u);
    *lo = ((u128)u[1] << 64) | u[0];
    *hi = ((u128)u[3] << 64) | u[2];
#else
    ulong a0 = _u128low(a), a1 = _u128high(a);
    u128 p0 = (u128)a0 * a0;
    u128 m = (u128)a0 * a1; /* 交叉积，加倍后 129 位 */
    u128 p1 = (u128)a1 * a1;
    u128 ml = m << 1;
    u128 mh = m >> 127; /* 加倍进位（0/1），属 hi 的第 64 位 */
    u128 l = (ml << 64) + p0;
    u128 cl = l < p0;
    *lo = l;
    *hi = p1 + (ml >> 64) + cl + (mh << 64);
#endif
}

/* 2 limb REDC：r = (T + q*n)/B^2，T = thi*B^2 + tlo < n*B^2，q = tlo*ninv
   mod B^2（u128 截断乘恰为其低 128 位）。n 接近 B^2 时和可达 129 位，
   回绕进位下按模 2^128 回绕减 n 恰为真值且已 < n */
static inline u128 mont2_redc(u128 thi, u128 tlo, u128 n, u128 ninv) {
    u128 q = tlo * ninv;
    u128 qh, ql, lo, hi, c, c1, c2;
    mul_128(q, n, &qh, &ql);
    lo = tlo + ql;
    c = lo < tlo;
    hi = thi + qh;
    c1 = hi < thi;
    hi += c;
    c2 = c && hi == 0;
    if (c1 | c2)
        hi -= n;
    else if (hi >= n)
        hi -= n;
    return hi;
}

static inline u128 mont2_sqr(u128 a, u128 n, u128 ninv) {
    u128 h, l;
    sqr_128(a, &h, &l);
    return mont2_redc(h, l, n, ninv);
}

static inline u128 mont2_mul_2(u128 a, u128 b, u128 n, u128 ninv) {
    u128 h, l;
    mul_128(a, b, &h, &l);
    return mont2_redc(h, l, n, ninv);
}

mp_limb_t lmmp_redc_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict   tp,
    mp_srcptr restrict ninv,
    mp_srcptr restrict   mp,
    mp_size_t             n
) {
    lmmp_param_assert(dst != NULL && tp != NULL);
    lmmp_param_assert(ninv != NULL && mp != NULL);
    lmmp_param_assert(n > 0 && mp[n - 1] > 0);
    lmmp_param_assert(mp[0] % 2 == 1);
    TEMP_DECL;

    /* basecase 层：链式归约 clobber 工作区，先复制以维持 tp 只读契约 */
    if (n < REDC_BASECASE_THRESHOLD) {
        mp_ptr restrict up = TALLOC_TYPE(2 * n, mp_limb_t);
        lmmp_copy(up, tp, 2 * n);
        mp_limb_t cy = lmmp_mont_redc_bc_(dst, up, mp, n, ninv[0]);
        TEMP_FREE;
        return cy;
    }

    mp_ptr restrict tpw = TALLOC_TYPE(2 * n, mp_limb_t);
    mp_ptr restrict q = TALLOC_TYPE(n, mp_limb_t);
    mp_ptr restrict hi;
    /* t_lo != 0 时低半 t_lo + L == B^n 恰产生 1 进位；t_lo == 0 时 L == 0 */
    mp_limb_t carry = !lmmp_zero_q_(tp, n);

    if (n < MULLO_DC_THRESHOLD) {
        lmmp_mullo_dc_(q, tp, ninv, tpw, n);
    } else {
        lmmp_mullo_fft_(q, tp, ninv, n, tpw);
    }

    if (n < REDC_MERSENNE_THRESHOLD) {
        mp_ptr restrict prod = TALLOC_TYPE(2 * n, mp_limb_t);
        lmmp_mul_n_(prod, q, mp, n);
        hi = prod + n;
    } else {
        mp_size_t msz = lmmp_fft_next_size_((2 * n + 1) >> 1);
        lmmp_debug_assert(2 * n > msz && msz >= n);
        mp_ptr restrict V = TALLOC_TYPE(msz, mp_limb_t);
        mp_ptr restrict L = TALLOC_TYPE(n, mp_limb_t);
        hi = TALLOC_TYPE(n, mp_limb_t);
        lmmp_mul_mersenne_(V, msz, q, n, mp, n);
        if (carry) {
            lmmp_not_(L, tp, n);
            lmmp_inc(L); /* t_lo != 0，取反加一不会越界 */
        } else {
            lmmp_zero(L, n);
        }
        lmmp_mont_fold_hi_(hi, V, L, n, msz);
    }

    mp_limb_t cy = lmmp_add_nc_(dst, tp + n, hi, n, carry);
    TEMP_FREE;
    return cy;
}

void lmmp_powmod_1_(
    mp_ptr    restrict dst,
    mp_srcptr restrict  bp,
    mp_srcptr restrict  ep,
    mp_size_t           en,
    mp_limb_t          mod
) {
    lmmp_param_assert(dst != NULL && bp != NULL && ep != NULL);
    lmmp_param_assert(en > 0 && ep[en - 1] > 0);
    lmmp_param_assert(mod % 2 == 1 && mod > 1);
    lmmp_param_assert(bp[0] < mod);

    if (bp[0] == 0) {
        dst[0] = 0;
        return;
    }
    mp_bitcnt_t ebi = count_bits(ep, en);
    /* 指数为 1：契约 [bp,1] < mod，直接拷贝即已取模 */
    if (ebi == 1) {
        dst[0] = bp[0];
        return;
    }

    unsigned windowsize = lmmp_powmod_win_size_(ebi);
    lmmp_debug_assert(windowsize < ebi);
    mp_limb_t ninv = 0 - lmmp_binvert_ulong_(mod);
    /* 进蒙域：x_m = b*B mod m，128/64 一步除法（b < mod 保证商不溢出） */
    mp_limb_t u;
    (void)_udiv128by64to64_(bp[0], 0, mod, &u);

    mp_bitcnt_t cnt;
    unsigned this_windowsize;
    mp_limb_t expbits;
    TEMP_DECL;
    mp_ptr restrict pp = TALLOC_TYPE((mp_size_t)1 << (windowsize - 1), mp_limb_t);

    pp[0] = u;
    if (windowsize > 1) {
        mp_limb_t b2 = powmod1_redc_((u128)u * u, mod, ninv);
        for (mp_size_t i = 1; i < ((mp_size_t)1 << (windowsize - 1)); i++)
            pp[i] = powmod1_redc_((u128)pp[i - 1] * b2, mod, ninv);

        expbits = getbits(ep, ebi, windowsize);
        ebi -= windowsize;

        ctz_shr_u64(expbits, expbits, cnt);
        ebi += cnt;

        u = pp[expbits >> 1];
    } else {
        --ebi;
    }

    while (ebi != 0) {
        while (getbit(ep, ebi) == 0) {
            u = powmod1_redc_((u128)u * u, mod, ninv);
            if (--ebi == 0)
                goto done;
        }

        /* The next bit of the exponent is 1.  Now extract the largest block of
           bits <= windowsize, and such that the least significant bit is 1.  */

        expbits = getbits(ep, ebi, windowsize);
        this_windowsize = LMMP_MIN(windowsize, ebi);

        ctz_shr_u64(expbits, expbits, cnt);
        this_windowsize -= cnt;
        ebi -= this_windowsize;

        while (this_windowsize > 1) {
            u = powmod1_redc_((u128)u * u, mod, ninv);
            u = powmod1_redc_((u128)u * u, mod, ninv);
            this_windowsize -= 2;
        }

        if (this_windowsize != 0)
            u = powmod1_redc_((u128)u * u, mod, ninv);
        u = powmod1_redc_((u128)u * pp[expbits >> 1], mod, ninv);
    }

done:
    /* 出蒙域：REDC(u * 1) = u*B^(-1) mod m */
    dst[0] = powmod1_redc_(u, mod, ninv);
    TEMP_FREE;
}

void lmmp_powmod_2_(
    mp_ptr    restrict dst,
    mp_srcptr restrict  bp,
    mp_srcptr restrict  ep,
    mp_size_t           en,
    mp_srcptr restrict mod
) {
    lmmp_param_assert(dst != NULL && bp != NULL && ep != NULL && mod != NULL);
    lmmp_param_assert(en > 0 && ep[en - 1] > 0);
    lmmp_param_assert(mod[0] % 2 == 1);
    lmmp_param_assert(mod[1] != 0);

    u128 n = _u128load(mod);
    if (_u128load(bp) == 0) {
        lmmp_zero(dst, 2);
        return;
    }
    mp_bitcnt_t ebi = count_bits(ep, en);
    /* 指数为 1：契约 [bp,2] < [mod,2]，直接拷贝即已取模 */
    if (ebi == 1) {
        lmmp_copy(dst, bp, 2);
        return;
    }

    unsigned windowsize = lmmp_powmod_win_size_(ebi);
    lmmp_debug_assert(windowsize < ebi);
    /* ninv = -m^(-1) mod B^2：u128 回绕取负 */
    mp_limb_t inv[2];
    lmmp_binvert_2_(inv, mod);
    u128 ninv = 0 - _u128load(inv);

    /* 进蒙域：x_m = b*B^2 mod m，一次 4/2 除法（低位补两个零 limb 实现 <<B^2） */
    u128 u;
    {
        mp_limb_t num[4] = {0, 0, bp[0], bp[1]};
        mp_limb_t r[2];
        lmmp_div_(NULL, r, num, 4, mod, 2);
        u = _u128load(r);
    }

    mp_bitcnt_t cnt;
    unsigned this_windowsize;
    mp_limb_t expbits;
    TEMP_DECL;
    u128* restrict pp = TALLOC_TYPE((mp_size_t)1 << (windowsize - 1), u128);

    pp[0] = u;
    if (windowsize > 1) {
        u128 b2 = mont2_sqr(u, n, ninv);
        for (mp_size_t i = 1; i < ((mp_size_t)1 << (windowsize - 1)); i++)
            pp[i] = mont2_mul_2(pp[i - 1], b2, n, ninv);

        expbits = getbits(ep, ebi, windowsize);
        ebi -= windowsize;

        ctz_shr_u64(expbits, expbits, cnt);
        ebi += cnt;

        u = pp[expbits >> 1];
    } else {
        --ebi;
    }

    while (ebi != 0) {
        while (getbit(ep, ebi) == 0) {
            u = mont2_sqr(u, n, ninv);
            if (--ebi == 0)
                goto done;
        }

        /* The next bit of the exponent is 1.  Now extract the largest block of
           bits <= windowsize, and such that the least significant bit is 1.  */

        expbits = getbits(ep, ebi, windowsize);
        this_windowsize = LMMP_MIN(windowsize, ebi);

        ctz_shr_u64(expbits, expbits, cnt);
        this_windowsize -= cnt;
        ebi -= this_windowsize;

        while (this_windowsize > 1) {
            u = mont2_sqr(u, n, ninv);
            u = mont2_sqr(u, n, ninv);
            this_windowsize -= 2;
        }

        if (this_windowsize != 0)
            u = mont2_sqr(u, n, ninv);
        u = mont2_mul_2(u, pp[expbits >> 1], n, ninv);
    }

done:
    /* 出蒙域：REDC(u * 1) = u*B^(-2) mod m */
    _u128store(dst, mont2_redc(0, u, n, ninv));
    TEMP_FREE;
}

void lmmp_powmod_odd_mont_(
    mp_ptr          restrict u,
    mp_srcptr       restrict bp,
    mp_srcptr       restrict ep,
    mp_size_t                 en,
    lmmp_mont_t* restrict     mc,
    mp_ptr          restrict b2,
    mp_ptr          restrict prod,
    mp_ptr          restrict pp
) {
    lmmp_param_assert(u != NULL && bp != NULL && ep != NULL && mc != NULL);
    lmmp_param_assert(b2 != NULL && prod != NULL && pp != NULL);
    lmmp_param_assert(en > 0 && ep[en - 1] > 0);

    mp_size_t n = mc->n;
    mp_bitcnt_t ebi, cnt;
    unsigned windowsize, this_windowsize;
    mp_limb_t expbits;

    ebi = count_bits(ep, en);

    /* 进蒙域：pp[0] = b*B^n mod m，一次 2n/n 除法（低位补 n 零 limb 实现
       <<B^n，比 RR=B^2n mod m 加乘加归约省约两个 M(n)） */
    lmmp_mont_redcify_(pp, bp, n, prod, mc->m);

    /* 指数为 1：梯子即入蒙域本体 */
    if (ebi == 1) {
        lmmp_copy(u, pp, n);
        return;
    }

    windowsize = lmmp_powmod_win_size_(ebi);

    if (windowsize > 1) {
        lmmp_debug_assert(windowsize < ebi);

        /* b2 = b^2*B^n mod m，用于生成奇次幂表 */
        lmmp_sqr_(prod, pp, n);
        lmmp_mont_redc_(b2, prod, mc);

        for (mp_size_t i = 1; i < ((mp_size_t)1 << (windowsize - 1)); i++) {
            lmmp_mul_(prod, pp + (i - 1) * n, n, b2, n);
            lmmp_mont_redc_(pp + i * n, prod, mc);
        }

        expbits = getbits(ep, ebi, windowsize);
        ebi -= windowsize;

        ctz_shr_u64(expbits, expbits, cnt);
        ebi += cnt;

        lmmp_copy(u, pp + n * (expbits >> 1), n);
    } else {
        lmmp_copy(u, pp, n);
        --ebi;
    }

    while (ebi != 0) {
        while (getbit(ep, ebi) == 0) {
            lmmp_sqr_(prod, u, n);
            lmmp_mont_redc_(u, prod, mc);
            if (--ebi == 0)
                return;
        }

        /* The next bit of the exponent is 1.  Now extract the largest block of
           bits <= windowsize, and such that the least significant bit is 1.  */

        expbits = getbits(ep, ebi, windowsize);
        this_windowsize = LMMP_MIN(windowsize, ebi);

        ctz_shr_u64(expbits, expbits, cnt);
        this_windowsize -= cnt;
        ebi -= this_windowsize;

        while (this_windowsize > 1) {
            lmmp_sqr_(prod, u, n);
            lmmp_mont_redc_(u, prod, mc);
            lmmp_sqr_(prod, u, n);
            lmmp_mont_redc_(u, prod, mc);
            this_windowsize -= 2;
        }

        if (this_windowsize != 0) {
            lmmp_sqr_(prod, u, n);
            lmmp_mont_redc_(u, prod, mc);
        }
        lmmp_mul_(prod, u, n, pp + n * (expbits >> 1), n);
        lmmp_mont_redc_(u, prod, mc);
    }
    /* 不出蒙域：[u,n] = b^e*B^n mod m < m（含规范化），供调用者蒙域内继续
       运算或 ±1 探测 */
}

void lmmp_powmod_odd_(
    mp_ptr    restrict dst,
    mp_srcptr restrict  bp,
    mp_srcptr restrict  ep,
    mp_size_t           en,
    mp_srcptr restrict  mp,
    mp_size_t            n
) {
    lmmp_param_assert(dst != NULL && bp != NULL && ep != NULL && mp != NULL);
    lmmp_param_assert(en > 0 && ep[en - 1] > 0);
    lmmp_param_assert(n > 0 && mp[n - 1] > 0);
    lmmp_param_assert(mp[0] % 2 == 1);

    /* 1/2 limb 特化：标量 Montgomery 梯子，免去通用路径的全部缓冲区与预处理 */
    if (n == 1) {
        lmmp_powmod_1_(dst, bp, ep, en, mp[0]);
        return;
    }
    if (n == 2 && mp[1] != 0) {
        lmmp_powmod_2_(dst, bp, ep, en, mp);
        return;
    }

    if (lmmp_zero_q_(bp, n)) {
        lmmp_zero(dst, n);
        return;
    }

    mp_bitcnt_t ebi = count_bits(ep, en);

    /* 指数为 1：契约 [bp,n] < [mp,n]，直接拷贝即已取模，免去全部蒙域预处理 */
    if (ebi == 1) {
        lmmp_copy(dst, bp, n);
        return;
    }

    TEMP_DECL;
    lmmp_mont_t mc;

    /*
       单块工作区分段（蒙域段在前，梯子段续后，段布局见 impl/powmod.h）：
         [蒙域段]     basecase 0 | 中层 6n | 折叠 6n+msz（lmmp_mont_need_）
         [u(n)]       梯子累加器
         [b2(n)]      表生成暂存
         [prod(2n)]   全积兼 redcify 工作区
         [pp(ppn)]    奇次幂表，ppn = n << (win-1)
     */
    unsigned win = lmmp_powmod_win_size_(ebi);
    mp_size_t mn = lmmp_mont_need_(n, 0);
    mp_ptr restrict arena = TALLOC_TYPE(mn + 4 * n + ((mp_size_t)n << (win - 1)), mp_limb_t);
    mp_ptr restrict u = arena + mn;
    mp_ptr restrict b2 = u + n;
    mp_ptr restrict prod = b2 + n;
    mp_ptr restrict pp = prod + 2 * n;

    lmmp_mont_init_(&mc, mp, n, arena, 0, prod);
    lmmp_powmod_odd_mont_(u, bp, ep, en, &mc, b2, prod, pp);

    /* 离开 Montgomery 域：r = REDC(u * 1) = u*B^(-n) mod m < m */
    lmmp_copy(prod, u, n);
    lmmp_zero(prod + n, n);
    lmmp_mont_redc_(u, prod, &mc);
    lmmp_copy(dst, u, n);

    lmmp_mont_free_(&mc);
    TEMP_FREE;
}

/*
    任意模数模幂：m 偶时以 2-adic 分解 + CRT 合成

    分解 m = 2^k * m_odd（gcd(2^k, m_odd) = 1），中国剩余定理给出唯一解：

        r == r1 (mod m_odd),  r == r2 (mod 2^k),  r < m

    其中 r1 = b^e mod m_odd 走 lmmp_powmod_odd_（b < m 不保证 b < m_odd，
    先一次除法归约），r2 = b^e mod 2^k 走 lmmp_powlo_ 后截取低 k 位。
    合成取 r = r1 + m_odd * t，代入 2^k 同余解出
    t = (r2 - r1) * m_odd^(-1) mod 2^k——t 截到 k 位即得
    r <= (m_odd-1) + m_odd*(2^k-1) = m - 1 < m，天然规范，免末次比较减。
    模逆只依赖 m_odd 的低 nb2 = ceil(k/64) 个 limb（截断输入做平衡 binvert），
    模 B^nb2 乘加链只在低 k 位上与模 2^k 严格一致。
    纯 2 幂模数（m_odd == 1）越过梯子与 CRT，退化为 powlo + 掩码，无临时分配。
    偶底数在 v2(b)*e >= k 时 2^k 部分恒为 0，跳过 powlo（en > 1 时
    e >= B > 64n >= k，n < 2^58 limb 已超出可实现规模）。
    工作区单块打包 + 段间接力：r1 直接落在 dst 低 nodd limb；归约底数段
    死后续作 r2；合成积跨 wrk+inv 段（mullo 后二者均死），见函数内注释。
*/
void lmmp_powmod_(
    mp_ptr    restrict dst,
    mp_srcptr restrict  bp,
    mp_srcptr restrict  ep,
    mp_size_t           en,
    mp_srcptr restrict  mp,
    mp_size_t            n
) {
    lmmp_param_assert(dst != NULL && bp != NULL && ep != NULL && mp != NULL);
    lmmp_param_assert(en > 0 && ep[en - 1] > 0);
    lmmp_param_assert(n > 0 && mp[n - 1] > 0);

    if (mp[0] % 2 == 1) {
        lmmp_powmod_odd_(dst, bp, ep, en, mp, n);
        return;
    }

    /* b = 0：正指数幂为 0；e = 1：契约 [bp,n] < [mp,n]，拷贝即已取模 */
    if (lmmp_zero_q_(bp, n)) {
        lmmp_zero(dst, n);
        return;
    }
    if (en == 1 && ep[0] == 1) {
        lmmp_copy(dst, bp, n);
        return;
    }

    /* 2-adic 赋值 k = v2(m)：z 个低零 limb，ctz 为首个非零 limb 的消零数
       （该 limb 为奇时 ctz == 0，k 恰为 limb 对齐值，无需掩码） */
    mp_size_t z = 0;
    while (mp[z] == 0) z++;
    mp_bitcnt_t ctz = lmmp_tailing_zeros_(mp[z]);
    mp_size_t nb2 = z + (ctz != 0); /* 2^k 部分的 limb 数 = ceil(k/64) */
    mp_bitcnt_t k = (mp_bitcnt_t)z * LIMB_BITS + ctz;
    mp_size_t nodd = n - z;

    /* 偶底数短路：v2(b)*e >= k 时 b^e mod 2^k == 0，跳过整个 powlo 梯子 */
    int r2_zero = 0;
    if ((bp[0] & 1) == 0) {
        mp_size_t zb = 0;
        while (bp[zb] == 0) zb++; /* b != 0 已在上方保证 */
        u128 v2b = (u128)(zb * LIMB_BITS + lmmp_tailing_zeros_(bp[zb]));
        r2_zero = en > 1 || v2b * ep[0] >= (u128)k;
    }

    /* 纯 2 幂（m == 2^k ⟺ nodd==1 且首个非零 limb 消零后恰为 1）：r 即
       powlo 低 k 位，掩码 + 高位补零即成，全程无临时分配 */
    if (nodd == 1 && (ctz != 0 ? mp[z] >> ctz == 1 : mp[z] == 1)) {
        if (r2_zero) {
            lmmp_zero(dst, n);
        } else {
            lmmp_powlo_(dst, bp, nb2, ep, en);
            if (ctz != 0)
                dst[nb2 - 1] &= ((mp_limb_t)1 << ctz) - 1;
            lmmp_zero(dst + nb2, n - nb2);
        }
        return;
    }

    TEMP_DECL;

    /*
       [modd | sbn ] m_odd 移位缓冲（ctz==0 时免配，直接用 mp+z 视图；
                      移位归一化只缩短 nodd，不影响后续段的偏移）
       [wrk  | wn  ] 归约底数 bmod → 2^k 部结果 r2 接力（powmod_odd_
                      返回后 bmod 已死）→ 合成积前段（mul 时 r2 已死）
       [inv  | nb2 ] 模逆 → 合成积后段接力（mullo 之后已死；因合成积
                      经 wrk 跨写本段，inv 不加 restrict）
       [t    | nb2 ] 截断乘输出（mullo 写、mul 读，与 inv/r2 并发，独占）
       合成积落在 [wrk, wn+nb2)（跨 wrk+inv 两段，mul 时均已死），且
       wn + nb2 >= nodd + nb2 恰好容纳
    */
    mp_size_t sbn = ctz != 0 ? nodd : 0;
    mp_size_t wn = LMMP_MAX(nodd, nb2);
    mp_ptr restrict wrkbase = TALLOC_TYPE(sbn + wn + 2 * nb2, mp_limb_t);
    mp_srcptr modd;
    if (ctz != 0) {
        lmmp_shr_(wrkbase, mp + z, nodd, (mp_size_t)ctz);
        nodd -= wrkbase[nodd - 1] == 0;
        modd = wrkbase;
    } else {
        modd = mp + z;
    }
    mp_ptr restrict wrk = wrkbase + sbn;
    mp_ptr inv = wrk + wn;
    mp_ptr restrict t = inv + nb2;

    /* 奇部 r1 = b^e mod m_odd 直接写入 dst 低 nodd limb（此后作为 r1 载体
       直至最终加法，省一块 nodd 缓冲与一次拷贝）；底数归约借用 wrk 段 */
    lmmp_div_(NULL, wrk, bp, n, modd, nodd);
    lmmp_powmod_odd_(dst, wrk, ep, en, modd, nodd);

    /* 2 部 r2 = b^e mod B^nb2（复用已死的归约底数段） */
    if (r2_zero)
        lmmp_zero(wrk, nb2);
    else
        lmmp_powlo_(wrk, bp, nb2, ep, en);

    /* t = (r2 - r1) * m_odd^(-1) mod B^nb2，再截到 k 位 */
    lmmp_binvert_(inv, modd, LMMP_MIN(nodd, nb2), nb2);

    mp_size_t mn = LMMP_MIN(nb2, nodd);
    mp_limb_t bo = lmmp_sub_n_(wrk, wrk, dst, mn); /* r1 载于 dst */
    if (mn < nb2)
        (void)lmmp_sub_1_(wrk + mn, wrk + mn, nb2 - mn, bo); /* 借位在 B^nb2 窗内传播 */

    lmmp_mullo_(t, inv, wrk, nb2);
    if (ctz != 0)
        t[nb2 - 1] &= ((mp_limb_t)1 << ctz) - 1;

    /* r = r1 + m_odd * t < m：积写入 [wrk, wn+nb2)（wrk/inv 段此刻均死），
       积 < m < B^n（nodd+nb2 <= n+1 时顶 limb 必为 0），dst 高位补零后
       整体相加，进位必为 0。lmmp_mul_ 大操作数在前 */
    if (nb2 > nodd)
        lmmp_mul_(wrk, t, nb2, modd, nodd);
    else
        lmmp_mul_(wrk, modd, nodd, t, nb2);
    lmmp_zero(dst + nodd, n - nodd);
    (void)lmmp_add_n_(dst, dst, wrk, n);
    TEMP_FREE;
}
