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
    特化、1 limb 基底短乘特化）、强 Lucas-Selfridge 测试（U 阶梯）与
    强度分档入口 lmmp_is_prime_n_。单轮 MR 原语（lmmp_is_sprp_base2_ 与
    lmmp_is_sprp_1_）均为本文件内部 static 实现，不对外暴露。

    记 n = [np,nn]（奇，n > 2^128）。MR 单轮：n-1 = d*2^t（d 奇），梯子计算
    蒙域 y = b^d，判 y ∈ {1,-1} 或 y^(2^j) = -1（0 < j < t）。基底形态：
      - 基底 2：蒙域乘 2 即剩余加倍（2*R ≡ 倍加），逢 1 指数位以 O(n) 加法
        替代一次 M(n) 蒙乘，单轮约省 1/4 ~ 1/3（lmmp_is_sprp_base2_）；
      - 1 limb 基底：乘底步走"短乘短除"内核（lmmp_is_sprp_1_），利用
        蒙域表示对裸标量的线性，以两趟 O(nn) 完成一次乘底，免 REDC 与基底
        入蒙域的全宽预处理（见下方内核注释）；
      - >=2 limb 基底：改调 lmmp_powmod_odd_（powmod.c 的滑动窗口梯子，
        大尺寸省 ~30% 乘法；d 各轮固定），y 为普通域值直接 ±1 探测，t-1 次
        探测平方走 sqr_+div_。1 limb 短乘路径与宽基底路径的 SPRP 判据、
        d/2^t 分解与早退条件严格一致。

    强 Lucas（Selfridge 方法 A）：P = 1，Q = (1-D)/4，D 取 5,-7,9,-11,...
    中首个 (D|n) = -1 者；判据与标准 U/V 强 Lucas 测试一致：
      U_d = 0，或 V_{d*2^r} = 0（0 <= r < s），d*2^s = n+1
    阶梯为 Hackman 平方型 U 递推（GMP 6.3 lucmod.c 同构，见
    lmmp_is_strong_lucas_ 注释），Q 仅以小标量进入运算，Q^d 于末尾经
    V_d^2 - D*U_d^2 = 4Q^d 一次性恢复（模 n 两次折半代 GMP 的精确 /4）。

    Montgomery 核与 powmod.c 同源（R = B^n，三层 REDC：basecase 链式
    Hensel 归约 / 全积取高半 / 梅森折叠，含 FFT 变换缓存），按项目惯例
    按需复制为 static（见 powmod.c 头注；is_prime_2.c 的标量蒙域同例）。
    进蒙域走 redcify 除法（x*B^n mod n），one = B^n mod n 为各测试公共的
    比较基准，m1 = m - one 即 -1 的蒙域剩余。

    强度分档（strength ∈ [0,7]，随机基底取自全局 RNG）。随机轮分两层：
    先跑至多 x 轮 1 limb 基底（a ∈ [3,B) 均匀，单轮最廉，尽早拒判），再跑
    至多 y 轮全域（n limb，[2,n-2] 均匀）基底：
        档 | 基底2 MR  | 1 limb 轮 | n limb 轮 | 强 Lucas | 试除上界
        0  |    √     |     2     |     2     |    -     |    100
        1  |    √     |     3     |     2     |    -     |    300
        2  |    √     |     4     |     3     |    -     |   1000
        3  |    √     |     5     |     4     |    -     |   3000
        4  |    √     |     -     |     -     |    √     |   1000
        5  |    √     |     2     |     0     |    √     |   3000
        6  |    √     |     2     |     1     |    √     |   5000
        7  |    √     |     3     |     2     |    √     |  10000
    各档均先做基底 2 特化 MR；"至多 N 轮"指任一轮检出合数即提前终止，
    实际执行轮数不超过 N。试除上界随强度递增：高强度下幸存者代价更大
    （更多轮次/含 Lucas），更深的初筛以近零代价换更高的提前淘汰率。
    各档独立基底轮总数（含基底 2）为 s0..s3 = 5/6/8/10、s4..s7 = 1/3/4/6；
    1 limb 基底空间仅 2^64，由 AGP 定理（无穷多合数的最小 MR 见证
    > (log n)^(1/3)）知窄基底类的最坏/对抗情形弱于全宽随机轮，本"1+n"
    搭配定位于随机候选（keygen）场景。
*/

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/mul_cache.h"
#include "../../../include/lmmp/impl/rand_state.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/mprand.h"
#include "../../../include/lmmp/numth.h"

