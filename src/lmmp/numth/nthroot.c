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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/log2_exp2.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


/**
 * @brief 取 [p,n] >> s 的低 64bit（自 cbrt.c 同名辅助移植）
 * @param p 源操作数指针
 * @param n 源操作数的 limb 长度
 * @param s 右移位数
 * @return (p >> s) 的低 64bit
 * @warning n>0, 0 <= s < 64*n
 */
static inline mp_limb_t lmmp_shr64_(mp_srcptr p, mp_size_t n, ulong s) {
    mp_size_t w = (mp_size_t)(s >> 6);
    ulong b = s & 63;
    mp_limb_t lo = p[w];
    mp_limb_t hi = (w + 1 < n) ? p[w + 1] : 0;
    return b ? ((lo >> b) | (hi << (64 - b))) : lo;
}

/*
    窗口截断幂链的截断+归一化：v（*plen limb）截至顶 wlimbs limb 并把
    顶 bit 左移到 limb 顶（MSB=1），丢弃位数（bit 计）累入 *dr。
    语义不变量：v_stored·2^dr ≈ 真值。
*/
/**
 * @brief 截断幂链的逐层截断+归一化（nthroot_pow_trunc_ 的层内步骤）
 * @param v 值缓冲（原地覆写为截断结果）
 * @param plen 输入 v 的有效长度，输出截断后长度
 * @param dr 丢弃位数（bit 计）的累计记账（输出累入本次截断量）
 * @param wlimbs 窗口宽度（limb）
 * @warning plen!=NULL, dr!=NULL
 * @note 不变量 [v,*plen]·2^(*dr) ≈ 调用前的真值；顶 limb 恒 MSB=1
 *       （或 *plen==0），供后续截断/除法使用。
 *       顶零 limb 至多 1 个（值界：入口/sqr 顶恒非零；mul 积
 *       ≥ 2^(64l-65) ⟹ bitlen ≥ 64(l-1)），单次顶 limb 探测即精确。
 */
static void nthroot_chop_(mp_ptr v, mp_size_t *plen, slong *dr, mp_size_t wlimbs) {
    mp_size_t l = *plen;
    if (l > wlimbs) {
        *dr += 64 * (slong)(l - wlimbs);
        lmmp_copy(v, v + (l - wlimbs), wlimbs);  // memmove 前向安全
        l = wlimbs;
    }
    if (l > 0 && v[l - 1] == 0) --l;
    if (l > 0) {
        unsigned b = (unsigned)lmmp_limb_bits_(v[l - 1]);
        if (b < 64) {
            lmmp_shl_(v, v, l, 64 - b);  // 顶 bit 移至 63，进位恒 0
            *dr -= 64 - b;
        }
    }
    *plen = l;
}

/*
    截断幂链：dst（归一化，≤ wlimbs limb）≈ base^exp / 2^(*drop)

    左扫二进制幂，每次 sqr 后先截断再乘 base（乘积 ≤ wlimbs+bn ≤
    2·wlimbs limb），bufa/bufb 乒乓（各 2·wlimbs+8 limb）。相对误差
    ≤ (2·log2(exp)+4)·2^-(64·wlimbs)，由层的守卫 limb 吸收。
*/
/**
 * @brief 计算窗口截断幂链 [dst] ≈ [base,bn]^exp / 2^(*drop)
 * @param dst 结果指针（归一化，≤ wlimbs limb）
 * @param drop 输出：为匹配窗口而丢弃的位数（bit 计，精确记账）
 * @param wlimbs 窗口宽度（limb）
 * @param base 底数指针（顶 limb 允许为 0，内部截断时归一）
 * @param bn 底数的 limb 长度（≤ wlimbs 或顶 limb 为 0 等价归一后 ≤ wlimbs）
 * @param exp 指数
 * @param bufa 幂链乒乓缓冲（≥ 2·wlimbs+8 limb）
 * @param bufb 同 bufa，sep(bufa,bufb)
 * @return dst 长度
 * @warning exp >= 2, sep(dst,bufa,bufb), base 与两缓冲 sep
 * @note 不变量 [dst,返回值]·2^(*drop) = base^exp·(1±ε)，
 *       ε ≤ (2·log2(exp)+4)·2^-(64·wlimbs)，由调用方的守卫 limb 吸收。
 */
static mp_size_t nthroot_pow_trunc_(
    mp_ptr dst, slong *drop, mp_size_t wlimbs,
    mp_srcptr base, mp_size_t bn, ulong exp, mp_ptr bufa, mp_ptr bufb
) {
    mp_ptr P = bufa;
    mp_size_t pl = bn < wlimbs ? bn : wlimbs;
    lmmp_copy(P, base + (bn - pl), pl);  // 底数超窗时取顶 pl limb（低位 64·(bn-pl) bit 计入 dr）
    slong dr = 64 * (slong)(bn - pl);
    nthroot_chop_(P, &pl, &dr, wlimbs);

    int hb = lmmp_limb_bits_(exp) - 1;
    for (int i = hb - 1; i >= 0; --i) {
        mp_ptr Q = (P == bufa) ? bufb : bufa;  // sqr 目标（P 旧值即死）
        lmmp_sqr_(Q, P, pl);
        mp_size_t ql = 2 * pl;
        dr *= 2;  // (P·2^dr)² = P²·2^(2dr)：平方使累积 drop 翻倍（乘 base 不变）
        nthroot_chop_(Q, &ql, &dr, wlimbs);
        if ((exp >> i) & 1) {
            // 乘回 base：写回 P 旧缓冲（长操作数在前）
            if (ql >= bn) lmmp_mul_(P, Q, ql, base, bn);
            else          lmmp_mul_(P, base, bn, Q, ql);
            pl = ql + bn;
            nthroot_chop_(P, &pl, &dr, wlimbs);
        } else {
            P = Q;
            pl = ql;
        }
    }
    lmmp_copy(dst, P, pl);
    *drop = dr;
    return pl;
}

