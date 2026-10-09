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
    特化、1 limb 基底短乘短除特化）、强 Lucas-Selfridge 测试（U 阶梯）与
    强度分档入口 lmmp_is_prime_n_。单轮 MR 原语（ipn_sprp_base2_ 与
    ipn_sprp_1_）均为本文件内部 static 实现，不对外暴露。

    记 n = [np,nn]（奇，n > 2^128）。MR 单轮：n-1 = d*2^t（d 奇），梯子计算
    蒙域 y = b^d，判 y ∈ {1,-1} 或 y^(2^j) = -1（0 < j < t）。基底形态：
      - 基底 2：蒙域乘 2 即剩余加倍（2*R ≡ 倍加），逢 1 指数位以 O(n) 加法
        替代一次 M(n) 蒙乘，单轮约省 1/4 ~ 1/3（ipn_sprp_base2_）；
      - 1 limb 基底：乘底步走"短乘短除"内核（ipn_sprp_1_），利用蒙域表示
        对裸标量的线性，以两趟 O(nn) 完成一次乘底，免 REDC 与基底入蒙域
        的全宽预处理（见下方内核注释）；
      - >=2 limb 基底：改调蒙域梯子 lmmp_powmod_odd_mont_（powmod.c 的
        滑动窗口梯子，大尺寸省 ~30% 乘法；d 各轮固定）。
    三条路径的梯子与二次探测全程留在蒙域：探测基准为 one = B^nn mod n 与
    m1 = n - one——x -> x*B^nn mod n 为环同构（n 奇），b^d ≡ ±1 (mod n)
    ⟺ 蒙域梯子终值 ∈ {one, m1}，平方链逐环保持，判定集与普通域 ±1 探测
    严格一致；由此免每轮出蒙域 REDC 与探测尾部的全宽除法（旧 sqr_+div_），
    探测尾部统一为 sqr_+REDC。

    蒙域核心（lmmp_mont_t：三层 REDC、进蒙域 redcify、上下文）的单一定义
    见 impl/powmod.h（与 powmod.c 共用）。上下文在入口处 init 一次，贯穿
    基底 2 / 随机基底各轮 / 强 Lucas：各轮免 binvert、one/m1 的 redcify
    预处理与 FFT 变换缓存的重建与释放（含折叠层 m 一侧变换、mullo 层
    ninv 一侧变换的跨轮保活）。

    强 Lucas（Selfridge 方法 A）：P = 1，Q = (1-D)/4，D 取 5,-7,9,-11,...
    中首个 (D|n) = -1 者；判据与标准 U/V 强 Lucas 测试一致：
      U_d = 0，或 V_{d*2^r} = 0（0 <= r < s），d*2^s = n+1
    阶梯为 Hackman 平方型 U 递推（GMP 6.3 lucmod.c 同构，见 ipn_lucas_i_
    注释），Q 仅以小标量进入运算，Q^d 于末尾经
    V_d^2 - D*U_d^2 = 4Q^d 一次性恢复（模 n 两次折半代 GMP 的精确 /4）。

    强度分档（strength ∈ [0,7]，随机基底取自全局 RNG）。随机轮分两层：
    先跑至多 x 轮 1 limb 基底（a ∈ [3,B) 均匀，单轮最廉，尽早拒判），再跑
    至多 y 轮全域（n limb，[2,n-2] 均匀）基底：
        档 | 基底2 MR  | 1 limb 轮 | n limb 轮 | 强 Lucas | 试除上界
        0  |    √     |     2     |     2     |    -     |    100
        1  |    √     |     3     |     2     |    -     |    300
        2  |    √     |     4     |     3     |    -     |   1000
        3  |    √     |     5     |     4     |    -     |    3000
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

    Proth 形状独立入口 lmmp_is_prothprime_(k, n)（判 k*2^n+1，k 奇 < 2^32，
    契约 k>0 奇、n>0）：
    普罗斯定理的单梯子双方向确定性判决，不经强度分档，也不接入
    lmmp_is_prime_n_ 的通用分派（保持通用路径的形状无关性），设计见
    文末 Proth 节注释。
*/

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/powmod.h"
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

/* ============ 蒙域小运算（Lucas 阶梯专用，基数为模数 n） ============ */

/* 蒙域加倍 x+x-m。值 < 2m：进位回绕（2x >= B^n）时 B^n 与 m 的差额由
   减法借位在 B^n 下恰巧抵消，回绕结果即 2x-m（见 is_prime_2.c mont2_dbl）*/