/* ============ 强度分档参数 ============ */

/* 1 limb 基底随机 MR 轮数上限（检出合数提前终止） */
static const uchar ipn_r1_rounds[8] = {2, 3, 4, 5, 0, 2, 2, 3};

/* 全域（n limb，[2,n-2] 均匀）基底随机 MR 轮数上限（检出合数提前终止） */
static const uchar ipn_rn_rounds[8] = {2, 2, 3, 4, 0, 0, 1, 2};

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
 * @brief basecase REDC：链式 Hensel 归约，逐 limb 消零
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
 * @brief 进蒙域：[dst,n] = [xp,n]*B^n mod [mp,n]
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

/* 蒙域模加 a+b（a,b < m）。和 < 2m < B^n+m：进位回绕（和 >= B^n）时
   回绕值 < m，随后减 m 的借位在 B^n 下恰好补回，回绕结果即 a+b-m */
static inline void ipn_mont_add_(mp_ptr dst, mp_srcptr a, mp_srcptr b, ipn_mont_t* restrict mc) {
    mp_limb_t c = lmmp_add_n_(dst, a, b, mc->n);
    if (c)
        (void)lmmp_sub_n_(dst, dst, mc->m, mc->n);
    else if (lmmp_cmp_(dst, mc->m, mc->n) >= 0)
        lmmp_sub_n_(dst, dst, mc->m, mc->n);
}

/* 蒙域值模 n 折半 x <- x/2 mod n（n 奇：x 偶直接移位；x 奇先加 n 再移，
   加法进位跨 limb 并入移位高位，B^n 回绕无损：真值 < 2n < 2B^n 且折半
   后 < B^n 可表示） */
static inline void ipn_mont_half_(mp_ptr x, ipn_mont_t* restrict mc) {
    mp_size_t n = mc->n;
    mp_limb_t c = (x[0] & 1) ? lmmp_add_n_(x, x, mc->m, n) : 0;
    lmmp_shr_(x, x, n, 1);
    x[n - 1] |= c << (LIMB_BITS - 1);
}

