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

    记 R = B^n，m 为奇模数（n limb），ninv = -m^(-1) mod B^n。
    单步 REDC：给定 t = a*b < m*B^n（[tp,2n]），令

        q = t_lo * ninv mod B^n        （q*m ≡ -t_lo (mod B^n)）

    则 (t + q*m)/B^n ≡ t*R^(-1) (mod m) 且 < 2m。低半部分 t_lo + (q*m)_lo
    恰为 B^n*[t_lo!=0]，无需逐 limb 相加，故

        u = t_hi + mulhi(q, m) + [t_lo!=0]

    高半积 mulhi(q,m) 的三种来源（按规模分层）：
      1. basecase（n < REDC_BASECASE_THRESHOLD，实测 41）：链式 Hensel 归约——
         n 次 addmul_1 逐 limb 消零（q_j = up[0]*ninv1 mod B），成本约一次
         basecase 乘法，且只需单 limb 逆元（对标 GMP mpn_redc_1，其 clobber
         输入的特性见各调用点：梯子内直接破坏 prod，公开 lmmp_redc_ 为维持
         tp 只读契约先复制一份）；
      2. 全积取高半：lmmp_mul_n_ 后读高 n limb（中尺寸段）；
      3. 梅森折叠（n >= REDC_MERSENNE_THRESHOLD，实测 309）：由于 (q*m) mod B^n
         = -t_lo mod B^n 是已知量 L，与 binvert_mulhi_ 同构——先算 V = q*m
         mod (B^msz-1)（msz 为 admissible 尺寸，模数 m 一侧的变换全程缓存
         复用），从 V 中减去 L 后旋转载出高半。两操作数均 < B^n 保证
         hi <= B^n-2，拼接表示不会落在 B^msz-1 的二义点上；而 L==0 强制
         q==0、积为零，也不与零类的非规范表示冲突。
    入蒙域用一次 (b*B^n) mod m 除法（GMP redcify 模式），比 RR=B^2n mod m
    加乘加归约的旧路径省约两个 M(n) 的预处理；basecase 层连全长 binvert
    也一并省去（仅需低 limb 逆元）。
*/

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/mul_cache.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


static inline mp_size_t win_size(mp_size_t eb) {
    mp_size_t k;
    static mp_bitcnt_t x[] = {7, 25, 81, 241, 673, 1793, 4609, 11521, 28161, ~(mp_bitcnt_t)0};
    for (k = 0; eb > x[k++];);
    return k;
}

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

/**
 * @brief 从梅森折叠积中提取高半积（低位已知变体的 mulhi）
 * @param hi 结果指针（n 个limb），接收 [q,n]*[mp,n] div B^n
 * @param V 输入兼工作区（msz 个limb），入口为 q*m mod B^msz-1，出口被破坏
 * @param L 已知低位（n 个limb），满足 q*m ≡ L (mod B^n)，此处 L = -t_lo mod B^n
 * @param n 操作数长度
 * @param msz 折叠尺寸，admissible 且 n <= msz < 2n
 * @warning sep(hi,V), V 的 msz-n 个高位 limb 亦会被读写
 * @note 设 P = q*m = hi*B^n + L，则 W = (V-L) mod B^msz-1 = hi*B^n mod B^msz-1，
 *       即 hi 的第 j limb 恰位于 W[(j+n) mod msz]，旋转载出即得
 */
static void powmod_fold_hi_(
    mp_ptr    restrict hi,
    mp_ptr    restrict V,
    mp_srcptr restrict L,
    mp_size_t             n,
    mp_size_t             msz
) {
    mp_size_t fn = msz - n; /* hi 中线性对位部分长度（取自 V+n） */
    mp_size_t sn = n - fn;  /* hi 中环绕部分长度（取自 V 头部） */
    lmmp_debug_assert(msz >= n && 2 * n > msz);

    mp_limb_t bor = lmmp_sub_n_(V, V, L, n);
    if (bor) {
        /* 数组当前值 = V - L + B^n（借位等价于吸收在第 n 个 limb 上） */
        if (msz > n)
            bor = lmmp_sub_1_(V + n, V + n, msz - n, 1);
        /* 若高位段借位（此时 V < L，高位段必为全零，减 1 后变全 1），
           或 msz==n（借位即 V<L），整体再减 1 补回 B^msz-1 */
        if (msz == n || bor)
            lmmp_dec(V);
    }
    lmmp_copy(hi, V + n, fn);
    lmmp_copy(hi + fn, V, sn);
}

/**
 * @brief basecase REDC：链式 Hensel 归约，逐 limb 消零（对标 GMP mpn_redc_1）
 * @param dst 结果指针（n 个limb），接收 ([up,2n] + q*m)/B^n 的低 n limb
 * @param up 输入兼工作区（2n 个limb，出口被破坏）
 * @param m 模数（n 个limb）
 * @param n 操作数长度
 * @param ninv1 -m^(-1) mod B（单 limb）
 * @warning sep(dst,up), n < REDC_BASECASE_THRESHOLD 时才是优选路径
 * @return 进位（[0|1]），返回值:[dst,n] 即 REDC 结果 < 2m（t < B^n*m 时）
 * @note 每轮 q = up[0]*ninv1 mod B 使 up[0] 恰好归零，addmul_1 的进位
 *       存入刚清零的 limb；n 轮后高半与累积进位的低半相加即得结果。
 *       总成本 n 次 addmul_1 ≈ 一次 basecase 乘法
 */