/*
    Nseg 提取：A 的顶 nlimbs 个 limb（左补零语义）

        值 = floor(A / 2^s)，s = bl - 64·nlimbs（s < 0 时为 A·2^(-s)）

    即把 A 视作左端补零到恰 nlimbs limb 的整体；值域恰 [2^(64·nlimbs-1),
    2^(64·nlimbs)) 的截断情形顶 bit 为 A 的顶 bit。
*/
/**
 * @brief 提取 [numa,na] 的顶 nlimbs 个 limb（左补零语义）
 * @param dst 结果指针（容量 ≥ nlimbs+1）
 * @param nlimbs 提取的 limb 数
 * @param numa 源操作数指针
 * @param na 源操作数的 limb 长度
 * @param bl bitlen([numa,na])（调用方已算好，避免重扫）
 * @warning na>0, numa[na-1]!=0, sep(dst,numa)
 * @note 值 = floor(A/2^s)，s = bl-64·nlimbs（s<0 时为 A·2^(-s)）；
 *       s>0 且移出 bit 非零时值暂占 nlimbs+1 limb，顶 limb 恒 0。
 */
static void nthroot_top_limbs_(
    mp_ptr dst, mp_size_t nlimbs, mp_srcptr numa, mp_size_t na, ulong bl
) {
    slong s = (slong)bl - 64 * (slong)nlimbs;
    if (s >= 0) {
        mp_size_t off = (mp_size_t)(s >> 6);
        unsigned bit = (unsigned)(s & 63);
        if (off >= na) {
            lmmp_zero(dst, nlimbs);
            return;
        }
        mp_size_t m = na - off;  // ≤ nlimbs+1（bit>0 时顶 limb 移位后恒 0）
        lmmp_copy(dst, numa + off, m);
        lmmp_zero(dst + m, nlimbs + 1 - m);
        if (bit) lmmp_shr_(dst, dst, m, bit);  // 原地右移（契约允许）
    } else {
        mp_size_t loff = (mp_size_t)((-s) >> 6);
        unsigned bit = (unsigned)((-s) & 63);
        lmmp_zero(dst, nlimbs);
        mp_size_t m = na;
        if (m > nlimbs - loff) m = nlimbs - loff;
        if (bit) {
            mp_limb_t cy = lmmp_shl_(dst + loff, numa, m, bit);
            if (loff + m < nlimbs) dst[loff + m] = cy;  // 进位必 0（bitlen 恒等）
        } else {
            lmmp_copy(dst + loff, numa, m);
        }
    }
}

/**
 * @brief 零扩展比较：把 [a,an] 与 [b,bn] 视作等长后比较
 * @param a 左操作数指针
 * @param an 左操作数的比较长度（高位允许 0）
 * @param b 右操作数指针
 * @param bn 右操作数的比较长度（高位允许 0）
 * @return 比较结果（<0 / 0 / >0 对应 a < / = / > b）
 * @warning an>0, bn>0
 * @note 高位段非零者大，再比较公共低位段——避免为 cmp_ 反复 trim。
 */
static int nthroot_cmp_ext_(mp_srcptr a, mp_size_t an, mp_srcptr b, mp_size_t bn) {
    for (mp_size_t i = an; i-- > bn;) if (a[i]) return 1;
    for (mp_size_t i = bn; i-- > an;) if (b[i]) return -1;
    for (mp_size_t i = an < bn ? an : bn; i-- > 0;)
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return 0;
}

/**
 * @brief [y,yn] += 1 并维护精确长度
 * @param y 值指针（原地）
 * @param yn 值的精确 limb 长度（顶 limb 非零）
 * @return 加 1 后的精确 limb 长度
 * @warning yn>0, y 容量 ≥ yn+1
 * @note 全 1 形态进位写到 y[yn]（长度 +1），其余形态长度不变——
 *       判定不读 y[yn]，缓冲上方残留垃圾不影响。
 */
static mp_size_t nthroot_inc_len_(mp_ptr y, mp_size_t yn) {
    mp_size_t i = 0;
    while (i < yn && y[i] == LIMB_MAX) y[i++] = 0;
    if (i == yn) {
        y[yn] = 1;
        return yn + 1;
    }
    ++y[i];
    return yn;
}

/**
 * @brief [y,yn] -= 1 并维护精确长度
 * @param y 值指针（原地）
 * @param yn 值的精确 limb 长度（顶 limb 非零）
 * @return 减 1 后的精确 limb 长度
 * @warning yn>0, 值 ≥ 2（减后非零）
 * @note 仅 2^(64k) 精确形态缩短 1 limb（bitlen ≥ 64(yn-1)+2 的值减 1
 *       后 bitlen ≥ 64(yn-1)+1，顶 limb 恒非零）——单次顶 limb 探测。
 */
static mp_size_t nthroot_dec_len_(mp_ptr y, mp_size_t yn) {
    lmmp_dec(y);
    if (y[yn - 1] == 0) --yn;
    return yn;
}

/**
 * @brief [p,n] += c0 + c1·B（探针证书的小常数增量）
 * @param p 值指针（原地）
 * @param n 值的 limb 长度
 * @param c0 低 limb 增量
 * @param c1 次 limb 增量
 * @warning n >= 2
 * @note 调用方值域保证无 p[n] 溢出，debug 断言为绊线。
 */
static void nthroot_add_small_(mp_ptr p, mp_size_t n, mp_limb_t c0, mp_limb_t c1) {
    mp_limb_t cc = c0 ? lmmp_add_1_(p, p, n, c0) : 0;
    lmmp_debug_assert(cc == 0);
    mp_limb_t cc2 = lmmp_add_1_(p + 1, p + 1, n - 1, c1 + cc);
    lmmp_debug_assert(cc2 == 0);
}

/**
 * @brief [p,n] -= c0 + c1·B（探针证书的小常数量减）
 * @param p 值指针（原地）
 * @param n 值的 limb 长度
 * @param c0 低 limb 减量
 * @param c1 次 limb 减量
 * @warning n >= 2
 * @note 调用方值域保证无下借，debug 断言为绊线。
 */