/*
    蒙域小标量乘 dst = (±q)*x (mod m)。倍加链 r=0; 逐高位 r=2r(+x)，
    全程 O(n) 模加/模加倍，对任意 ulong 的 q 均正确（大 q 仅减慢，
    正常路径 |Q|、|D| 为个位数）；sep(dst,x)
*/
static void ipn_mont_mul_small_(
    mp_ptr           dst,
    mp_srcptr         x,
    ulong           qabs,
    int              qneg,
    ipn_mont_t* restrict mc
) {
    if (qabs == 0) {
        lmmp_zero(dst, mc->n);
        return;
    }
    mp_bitcnt_t hb = lmmp_limb_bits_(qabs) - 1;
    lmmp_copy(dst, x, mc->n);
    for (mp_bitcnt_t i = hb; i-- > 0;) {
        ipn_mont_dbl_(dst, dst, mc);
        if ((qabs >> i) & 1)
            ipn_mont_add_(dst, dst, x, mc);
    }
    /* 负标量最后取负（in-place：sub_n 逐 limb 先读后写，安全） */
    if (qneg && !lmmp_zero_q_(dst, mc->n))
        ipn_mont_neg_(dst, dst, mc);
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

/**
 * @brief 单轮 Rabin-Miller 强伪素数测试（大于 128 位，基底硬编码为 2）
 * @param np 待测奇数指针（nn 个limb）
 * @param nn 待测数的 limb 长度
 * @warning np!=NULL, nn>2, np[nn-1]>0, np[0]%2==1
 * @note 蒙域 L2R 梯子：u = 2^d（蒙域剩余）。基底 2 特化为"平方 + 倍加"
 *       （蒙域乘 2 即剩余加倍 2*R ≡ 倍加，O(n) 替代一次 M(n) 蒙乘，蒙域 2
 *       由 one 倍加而来，免去基底入蒙域的 redcify 除法）。判定用蒙域基准
 *       one = B^n mod n 与 m1 = n - one（即 -1 的蒙域剩余），二次探测：
 *       y ∈ {1,-1} 或 y^(2^j) = -1（0 < j < t，n-1 = d*2^t）
 * @return 1 = 通过基 2 的 Miller-Rabin 测试；0 = 合数
 */
static int lmmp_is_sprp_base2_(mp_srcptr np, mp_size_t nn) {
    lmmp_param_assert(np != NULL);
    lmmp_param_assert(nn > 2 && np[nn - 1] > 0);
    lmmp_param_assert(np[0] % 2 == 1);

    TEMP_DECL;
    ipn_mont_t mc;

    /*
       单块工作区分段（蒙域段在前，调用者段续后）：
         [prod(2n)]   平方全积（REDC 被归约数）兼 redcify 工作区
         [nm1(n)]     n-1，低 z 零 limb 折叠为指针偏移后即 d 所在段
         [u(n)]       梯子累加器
       蒙域段：basecase 2n | 中层 8n | 折叠 8n+msz（ipn_mont_need_）
    */
    mp_size_t mn = ipn_mont_need_(nn);
    mp_ptr restrict arena = TALLOC_TYPE(mn + 4 * nn, mp_limb_t);
    mp_ptr restrict prod = arena + mn;
    mp_ptr restrict nm1 = prod + 2 * nn;
    mp_ptr restrict u = nm1 + nn;

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

    /* 蒙域初值 one+one = 蒙域 2，逐位平方 + bit=1 倍加 */
    ipn_mont_dbl_(u, mc.one, &mc);
    for (mp_bitcnt_t i = dbits - 1; i-- > 0;) {
        lmmp_sqr_(prod, u, nn);
        ipn_redc_(u, prod, &mc);
        if ((d[i / LIMB_BITS] >> (i % LIMB_BITS)) & 1)
            ipn_mont_dbl_(u, u, &mc);
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

/* ============ 1 limb 基底 SPRP 特化：短乘短除内核 ============ */

/*
    蒙域表示对裸标量线性：x̃ ≡ x*B^nn (mod n)，两侧乘裸整数 a 得
    x̃*a ≡ (a*x)*B^nn，即乘裸标量后仍是同一模数下的合法蒙域剩余。故乘底步
    既不需要 REDC，也不需要把基底提升成蒙域形式 a*B^nn（提升即全宽，优势尽
    失）；1 limb 基底用二进制梯子而非窗口表（窗口表条目 a^j 全宽，会把短乘
    优势全部吃掉）。

    1 limb 基底（lmmp_is_sprp_1_）：t = x̃*a < n*B，故商 q = t div n 恰为
    1 limb。归一化移位 sh = clz(n[nn-1])、归一化除数顶 2 limb (d1,d0) 及其
    2/1 逆元于上下文初始化时一次算好；每步以 t<<sh 的顶 3 limb (u2,u1,u0)
    对 (d1,d0) 作 3/2 除法（_udiv_qr_3by2）估商 q̂。由 t<<sh < (n<<sh)*B 知
    q̂ 的顶 limb u2 <= d1：u2 == d1 时真商必为 B-1 或 B-2（取 q̂ = B-1，至多
    回补一次）；u2 < d1 时（u2,u1,u0）< (d1,d0)*B 成立、3/2 除法契约满足，
    截去 t 与 n 的低位分别使 q̂ >= q、q̂ <= q+1（实测回补率 ~0，2/1 估商则
    高达 15%~34%，故取 3/2），乘减后回补 n 至多 1 次。除末尾回补外全程只有
    mul_1 与 submul_1 两趟 O(nn)，无除法循环；平方步仍为全宽 sqr_ + REDC
    （本测试的复杂度主导项，与基底宽无关，故不动）。

    2 limb 及以上基底不再特化（裁定沿用 lmmp_powmod_odd_ 窗口梯子）：短乘短除
    的收益来自"以两趟 O(nn) 替代一次全宽乘"，在 nn=3（唯一可能采样到 <=2 limb
    基底的尺寸）全宽乘仅约 nn^2 = 9 次乘加，而 lmmp_div_ 的固定开销（归一化、
    临时分配、部分除 + 修正）反超其节省——实测 2 limb 基底走"mul_1 + 移位
    addmul_1 + lmmp_div_"内核比通用路径慢约 22%，nn>=8 才反超（+11%~24%）；
    而 nn>=4 的均匀采样基底退化为 <=2 limb 的概率仅 ~2^-64，分派无从生效。
    故宽度分派只保留宽度 1（见 lmmp_is_prime_n_ 内注释）。

    实测（M5/arm64/Release/best-of-5，与 lmmp_powmod_odd_ 轮同 n、同 d、同
    基底值域）：nn=16/32/64 时 is_sprp_1_ 单轮比通用随机轮快 10.9%/13.0%/
    12.5%；而与"乘底步退化为 O(nn) 倍加"的基底 2 特化轮相比仅慢约 1%——
    即乘底步已压到几乎可忽略，剩余差距全部来自窗口梯子自身的乘底份额
    （64 limb 实测约 13%，而非设计预期的 1/6~1/3，故 17%~20% 的估计提速
    不可达；本内核已达该思路的上限）。
*/

/* 短除上下文：归一化除数顶 2 limb 及其 2/1 逆元（1 limb 基底路径专用） */
typedef struct {
    mp_srcptr n;    /* 模数 [n,nn]（奇，顶 limb 非零，nn>=3） */
    mp_limb_t d1;   /* [n,nn]<<sh 的顶 limb（MSB 置 1） */
    mp_limb_t d0;   /* [n,nn]<<sh 的次顶 limb */
    mp_limb_t dinv; /* (B^3-1)/(d1*B+d0) - B（_udiv_qr_3by2 的 dinv） */
    mp_size_t nn;   /* 模数 limb 长度 */
    mp_bitcnt_t sh; /* clz(n[nn-1]) */
} ipn_sdiv_t;

/**
 * @brief 短除上下文初始化：归一化除数顶 2 limb 与其 2/1 逆元
 * @param sd 上下文
 * @param np 模数（nn 个limb）
 * @param nn 模数 limb 长度
 * @warning np!=NULL, nn>2, np[nn-1]>0, np[0]%2==1
 */
static void ipn_sdiv_init_(ipn_sdiv_t* sd, mp_srcptr np, mp_size_t nn) {
    mp_bitcnt_t sh = lmmp_leading_zeros_(np[nn - 1]);
    sd->n = np;
    sd->nn = nn;
    sd->sh = sh;
    if (sh == 0) {
        sd->d1 = np[nn - 1];
        sd->d0 = np[nn - 2];
    } else {
        sd->d1 = (np[nn - 1] << sh) | (np[nn - 2] >> (LIMB_BITS - sh));
        sd->d0 = (np[nn - 2] << sh) | (np[nn - 3] >> (LIMB_BITS - sh));
    }
    sd->dinv = lmmp_inv_2_1_(sd->d1, sd->d0);
}

/**
 * @brief 蒙域内乘裸 1 limb 标量：x <- x*a mod n（结果仍是蒙域剩余）
 * @param x 蒙域剩余（nn 个limb，原地读写）
 * @param a 裸标量（a < B）
 * @param sd 短除上下文
 * @warning x < [sd->n,sd->nn], sep(x,sd->n)
 * @note 估商与回补的界见本节顶部注；"乘减 q*n"大操作数 n 在前、标量 q 在后
 */
static inline void ipn_sdiv_mul1_(mp_ptr x, mp_limb_t a, const ipn_sdiv_t* sd) {
    mp_size_t nn = sd->nn;
    /* t = x*a：低 nn limb 落回 x，第 nn+1 limb 为进位 */
    mp_limb_t cy = lmmp_mul_1_(x, x, nn, a);

    /* t<<sh 的顶 3 limb (u2,u1,u0)；t<<sh < (n<<sh)*B ⟹ u2 <= d1 */
    mp_limb_t u2, u1, u0;
    if (sd->sh == 0) {
        u2 = cy;
        u1 = x[nn - 1];
        u0 = x[nn - 2];
    } else {
        u2 = (cy << sd->sh) | (x[nn - 1] >> (LIMB_BITS - sd->sh));
        u1 = (x[nn - 1] << sd->sh) | (x[nn - 2] >> (LIMB_BITS - sd->sh));
        u0 = (x[nn - 2] << sd->sh) | (x[nn - 3] >> (LIMB_BITS - sd->sh));
    }

    mp_limb_t q, r1, r0;
    if (u2 >= sd->d1) {
        /* u2 == d1：真商 ∈ {B-1,B-2}，取 B-1 至多回补一次 */
        q = LIMB_MAX;
    } else {
        /* u2 < d1 ⟹ (u2,u1,u0) < (d1,d0)*B，3/2 除法契约满足 */
        _udiv_qr_3by2(q, r1, r0, u2, u1, u0, sd->d1, sd->d0, sd->dinv);
    }
    (void)r1;
    (void)r0;

    /* x <- t - q*n；估商偏高时差值为负（hi != 0 即 B^{nn+1} 补码），回补 n */
    mp_limb_t hi = cy - lmmp_submul_1_(x, sd->n, nn, q);
    while (hi != 0)
        hi += lmmp_add_n_(x, x, sd->n, nn);
}

/**
 * @brief 短基底 MR 单轮：基底为 1 limb 裸标量，SPRP 判据与基底 2 轮一致
 * @param np 待测奇数（nn 个limb）
 * @param nn 待测数 limb 长度
 * @param a 基底（1 个limb，a < B）
 * @warning np!=NULL, nn>2, np[nn-1]>0, np[0]%2==1, 2 <= a <= [np,nn]-2
 *          （契约域 n > 2^128 > B 恒成立）
 * @return 1 = 通过该基底的 Miller-Rabin 测试；0 = 合数
 */
static int ipn_sprp_short_(mp_srcptr np, mp_size_t nn, mp_limb_t a) {
    TEMP_DECL;
    ipn_mont_t mc;
    ipn_sdiv_t sd;

    /*
       单块工作区分段（蒙域段在前，调用者段续后）：
         [prod(2n)]  平方全积（REDC 被归约数）兼 redcify 工作区
         [nm1(n)]    n-1（-1 探测基准，亦是 d 分解的来源）
         [d(n)]      (n-1)>>t
         [x(n)]      梯子累加器（兼出蒙域结果与探测载体）
       蒙域段：basecase 2n | 中层 8n | 折叠 8n+msz（ipn_mont_need_）
    */
    mp_size_t mn = ipn_mont_need_(nn);
    mp_ptr restrict arena = TALLOC_TYPE(mn + 5 * nn, mp_limb_t);
    mp_ptr restrict prod = arena + mn;
    mp_ptr restrict nm1 = prod + 2 * nn;
    mp_ptr restrict d = nm1 + nn;
    mp_ptr restrict x = d + nn;

    ipn_mont_init_(&mc, np, nn, arena, prod);
    ipn_sdiv_init_(&sd, np, nn);

    /* n-1 = d*2^t（d 奇）：低零 limb 折叠为指针偏移，余位单次移位 */
    lmmp_copy(nm1, np, nn);
    lmmp_dec(nm1);
    mp_bitcnt_t tbits = ipn_ctz_(nm1);
    mp_size_t dn = ipn_norm_(nm1 + tbits / LIMB_BITS, nn - tbits / LIMB_BITS);
    lmmp_shr_(d, nm1 + tbits / LIMB_BITS, dn, tbits % LIMB_BITS);
    dn = ipn_norm_(d, dn);
    mp_bitcnt_t dbits = (dn - 1) * LIMB_BITS + lmmp_limb_bits_(d[dn - 1]);

    /* 梯子初值 x̃ = a*B^nn mod n = one*a：复用同一乘底内核，免去基底入蒙域的
       全宽 redcify 除法 */
    lmmp_copy(x, mc.one, nn);
    ipn_sdiv_mul1_(x, a, &sd);

    /* 二进制 L2R 梯子：逐位平方 + bit=1 乘底 */
    for (mp_bitcnt_t i = dbits - 1; i-- > 0;) {
        lmmp_sqr_(prod, x, nn);
        ipn_redc_(x, prod, &mc);
        if ((d[i / LIMB_BITS] >> (i % LIMB_BITS)) & 1)
            ipn_sdiv_mul1_(x, a, &sd);
    }

    /* 出蒙域：y = a^d mod n（普通域），探测结构与 is_prime_n_ 随机轮同构 */
    lmmp_copy(prod, x, nn);
    lmmp_zero(prod + nn, nn);
    ipn_redc_(x, prod, &mc);

    /* 二次探测：y ∈ {1,-1} 或 y^(2^j) = -1（0 < j < t，共 t-1 次平方） */
    int ret = 0;
    if ((x[0] == 1 && lmmp_zero_q_(x + 1, nn - 1)) || lmmp_cmp_(x, nm1, nn) == 0) {
        ret = 1;
    } else {
        for (mp_bitcnt_t j = tbits - 1; j > 0; j--) {
            lmmp_sqr_(prod, x, nn);
            lmmp_div_(NULL, x, prod, 2 * nn, np, nn);
            if (lmmp_cmp_(x, nm1, nn) == 0) {
                ret = 1;
                break;
            }
        }
    }

    ipn_mont_free_(&mc);
    TEMP_FREE;
    return ret;
}

static int lmmp_is_sprp_1_(mp_srcptr np, mp_size_t nn, mp_limb_t a) {
    lmmp_param_assert(np != NULL);
    lmmp_param_assert(nn > 2 && np[nn - 1] > 0);
    lmmp_param_assert(np[0] % 2 == 1);
    lmmp_param_assert(a >= 2);
    return ipn_sprp_short_(np, nn, a);
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
    强 Lucas-Selfridge 测试（U 阶梯，Hackman 平方型公式，P=1 特化，
    结构移植自 GMP 6.3 mpz/lucmod.c）。阶梯递推（蒙域，Q 仅以小标量
    出现，Q^k 链整体取消，Q^d 于末尾一次性恢复）：
      U_{2k}   = U_{k+1}^2 - (U_{k+1}-U_k)^2
      U_{2k+1} = U_{k+1}^2 - Q*U_k^2
      U_{2k+2} = U_{2k+1} - Q*U_{2k}（bit=1 时经递推免去第三次平方）
    每 bit 3 次平方 + 3 次 REDC + O(n) 标量运算（V/Q 双阶梯旧结构为
    4 次乘族 + 4 次 REDC）。D 搜索：非完全平方的 n 其 Jacobi 特征
    非平凡，序列中必存在 (D|n) = -1，搜索必然终止；完全平方是唯一
    不终止情形，经 Dabs==17 处的延迟检测排除（lmmp_perfsqr_，不进
    常见路径；固定小上界不可行，见 is_prime_2.c 同段注释）
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
         [prod(2n)]   平方全积（REDC 被归约数）
         [np1(n+1)]   n+1（防全 1 顶进位越顶）→ d 所在段
         [Uk|Uk1(2n)] U 阶梯状态对 (U_k, U_{k+1})
         [T1|T2|T3(3n)] 差值平方 / A / B 载体与标量乘输出
       蒙域段之后的调用者段合计 8n+1
    */
    mp_size_t mn = ipn_mont_need_(nn);
    mp_ptr restrict arena = TALLOC_TYPE(mn + 8 * nn + 1, mp_limb_t);
    mp_ptr restrict prod = arena + mn;
    mp_ptr restrict np1 = prod + 2 * nn;
    mp_ptr Uk = np1 + nn + 1, Uk1 = Uk + nn;
    mp_ptr T1 = Uk1 + nn, T2 = T1 + nn, T3 = T2 + nn;

    lmmp_copy(np1, np, nn);
    np1[nn] = 0;
    lmmp_inc(np1);
    mp_bitcnt_t s = ipn_ctz_(np1);
    mp_ptr d = np1 + s / LIMB_BITS;
    mp_size_t dn = ipn_norm_(d, nn + 1 - s / LIMB_BITS);
    lmmp_shr_(d, d, dn, s % LIMB_BITS);
    dn = ipn_norm_(d, dn);

    ipn_mont_init_(&mc, np, nn, arena, prod);

    /* 阶梯初态 k=1：(U_1, U_2) = (1, 1)（P=1, U_0=0 → U_2 与 Q 无关） */
    lmmp_copy(Uk, mc.one, nn);
    lmmp_copy(Uk1, mc.one, nn);

    mp_bitcnt_t dbits = (dn - 1) * LIMB_BITS + lmmp_limb_bits_(d[dn - 1]);
    for (mp_bitcnt_t i = dbits - 1; i-- > 0;) {
        ipn_mont_sub_(T1, Uk1, Uk, &mc);         /* (U_{k+1}-U_k)R */
        lmmp_sqr_(prod, Uk1, nn);
        ipn_redc_(T2, prod, &mc);                /* A = U_{k+1}^2 R */
        lmmp_sqr_(prod, Uk, nn);
        ipn_redc_(T3, prod, &mc);                /* B = U_k^2 R */
        lmmp_sqr_(prod, T1, nn);
        ipn_redc_(T1, prod, &mc);                /* D = (U_{k+1}-U_k)^2 R */
        ipn_mont_mul_small_(Uk, T3, Qabs, !Dneg, &mc); /* Q*U_k^2 R */
        ipn_mont_sub_(Uk1, T2, Uk, &mc);         /* U_{2k+1} = A - Q*B */
        ipn_mont_sub_(Uk, T2, T1, &mc);          /* U_{2k}   = A - D */
        if ((d[i / LIMB_BITS] >> (i % LIMB_BITS)) & 1) {
            ipn_mont_mul_small_(T3, Uk, Qabs, !Dneg, &mc); /* Q*U_{2k} R */
            mp_ptr sw = Uk; Uk = Uk1; Uk1 = sw;  /* (U_{2k+1}, _) */
            ipn_mont_sub_(Uk1, Uk, T3, &mc);     /* U_{2k+2} = U_{2k+1} - Q*U_{2k} */
        }
    }

    /* 判据：U_d = 0，或 V_d = 0，或 V_{d*2^r} = 0（0 < r < s）。
       U_d 由阶梯直接给出（标准强 Lucas 判据；旧 V-only 阶梯以
       2V_{d+1} = V_d 间接检出，依 P=1 恒等式 D*U_d = 2V_{d+1} - V_d
       与 gcd(D,n)=1（D 搜索保证）精确等价，接受集不变）。
       V_d = 2U_{d+1} - U_d（联立 U_{d+1} = U_d - Q*U_{d-1} 与
       V_d = U_{d+1} - Q*U_{d-1}，阶梯末态恰为 (U_d, U_{d+1})，免 Q
       参与；GMP 同式而阶梯终点为 (U_{d-1},U_d)，故其写成
       U_d - 2Q*U_{d-1}）。Q^d = (V_d^2 - D*U_d^2)/4 */
    int ret = 0;
    if (lmmp_zero_q_(Uk, nn)) {
        ret = 1;
        goto done;
    }
    ipn_mont_dbl_(T1, Uk1, &mc);
    ipn_mont_sub_(T1, T1, Uk, &mc); /* V_d = 2U_{d+1} - U_d */
    if (lmmp_zero_q_(T1, nn)) {
        ret = 1;
        goto done;
    }
    if (s > 1) {
        /* Q^d R = (V_d^2 - D*U_d^2)R/4：两平方 REDC，D 以小标量合成，
           再模 n 两次折半（GMP 在普通域有精确整除 /4 便车，蒙域以
           折半替代，O(n)） */
        lmmp_sqr_(prod, T1, nn);
        ipn_redc_(T2, prod, &mc); /* V_d^2 R */
        lmmp_sqr_(prod, Uk, nn);
        ipn_redc_(Uk, prod, &mc); /* U_d^2 R（U_d 槽至此耗尽） */
        ipn_mont_mul_small_(T3, Uk, Dabs, 0, &mc);
        if (!Dneg)
            ipn_mont_sub_(T2, T2, T3, &mc); /* D>0：V_d^2 - D*U_d^2 */
        else
            ipn_mont_add_(T2, T2, T3, &mc); /* D<0：V_d^2 + |D|*U_d^2 */
        ipn_mont_half_(T2, &mc); /* (V^2 - D U^2) = 4Q^d → 2Q^d */
        ipn_mont_half_(T2, &mc); /* → Q^d（阶为蒙域剩余） */
        for (; s > 1; s--) {
            ipn_mont_dbl_(T3, T2, &mc); /* 2Q^m R */
            lmmp_sqr_(prod, T1, nn);
            ipn_redc_(Uk, prod, &mc); /* V_m^2 R */
            lmmp_sqr_(prod, T2, nn);
            ipn_redc_(T2, prod, &mc); /* Q^{2m} R（旧值已被消费） */
            ipn_mont_sub_(T1, Uk, T3, &mc); /* V_{2m} = V_m^2 - 2Q^m */
            if (lmmp_zero_q_(T1, nn)) {
                ret = 1;
                goto done;
            }
        }
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
    if (lmmp_trialdiv_(np, nn, ipn_trial_bound[strength])) return 0;

    TEMP_DECL;
    /*
       [b2(n)]    全域（n limb）随机轮的采样基底载体
       [y(n)]     宽基底轮 y = b^d mod n（普通域；powmod_odd_ 要求 sep(dst,bp)）
       [nm1(n)]   n-1（随机基底上界 n-1 与 -1 探测基准）
       [d(n)]     (n-1)>>t（各随机轮固定不变）
       [prod(2n)] 探测平方工作区（sqr 全积兼 div 被除数）
    */
    mp_ptr restrict b2 = TALLOC_TYPE(6 * nn, mp_limb_t);
    mp_ptr restrict y = b2 + nn;
    mp_ptr restrict nm1 = y + nn;
    mp_ptr restrict d = nm1 + nn;
    mp_ptr restrict prod = d + nn;

    /* 各档公共首步：基底 2 特化 MR（BPSW 的 MR 半部，倍加梯子） */
    if (!lmmp_is_sprp_base2_(np, nn)) goto composite;

    /* nm1 = n-1；t = v2(n-1)；d = (n-1)>>t（奇），各随机轮共用。随机基底轮
       分两层：1 limb 基底走短乘短除内核（最廉价，先跑），全域基底按有效
       limb 宽分派（宽 1 亦走短乘，宽 >=2 走 lmmp_powmod_odd_ 的滑动窗口
       梯子，大尺寸省 ~30% 乘法）。两条路径的 y 均为普通域值直接 ±1
       探测（蒙域/普通域经 redcify 双射等价），t-1 次探测平方走 sqr_+div_ */
    lmmp_copy(nm1, np, nn);
    lmmp_dec(nm1);
    mp_bitcnt_t t = ipn_ctz_(nm1);
    mp_size_t dn = ipn_norm_(nm1 + t / LIMB_BITS, nn - t / LIMB_BITS);
    lmmp_shr_(d, nm1 + t / LIMB_BITS, dn, t % LIMB_BITS);
    dn = ipn_norm_(d, dn);

    /* 1 limb 基底 MR：a ∈ [3,B-1] 均匀（全局 RNG 单 limb 直取，免去
       lmmp_random_ 的整块播种开销）。契约域 n > 2^128 > B 保证任意 a 满足
       3 <= a <= n-2，无需拒绝采样 */
    for (int r = ipn_r1_rounds[strength]; r > 0; r--) {
        mp_limb_t a;
        do {
            a = lmmp_randlimb_();
        } while (a < 3);
        if (!lmmp_is_sprp_1_(np, nn, a)) goto composite;
    }

    /* 全域（n limb）基底 MR（至多 N 轮，检出即止）：b ∈ [2,n-2] 均匀。
       顶 limb 掩码到 n 的位长内再整体拒绝采样：位长内均匀故 P(>=n) <= 1/2，
       期望 <= 2 次填充，免去每轮一次 nn/nn 全除归约（旧路径 lmmp_div_） */
    uint tb = lmmp_limb_bits_(np[nn - 1]);
    mp_limb_t tmask = (tb == LIMB_BITS) ? ~(mp_limb_t)0 : (((mp_limb_t)1 << tb) - 1);
    for (int r = ipn_rn_rounds[strength]; r > 0; r--) {
        for (;;) {
            lmmp_random_(b2, nn);
            b2[nn - 1] &= tmask;
            if (lmmp_cmp_(b2, nm1, nn) < 0 && (b2[0] >= 2 || !lmmp_zero_q_(b2 + 1, nn - 1)))
                break;
        }
        /* 基底有效 limb 宽：nn=3 且 n 接近 2^128 时 <=1 limb 的基底占比可达
           可观比例（均匀采样下约 2^128/n），窄基底走短乘可省下全宽乘。
           宽度 2 不特化：其唯一可能出现的尺寸 nn=3 上短除内核慢于通用路径
           （见本节顶部实测注），而 nn>=4 采样退化为 <=2 limb 的概率 ~2^-64 */
        mp_size_t bw = nn;
        while (bw > 1 && b2[bw - 1] == 0) bw--;
        if (bw == 1) {
            if (!lmmp_is_sprp_1_(np, nn, b2[0])) goto composite;
            continue;
        }
        lmmp_powmod_odd_(y, b2, d, dn, np, nn);
        int ok = 0;
        if ((y[0] == 1 && lmmp_zero_q_(y + 1, nn - 1)) || lmmp_cmp_(y, nm1, nn) == 0) {
            ok = 1;
        } else {
            for (mp_bitcnt_t j = t - 1; j > 0; j--) {
                lmmp_sqr_(prod, y, nn);
                lmmp_div_(NULL, y, prod, 2 * nn, np, nn);
                if (lmmp_cmp_(y, nm1, nn) == 0) {
                    ok = 1;
                    break;
                }
            }
        }
        if (!ok) goto composite;
    }

    /* 4 档及以上：BPSW 的 Lucas 半部（嵌套 TEMP，免提前回收） */
    if (strength >= 4 && !lmmp_is_strong_lucas_(np, nn)) goto composite;
    TEMP_FREE;
    return 2;

composite:
    TEMP_FREE;
    return 0;
}