static inline void ipn_mont_dbl_(mp_ptr dst, mp_srcptr x, lmmp_mont_t* restrict mc) {
    mp_limb_t c = lmmp_add_n_(dst, x, x, mc->n);
    if (c)
        (void)lmmp_sub_n_(dst, dst, mc->m, mc->n);
    else if (lmmp_cmp_(dst, mc->m, mc->n) >= 0)
        lmmp_sub_n_(dst, dst, mc->m, mc->n);
}

/* 蒙域模减 a-b（a,b < m）：借位回绕由加 m 补偿，结果 < m */
static inline void ipn_mont_sub_(mp_ptr dst, mp_srcptr a, mp_srcptr b, lmmp_mont_t* restrict mc) {
    mp_limb_t bo = lmmp_sub_n_(dst, a, b, mc->n);
    if (bo)
        (void)lmmp_add_n_(dst, dst, mc->m, mc->n);
}

/* 蒙域取负 m-x（x != 0，结果 < m） */
static inline void ipn_mont_neg_(mp_ptr dst, mp_srcptr x, lmmp_mont_t* restrict mc) {
    (void)lmmp_sub_n_(dst, mc->m, x, mc->n);
}

/* 蒙域模加 a+b（a,b < m）。和 < 2m < B^n+m：进位回绕（和 >= B^n）时
   回绕值 < m，随后减 m 的借位在 B^n 下恰好补回，回绕结果即 a+b-m */
static inline void ipn_mont_add_(mp_ptr dst, mp_srcptr a, mp_srcptr b, lmmp_mont_t* restrict mc) {
    mp_limb_t c = lmmp_add_n_(dst, a, b, mc->n);
    if (c)
        (void)lmmp_sub_n_(dst, dst, mc->m, mc->n);
    else if (lmmp_cmp_(dst, mc->m, mc->n) >= 0)
        lmmp_sub_n_(dst, dst, mc->m, mc->n);
}

/* 蒙域值模 n 折半 x <- x/2 mod n（n 奇：x 偶直接移位；x 奇先加 n 再移，
   加法进位跨 limb 并入移位高位，B^n 回绕无损：真值 < 2n < 2B^n 且折半
   后 < B^n 可表示） */
