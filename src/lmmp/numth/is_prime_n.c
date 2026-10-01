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
    大整数素性检验（n > 2 limb）：单轮 Rabin-Miller 强伪素数测试（基底 2
    特化）、强 Lucas-Selfridge 测试与强度分档入口 lmmp_is_prime_n_。

    记 n = [np,nn]（奇，n > 2^128）。MR 单轮：n-1 = d*2^t（d 奇），梯子计算
    蒙域 y = b^d，判 y ∈ {1,-1} 或 y^(2^j) = -1（0 < j < t）。基底 2 特化：
    蒙域乘 2 即剩余加倍（2*R ≡ 倍加），逢 1 指数位以 O(n) 加法替代一次
    M(n) 蒙乘，单轮约省 1/4 ~ 1/3。强 Lucas（Selfridge 方法 A）：P = 1，
    Q = (1-D)/4，D 取 5,-7,9,-11,... 中首个 (D|n) = -1 者；V 阶梯判据与
    128 位版（is_prime_2.c）一致：
      V_d = 0，或 V_d^2 = 4Q^d，或 V_{d*2^r} = 0（0 < r < s），d*2^s = n+1
    （gcd(D,n)=1 下与 U/V 强 Lucas 判据等价，U_d = 0 经恒等式
    V_d^2 - D*U_d^2 = 4Q^d 转为 V_d^2 = 4Q^d）。

    Montgomery 核与 powmod.c 同源（R = B^n，三层 REDC：basecase 链式
    Hensel 归约 / 全积取高半 / 梅森折叠，含 FFT 变换缓存），按项目惯例
    按需复制为 static（见 powmod.c 头注；is_prime_2.c 的标量蒙域同例）。
    进蒙域走 redcify 除法（x*B^n mod n），one = B^n mod n 为各测试公共的
    比较基准，m1 = m - one 即 -1 的蒙域剩余。

    强度分档（strength ∈ [0,7]，随机基底取自全局 RNG，[2,n-2] 均匀）：
        档 | 基底2 MR  | 随机基底 MR（至多）   | 强 Lucas | 试除上界
        0  |    √     |         4          |    -     |    100
        1  |    √     |         5          |    -     |    300
        2  |    √     |         6          |    -     |   1000
        3  |    √     |         8          |    -     |   3000
        4  |    √     |         -          |    √     |   1000
        5  |    √     |         2          |    √     |   3000
        6  |    √     |         4          |    √     |   5000
        7  |    √     |         6          |    √     |  10000
    各档均先做基底 2 特化 MR；"至多 N 轮"指任一轮检出合数即提前终止，
    实际执行轮数不超过 N。试除上界随强度递增：高强度下幸存者代价更大
    （更多轮次/含 Lucas），更深的初筛以近零代价换更高的提前淘汰率，
    上界量级与 GMP probab_prime_p 的常用配置对齐。
*/

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/mul_cache.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/mprand.h"

/* ============ 强度分档参数 ============ */

/* 随机基底 MR 轮数上限（检出合数提前终止） */
static const uchar ipn_rnd_rounds[8] = {4, 5, 6, 8, 0, 2, 4, 6};

/* 小素数试除上界（N 传给 lmmp_trialdiv_，即试除 <= N 的全部素数） */
static const ushort ipn_trial_bound[8] = {100, 300, 1000, 3000, 1000, 3000, 5000, 10000};

typedef struct {
    mp_size_t n;
    mp_srcptr m;     /* 模数 */
    mp_ptr ninv;     /* -m^(-1) mod B^n（basecase 层不使用，为 NULL） */
    mp_limb_t ninv1; /* -m^(-1) mod B（basecase 层专用） */
    int fold;        /* n >= REDC_MERSENNE_THRESHOLD 时走梅森折叠 */
    mp_size_t msz;   /* 折叠尺寸（fold 时有效） */
    mp_ptr one;      /* [n] B^n mod m（蒙域 1） */
    mp_ptr m1;       /* [n] m - one（蒙域 -1） */
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
} ipn_mont_t;