static void nthroot_sub_small_(mp_ptr p, mp_size_t n, mp_limb_t c0, mp_limb_t c1) {
    mp_limb_t bb = c0 ? lmmp_sub_1_(p, p, n, c0) : 0;
    lmmp_debug_assert(bb == 0);
    mp_limb_t bb2 = lmmp_sub_1_(p + 1, p + 1, n - 1, c1);
    lmmp_debug_assert(bb2 == 0);
}

/*
    tp 需求（nr = ceil(na/root)，nq = nr+1 ≥ ceil(L'/64)）：

        Y, Y2   终点幂/试探幂   2·Ycap
        work    余数中转        na + 4
        xb/asmb/qb/a1/t2 精化   5·(nq + 8)
        Db      除数            2·nq + 18
        bufa/bufb 幂链乒乓      2·(2·nq + 16)
        Nseg    被除数段/移位   2·nq + 13

    Ycap = max(root·nr+8, na+root/64+4)：全幂端点幂的值界
    y^root < 2^(bl+root+1)（y ≤ T+8，层域 T ≥ 2^128 ⟹ (1+8·root/T)^root
    ≤ 1+2^-9，增值至多 1 bit）需 na+root/64+4 limb——对齐大 root 形状
    （root=10^6、na=root、nr=1）可比 root·nr+8 大 ~root/64，取 max。
    root=4 专用路线的分段（q1/r1/r2 各 na/2+2、sum na/2+3、S nr+2、
    prd 3·nr+4，合计 2·na+4·nr+16 ≤ 2·Ycap+na+4·nr+4，由 na ≤ 4·nr
    恒被覆盖）。合计 2·Ycap + na + 13·nq + 127。
*/
mp_size_t lmmp_nthroot_size_(mp_size_t na, ulong root) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(root >= 4 && root <= (1ull << 56));
    mp_size_t nr = (na + (mp_size_t)root - 1) / (mp_size_t)root;
    mp_size_t nq = nr + 1;  // ceil(L'/64) ≤ ceil((64na+root)/(64root)) + 0 ≤ nr+1
    mp_size_t ycap = (mp_size_t)root * nr + 8;
    mp_size_t yneed = na + (mp_size_t)(root >> 6) + 4;
    if (ycap < yneed) ycap = yneed;
    return 2 * ycap + na + 4 + 13 * nq + 123;
}