static inline void ipn_mont_half_(mp_ptr x, lmmp_mont_t* restrict mc) {
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
    lmmp_mont_t* restrict mc
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
 * @param mc 蒙域上下文（含 one/m1，已完成 lmmp_mont_init_）
 * @param d (n-1)>>t（奇，探测梯子的指数窗口）
 * @param dbits d 的位长
 * @param t v2(n-1)
 * @param u 梯子累加器兼探测载体（mc->n 个limb）
 * @param prod 工作区（2*mc->n 个limb，平方全积，出口被破坏）
 * @warning sep(u,prod), sep([u|prod], mc 内各缓冲区)
 * @note 蒙域 L2R 梯子：u = 2^d（蒙域剩余）。基底 2 特化为"平方 + 倍加"
 *       （蒙域乘 2 即剩余加倍 2*R ≡ 倍加，O(n) 替代一次 M(n) 蒙乘，蒙域 2
 *       由 one 倍加而来，免去基底入蒙域的 redcify 除法）。蒙域基准
 *       one = B^n mod n 与 m1 = n - one（即 -1 的蒙域剩余）二次探测：
 *       y ∈ {1,-1} 或 y^(2^j) = -1（0 < j < t，n-1 = d*2^t）
 * @return 1 = 通过基 2 的 Miller-Rabin 测试；0 = 合数
 */
static int ipn_sprp_base2_(
    lmmp_mont_t* restrict mc,
    mp_srcptr           d,
    mp_bitcnt_t       dbits,
    mp_bitcnt_t            t,
    mp_ptr              u,
    mp_ptr restrict    prod
) {
    mp_size_t nn = mc->n;

    /* 蒙域初值 one+one = 蒙域 2，逐位平方 + bit=1 倍加 */
    ipn_mont_dbl_(u, mc->one, mc);
    for (mp_bitcnt_t i = dbits - 1; i-- > 0;) {
        lmmp_sqr_(prod, u, nn);
        lmmp_mont_redc_(u, prod, mc);
        if ((d[i / LIMB_BITS] >> (i % LIMB_BITS)) & 1)
            ipn_mont_dbl_(u, u, mc);
    }

    /* 二次探测：y ∈ {1,-1} 或 y^(2^j) = -1（0 < j < t，共 t-1 次平方） */
    int ret = 0;
    if (lmmp_cmp_(u, mc->one, nn) == 0 || lmmp_cmp_(u, mc->m1, nn) == 0) {
        ret = 1;
    } else {
        for (; t > 1; t--) {
            lmmp_sqr_(prod, u, nn);
            lmmp_mont_redc_(u, prod, mc);
            if (lmmp_cmp_(u, mc->m1, nn) == 0) {
                ret = 1;
                break;
            }
        }
    }
    return ret;
}

/* ============ 1 limb 基底 SPRP 特化：短乘短除内核 ============ */

/*
    蒙域表示对裸标量线性：x̃ ≡ x*B^nn (mod n)，两侧乘裸整数 a 得
    x̃*a ≡ (a*x)*B^nn，即乘裸标量后仍是同一模数下的合法蒙域剩余。故乘底步
    既不需要 REDC，也不需要把基底提升成蒙域形式 a*B^nn（提升即全宽，优势尽
    失）；1 limb 基底用二进制梯子而非窗口表（窗口表条目 a^j 全宽，会把短乘
    优势全部吃掉）。

    1 limb 基底（ipn_sprp_1_）：t = x̃*a < n*B，故商 q = t div n 恰为
    1 limb。归一化移位 sh = clz(n[nn-1])、归一化除数顶 2 limb (d1,d0) 及其
    2/1 逆元于上下文初始化时一次算好；每步以 t<<sh 的顶 3 limb (u2,u1,u0)
    对 (d1,d0) 作 3/2 除法（_udiv_qr_3by2）估商 q̂。由 t<<sh < (n<<sh)*B 知
    q̂ 的顶 limb u2 <= d1：u2 == d1 时真商必为 B-1 或 B-2（取 q̂ = B-1，至多
    回补一次）；u2 < d1 时（u2,u1,u0）< (d1,d0)*B 成立、3/2 除法契约满足，
    截去 t 与 n 的低位分别使 q̂ >= q、q̂ <= q+1（实测回补率 ~0，2/1 估商则
    高达 15%~34%，故取 3/2），乘减后回补 n 至多 1 次。除末尾回补外全程只有
    mul_1 与 submul_1 两趟 O(nn)，无除法循环；平方步仍为全宽 sqr_ + REDC
    （本测试的复杂度主导项，与基底宽无关，故不动）。

    2 limb 及以上基底不再特化（裁定沿用 lmmp_powmod_odd_mont_ 窗口梯子）：
    短乘短除的收益来自"以两趟 O(nn) 替代一次全宽乘"，在 nn=3（唯一可能
    采样到 <=2 limb 基底的尺寸）全宽乘仅约 nn^2 = 9 次乘加，而 lmmp_div_
    的固定开销（归一化、临时分配、部分除 + 修正）反超其节省——实测 2 limb
    基底走"mul_1 + 移位 addmul_1 + lmmp_div_"内核比通用路径慢约 22%，
    nn>=8 才反超（+11%~24%）；而 nn>=4 的均匀采样基底退化为 <=2 limb 的
    概率仅 ~2^-64，分派无从生效。故宽度分派只保留宽度 1（见
    lmmp_is_prime_n_ 内注释）。

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
 * @brief 短基底 MR 单轮：基底为 1 limb 裸标量，梯子与探测全程蒙域
 * @param mc 蒙域上下文（含 one/m1，已完成 lmmp_mont_init_）
 * @param sd 短除上下文（ipn_sdiv_init_ 就绪，模数同 mc）
 * @param a 基底（1 个limb，a < B）
 * @param d (n-1)>>t（奇）
 * @param dbits d 的位长
 * @param t v2(n-1)
 * @param u 梯子累加器兼探测载体（mc->n 个limb）
 * @param prod 工作区（2*mc->n 个limb，平方全积，出口被破坏）
 * @warning 2 <= a <= [mc->m,mc->n]-2（契约域 n > 2^128 > B 恒成立），
 *           sep(u,prod), sep([u|prod], mc 内各缓冲区)
 * @return 1 = 通过该基底的 Miller-Rabin 测试；0 = 合数
 * @note SPRP 判据与基底 2 轮一致：u ∈ {one,m1} 或 u^(2^j) 命中 m1
 */
static int ipn_sprp_1_(
    lmmp_mont_t* restrict   mc,
    const ipn_sdiv_t*       sd,
    mp_limb_t                a,
    mp_srcptr                d,
    mp_bitcnt_t          dbits,
    mp_bitcnt_t               t,
    mp_ptr                   u,
    mp_ptr restrict       prod
) {
    mp_size_t nn = mc->n;

    /* 梯子初值 u = a*B^nn mod n = one*a：复用同一乘底内核，免去基底入蒙域的
       全宽 redcify 除法 */
    lmmp_copy(u, mc->one, nn);
    ipn_sdiv_mul1_(u, a, sd);

    /* 二进制 L2R 梯子：逐位平方 + bit=1 乘底 */
    for (mp_bitcnt_t i = dbits - 1; i-- > 0;) {
        lmmp_sqr_(prod, u, nn);
        lmmp_mont_redc_(u, prod, mc);
        if ((d[i / LIMB_BITS] >> (i % LIMB_BITS)) & 1)
            ipn_sdiv_mul1_(u, a, sd);
    }

    /* 蒙域二次探测：y ∈ {one,m1} 或 y^(2^j) 命中 m1（0 < j < t，共 t-1 次
       sqr+REDC；免出蒙域 REDC 与旧路径的全宽除法尾部） */
    int ret = 0;
    if (lmmp_cmp_(u, mc->one, nn) == 0 || lmmp_cmp_(u, mc->m1, nn) == 0) {
        ret = 1;
    } else {
        for (mp_bitcnt_t j = t - 1; j > 0; j--) {
            lmmp_sqr_(prod, u, nn);
            lmmp_mont_redc_(u, prod, mc);
            if (lmmp_cmp_(u, mc->m1, nn) == 0) {
                ret = 1;
                break;
            }
        }
    }
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
static int ipn_lucas_i_(
    mp_srcptr          np,
    mp_size_t          nn,
    lmmp_mont_t* restrict mc,
    mp_ptr restrict   prod
) {
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

    /*
       蒙域段由调用者提供（共享上下文），本函数仅分配阶梯段（嵌套 TEMP）：
         [prod(2n)]   平方全积（REDC 被归约数，调用者段）
         [np1(n+1)]   n+1（防全 1 顶进位越顶）→ d 所在段
         [Uk|Uk1(2n)] U 阶梯状态对 (U_k, U_{k+1})
         [T1|T2|T3(3n)] 差值平方 / A / B 载体与标量乘输出
    */
    mp_ptr restrict arena = TALLOC_TYPE(6 * nn + 1, mp_limb_t);
    mp_ptr restrict np1 = arena;
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

    /* 阶梯初态 k=1：(U_1, U_2) = (1, 1)（P=1, U_0=0 → U_2 与 Q 无关） */
    lmmp_copy(Uk, mc->one, nn);
    lmmp_copy(Uk1, mc->one, nn);

    mp_bitcnt_t dbits = (dn - 1) * LIMB_BITS + lmmp_limb_bits_(d[dn - 1]);
    for (mp_bitcnt_t i = dbits - 1; i-- > 0;) {
        ipn_mont_sub_(T1, Uk1, Uk, mc);         /* (U_{k+1}-U_k)R */
        lmmp_sqr_(prod, Uk1, nn);
        lmmp_mont_redc_(T2, prod, mc);          /* A = U_{k+1}^2 R */
        lmmp_sqr_(prod, Uk, nn);
        lmmp_mont_redc_(T3, prod, mc);          /* B = U_k^2 R */
        lmmp_sqr_(prod, T1, nn);
        lmmp_mont_redc_(T1, prod, mc);          /* D = (U_{k+1}-U_k)^2 R */
        ipn_mont_mul_small_(Uk, T3, Qabs, !Dneg, mc); /* Q*U_k^2 R */
        ipn_mont_sub_(Uk1, T2, Uk, mc);         /* U_{2k+1} = A - Q*B */
        ipn_mont_sub_(Uk, T2, T1, mc);          /* U_{2k}   = A - D */
        if ((d[i / LIMB_BITS] >> (i % LIMB_BITS)) & 1) {
            ipn_mont_mul_small_(T3, Uk, Qabs, !Dneg, mc); /* Q*U_{2k} R */
            mp_ptr sw = Uk; Uk = Uk1; Uk1 = sw;  /* (U_{2k+1}, _) */
            ipn_mont_sub_(Uk1, Uk, T3, mc);     /* U_{2k+2} = U_{2k+1} - Q*U_{2k} */
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
    ipn_mont_dbl_(T1, Uk1, mc);
    ipn_mont_sub_(T1, T1, Uk, mc); /* V_d = 2U_{d+1} - U_d */
    if (lmmp_zero_q_(T1, nn)) {
        ret = 1;
        goto done;
    }
    if (s > 1) {
        /* Q^d R = (V_d^2 - D*U_d^2)R/4：两平方 REDC，D 以小标量合成，
           再模 n 两次折半（GMP 在普通域有精确整除 /4 便车，蒙域以
           折半替代，O(n)） */
        lmmp_sqr_(prod, T1, nn);
        lmmp_mont_redc_(T2, prod, mc); /* V_d^2 R */
        lmmp_sqr_(prod, Uk, nn);
        lmmp_mont_redc_(Uk, prod, mc); /* U_d^2 R（U_d 槽至此耗尽） */
        ipn_mont_mul_small_(T3, Uk, Dabs, 0, mc);
        if (!Dneg)
            ipn_mont_sub_(T2, T2, T3, mc); /* D>0：V_d^2 - D*U_d^2 */
        else
            ipn_mont_add_(T2, T2, T3, mc); /* D<0：V_d^2 + |D|*U_d^2 */
        ipn_mont_half_(T2, mc); /* (V^2 - D U^2) = 4Q^d → 2Q^d */
        ipn_mont_half_(T2, mc); /* → Q^d（阶为蒙域剩余） */
        for (; s > 1; s--) {
            ipn_mont_dbl_(T3, T2, mc); /* 2Q^m R */
            lmmp_sqr_(prod, T1, nn);
            lmmp_mont_redc_(Uk, prod, mc); /* V_m^2 R */
            lmmp_sqr_(prod, T2, nn);
            lmmp_mont_redc_(T2, prod, mc); /* Q^{2m} R（旧值已被消费） */
            ipn_mont_sub_(T1, Uk, T3, mc); /* V_{2m} = V_m^2 - 2Q^m */
            if (lmmp_zero_q_(T1, nn)) {
                ret = 1;
                goto done;
            }
        }
    }

done:
    TEMP_FREE;
    return ret;
}

int lmmp_is_strong_lucas_(mp_srcptr np, mp_size_t nn) {
    lmmp_param_assert(np != NULL);
    lmmp_param_assert(nn > 2 && np[nn - 1] > 0);
    lmmp_param_assert(np[0] % 2 == 1);

    TEMP_DECL;
    lmmp_mont_t mc;
    mp_size_t mn = lmmp_mont_need_(nn, 1);
    mp_ptr restrict arena = TALLOC_TYPE(mn + 2 * nn, mp_limb_t);
    lmmp_mont_init_(&mc, np, nn, arena, 1, arena + mn);
    int ret = ipn_lucas_i_(np, nn, &mc, arena + mn);
    lmmp_mont_free_(&mc);
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

    /* n-1 = d*2^t（d 奇）的形状参数：t 与 d 位长先于单块工作区求出
       （全域轮奇次幂表 ppn 依 d 位长取窗口尺寸） */
    mp_bitcnt_t t, dbits;
    {
        TEMP_DECL;
        mp_ptr nm1 = TALLOC_TYPE(nn, mp_limb_t);
        lmmp_copy(nm1, np, nn);
        lmmp_dec(nm1);
        t = ipn_ctz_(nm1);
        mp_size_t bn = ipn_norm_(nm1, nn);
        dbits = (mp_bitcnt_t)(bn - 1) * LIMB_BITS + lmmp_limb_bits_(nm1[bn - 1]) - t;
        TEMP_FREE;
    }

    int rn = ipn_rn_rounds[strength];
    mp_size_t ppn = rn > 0 ? (mp_size_t)nn << (lmmp_powmod_win_size_(dbits) - 1) : 0;

    TEMP_DECL;
    lmmp_mont_t mc;
    ipn_sdiv_t sd;
    /*
       全程共享单块工作区（蒙域上下文 init 一次，贯穿基底 2 / 随机基底
       各轮 / 强 Lucas，各轮免 binvert、one/m1 的 redcify 与 FFT 变换
       缓存重建）：
         [蒙域段]      basecase 2n | 中层 8n | 折叠 8n+msz（lmmp_mont_need_）
         [prod(2n)]    平方全积兼 redcify 工作区（各轮共用）
         [nm1(n)]      n-1（d 分解来源与全域轮采样上界）
         [d(n)]        (n-1)>>t（各随机轮固定不变）
         [u(n)]        蒙域梯子累加器/探测载体（基底2、短基底、全域轮接力）
         [bs(n)]       全域轮采样基底
         [b2(n)]       全域轮表生成暂存（b²·R mod n，出口死）
         [pp(ppn)]     全域轮奇次幂表（rn==0 档免配）
       强 Lucas 的 U/T 阶梯段经嵌套 TEMP 分配，蒙域段与 prod 复用外层
    */
    mp_size_t mn = lmmp_mont_need_(nn, 1);
    mp_ptr restrict arena = TALLOC_TYPE(mn + 8 * nn + ppn, mp_limb_t);
    mp_ptr restrict prod = arena + mn;
    mp_ptr restrict nm1 = prod + 2 * nn;
    mp_ptr restrict d = nm1 + nn;
    mp_ptr restrict u = d + nn;
    mp_ptr restrict bs = u + nn;
    mp_ptr b2 = bs + nn;
    mp_ptr pp = b2 + nn;

    lmmp_mont_init_(&mc, np, nn, arena, 1, prod);
    ipn_sdiv_init_(&sd, np, nn);

    /* n-1 = d*2^t：低零 limb 折叠为指针偏移，余位单次移位 */
    lmmp_copy(nm1, np, nn);
    lmmp_dec(nm1);
    mp_size_t dn = ipn_norm_(nm1 + t / LIMB_BITS, nn - t / LIMB_BITS);
    lmmp_shr_(d, nm1 + t / LIMB_BITS, dn, t % LIMB_BITS);
    dn = ipn_norm_(d, dn);

    /* 各档公共首步：基底 2 特化 MR（BPSW 的 MR 半部，倍加梯子） */
    if (!ipn_sprp_base2_(&mc, d, dbits, t, u, prod)) goto composite;

    /* 1 limb 基底 MR：a ∈ [3,B-1] 均匀（全局 RNG 单 limb 直取，免去
       lmmp_random_ 的整块播种开销）。契约域 n > 2^128 > B 保证任意 a 满足
       3 <= a <= n-2，无需拒绝采样 */
    for (int r = ipn_r1_rounds[strength]; r > 0; r--) {
        mp_limb_t a;
        do {
            a = lmmp_randlimb_();
        } while (a < 3);
        if (!ipn_sprp_1_(&mc, &sd, a, d, dbits, t, u, prod)) goto composite;
    }

    /* 全域（n limb）基底 MR（至多 N 轮，检出即止）：b ∈ [2,n-2] 均匀。
       顶 limb 掩码到 n 的位长内再整体拒绝采样：位长内均匀故 P(>=n) <= 1/2，
       期望 <= 2 次填充，免去每轮一次 nn/nn 全除归约（旧路径 lmmp_div_）。
       1 limb 短基底轮与全域轮的梯子均留在蒙域，y 的 ±1 探测以 one/m1
       为基准，尾部探测平方走 sqr_+REDC（与基底 2 轮同构） */
    uint tb = lmmp_limb_bits_(np[nn - 1]);
    mp_limb_t tmask = (tb == LIMB_BITS) ? ~(mp_limb_t)0 : (((mp_limb_t)1 << tb) - 1);
    for (int r = rn; r > 0; r--) {
        for (;;) {
            lmmp_random_(bs, nn);
            bs[nn - 1] &= tmask;
            if (lmmp_cmp_(bs, nm1, nn) < 0 && (bs[0] >= 2 || !lmmp_zero_q_(bs + 1, nn - 1)))
                break;
        }
        /* 基底有效 limb 宽：nn=3 且 n 接近 2^128 时 <=1 limb 的基底占比可达
           可观比例（均匀采样下约 2^128/n），窄基底走短乘可省下全宽乘。
           宽度 2 不特化：其唯一可能出现的尺寸 nn=3 上短除内核慢于通用路径
           （见本节顶部实测注），而 nn>=4 采样退化为 <=2 limb 的概率 ~2^-64 */
        mp_size_t bw = nn;
        while (bw > 1 && bs[bw - 1] == 0) bw--;
        if (bw == 1) {
            if (!ipn_sprp_1_(&mc, &sd, bs[0], d, dbits, t, u, prod)) goto composite;
            continue;
        }
        lmmp_powmod_odd_mont_(u, bs, d, dn, &mc, b2, prod, pp);
        int ok = 0;
        if (lmmp_cmp_(u, mc.one, nn) == 0 || lmmp_cmp_(u, mc.m1, nn) == 0) {
            ok = 1;
        } else {
            for (mp_bitcnt_t j = t - 1; j > 0; j--) {
                lmmp_sqr_(prod, u, nn);
                lmmp_mont_redc_(u, prod, &mc);
                if (lmmp_cmp_(u, mc.m1, nn) == 0) {
                    ok = 1;
                    break;
                }
            }
        }
        if (!ok) goto composite;
    }

    /* 4 档及以上：BPSW 的 Lucas 半部（嵌套 TEMP 分配阶梯段，蒙域段复用） */
    if (strength >= 4 && !ipn_lucas_i_(np, nn, &mc, prod)) goto composite;
    lmmp_mont_free_(&mc);
    TEMP_FREE;
    return 2;

composite:
    lmmp_mont_free_(&mc);
    TEMP_FREE;
    return 0;
}

/* ============ Proth 形状素性检验（lmmp_is_prothprime_） ============ */

/*
    普罗斯定理：N = k*2^t+1（k 奇，k < 2^t）为素数 ⟺ 存在 a 使
    a^((N-1)/2) ≡ -1 (mod N)。正向（≡ -1 ⟹ N 素）为 Pocklington 型
    阶论证，无需分解 k：任一素因子 p 满足 ord_p(a) | N-1 且
    ∤ (N-1)/2，故 v2(ord_p(a)) = t，2^t | p-1，p >= 2^t+1 > sqrt(N)
    （k < 2^t ⟹ N < 2^(2t)+1），与合数必有不超 sqrt(N) 的素因子矛盾。
    反向仅在 Jacobi(a,N) = -1 的见证上成立（N 素时 Euler 判据强制
    a^((N-1)/2) ≡ (a|N) = -1）：故 Jacobi = 0 即 gcd(a,N) > 1 判合数
    （本域 a < N 恒成立），首个 Jacobi = -1 的奇数 a 上单条梯子出双
    方向定论；Jacobi = +1 的基对判决无贡献（素数给 +1，平方数亦恒给
    +1，(a|p^2) = (a|p)^2 且 a^((p^2-1)/2) = (±1)^(p+1) = +1，两侧
    不可区分），直接跳过。

    见证搜索沿奇数 a = 3,5,7,... 至防御上界 IPN_PROTH_WITNESS_MAX（与
    ipn_lucas_i_ 的 D 搜索同哲学：非平凡特征必存在见证，可证的小上界
    不存在）。终止性：唯一无
    -1 见证的情形是 N 为完全平方，而本域（N >= 2^64，kk < 2^32，
    ne = t >= 64）内平方不可能——若 N = p^2 且 (p-1)(p+1) = kk*2^t，
    记 p-1 = 2^a*u、p+1 = 2^b*v（u,v 奇，{a,b} 含 1），p >= 2^32 与
    uv = kk < 2^32 联立迫使小 v2 侧奇部 >= 2^31、另一侧奇部 = 1，
    只剩 p ∈ {2^32-1, 2^33-1, 2^32+1}，均非素——故 (·|N) 非平凡，
    必有奇素数见证 < N，搜索有限步终止（实际首见证个位数到几十）。

    梯子结构：指数 (N-1)/2 = k*2^(t-1) 低 t-1 位全零——k 部分（<= 32
    位）走 L2R 二进制梯（乘底至多 31 次全宽乘，占梯子份额可忽略，故
    不做窗口/短乘特化），尾部 t-1 步为纯平方链。全程蒙域，终点与 m1
    （蒙域 -1）比较，免出蒙域归约；指数无需物化为 limb 数组，两段
    循环由 kk 与 ne 直接驱动。

    蒙域上下文走稀疏模数旁路（lmmp_mont_sp_t）：驱动域内 N = 1 +
    (kk<<s)·B^p 恒命中 +1 形（kk < 2^32 ⟹ 簇 <= 2 limb），REDC 每步
    O(n)（伸缩恒等式坍缩，见 impl/powmod.h 稀疏节注释），梯子整体
    平方主导；m1（蒙域 -1）经一次 redcify 除法装配 one 后取 N − one，
    不经 lmmp_mont_t 的通用三层分派。
*/

/*
    普罗斯定理驱动（N = kk*2^ne+1 已装配为 [np,nn]，kk 奇 < 2^32，
    ne >= 64 ⟹ nn >= 2、limb0 恒纯 1（稀疏形状必命中）、kk < 2^ne 自动
    成立）：首个 Jacobi = -1 的奇数 a 上跑 a^((N-1)/2) 蒙域梯子
*/

/* 见证搜索上界（防御性）：域内非平方引理保证存在 Jacobi = -1 的奇素
   见证（实际最小见证个位数到几十），上界仅为拦截不可达的异常情形
   （形状前提被破坏/宇宙射线）——越界回退强度分档检验（概率性），
   保持判决正确性而非悬挂 */
#define IPN_PROTH_WITNESS_MAX 65535u

static int ipn_proth_i_(mp_srcptr np, mp_size_t nn, ulong kk, mp_bitcnt_t ne) {
    /* 见证搜索（纯 Jacobi，无分配）：首个 Jacobi != +1 者即决 */
    ulong a;
    int j = 1; /* 初值 1：上界耗尽/空转均归入防御回退分支 */
    for (a = 3; a <= IPN_PROTH_WITNESS_MAX; a += 2) {
        j = ipn_jacobi_D_((uint)a, 0, np, nn);
        if (j != 1) break;
    }
    if (j == 0) return 0; /* gcd(a,N) > 1 且 a < N（N > 2^64 > a）*/
    if (j == 1) return lmmp_is_prime_n_(np, nn, 4); /* 上界耗尽（不可达）：防御回退，值透传 */

    TEMP_DECL;
    lmmp_mont_sp_t mc;
    int form = lmmp_mont_sp_init_(&mc, np, nn);
    lmmp_debug_assert(form == 1);
    /*
       [m1(n)]     蒙域 -1（one 装配载体转 m1）
       [prod(2n)]  redcify/平方/乘底全积（sp REDC 被归约数）
       [u(n)]      梯子累加器
       [abar(n)]   蒙域基底 ā（先作普通域基底载体，eqsep 原地 redcify）
    */
    mp_ptr restrict m1 = TALLOC_TYPE(5 * nn, mp_limb_t);
    mp_ptr restrict prod = m1 + nn;
    mp_ptr restrict u = prod + 2 * nn;
    mp_ptr restrict abar = u + nn;

    /* m1 = N − one（蒙域 -1）：one = B^nn mod N 经一次 redcify 除法 */
    lmmp_zero(m1, nn);
    m1[0] = 1;
    lmmp_mont_redcify_(abar, m1, nn, prod, np);
    (void)lmmp_sub_n_(m1, np, abar, nn);

    /* ā = a*B^nn mod N */
    lmmp_zero(abar, nn);
    abar[0] = a;
    lmmp_mont_redcify_(abar, abar, nn, prod, np);

    /* k 部分：u = ā^kk（L2R；kk <= 2^32-1） */
    lmmp_copy(u, abar, nn);
    for (mp_bitcnt_t i = lmmp_limb_bits_(kk) - 1; i-- > 0;) {
        lmmp_sqr_(prod, u, nn);
        lmmp_mont_sp_redc_(u, prod, &mc);
        if ((kk >> i) & 1) {
            lmmp_mul_n_(prod, u, abar, nn);
            lmmp_mont_sp_redc_(u, prod, &mc);
        }
    }

    /* 尾部 t-1 次纯平方：u = u^(2^(ne-1)) */
    for (mp_bitcnt_t i = ne - 1; i-- > 0;) {
        lmmp_sqr_(prod, u, nn);
        lmmp_mont_sp_redc_(u, prod, &mc);
    }

    /* a^((N-1)/2) ≡ -1 ⟺ 蒙域终值 = m1：素数（定理正向）；否则该
       Jacobi = -1 见证下必为合数（Euler 反证） */
    int ret = lmmp_cmp_(u, m1, nn) == 0;
    TEMP_FREE;
    return ret;
}

int lmmp_is_prothprime_(uint k, mp_size_t n) {
    lmmp_param_assert(k > 0 && n > 0);
    lmmp_param_assert(k & 1 == 1);
    /* 契约域：k 奇（无 2 因子归一化），ne := n 即 v2(N-1) */
    ulong kk = k;
    mp_bitcnt_t ne = (mp_bitcnt_t)n;
    uint bk = (uint)lmmp_limb_bits_(kk);

    /* N < 2^64（ne + bitlen(kk) <= 64）：确定性小数路径 */
    if (ne + bk <= 64) return lmmp_is_prime_ulong_((kk << ne) + 1) ? 1 : 0;

    /* ne < 64：kk 簇低位与 limb0 的 1 共享 limb（N < 2^32*2^63 < 2^96，恰
       双 limb），非稀疏形状——走 lmmp_is_prime_2_ 三态路径值透传
       （<SWbound 确定性返回 1，之上 BPSW 型返回 2） */
    if (ne < 64) {
        u128 N = ((u128)kk << ne) + 1;
        return lmmp_is_prime_2_((mp_limb_t)N, (mp_limb_t)(N >> 64));
    }

    /* 普罗斯定理驱动域：ne >= 64 ⟹ N >= 2^64 且 limb0 恒为纯 1（kk 簇整
       体位于 limb >= 1，稀疏形状必命中），nn >= 2，kk < 2^32 <= 2^ne（定
       理条件 k 奇、k < 2^t 自动成立）。装配 N = kk*2^ne + 1 */
    mp_size_t nn = (mp_size_t)((ne + bk - 1) / LIMB_BITS) + 1;
    TEMP_DECL;
    mp_ptr np = TALLOC_TYPE(nn, mp_limb_t);
    lmmp_zero(np, nn);
    np[0] = 1;
    mp_size_t w = (mp_size_t)(ne / LIMB_BITS);
    mp_bitcnt_t s = ne % LIMB_BITS;
    if (s == 0) {
        np[w] = (mp_limb_t)kk;
    } else {
        np[w] |= (mp_limb_t)(kk << s);
        mp_limb_t hi = (mp_limb_t)(kk >> (LIMB_BITS - s));
        if (hi) np[w + 1] = hi; /* hi != 0 ⟺ 顶位跨 limb ⟺ w+1 == nn-1 */
    }
    int ret = ipn_proth_i_(np, nn, kk, ne);
    TEMP_FREE;
    return ret;
}