/**
 * @brief 从梅森折叠积中提取高半积（低位已知变体的 mulhi），与 powmod.c
 *        的 powmod_fold_hi_ 同源
 * @param hi 结果指针（n 个limb），接收 [q,n]*[mp,n] div B^n
 * @param V 输入兼工作区（msz 个limb），入口为 q*m mod B^msz-1，出口被破坏
 * @param L 已知低位（n 个limb），满足 q*m ≡ L (mod B^n)，此处 L = -t_lo mod B^n
 * @param n 操作数长度
 * @param msz 折叠尺寸，admissible 且 n <= msz < 2n
 * @warning sep(hi,V), V 的 msz-n 个高位 limb 亦会被读写
 * @note 设 P = q*m = hi*B^n + L，则 W = (V-L) mod B^msz-1 = hi*B^n mod B^msz-1，
 *       即 hi 的第 j limb 恰位于 W[(j+n) mod msz]，旋转载出即得
 */
static void ipn_fold_hi_(
    mp_ptr    restrict hi,
    mp_ptr    restrict  V,
    mp_srcptr restrict  L,
    mp_size_t           n,
    mp_size_t         msz
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
 * @brief basecase REDC：链式 Hensel 归约，逐 limb 消零（对标 GMP mpn_redc_1），
 *        与 powmod.c 的 powmod_redc_basecase_ 同源
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
static inline mp_limb_t ipn_redc_basecase_(
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

/**
 * @brief REDC 单步并规范化：[dst,n] = (t + q*m)/B^n mod m，结果 < m，
 *        与 powmod.c 的 powmod_redc_ 同源
 * @param dst 结果指针（n 个limb）
 * @param tp 被归约数兼工作区（2n 个limb，t < B^n*[m,n]，出口被破坏）
 * @param mc 蒙域上下文
 * @warning sep(dst,tp), dst 与 mc 内各缓冲区均分离
 */
static void ipn_redc_(mp_ptr restrict dst, mp_ptr restrict tp, ipn_mont_t* restrict mc) {
    mp_size_t n = mc->n;
    mp_ptr restrict hi;

    if (n < REDC_BASECASE_THRESHOLD) {
        mp_limb_t cy = ipn_redc_basecase_(dst, tp, mc->m, n, mc->ninv1);
        if (cy) {
            /* 值 >= B^n > m，减 m 必然可行；借位恰好抵消进位 limb */
            (void)lmmp_sub_n_(dst, dst, mc->m, n);
        } else if (lmmp_cmp_(dst, mc->m, n) >= 0) {
            lmmp_sub_n_(dst, dst, mc->m, n);
        }
        return;
    }

    mp_limb_t carry = !lmmp_zero_q_(tp, n);

    if (n < MULLO_DC_THRESHOLD) {
        lmmp_mullo_dc_(mc->q, tp, mc->ninv, mc->mscratch, n);
    } else if (mc->ncache_on == 0) {
        lmmp_mullo_fft_cache_init_(mc->q, tp, mc->ninv, n, mc->mscratch, &mc->ncache);
        mc->ncache_on = 1;
    } else {
        lmmp_mullo_fft_cache_(mc->q, tp, mc->mscratch, &mc->ncache);
    }

    if (mc->fold == 0) {
        lmmp_mul_n_(mc->mulhi, mc->q, mc->m, n);
        hi = mc->mulhi + n;
    } else {
        if (mc->mcache_on == 0) {
            lmmp_mul_mersenne_cache_init_(mc->V, mc->msz, mc->q, n, mc->m, n, &mc->mcache);
            mc->mcache_on = 1;
        } else {
            lmmp_mul_mersenne_cache_(mc->V, mc->q, &mc->mcache);
        }
        if (carry) {
            lmmp_not_(mc->Lbuf, tp, n);
            lmmp_inc(mc->Lbuf);
        } else {
            lmmp_zero(mc->Lbuf, n);
        }
        ipn_fold_hi_(mc->hi, mc->V, mc->Lbuf, n, mc->msz);
        hi = mc->hi;
    }

    /* u = t_hi + hi + carry，结果 carry_out:[dst,n] < 2m */
    mp_limb_t cy = lmmp_add_nc_(dst, tp + n, hi, n, carry);
    if (cy) {
        /* 值 >= B^n > m，减 m 必然可行；n limb 减法的借位恰好抵消进位 limb */
        (void)lmmp_sub_n_(dst, dst, mc->m, n);
    } else if (lmmp_cmp_(dst, mc->m, n) >= 0) {
        lmmp_sub_n_(dst, dst, mc->m, n);
    }
}

/**
 * @brief 蒙域上下文的分段工作区长度（见 ipn_mont_init_ 的布局说明）
 * @param n 模数 limb 长度
 * @return 蒙域段所需的 limb 数
 */
static mp_size_t ipn_mont_need_(mp_size_t n) {
    if (n < REDC_BASECASE_THRESHOLD) return 2 * n;
    if (n >= REDC_MERSENNE_THRESHOLD) return 8 * n + lmmp_fft_next_size_((2 * n + 1) >> 1);
    return 8 * n;
}

/**
 * @brief 进蒙域：[dst,n] = [xp,n]*B^n mod [mp,n]（GMP redcify 除法模式）
 * @param dst 结果指针（n 个limb）
 * @param xp 普通域输入（n 个limb，eqsep(dst,xp)）
 * @param n 操作数长度
 * @param prod 工作区（2n 个limb，出口被破坏，sep(dst,xp,mp)）
 * @param mp 模数（n 个limb）
 * @warning n>0, mp[0]%2==1, mp[n-1]>0, sep([prod|dst],mp)
 */
static void ipn_redcify_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict   xp,
    mp_size_t             n,
    mp_ptr    restrict prod,
    mp_srcptr restrict   mp
) {
    lmmp_zero(prod, n);
    lmmp_copy(prod + n, xp, n);
    lmmp_div_(NULL, dst, prod, 2 * n, mp, n);
}

/*
    蒙域初始化：在 arena 头部切分 REDC 工作段，并预计算 one 与 m1。
    分段布局（随规模裁剪）：
      basecase（n < REDC_BASECASE_THRESHOLD）：
          one(n) | m1(n)                                   == 2n
      中层：另配 ninv(n) | q(n) | mscratch(2n) | mulhi(2n)   == 8n
      折叠层：mulhi(2n) 换 Lbuf(n) | hi(n) | V(msz)         == 8n + msz
    one/m1 之后的段与调用者的梯子段续接（单块打包），prod 为调用者提供的
    [2n] redcify 工作区（与 arena 分段分离）
*/
static void ipn_mont_init_(
    ipn_mont_t*        mc,
    mp_srcptr          mp,
    mp_size_t           n,
    mp_ptr          arena,
    mp_ptr restrict  prod
) {
    mc->n = n;
    mc->m = mp;
    mc->fold = n >= REDC_MERSENNE_THRESHOLD;
    mc->msz = mc->fold ? lmmp_fft_next_size_((2 * n + 1) >> 1) : 0;
    mc->mcache_on = 0;
    mc->ncache_on = 0;
    if (mc->fold)
        lmmp_debug_assert(2 * n > mc->msz && mc->msz >= n);

    mc->one = arena;
    mc->m1 = arena + n;
    if (n < REDC_BASECASE_THRESHOLD) {
        mc->ninv = NULL;
        mc->ninv1 = 0 - lmmp_binvert_ulong_(mp[0]);
        mc->q = mc->Lbuf = mc->hi = mc->mscratch = mc->mulhi = mc->V = NULL;
    } else {
        mc->ninv1 = 0;
        mc->ninv = arena + 2 * n;
        mc->q = arena + 3 * n;
        mc->mscratch = arena + 4 * n;
        if (mc->fold) {
            mc->Lbuf = arena + 6 * n;
            mc->hi = arena + 7 * n;
            mc->V = arena + 8 * n;
            mc->mulhi = NULL;
        } else {
            mc->mulhi = arena + 6 * n;
            mc->Lbuf = mc->hi = mc->V = NULL;
        }
        /* ninv = -m^(-1) mod B^n：m 奇故其逆亦奇，取反加一不会越顶 */
        lmmp_binvert_(mc->ninv, mp, n, n);
        lmmp_not_(mc->ninv, mc->ninv, n);
        lmmp_inc(mc->ninv);
    }

    /* one = B^n mod m：低位补 n 零 limb 实现 <<B^n，除法取余（m1 段先
       兼作被除数的高半载体） */
    lmmp_zero(mc->m1, n);
    mc->m1[0] = 1;
    ipn_redcify_(mc->one, mc->m1, n, prod, mp);
    /* m1 = m - one（one != 0 恒成立，无借位） */
    (void)lmmp_sub_n_(mc->m1, mp, mc->one, n);
}

/* 释放蒙域上下文的 FFT 变换缓存（TEMP 工作区由调用者 TEMP_FREE 统一回收） */
static inline void ipn_mont_free_(ipn_mont_t* mc) {
    if (mc->mcache_on)
        lmmp_fft_gr_cache_free_(&mc->mcache);
    if (mc->ncache_on)
        lmmp_mullo_cache_free_(&mc->ncache);
}

/* 蒙域加倍 x+x-m。值 < 2m：进位回绕（2x >= B^n）时 B^n 与 m 的差额由
   减法借位在 B^n 下恰巧抵消，回绕结果即 2x-m（见 is_prime_2.c mont2_dbl）*/
static inline void ipn_mont_dbl_(mp_ptr dst, mp_srcptr x, ipn_mont_t* restrict mc) {
    mp_limb_t c = lmmp_add_n_(dst, x, x, mc->n);
    if (c)
        (void)lmmp_sub_n_(dst, dst, mc->m, mc->n);
    else if (lmmp_cmp_(dst, mc->m, mc->n) >= 0)
        lmmp_sub_n_(dst, dst, mc->m, mc->n);
}

/* 蒙域模减 a-b（a,b < m）：借位回绕由加 m 补偿，结果 < m */
static inline void ipn_mont_sub_(mp_ptr dst, mp_srcptr a, mp_srcptr b, ipn_mont_t* restrict mc) {
    mp_limb_t bo = lmmp_sub_n_(dst, a, b, mc->n);
    if (bo)
        (void)lmmp_add_n_(dst, dst, mc->m, mc->n);
}

/* 蒙域取负 m-x（x != 0，结果 < m） */
static inline void ipn_mont_neg_(mp_ptr dst, mp_srcptr x, ipn_mont_t* restrict mc) {
    (void)lmmp_sub_n_(dst, mc->m, x, mc->n);
}

/* d 低位起 0 的个数；调用前应保证 [dp,n] != 0 */
static inline mp_bitcnt_t ipn_ctz_(mp_srcptr dp) {
    mp_size_t z = 0;
    while (dp[z] == 0) z++;
    return (mp_bitcnt_t)z * LIMB_BITS + lmmp_tailing_zeros_(dp[z]);
}

/* 规范化长度（自顶去零） */
static inline mp_size_t ipn_norm_(mp_srcptr dp, mp_size_t dn) {
    while (dn > 0 && dp[dn - 1] == 0) dn--;
    return dn;
}

/*
    蒙域 L2R 梯子：u = b^d。通用基底逐位"平方 + 蒙乘 bm"；基底 2 特化为
    "平方 + 倍加"（蒙域乘 2 = 剩余加倍，O(n) 替代 M(n) 蒙乘，蒙域 2 由
    one 倍加而来，免去 redcify 除法）。二次探测：y ∈ {1,-1} 或
    y^(2^j) = -1（0 < j < t）
*/
int lmmp_is_sprp_(mp_srcptr np, mp_size_t nn, mp_srcptr bp) {
    lmmp_param_assert(np != NULL && bp != NULL);
    lmmp_param_assert(nn > 2 && np[nn - 1] > 0);
    lmmp_param_assert(np[0] % 2 == 1);

    int is2 = (bp[0] == 2) && lmmp_zero_q_(bp + 1, nn - 1);
    TEMP_DECL;
    ipn_mont_t mc;

    /*
       单块工作区分段（蒙域段在前，调用者段续后）：
         [prod(2n)]   平方/全积兼 redcify 被除数
         [nm1(n)]     n-1，低 z 零 limb 折叠为指针偏移后即 d 所在段
         [u(n)]       梯子累加器
         [bm(n)]      蒙域基底（is2 时免配）
       蒙域段：basecase 2n | 中层 8n | 折叠 8n+msz（ipn_mont_need_）
    */
    mp_size_t mn = ipn_mont_need_(nn);
    mp_ptr restrict arena = TALLOC_TYPE(mn + (is2 ? 4 : 5) * nn, mp_limb_t);
    mp_ptr restrict prod = arena + mn;
    mp_ptr restrict nm1 = prod + 2 * nn;
    mp_ptr restrict u = nm1 + nn;
    mp_ptr restrict bm = u + nn;

    ipn_mont_init_(&mc, np, nn, arena, prod);

    /* n-1 = d*2^t（d 奇）：z 个低零 limb 折叠为偏移，余位单次原地 shr */
    lmmp_copy(nm1, np, nn);
    lmmp_dec(nm1);
    mp_bitcnt_t t = ipn_ctz_(nm1);
    mp_ptr d = nm1 + t / LIMB_BITS;
    mp_size_t dn = ipn_norm_(d, nn - t / LIMB_BITS);
    lmmp_shr_(d, d, dn, t % LIMB_BITS);
    dn = ipn_norm_(d, dn);
    mp_bitcnt_t dbits = (dn - 1) * LIMB_BITS + lmmp_limb_bits_(d[dn - 1]);

    if (is2) {
        ipn_mont_dbl_(u, mc.one, &mc);
        for (mp_bitcnt_t i = dbits - 1; i-- > 0;) {
            lmmp_sqr_(prod, u, nn);
            ipn_redc_(u, prod, &mc);
            if ((d[i / LIMB_BITS] >> (i % LIMB_BITS)) & 1)
                ipn_mont_dbl_(u, u, &mc);
        }
    } else {
        ipn_redcify_(bm, bp, nn, prod, np);
        lmmp_copy(u, bm, nn);
        for (mp_bitcnt_t i = dbits - 1; i-- > 0;) {
            lmmp_sqr_(prod, u, nn);
            ipn_redc_(u, prod, &mc);
            if ((d[i / LIMB_BITS] >> (i % LIMB_BITS)) & 1) {
                lmmp_mul_(prod, u, nn, bm, nn);
                ipn_redc_(u, prod, &mc);
            }
        }
    }

    /* 二次探测：y ∈ {1,-1} 或 y^(2^j) = -1（0 < j < t，共 t-1 次平方） */
    int ret = 0;
    if (lmmp_cmp_(u, mc.one, nn) == 0 || lmmp_cmp_(u, mc.m1, nn) == 0) {
        ret = 1;
    } else {
        for (; t > 1; t--) {
            lmmp_sqr_(prod, u, nn);
            ipn_redc_(u, prod, &mc);
            if (lmmp_cmp_(u, mc.m1, nn) == 0) {
                ret = 1;
                break;
            }
        }
    }

    ipn_mont_free_(&mc);
    TEMP_FREE;
    return ret;
}

/* (b|A) 二进制算法：A 奇，b < A（移位/比较/减法，几次迭代），
   与 is_prime_2.c 的 jacobi_small 同源 */
static inline int ipn_jacobi_small_(ulong b, ulong A) {
    int sign = 1;
    if (b == 0) return 0;
    for (;;) {
        int tz = lmmp_tailing_zeros_(b);
        b >>= tz;
        if ((tz & 1) && ((A & 7) == 3 || (A & 7) == 5)) sign = -sign;
        if (b == 1) return sign;
        if (b < A) {
            ulong s = b;
            b = A;
            A = s;
            if ((b & 3) == 3 && (A & 3) == 3) sign = -sign;
        }
        b -= A;
        if (b == 0) return 0;
    }
}

/*
    (±Dabs|n)：Dabs 奇（uint 范围），n 为 nn limb 奇数。[np,nn] 经
    lmmp_mod_1_ 折叠到 mod |D|（预逆除法，O(nn)），再经二次互反律
    (A|n) = (n|A)*(-1)^(((A-1)/2)((n-1)/2)) 归结到 (n mod A | A)；
    D 为负时乘 (-1|n) 因子。gcd(|D|,n)>1 返回 0
*/
static int ipn_jacobi_D_(uint Dabs, int Dneg, mp_srcptr np, mp_size_t nn) {
    ulong A = Dabs;
    ulong b = lmmp_mod_1_(np, nn, A);
    int sign = 1;
    if ((A & 3) == 3 && (np[0] & 3) == 3) sign = -sign;
    sign *= ipn_jacobi_small_(b, A);
    if (sign == 0) return 0;
    if (Dneg && (np[0] & 3) == 3) sign = -sign;
    return sign;
}

/*
    强 Lucas-Selfridge 测试（V-only 阶梯，判据等价性见文件头注）。
    阶梯递推（蒙域，Q 可为负，Qm 为其蒙域剩余）：
      V_{2k}   = V_k^2 - 2Q^k
      V_{2k+1} = V_k*V_{k+1} - Q^k
      V_{2k+2} = V_{k+1}^2 - 2Q^{k+1}
    Q 链同步阶梯：(Q^k, Q^{k+1}) --bit0--> ((Q^k)^2, (Q^k)^2*Q)，
    --bit1--> (Q^k*Q^{k+1}, (Q^{k+1})^2)。D 搜索：非完全平方的 n 其
    Jacobi 特征非平凡，序列中必存在 (D|n) = -1，搜索必然终止；完全平方
    是唯一不终止情形，经 Dabs==17 处的延迟检测排除（lmmp_perfsqr_，
    不进常见路径；固定小上界不可行，见 is_prime_2.c 同段注释）
*/
int lmmp_is_strong_lucas_(mp_srcptr np, mp_size_t nn) {
    lmmp_param_assert(np != NULL);
    lmmp_param_assert(nn > 2 && np[nn - 1] > 0);
    lmmp_param_assert(np[0] % 2 == 1);

    uint Dabs = 5;
    int Dneg = 0;
    for (;;) {
        int j = ipn_jacobi_D_(Dabs, Dneg, np, nn);
        if (j == 0) return 0; /* gcd(|D|,n)>1 且 0 < |D| < n → 合数 */
        if (j == -1) break;
        Dabs += 2;
        Dneg ^= 1;
        if (Dabs == 17 && lmmp_perfsqr_(np, nn)) return 0;
    }

    /* Q = (1-D)/4，可为负；d*2^s = n+1 */
    uint Qabs = Dneg ? (Dabs + 1) / 4 : (Dabs - 1) / 4;

    TEMP_DECL;
    ipn_mont_t mc;

    /*
       单块工作区分段：
         [prod(2n)]   平方/全积兼 redcify 被除数
         [np1(n+1)]   n+1（防全 1 顶进位越顶）→ d 所在段
         [Qm(n)]      蒙域 Q
         [Vk|Vk1|Qk|Qk1(4n)]   V/Q 阶梯槽（经指针换名与 T 槽接力）
         [T1|T2|T3(3n)]        蒙乘/倍加中转槽
    */
    mp_size_t mn = ipn_mont_need_(nn);
    mp_ptr restrict arena = TALLOC_TYPE(mn + 10 * nn + 1, mp_limb_t);
    mp_ptr restrict prod = arena + mn;
    mp_ptr restrict np1 = prod + 2 * nn;
    mp_ptr restrict Qm = np1 + nn + 1;
    mp_ptr Vk = Qm + nn, Vk1 = Vk + nn, Qk = Vk1 + nn, Qk1 = Qk + nn;
    mp_ptr T1 = Qk1 + nn, T2 = T1 + nn, T3 = T2 + nn;

    lmmp_copy(np1, np, nn);
    np1[nn] = 0;
    lmmp_inc(np1);
    mp_bitcnt_t s = ipn_ctz_(np1);
    mp_ptr d = np1 + s / LIMB_BITS;
    mp_size_t dn = ipn_norm_(d, nn + 1 - s / LIMB_BITS);
    lmmp_shr_(d, d, dn, s % LIMB_BITS);
    dn = ipn_norm_(d, dn);

    ipn_mont_init_(&mc, np, nn, arena, prod);

    /* Qm = 蒙域 Q（D>0 时 Q = -Qabs，取负） */
    lmmp_zero(Qm, nn);
    Qm[0] = Qabs;
    ipn_redcify_(Qm, Qm, nn, prod, np);
    if (!Dneg)
        ipn_mont_neg_(Qm, Qm, &mc);

    /* 阶梯初态 k=1：(V_1, V_2, Q^1, Q^2) = (1, 1-2Q, Q, Q^2)（P=1） */
    lmmp_copy(Vk, mc.one, nn);
    ipn_mont_dbl_(T1, Qm, &mc);
    ipn_mont_sub_(Vk1, mc.one, T1, &mc);
    lmmp_copy(Qk, Qm, nn);
    lmmp_sqr_(prod, Qm, nn);
    ipn_redc_(Qk1, prod, &mc);

    int ret = 0;
    mp_bitcnt_t dbits = (dn - 1) * LIMB_BITS + lmmp_limb_bits_(d[dn - 1]);
    for (mp_bitcnt_t i = dbits - 1; i-- > 0;) {
        mp_ptr sw;
        if ((d[i / LIMB_BITS] >> (i % LIMB_BITS)) & 1) {
            lmmp_mul_(prod, Vk, nn, Vk1, nn);
            ipn_redc_(T1, prod, &mc); /* V_{2k+1} 主项 */
            lmmp_sqr_(prod, Vk1, nn);
            ipn_redc_(T2, prod, &mc); /* V_{2k+2} 主项 */
            ipn_mont_dbl_(T3, Qk1, &mc);
            ipn_mont_sub_(Vk, T1, Qk, &mc);  /* V_{2k+1} = V_k*V_{k+1} - Q^k */
            ipn_mont_sub_(Vk1, T2, T3, &mc); /* V_{2k+2} = V_{k+1}^2 - 2Q^{k+1} */
            lmmp_mul_(prod, Qk, nn, Qk1, nn);
            ipn_redc_(T1, prod, &mc); /* Q^{2k+1} */
            lmmp_sqr_(prod, Qk1, nn);
            ipn_redc_(T2, prod, &mc); /* Q^{2k+2} */
        } else {
            lmmp_sqr_(prod, Vk, nn);
            ipn_redc_(T1, prod, &mc); /* V_{2k} 主项 */
            lmmp_mul_(prod, Vk, nn, Vk1, nn);
            ipn_redc_(T2, prod, &mc); /* V_{2k+1} 主项 */
            ipn_mont_dbl_(T3, Qk, &mc);
            ipn_mont_sub_(Vk, T1, T3, &mc); /* V_{2k} = V_k^2 - 2Q^k */
            ipn_mont_sub_(Vk1, T2, Qk, &mc); /* V_{2k+1} = V_k*V_{k+1} - Q^k */
            lmmp_sqr_(prod, Qk, nn);
            ipn_redc_(T1, prod, &mc); /* Q^{2k} */
            lmmp_mul_(prod, T1, nn, Qm, nn);
            ipn_redc_(T2, prod, &mc); /* Q^{2k+1} */
        }
        /* Q 槽接力换新，降级旧槽为 T 槽 */
        sw = Qk; Qk = T1; T1 = sw;
        sw = Qk1; Qk1 = T2; T2 = sw;
    }

    /* 判据：V_d = 0，或 V_d^2 = 4Q^d，或 V_{d*2^r} = 0（0 < r < s） */
    if (lmmp_zero_q_(Vk, nn)) {
        ret = 1;
        goto done;
    }
    lmmp_sqr_(prod, Vk, nn);
    ipn_redc_(T1, prod, &mc); /* T1 = V_d^2 */
    ipn_mont_dbl_(T2, Qk, &mc);
    ipn_mont_dbl_(T2, T2, &mc); /* T2 = 4Q^d */
    if (lmmp_cmp_(T1, T2, nn) == 0) {
        ret = 1;
        goto done;
    }
    for (; s > 1; s--) {
        mp_ptr sw;
        ipn_mont_dbl_(T2, Qk, &mc);
        ipn_mont_sub_(Vk, T1, T2, &mc); /* V_{2d} = V_d^2 - 2Q^d（复用 T1） */
        lmmp_sqr_(prod, Qk, nn);
        ipn_redc_(T3, prod, &mc); /* Q^{2d} */
        lmmp_sqr_(prod, Vk, nn);
        ipn_redc_(T1, prod, &mc); /* V_{2d}^2 */
        if (lmmp_zero_q_(Vk, nn)) {
            ret = 1;
            goto done;
        }
        sw = Qk; Qk = T3; T3 = sw;
    }

done:
    ipn_mont_free_(&mc);
    TEMP_FREE;
    return ret;
}

int lmmp_is_prime_n_(mp_srcptr np, mp_size_t nn, int strength) {
    lmmp_param_assert(np != NULL);
    lmmp_param_assert(nn > 2 && np[nn - 1] > 0);
    lmmp_param_assert(strength >= 0 && strength <= 7);

    if ((np[0] & 1) == 0) return 0;

    /* 小素数试除初筛：契约域 n > 2^128 大于一切表内素数，命中即合数 */
    ushort rn;
    ushortp divs = lmmp_trialdiv_(np, nn, ipn_trial_bound[strength], &rn);
    if (divs != NULL) {
        lmmp_free(divs);
        return 0;
    }

    TEMP_DECL;
    /*
       [b2(n)] 基底载体：先基底 2，后随机基底
       [up(n)] 随机基底上界 n-2（排除平凡通过基底 n-1 与 <2 的退化基底）
    */
    mp_ptr restrict b2 = TALLOC_TYPE(2 * nn, mp_limb_t);
    mp_ptr restrict up = b2 + nn;
    lmmp_zero(b2, 2 * nn);
    b2[0] = 2;

    /* 各档公共首步：基底 2 特化 MR（BPSW 的 MR 半部） */
    if (!lmmp_is_sprp_(np, nn, b2)) goto composite;

    lmmp_copy(up, np, nn);
    lmmp_dec(up);
    lmmp_dec(up); /* up = n-2 */

    /* 随机基底 MR（至多 N 轮，检出即止） */
    for (int r = ipn_rnd_rounds[strength]; r > 0; r--) {
        for (;;) {
            lmmp_random_(b2, nn);
            lmmp_div_(NULL, b2, b2, nn, np, nn); /* eqsep 原地归约，b2 < n */
            if ((b2[0] >= 2 || !lmmp_zero_q_(b2 + 1, nn - 1)) && lmmp_cmp_(b2, up, nn) <= 0)
                break;
        }
        if (!lmmp_is_sprp_(np, nn, b2)) goto composite;
    }

    /* 4 档及以上：BPSW 的 Lucas 半部（嵌套 TEMP，免提前回收） */
    if (strength >= 4 && !lmmp_is_strong_lucas_(np, nn)) goto composite;
    TEMP_FREE;
    return 2;

composite:
    TEMP_FREE;
    return 0;
}