void lmmp_nthroot_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa, mp_size_t na, ulong root, mp_ptr tp) {
    lmmp_param_assert(na > 0 && numa != NULL && numa[na - 1] != 0);
    lmmp_param_assert(root >= 4 && root <= (1ull << 56));
    lmmp_param_assert(dsts != NULL && tp != NULL);
    mp_size_t nr = (na + (mp_size_t)root - 1) / (mp_size_t)root;  // 根长

    // 平凡域：A < 2^(64na) <= 2^root → 根为 1
    if ((ulong)na * LIMB_BITS <= root) {
        dsts[0] = 1;
        if (dstr) {
            lmmp_copy(dstr, numa, na);
            lmmp_dec(dstr);  // rem = A - 1（A >= 1，无下借）
            lmmp_zero(dstr + na, (root - 1) * nr + 1 - na);
        }
        return;
    }
    if (na == 1) {
        dsts[0] = lmmp_nthroot_ulong_(numa[0], root);
        if (dstr) {
            ulong acc = 1;
            for (ulong i = 0; i < root; ++i) acc *= dsts[0];
            dstr[0] = numa[0] - acc;
            lmmp_zero(dstr + 1, (root - 1) * nr);  // 契约区 [0, root) 余 [1, root)
        }
        return;
    }

    ulong bl = LIMB_BITS * (ulong)(na - 1) + (ulong)lmmp_limb_bits_(numa[na - 1]);

    /*
        root=4 专用路线：sqrt∘sqrt 嵌套 floor 恒等式（与顶入口的
        Ã = A·2^root 恢复同族定理）

            floor(sqrt(floor(sqrt(A)))) = floor(A^(1/4))

        双向夹逼：S = floor(A^(1/4)) ⟹ S² ≤ floor(sqrt(A)) ⟹ S⁴ ≤ A；
        反向 (S+1)² > floor(sqrt(A)) ⟹ (S+1)² ≥ floor(sqrt(A))+1 ⟹
        (S+1)⁴ > A。余数由两级 sqrtrem 展开：
            A = q1² + r1 = (S²+r2)² + r1 ⟹ A - S⁴ = r2·(q1+S²) + r1
        （q1 = floor(sqrt A)，(S,r2) = sqrtrem(q1)）。

        长度全解析（免 trim 扫描）：精确定理
        bitlen(floor(sqrt(A))) = ceil(bl/2)、bitlen(S) = ceil(bl/4)
        （2^(bl-1) ≤ A < 2^bl 两端开方夹逼，嵌套 ceil 复合），故两级
        sqrt 的输入/输出长度均由 bl 推出；r1/r2 取 sqrt_ 写满的余数区
        （含顶零）直接作 mul/add 操作数（加减乘不要求归一化），输出
        先清零 dstr 再拷贝乘积容器全长（顶零拷入零区无害）。
        实测 rem/root-only 全尺寸较通用路径快 15-40%（sqrt 自身的
        优化空间在 sqrt.c，不在本文件）。

        tp 分段（值域 na ≤ 4·nr 保证覆盖）：
        q1/r1/r2 各 na/2+2，sum na/2+3，S nr+2，prd 3·nr+4。
    */
    if (root == 4) {
        mp_ptr q1 = tp, r1 = q1 + na / 2 + 2, S = r1 + na / 2 + 2, r2v = S + nr + 2,
               sum = r2v + na / 2 + 2, prd = sum + na / 2 + 3;
        lmmp_sqrt_(q1, dstr ? r1 : NULL, numa, na, 0);
        mp_size_t nq1 = (mp_size_t)(((bl + 1) / 2 + 63) >> 6);  // bitlen(q1)=ceil(bl/2)
        lmmp_sqrt_(S, dstr ? r2v : NULL, q1, nq1, 0);
        mp_size_t sn = (mp_size_t)(((bl + 3) / 4 + 63) >> 6);  // bitlen(S)=ceil(bl/4)（= nr）
        if (dstr) {
            // rem = r2·(q1 + S²) + r1
            lmmp_sqr_(sum, S, sn);
            mp_size_t s2n = 2 * sn;
            mp_size_t un = nq1 > s2n ? nq1 : s2n;  // q1+S² 容器长（add 长在前）
            if (un > s2n) lmmp_zero(sum + s2n, un - s2n);  // sqr 只写 [0,2sn)，补零
            mp_limb_t cy = lmmp_add_(sum, sum, un, q1, nq1);
            if (cy) sum[un++] = cy;
            mp_size_t r2n = nq1 / 2 + 1;  // r2 余数区（sqrt_ 写满含零）
            mp_size_t r1n = na / 2 + 1;   // r1 余数区（sqrt_ 写满含零）
            lmmp_mul_(prd, sum, un, r2v, r2n);  // 长在前：un ≥ nq1 ≥ nq1/2+1
            mp_size_t pn = un + r2n;      // 乘积容器长（顶零 limb 无害）
            lmmp_debug_assert(pn >= r1n);  // un ≥ nq1 ≥ ceil(na/2)-1，+r2n ≥ r1n
            mp_limb_t cy2 = lmmp_add_(prd, prd, pn, r1, r1n);
            if (cy2) prd[pn++] = cy2;  // 值界 rem < 2^(192·nr+3)：pn ≥ 3nr 时进位恒 0
            mp_size_t outr = (root - 1) * nr + 1;
            if (pn > outr) pn = outr;  // 值界保证 [pn, outr) 段恒零
            lmmp_copy(dstr, prd, pn);        // 拷贝容器长（含值上方的零 limb）
            lmmp_zero(dstr + pn, outr - pn);
        }
        lmmp_copy(dsts, S, sn);  // sn == nr（bl ∈ (64(na-1), 64na] 双侧夹逼）
        return;
    }

    ulong B = bl + root;              // bitlen(Ã)
    ulong Lp = (B + root - 1) / root; // L' = bitlen(T)
    slong shx;                        // x ≈ T·2^-shx（scale 记账）

    // ---- scratch 布局（单块手动分段，生命周期接力；注释 [p,n)：p 段首，n 段长）----
    mp_size_t nqf = nr + 1;
    const mp_size_t XW = nqf + 8;          // 精化窗宽：xb/asmb/qb/a1/t2 五段共用
    const mp_size_t DW = 2 * nqf + 18;     // Db 段长（截断幂链结果/除数）
    const mp_size_t BW = 2 * (nqf + 4) + 8;  // 幂链乒乓单段长
    const mp_size_t NW = 2 * nqf + 13;     // Nseg 段长（被除数段/移位暂存）
    mp_size_t Ycap = (mp_size_t)root * nr + 8;
    {
        mp_size_t yneed = na + (mp_size_t)(root >> 6) + 4;
        if (Ycap < yneed) Ycap = yneed;  // 全幂端点幂值界（见 size_ 注）
    }
    mp_ptr Y = tp;                 // [Y,    Ycap]  终点幂/试探幂（回退）
    mp_ptr Y2 = Y + Ycap;          // [Y2,   Ycap]  试探幂 (y+1)^root（回退）
    mp_ptr work = Y2 + Ycap;       // [work, na+4]  余数中转
    mp_ptr xb = work + na + 4;     // [xb,     XW]  精化状态 x / 探针链结果 V
    mp_ptr asmb = xb + XW;         // [asmb,   XW]  商/装配累加器 / 探针 Wc·L
    mp_ptr qb = asmb + XW;         // [qb,     XW]  q·root 移位段 / 探针 slack·Sh / 回退 y+1
    mp_ptr a1 = qb + XW;           // [a1,     XW]  (root-1)·x 暂存 / y 候选
    mp_ptr t2 = a1 + XW;           // [t2,     XW]  q·root·2^-X 段 / 探针 M 低位
    mp_ptr Db = t2 + XW;           // [Db,     DW]  截断幂链结果/除数 D
    mp_ptr bufa = Db + DW;         // [bufa,   BW]  幂链乒乓 a
    mp_ptr bufb = bufa + BW;       // [bufb,   BW]  幂链乒乓 b
    mp_ptr Nseg = bufb + BW;       // [Nseg,   NW]  被除数段/移位暂存（探针 M 高位接力）

    // ---- 1) 种子：x0 ≈ T·2^(127-I)，shx = I-127；xn/bx 显式（xb 高段无读者，不清零）----
    mp_size_t xn;
    ulong bx;
    {
        // y = log2(Ã) = (B-1) + log2(1+f)，f 取 A 顶 129bit 去前导 1 的
        // 128bit 小数（Ã 顶 bit 同 A）。y/root = I + (rem+f)/root，
        // rem = (B-1) mod root（< 2^56 独占第 3 limb）：
        // 3 limb 定点 [f | rem] 除 root 得 128bit 商，exp2 后取 128bit
        // 归一化尾数。
        ulong flo, fhi;
        if (bl >= 129) {
            ulong s0 = bl - 129;
            flo = lmmp_shr64_(numa, na, s0);
            fhi = lmmp_shr64_(numa, na, s0 + 64);
        } else {
            mp_limb_t w[3] = {0, 0, 0};
            mp_size_t m = na < 3 ? na : 3;  // bl < 129 ⟹ na ≤ 3
            lmmp_copy(w, numa, m);
            unsigned sh = (unsigned)(129 - bl);
            if (sh == 64) {
                // bl=65（恰 2 limb、顶 limb=1）：整 limb 上移，越过 shl_ 的
                // [0,63] 契约域（asm 掩码行为未定义，须显式处理）
                w[2] = w[1];
                w[1] = w[0];
                w[0] = 0;
            } else {
                mp_limb_t cy = lmmp_shl_(w, w, 3, sh);
                (void)cy;  // 高于 129bit 的进位即前导 1 之上，弃
            }
            flo = w[0];
            fhi = w[1];
        }
        mp_limb_t fr[3];
        log2_fixed_128(fr, fhi, flo);          // fr = log2(1+f) 的 128bit 定点
        fr[2] = (mp_limb_t)((B - 1) % root);
        lmmp_div_1_(fr, fr, 3, root);          // (rem·2^128 + f)/root，商 ≤ 2^128
        mp_limb_t e[2];
        exp2_fixed_128(e, fr[1], fr[0]);       // 2^frac 的 128bit 小数
        xb[0] = (e[0] >> 1) | (e[1] << 63);
        xb[1] = (1ULL << 63) | (e[1] >> 1);    // x0 = 2^127 + (e >> 1)
        xn = 2;
        bx = 128;  // x0 ∈ [2^127, 2^128) 恰 128 bit
        shx = (slong)((B - 1) / root) - 127;
    }

    // ---- 2) 位窗牛顿精化：e_eff = L'-shx → L'（全程显式长度，零 trim 扫描/零全段清零）----
    slong lambda = (slong)lmmp_limb_bits_(root - 1) + 12;
    for (;;) {
        slong e_eff = (slong)Lp - shx;
        if (e_eff >= (slong)Lp) break;  // 种子直达域（shx ≤ 0）；层出口恒 shx=0
        slong e2 = 2 * e_eff - lambda;   // e_eff ≥ 128 > λ ⟹ e2 > e_eff
        int final_stage = 0;
        if (e2 >= (slong)Lp) {
            e2 = (slong)Lp;
            final_stage = 1;  // 终层后无条件退出（renorm ±1 不再依赖 e_eff 判定）
        }
        slong shx2 = (slong)Lp - e2;
        slong delta = e2 - e_eff;

        mp_size_t nq = (mp_size_t)((e2 + 63) / 64) + 1;
        mp_size_t Lseg = 2 * nq + 3;
        mp_size_t w = nq + 4;

        // D = root·P，P = x^(root-1) 截断至 w limb（drop 为丢弃 bit 数）
        slong drop;
        mp_size_t pl = nthroot_pow_trunc_(Db, &drop, w, xb, xn, root - 1, bufa, bufb);
        mp_limb_t cy = lmmp_mul_1_(Db, Db, pl, root);
        mp_size_t dl = pl;
        if (cy) Db[dl++] = cy;
        // 除数取 D 的顶 nd limb [Db+dl-nd, nd)（nd ≤ nq+3；顶 limb 恒非零），
        // 低位截断量 64·(dl-nd) 由尺度恒等式吸收。lmmp_div_ 内部归一化并
        // 支持仅算商（dstr=NULL）：div_s 的 MSB 契约、手工提窗移位与
        // 商顶 limb 回收一并免去
        mp_size_t nd = dl < nq + 3 ? dl : nq + 3;

        // Nseg = A 的顶 Lseg limb（左补零语义）
        nthroot_top_limbs_(Nseg, Lseg, numa, na, bl);

        // q_seg = floor(Nseg/Dseg)（仅商；[asmb, Lseg-nd+1]，顶 limb 可为 0，
        // mul_1/移位不要求归一化，容器长直传）
        lmmp_div_(asmb, NULL, Nseg, Lseg, Db + dl - nd, nd);
        mp_size_t ql = Lseg - nd + 1;

        /*
            尺度恒等式（层核心）：q_true := N_{e2}/y^(root-1) ≈ q_seg·root·2^-X，

            X = 64·(dl-nd) + 64·Lseg - bl + root·shx2 - root + (root-1)·delta + drop

            推导：Nseg = A·2^(64Lseg-bl)·(1±2^-64Lseg)，
            Dseg = floor(D/2^(64(dl-nd))) = root·P·2^-(64(dl-nd))，
            P = x^(root-1)·2^-drop，y^(root-1) = x^(root-1)·2^((root-1)·delta)，
            N_{e2} = A·2^(root-root·shx2)；代入相消即得（各 floor 误差
            ≤ 1 ulp 由守卫吸收）。root>=5 时恒 dl >= nq+3（首层 bitlen(D)
            >= 64(nq+3)，之后 (root-1)e_eff 增速快于 e2），min 为防御。
        */
        slong X = 64 * (slong)(dl - nd) + 64 * (slong)Lseg - (slong)bl + (slong)root * shx2
                  - (slong)root + (slong)(root - 1) * delta + drop;

        // t2 = (q_seg·root)·2^-X（值底对齐；X<0 时左移暂驻 Nseg，
        // 与 a1 的相加直接消费驻留形态，不回拷）
        mp_ptr tq;
        mp_size_t tl;
        {
            mp_limb_t cy2 = lmmp_mul_1_(t2, asmb, ql, root);
            tl = ql;
            if (cy2) t2[tl++] = cy2;
            if (X > 0) {
                mp_size_t loff = (mp_size_t)(X >> 6);
                if (loff >= tl) {
                    tl = 0;  // 商整体低于窗（早层）→ t2 = 0
                } else {
                    // shr_ 允许 dst<numa 重叠（含 shr=0 的整段拷贝分支）
                    lmmp_shr_(t2, t2 + loff, tl - loff, (mp_size_t)(X & 63));
                    tl -= loff;
                }
                tq = t2;
            } else if (X < 0) {
                mp_size_t loff = (mp_size_t)((-X) >> 6);
                unsigned bit = (unsigned)((-X) & 63);
                lmmp_zero(Nseg, loff);
                if (bit) {
                    mp_limb_t cy3 = lmmp_shl_(Nseg + loff, t2, tl, bit);
                    tl += loff;
                    if (cy3) Nseg[tl++] = cy3;
                } else {
                    lmmp_copy(Nseg + loff, t2, tl);
                    tl += loff;
                }
                lmmp_debug_assert(tl <= NW);
                tq = Nseg;
            } else {
                tq = t2;
            }
        }

        // a1 = (root-1)·x·2^delta 落位 asmb：[asmb, dloff) 清零 + 乘积左移
        mp_size_t al;
        {
            mp_size_t dloff = (mp_size_t)(delta >> 6);
            unsigned dbit = (unsigned)(delta & 63);
            al = xn;
            mp_limb_t cy4 = lmmp_mul_1_(asmb + dloff, xb, xn, root - 1);
            if (cy4) asmb[dloff + al++] = cy4;
            if (dbit) {
                mp_limb_t cy5 = lmmp_shl_(asmb + dloff, asmb + dloff, al, dbit);
                if (cy5) asmb[dloff + al++] = cy5;
            }
            lmmp_zero(asmb, dloff);
            al += dloff;
        }
        lmmp_debug_assert(al <= XW);

        // x' = floor((a1 + t2)/root)：长者在前的单次 add + 原地单limb除
        {
            mp_ptr np;
            mp_size_t nl;
            mp_limb_t c = 0;
            if (tl == 0) {
                np = asmb;
                nl = al;
            } else if (al >= tl) {
                c = lmmp_add_(asmb, asmb, al, tq, tl);
                np = asmb;
                nl = al;
            } else {
                c = lmmp_add_(tq, tq, tl, asmb, al);
                np = tq;
                nl = tl;
            }
            if (c) np[nl++] = c;
            lmmp_div_1_(np, np, nl, root);  // 原地（契约 eqsep），商高位补零

            /*
                x' 精确长度（免 trim 扫描）：记 t = bx+delta（x·2^delta 的
                位长，v := x·2^delta ≥ 2^(t-1)）。num ≥ (root-1)·v ⟹
                x' ≥ (root-1)/root·v - 1 ≥ (4/5)v - 1 ≥ 2^(t-2)（root≥5、
                t ≥ 4）⟹ bitlen(x') ≥ t-1；层误差定理（e2 = 2·e_eff-λ
                排程）给 x' ≤ v·(1+2^-59) < 2·v < 2^(t+1) ⟹ bitlen ≤ t+1。
                两界跨 ≤2 bit ⟹ xn ∈ {limbs(t+1), limbs(t+1)-1}，
                单次顶 limb 探测即精确。
            */
            xn = (mp_size_t)((bx + (ulong)delta + 64) >> 6);
            if (np[xn - 1] == 0) --xn;
            bx = 64 * (ulong)(xn - 1) + (ulong)lmmp_limb_bits_(np[xn - 1]);
            lmmp_copy(xb, np, xn);
        }
        // 不做 mantissa 归一化：shx ← shx2 保持排程精度语义（x' 的 bitlen
        // 允许略低于 64·nq——牛顿准确度由 e2 = 2·e_eff-λ 的排程保证，
        // 若按 limb 归一化上移会把"scale"虚标为"精度"，下层排程基于虚标
        // 精度直接 e2 → L' 会越过牛顿的二次收敛保证，终值误差 ~2^(L'-2e+λ)）
        shx = shx2;
        if (final_stage) break;
    }

    // ---- 3) 终点：y0 = x·2^-shx ≈ T；S_cand = y0 >> 1，A 域先降后升 ----
    // 层出口恒 shx=0（终层 e2=L' 钳制）→ 纯拷贝；种子直达域 shx<0
    // （|shx| ≤ 126，x0 恰 128bit ⟹ 移后 bitlen = 128+shx 精确已知）
    mp_ptr y = a1;  // [y, XW)
    mp_size_t yn;
    if (shx < 0) {
        mp_size_t loff = (mp_size_t)((-shx) >> 6);
        unsigned bit = (unsigned)((-shx) & 63);
        if (bit) lmmp_shr_(y, xb + loff, xn - loff, bit);
        else     lmmp_copy(y, xb + loff, xn - loff);
        yn = xn - loff;  // bitlen 恰 128+shx ≥ 2，顶 limb 非零
    } else {
        lmmp_debug_assert(shx == 0);
        lmmp_copy(y, xb, xn);
        yn = xn;
    }
    lmmp_shr_(y, y, yn, 1);      // S_cand = y0 >> 1（库移位，原地）
    if (y[yn - 1] == 0) --yn;    // 仅 2^(64k) 精确形态掉 1 limb

    /*
        认证式窗口探针（免终点全尺寸幂；第十一轮，回退根治）：

        以截断幂链 V·2^dr ≈ P（P = y^root；drop 精确记账，相对误差
        ε ≤ 2^(hb+66-64·w)，V 恰 pl limb 且 MSB=1）与 A 的同尺度顶窗
        Wc 做单侧认证比较。尺度对齐是精确的：C := A·2^-dr 为实值，
        bitlen(C) = bl - dr 恰已知，shift = bitlen(C) - 64·pl ∈ {-1,0,1}
        （由 C = V·(1±ε) 与 bitlen(V) = 64·pl 推出），Wc 取
        nthroot_top_limbs_ 的 64·pl bit 顶窗后按 shift 移位，恒有
        Wc ≤ C < Wc + 2。结合 P 的双侧界 P ∈ [(V-δ0)·2^dr,
        (V+δ0)·2^dr]（δ0 = 2^(hb+68) ≥ 4·ε·2^64·pl）与 A 的界
        A ∈ [Wc·2^dr, (Wc+2)·2^dr)，四项判决均为无条件定理：

            dec-cert   V - Wc >= 2^(hb+69)                    ⟹ P > A
            le-cert    Wc - V >= 2^(hb+69)                    ⟹ P <= A
            done-cert  (V-δ0)·root >= (Wc-V+δ0+2)·(ȳ+1)·2^s  ⟹ (y+1)^root > A
            inc-cert   (V+δ0)·root·(1+2^-c1) <= (Wc-V-δ0)·ȳ·2^s ⟹ (y+1)^root <= A

        done/inc 由二项式首项 (y+1)^root >= P + root·y^(root-1)（整数、
        精确）与 (1+1/y)^root <= 1+u(1+u)（u = root/y <= 2^-c1，
        c1 = bitlen(y)-58）承载不等式方向；ȳ = floor(y/2^s)，
        s = bitlen(y)-128（gate bitlen(y) >= 256 保证 2 limb ȳ 与富余）。
        le-cert + done-cert ⟹ y = S（定理），无需任何幂。

        落入不确定带（|V-Wc| < 2^(hb+69)，或两证书均失败；随机输入
        概率 ~2^-(hb+69)，完全幂/区间顶构造必中）或预算耗尽时回退
        全幂端点（先降后升单调循环，从任意起点收敛）。概率只决定
        快路径命中率，正确性由定理 + 精确回退承载。rem 口径在 y
        认证后仍需一次全幂（余数需要精确 y^root），root-only 免。

        y 的 ±1 调整经 nthroot_inc_len_/nthroot_dec_len_ 维护精确长度
        （yn 全程有效，探针/回退的后续读者不再扫描）。
    */
    int settled = 0;
    for (int iter = 0; iter < 4 && !settled; ++iter) {
        if (yn < 4 || 64 * (yn - 1) + lmmp_limb_bits_(y[yn - 1]) < 256) break;
        unsigned hb = lmmp_limb_bits_(root) - 1;  // root <= 2^56 ⟹ hb <= 55
        mp_limb_t d0hi = 1ULL << (hb + 4);        // δ0 = 2^(hb+68) 的 limb 对 {0, d0hi}
        slong dr;
        mp_ptr V = xb;                            // 链结果（pl limb，MSB=1）
        mp_size_t pl = nthroot_pow_trunc_(V, &dr, nr + 5, y, yn, root, bufa, bufb);
        lmmp_debug_assert(pl >= 8);
        slong shift = (slong)bl - dr - 64 * (slong)pl;
        if (shift < -1 || shift > 1) break;       // 定理域外（不可能）：回退
        mp_ptr Wc = asmb;                         // W0 → 按 shift 移位后的比较窗
        nthroot_top_limbs_(Wc, pl, numa, na, bl);
        mp_size_t wl = pl;
        if (shift > 0) {
            // C = Ã·2^shift（Ã ∈ [W0, W0+1)）⟹ shift=+1: C ∈ [2W0, 2W0+2)
            mp_limb_t wc = lmmp_shl_(Wc, Wc, pl, 1);  // W0 MSB=1 ⟹ 恒产生第 pl limb
            lmmp_debug_assert(wc == 1);
            Wc[pl] = wc;
            wl = pl + 1;
        } else if (shift < 0) {
            // shift=-1: C = Ã/2 ∈ [W0/2, W0/2 + 1/2) ⟹ C ∈ [W0>>1, (W0>>1)+1)
            lmmp_shr_(Wc, Wc, pl, 1);  // 顶 limb ≥ 2^62，wl 不变
        }
        // slack = |V - Wc|（SPL limb 容器，高位清零），方向 vgtw
        mp_size_t SPL = pl + 1;
        mp_ptr slk = qb;
        lmmp_zero(slk + wl, SPL - wl);
        int vgtw = nthroot_cmp_ext_(V, pl, Wc, wl) >= 0;
        if (vgtw) lmmp_sub_(slk, V, pl, Wc, wl);
        else      lmmp_sub_(slk, Wc, wl, V, pl);
        mp_size_t sn = SPL;
        while (sn > 0 && slk[sn - 1] == 0) --sn;  // 数据依赖（slack 值任意小）
        // 守卫带宽判定：slack >= 2^(hb+69) ⟺ bitlen(slack) >= hb+70
        if (sn < 2 || lmmp_limb_bits_(slk[sn - 1]) + 64 * (unsigned)(sn - 1) < hb + 70)
            break;  // 不确定带 → 回退全幂端点
        if (vgtw) {
            yn = nthroot_dec_len_(y, yn);  // dec-cert：P > A（y 过高，牛顿 +1 残余）
            continue;
        }
        // le-cert 成立：P <= A。done-cert / inc-cert 判定
        unsigned tb = lmmp_limb_bits_(y[yn - 1]);
        mp_size_t s = 64 * (yn - 1) + (mp_size_t)tb - 128;
        mp_limb_t alo = lmmp_shr64_(y, yn, (ulong)s);
        mp_limb_t ahi = lmmp_shr64_(y, yn, (ulong)s + 64);
        mp_size_t loff = s >> 6;
        unsigned sbit = (unsigned)(s & 63);
        // L = (V-δ0)·root；M = (slack+δ0+2)·(ȳ+1)·2^s（落位 t2+loff 再原地位移）
        mp_ptr L = asmb;  // Wc 已消费（slack 物化于 slk）
        lmmp_copy(L, V, pl);
        nthroot_sub_small_(L, pl, 0, d0hi);
        mp_limb_t lcy = lmmp_mul_1_(L, L, pl, (mp_limb_t)root);
        mp_size_t ln = pl;
        if (lcy) {
            L[ln] = lcy;
            ++ln;
        }
        nthroot_add_small_(slk, SPL, 2, d0hi);
        mp_limb_t bp1[3] = {alo + 1, ahi + (alo + 1 == 0), 0};
        bp1[2] = (bp1[0] == 0 && bp1[1] == 0);  // ȳ+1 进位到 2^128 的形态
        mp_size_t bpn = 2 + (bp1[2] != 0);
        lmmp_zero(t2, loff);  // M = 乘积 << (64·loff)：低位 loff limb 必须显式清零
        lmmp_mul_(t2 + loff, slk, SPL, bp1, bpn);
        mp_size_t mn = loff + SPL + bpn;
        if (sbit) {
            mp_limb_t mcy = lmmp_shl_(t2 + loff, t2 + loff, SPL + bpn, sbit);
            if (mcy) {
                t2[loff + SPL + bpn] = mcy;
                ++mn;
            }
        }
        if (nthroot_cmp_ext_(L, ln, t2, mn) >= 0) {
            settled = 1;  // done-cert：(y+1)^root > A，结合 le-cert 得 y = S
            break;
        }
        // L2 = (V+δ0)·root；M2 = (slack-δ0)·ȳ·2^s；判 L2·(1+2^-c1) <= M2
        lmmp_copy(L, V, pl);
        nthroot_add_small_(L, pl, 0, d0hi);
        lcy = lmmp_mul_1_(L, L, pl, (mp_limb_t)root);
        ln = pl;
        if (lcy) {
            L[ln] = lcy;
            ++ln;
        }
        nthroot_sub_small_(slk, SPL, 2, 2 * d0hi);  // (slack+δ0+2)-(2+2δ0) = slack-δ0 >= 0（le-cert 守卫 slack >= 2δ0）
        mp_limb_t ybar[2] = {alo, ahi};
        lmmp_zero(t2, loff);  // 同上：低位清零
        lmmp_mul_(t2 + loff, slk, SPL, ybar, 2);
        mn = loff + SPL + 2;
        if (sbit) {
            mp_limb_t mcy = lmmp_shl_(t2 + loff, t2 + loff, SPL + 2, sbit);
            if (mcy) {
                t2[loff + SPL + 2] = mcy;
                ++mn;
            }
        }
        // L2 += L2 >> c1 再 +1（整数化 L2·(1+2^-c1) 的上界）
        mp_size_t c1 = s + 70;
        mp_size_t l2 = c1 >> 6;
        unsigned b2 = (unsigned)(c1 & 63);
        mp_size_t shn = ln - l2;
        mp_ptr Sh = qb;  // slack 已消费（M2 已物化于 t2）
        if (b2) lmmp_shr_(Sh, L + l2, shn, b2);
        else    lmmp_copy(Sh, L + l2, shn);
        mp_limb_t acy = lmmp_add_(L, L, ln, Sh, shn);
        if (acy) {
            L[ln] = acy;
            ++ln;
        }
        acy = lmmp_add_1_(L, L, ln, 1);
        if (acy) {
            L[ln] = acy;
            ++ln;
        }
        if (nthroot_cmp_ext_(L, ln, t2, mn) <= 0) {
            yn = nthroot_inc_len_(y, yn);  // inc-cert：(y+1)^root <= A，y+1 成为新候选
            continue;
        }
        break;  // 不确定带 → 回退全幂端点
    }

    mp_size_t tn = 0;  // Y = y^root 的精确长度（回退/认证幂共用）
    if (!settled) {
    // 全幂回退端点（不确定带 / 小根 / bitlen(y) < 256 / 预算耗尽）：
    // 先降后升单调修正（降循环至 y^root <= A，升循环至 (y+1)^root > A），
    // 从任意起点收敛于真值；Y 恒持有 y^root 供余数。注意与上方探针的
    // 区别：早轮被否决的 wcmp 以未认证的截断比较直接作循环条件（无尺度
    // 恒等式、无双侧界、误判即失控）；现行探针的每次判决都是单侧定理 +
    // 精确回退。y 长度经 inc/dec 助手维护，幂比较用 cmp_ext 直接吃
    // 精确长度（无零填充）。
    int dec_cnt = 0, inc_cnt = 0;
    for (;;) {
        tn = lmmp_pow_(Y, Ycap, y, yn, root);
        if (nthroot_cmp_ext_(Y, tn, numa, na) <= 0) break;
        yn = nthroot_dec_len_(y, yn);
        ++dec_cnt;
    }
    for (;;) {
        lmmp_copy(qb, y, yn);
        mp_size_t qn = nthroot_inc_len_(qb, yn);
        mp_size_t tn2 = lmmp_pow_(Y2, Ycap, qb, qn, root);
        if (nthroot_cmp_ext_(Y2, tn2, numa, na) > 0) break;
        lmmp_copy(y, qb, qn);
        yn = qn;
        mp_ptr sw = Y;
        Y = Y2;
        Y2 = sw;  // Y 恒持有 y^root（供余数）
        tn = tn2;
        ++inc_cnt;
    }
    lmmp_debug_assert(dec_cnt <= 8 && inc_cnt <= 8);  // 误差预算绊线（定理 ≤ ~4）
    } else if (dstr) {
        // y = S 已由定理认证；余数需要精确 y^root，付一次全幂（root-only 免）
        tn = lmmp_pow_(Y, Ycap, y, yn, root);
        int yle = nthroot_cmp_ext_(Y, tn, numa, na);  // le-cert 绊线（cmp 先算局部变量）
        lmmp_debug_assert(yle <= 0);
    }

    // 输出根：yn ≤ nr 恒成立（y = S 精确，S < 2^(64nr)），min 为防御
    lmmp_copy(dsts, y, yn < nr ? yn : nr);
    if (yn < nr) lmmp_zero(dsts + yn, nr - yn);
    if (dstr) {
        // rem = A - y^root：值 < A < 2^(64na) ⟹ 差在 [work, na) 内；
        // 契约区 [(root-1)nr+1] 的顶零直接写 dstr（勿经 work 补零——
        // nr=1 大 root 形状 outr ≫ na+13nq 会越出 tp 末端）
        mp_size_t yl = tn > na ? na : tn;
        mp_limb_t bo = lmmp_sub_(work, numa, na, Y, yl);
        lmmp_debug_assert(bo == 0);
        mp_size_t outr = (root - 1) * nr + 1;
        mp_size_t on = outr < na ? outr : na;
        lmmp_copy(dstr, work, on);
        lmmp_zero(dstr + on, outr - on);
    }
}