static inline mp_limb_t powmod_redc_basecase_(
    mp_ptr    restrict dst,
    mp_ptr    restrict up,
    mp_srcptr restrict m,
    mp_size_t             n,
    mp_limb_t             ninv1
) {
    mp_ptr restrict up0 = up;
    for (mp_size_t j = n; j > 0; j--) {
        mp_limb_t cy = lmmp_addmul_1_(up, m, n, up[0] * ninv1);
        lmmp_debug_assert(up[0] == 0);
        up[0] = cy;
        up++;
    }
    return lmmp_add_n_(dst, up0 + n, up0, n);
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
        mp_limb_t cy = powmod_redc_basecase_(dst, up, mp, n, ninv[0]);
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
        powmod_fold_hi_(hi, V, L, n, msz);
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

    unsigned windowsize = win_size(ebi);
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

    unsigned windowsize = win_size(ebi);
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

/* REDC 梯子上下文：预分配全部工作区，并缓存 m / ninv 一侧的 FFT 变换 */
typedef struct {
    mp_size_t n;
    mp_srcptr m;     /* 模数 */
    mp_srcptr ninv;  /* -m^(-1) mod B^n（basecase 层不使用，为 NULL） */
    mp_limb_t ninv1; /* -m^(-1) mod B（basecase 层专用） */
    int fold;        /* n >= REDC_MERSENNE_THRESHOLD 时走梅森折叠 */
    mp_size_t msz;   /* 折叠尺寸（fold 时有效） */
    mp_ptr q;        /* [n]        q = t_lo * ninv mod B^n */
    mp_ptr Lbuf;     /* [n]        -t_lo mod B^n */
    mp_ptr hi;       /* [n]        q*m 的高 n limb（折叠路径输出） */
    mp_ptr mulhi;    /* [2n]       全积路径的 q*m（非折叠路径） */
    mp_ptr V;        /* [msz]      折叠积 q*m mod B^msz-1 */
    mp_ptr mscratch; /* [2n]       mullo scratch */
    fft_gr_cache mcache;
    int mcache_on;
    fft_mullo_cache ncache;
    int ncache_on;
} powmod_redc_t;

/**
 * @brief REDC 单步并规范化：[dst,n] = (t + q*m)/B^n mod m，结果 < m
 * @param dst 结果指针（n 个limb）
 * @param tp 被归约数兼工作区（2n 个limb，t < B^n*[m,n]，出口被破坏）
 * @param rc 梯子上下文
 * @warning sep(dst,tp), dst 与 rc 内各缓冲区均分离
 */
static void powmod_redc_(mp_ptr restrict dst, mp_ptr restrict tp, powmod_redc_t* restrict rc) {
    mp_size_t n = rc->n;
    mp_ptr restrict hi;

    if (n < REDC_BASECASE_THRESHOLD) {
        mp_limb_t cy = powmod_redc_basecase_(dst, tp, rc->m, n, rc->ninv1);
        if (cy) {
            /* 值 >= B^n > m，减 m 必然可行；借位恰好抵消进位 limb */
            (void)lmmp_sub_n_(dst, dst, rc->m, n);
        } else if (lmmp_cmp_(dst, rc->m, n) >= 0) {
            lmmp_sub_n_(dst, dst, rc->m, n);
        }
        return;
    }

    mp_limb_t carry = !lmmp_zero_q_(tp, n);

    if (n < MULLO_DC_THRESHOLD) {
        lmmp_mullo_dc_(rc->q, tp, rc->ninv, rc->mscratch, n);
    } else if (rc->ncache_on == 0) {
        lmmp_mullo_fft_cache_init_(rc->q, tp, rc->ninv, n, rc->mscratch, &rc->ncache);
        rc->ncache_on = 1;
    } else {
        lmmp_mullo_fft_cache_(rc->q, tp, rc->mscratch, &rc->ncache);
    }

    if (rc->fold == 0) {
        lmmp_mul_n_(rc->mulhi, rc->q, rc->m, n);
        hi = rc->mulhi + n;
    } else {
        if (rc->mcache_on == 0) {
            lmmp_mul_mersenne_cache_init_(rc->V, rc->msz, rc->q, n, rc->m, n, &rc->mcache);
            rc->mcache_on = 1;
        } else {
            lmmp_mul_mersenne_cache_(rc->V, rc->q, &rc->mcache);
        }
        if (carry) {
            lmmp_not_(rc->Lbuf, tp, n);
            lmmp_inc(rc->Lbuf);
        } else {
            lmmp_zero(rc->Lbuf, n);
        }
        powmod_fold_hi_(rc->hi, rc->V, rc->Lbuf, n, rc->msz);
        hi = rc->hi;
    }

    /* u = t_hi + hi + carry，结果 carry_out:[dst,n] < 2m */
    mp_limb_t cy = lmmp_add_nc_(dst, tp + n, hi, n, carry);
    if (cy) {
        /* 值 >= B^n > m，减 m 必然可行；n limb 减法的借位恰好抵消进位 limb */
        (void)lmmp_sub_n_(dst, dst, rc->m, n);
    } else if (lmmp_cmp_(dst, rc->m, n) >= 0) {
        lmmp_sub_n_(dst, dst, rc->m, n);
    }
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

    mp_bitcnt_t ebi, cnt;
    unsigned windowsize, this_windowsize;
    mp_limb_t expbits;
    mp_ptr restrict pp;
    TEMP_DECL;

    ebi = count_bits(ep, en);

    /* 指数为 1：契约 [bp,n] < [mp,n]，直接拷贝即已取模，免去全部蒙域预处理 */
    if (ebi == 1) {
        lmmp_copy(dst, bp, n);
        TEMP_FREE;
        return;
    }

    windowsize = win_size(ebi);

    powmod_redc_t rc;
    rc.n = n;
    rc.m = mp;
    rc.fold = n >= REDC_MERSENNE_THRESHOLD;
    rc.msz = rc.fold ? lmmp_fft_next_size_((2 * n + 1) >> 1) : 0;
    rc.mcache_on = 0;
    rc.ncache_on = 0;
    if (rc.fold)
        lmmp_debug_assert(2 * n > rc.msz && rc.msz >= n);

    mp_ptr restrict ninv = NULL;                   /* -m^(-1) mod B^n（非 basecase 层） */
    mp_ptr restrict u = TALLOC_TYPE(n, mp_limb_t);
    mp_ptr restrict b2 = TALLOC_TYPE(n, mp_limb_t);
    mp_ptr restrict prod = TALLOC_TYPE(2 * n, mp_limb_t);
    rc.q = TALLOC_TYPE(n, mp_limb_t);
    rc.Lbuf = TALLOC_TYPE(n, mp_limb_t);
    rc.hi = TALLOC_TYPE(n, mp_limb_t);
    rc.mscratch = TALLOC_TYPE(2 * n, mp_limb_t);
    rc.mulhi = rc.fold ? NULL : TALLOC_TYPE(2 * n, mp_limb_t);
    rc.V = rc.fold ? TALLOC_TYPE(rc.msz, mp_limb_t) : NULL;
    pp = TALLOC_TYPE((mp_size_t)n << (windowsize - 1), mp_limb_t);

    if (n >= REDC_BASECASE_THRESHOLD) {
        ninv = TALLOC_TYPE(n, mp_limb_t);
        rc.ninv = ninv;
        /* ninv = -m^(-1) mod B^n：m 奇故其逆亦奇，取反加一不会越界 */
        lmmp_binvert_(ninv, mp, n, n);
        lmmp_not_(ninv, ninv, n);
        lmmp_inc(ninv);
    } else {
        rc.ninv = NULL;
        rc.ninv1 = 0 - lmmp_binvert_ulong_(mp[0]);
    }

    /* 进蒙域：x_m = b*B^n mod m，一次 2n/n 除法（GMP redcify 模式）。
       低位补 n 个零 limb 实现 <<B^n，比 RR=B^2n mod m 加乘加归约省约两个 M(n) */
    lmmp_zero(prod, n);
    lmmp_copy(prod + n, bp, n);
    lmmp_div_(NULL, pp, prod, 2 * n, mp, n);

    if (windowsize > 1) {
        lmmp_debug_assert(windowsize < ebi);

        /* b2 = b^2*B^n mod m，用于生成奇次幂表 */
        lmmp_sqr_(prod, pp, n);
        powmod_redc_(b2, prod, &rc);

        for (mp_size_t i = 1; i < ((mp_size_t)1 << (windowsize - 1)); i++) {
            lmmp_mul_(prod, pp + (i - 1) * n, n, b2, n);
            powmod_redc_(pp + i * n, prod, &rc);
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
            powmod_redc_(u, prod, &rc);
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
            lmmp_sqr_(prod, u, n);
            powmod_redc_(u, prod, &rc);
            lmmp_sqr_(prod, u, n);
            powmod_redc_(u, prod, &rc);
            this_windowsize -= 2;
        }

        if (this_windowsize != 0) {
            lmmp_sqr_(prod, u, n);
            powmod_redc_(u, prod, &rc);
        }
        lmmp_mul_(prod, u, n, pp + n * (expbits >> 1), n);
        powmod_redc_(u, prod, &rc);
    }

done:
    /* 离开 Montgomery 域：r = REDC(u * 1) = u*B^(-n) mod m < m */
    lmmp_copy(prod, u, n);
    lmmp_zero(prod + n, n);
    powmod_redc_(u, prod, &rc);
    lmmp_copy(dst, u, n);

    if (rc.mcache_on)
        lmmp_fft_gr_cache_free_(&rc.mcache);
    if (rc.ncache_on)
        lmmp_mullo_cache_free_(&rc.ncache);
    TEMP_FREE;
}
