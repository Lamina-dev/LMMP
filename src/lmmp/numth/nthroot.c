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
#include "../../../include/lmmp/impl/log2_exp2.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


/*
    1 limb 根的 log2/exp2 估计（cbrt_3_ 的一般化，除数 3 -> root）：

        bits = bitlen(A)，y = log2(A) = (bits-1) + log2(1+f)，
        f 取 A 去掉前导 1 后的最高 64bit 小数（log2_fixed_64）
        r   = floor(2^(y/root)) = 2^s | (exp2_fixed_64(y/root 的小数) >> (64-s))

    log2/exp2 各带 +-2 ulp 误差，除法舍入再贡献 <1 ulp，经指数放大后
    估计偏差典型 <= 2；随后以单调幂比较（先降后升）修正到精确 floor，
    两个循环均在真值处终止（A >= B^(root-1) 保证真值 >= 1，
    A < B^root 保证真值 <= LIMB_MAX）。
*/
mp_limb_t lmmp_nthroot_1_(mp_srcptr numa, ulong root) {
    lmmp_param_assert(root >= 4 && root <= (1ull << 57));
    lmmp_param_assert(numa != NULL && numa[root - 1] != 0);

    mp_bitcnt_t hb = lmmp_limb_bits_(numa[root - 1]);
    mp_bitcnt_t bits = LIMB_BITS * (root - 1) + hb;
    mp_bitcnt_t sh = hb - 1;
    // a_hi = 前导 1 之后紧邻的 64bit 小数（root>=4 保证有低位 limb 存在）
    mp_limb_t a_hi = sh ? ((numa[root - 1] << (LIMB_BITS - sh)) | (numa[root - 2] >> sh)) : numa[root - 2];

    mp_limb_t x[2];
    x[0] = log2_fixed_64(a_hi);
    x[1] = bits - 1;

    // x 为 scale B 定点数（x[1] 为整数部），除 root 后取舍入值
    mp_limb_t rem = lmmp_div_1_(x, x, 2, root);
    if (rem >= root - rem) lmmp_inc(x);

    mp_bitcnt_t s = x[1];  // <= 64（进位到 64 时钳位交给修正循环）；root<64 时
    // 下限降至 64-64/root-1（如 root=4 时 ~47），仅上限 64 与 root 无关
    lmmp_debug_assert(s <= LIMB_BITS);
    mp_limb_t r;
    if (s == LIMB_BITS)
        r = LIMB_MAX;
    else
        r = (exp2_fixed_64(x[0]) >> (LIMB_BITS - s)) | (1ULL << s);

    TEMP_DECL;
    // 幂统一走 lmmp_pow_（二进制幂，O(log root) 次乘，契约域 root 至 2^57）；
    // pow_ 只写实际长度 tn，cmp/sub 均按满 root 长度取值，高位显式清零。
    // +3：pow 内部会在实际长度处试探性写进位 limb（pow_size_ 相对实际
    // 长度最坏超调 +3 的同一现象），结果恰占满 root limb 时防越界
    mp_ptr t = TALLOC_TYPE(root + 3, mp_limb_t);
    mp_size_t tn = lmmp_pow_(t, (mp_size_t)(root + 3), &r, 1, root);
    lmmp_zero(t + tn, (mp_size_t)root - tn);
    while (lmmp_cmp_(t, numa, (mp_size_t)root) > 0) {
        --r;
        tn = lmmp_pow_(t, (mp_size_t)root, &r, 1, root);
        lmmp_zero(t + tn, (mp_size_t)root - tn);
    }
    while (r < LIMB_MAX) {
        mp_limb_t u = r + 1;
        tn = lmmp_pow_(t, (mp_size_t)root, &u, 1, root);
        lmmp_zero(t + tn, (mp_size_t)root - tn);
        if (lmmp_cmp_(t, numa, (mp_size_t)root) > 0) break;
        ++r;
    }
    TEMP_FREE;
    return r;
}

// (p >> s) 的低 64bit，要求 0 <= s < 64*n（自 cbrt.c 同名辅助移植）
static inline mp_limb_t lmmp_shr64_(mp_srcptr p, mp_size_t n, uint64_t s) {
    mp_size_t w = (mp_size_t)(s >> 6);
    uint64_t b = s & 63;
    mp_limb_t lo = p[w];
    mp_limb_t hi = (w + 1 < n) ? p[w + 1] : 0;
    return b ? ((lo >> b) | (hi << (64 - b))) : lo;
}

/*
    2 limb 根的 log2/exp2 估计（cbrt6_est_ 的一般化，除数 3 -> root）：

        bits = bitlen(A) in (64*root, 128*root]
        y    = log2(A) = (bits-1) + log2(1+f)，f 取 A 的最高 129bit 中去掉
               前导 1 后的 128bit 小数（log2_fixed_128）
        r    = floor(2^(y/root)) = 2^s + (exp2_fixed_128(y/root 的小数) >> (128-s))

    估计误差主要来自 log2/exp2 各 +-2 ulp（128bit）与除 root 舍入，典型
    <= 2；随后以单调幂比较修正到精确 floor（先降后升，t 恒保持 r^root，
    供余数计算复用）。幂统一走 lmmp_pow_（二进制幂，O(log root) 次乘，
    契约域 root 至 2^57；其只写实际长度，高位显式清零供满长比较）。
    下降至 r = B 处终止（A >= B^(root+1) > B^root），
    上升在 r = B^2-1 处封顶（A < B^(2*root)）。
*/
void lmmp_nthroot_2_(mp_ptr dst, mp_ptr numa, mp_size_t na, ulong root, int calr) {
    lmmp_param_assert(root >= 4 && root <= (1ull << 57));
    lmmp_param_assert(na > root && na <= 2 * root);
    lmmp_param_assert(numa != NULL && dst != NULL && numa[na - 1] != 0);
    lmmp_param_assert(calr == 0 || calr == 1);

    if (na < 2 * root) lmmp_zero(numa + na, 2 * root - na);  // 高位补零（覆写），供幂比较

    // bits 取实际最高非零 limb（补零窗口顶 limb 恒为 0，不可用）
    mp_bitcnt_t hb = lmmp_limb_bits_(numa[na - 1]);
    uint64_t bits = LIMB_BITS * (na - 1) + hb;  // in (64*root, 128*root]
    uint64_t s0 = bits - 129;                   // 最高 129bit 的起始位

    mp_limb_t x[3];
    log2_fixed_128(x, lmmp_shr64_(numa, 2 * root, s0 + 64), lmmp_shr64_(numa, 2 * root, s0));
    x[2] = bits - 1;

    // x 为 scale B^2 定点数（x[2] 为整数部），除 root 后取舍入值
    mp_limb_t rem = lmmp_div_1_(x, x, 3, root);
    if (rem >= root - rem) lmmp_inc(x);

    uint64_t s = x[2];  // in [64,128]，进位到 128 时钳位交给修正循环
    lmmp_debug_assert(s >= LIMB_BITS && s <= 2 * LIMB_BITS);
    mp_limb_t r[2], e[2];
    if (s >= 2 * LIMB_BITS) {
        // 舍入进位到 2^128，钳位后交给修正循环
        r[0] = r[1] = LIMB_MAX;
    } else {
        exp2_fixed_128(e, x[1], x[0]);
        // r = 2^s + (e >> (128-s))，总 < 2^(s+1) <= 2^128 不溢出
        uint64_t d = 2 * LIMB_BITS - s;  // in (0,64]
        if (d == LIMB_BITS) {
            r[0] = e[1];
            r[1] = 1;
        } else {
            r[0] = (e[1] << (LIMB_BITS - d)) | (e[0] >> d);
            r[1] = (e[1] >> d) | (1ULL << (s - LIMB_BITS));
        }
    }

    TEMP_DECL;
    // +3 同 nthroot_1_：pow 在实际长度处的试探性进位写不越界
    mp_ptr t = TALLOC_TYPE(2 * root + 3, mp_limb_t);   // r^root（恒有效，余数复用）
    mp_ptr ut = TALLOC_TYPE(2 * root + 3, mp_limb_t);  // (r+1)^root，与 t 分离防覆写
    mp_limb_t u[2];
    mp_size_t tn = lmmp_pow_(t, 2 * (mp_size_t)root + 3, r, 2, root);
    lmmp_zero(t + tn, 2 * (mp_size_t)root - tn);
    while (lmmp_cmp_(t, numa, 2 * (mp_size_t)root) > 0) {
        lmmp_dec(r);
        tn = lmmp_pow_(t, 2 * (mp_size_t)root + 3, r, 2, root);
        lmmp_zero(t + tn, 2 * (mp_size_t)root - tn);
    }
    for (;;) {
        if (r[0] == LIMB_MAX && r[1] == LIMB_MAX) break;  // A < B^(2root)，不会到达
        u[0] = r[0] + 1;
        u[1] = r[1] + (u[0] == 0);
        tn = lmmp_pow_(ut, 2 * (mp_size_t)root + 3, u, 2, root);
        lmmp_zero(ut + tn, 2 * (mp_size_t)root - tn);
        if (lmmp_cmp_(ut, numa, 2 * (mp_size_t)root) > 0) break;
        mp_ptr sw = t;
        t = ut;
        ut = sw;  // 指针交换代替 2*root limb 回拷，t 恒持有 r^root
        r[0] = u[0];
        r[1] = u[1];
    }
    dst[0] = r[0];
    dst[1] = r[1];
    if (calr) {
        // rem = A - r^root < root*(B^2-1)^(root-1) < B^(2*root-1)，高位 limb 为 0
        mp_limb_t bo = lmmp_sub_n_(numa, numa, t, 2 * root);
        lmmp_debug_assert(bo == 0);
    }
    TEMP_FREE;
}

/*
    divide 路径的顶 limb 归一化下限（cbrt 的 CBRT_DIVIDE_MIN 推广）：

    记输入 A = [numa, root*ns] 顶 limb 为 tau，beta = tau/B。逐层不变式
    “子问题顶 limb = A 顶 limb”使每层 Ahi >= beta*B^(root*hi)，故

        Ahr = floor(Ahi^(1/root)) >= beta^(1/root)*B^hi - 1

    除数 D1 = Ahr^(root-1) 恰 (root-1)*hi 个 limb 且 MSB=1（div_s 归一化）
    要求 Ahr >= 2^(-1/(root-1))*B^hi，即

        beta >= 2^(-root/(root-1)) * (1 + ~2^-57)   （floor 的 -1 与
        hi>=1 的相对损失 ~2^-64 均被覆盖）

    root=3 时该式即 cbrt 的 3B/8 > 2^(-3/2)*B 推导。运行时由
    lmmp_nthroot_divide_min_ 按上式取富余值。同一归一化还保证 Alr 至多
    高估 1（见下方推导），故 min_ 是正确性的一部分，非仅性能条件。
*/
mp_limb_t lmmp_nthroot_divide_min_(ulong root) {
    lmmp_param_assert(root >= 4 && root <= (1ull << 57));
    // B*2^(-root/(root-1)) = 2^(64 - root/(root-1))，指数非整数，不可用
    // ldexp（指数会被截断为整），pow 相对误差 ~2^-52；
    // 1e-9 相对富余 >> pow 误差与需求余量(2^-57)
    double v = pow(2.0, 64.0 - (double)root / (double)(root - 1));
    return (mp_limb_t)(v * (1.0 + 1e-9)) + 1;
}

/*
    tp 需求（ns>=3，nsp = calr ? ns : ns+1；公式按 calr=0 的 nsp=ns+1 计）：

        W    x^(root-1)     (root-1)*nsp + 8   （pow_size_ 相对实际最坏超调 +3）
        Xk   x^root         root*nsp + 8       （mul_ 写满 na+nb 个 limb）
        Mb   牛顿步被除数场  (root+1)*nsp + 8   （sub 容器 root*c，场顶 z+dn+1+b）
        q    商 δ 缓冲       nsp + 4
        xseg 根缓冲（x 居顶）nsp + 2
        win2 顶窗副本       2*root + 2

    合计 (3*root+2)*nsp + 2*root + 32。ns<=2 的路径另计。
*/
mp_size_t lmmp_nthroot_divide_size_(mp_size_t ns, ulong root) {
    lmmp_param_assert(ns > 0);
    lmmp_param_assert(root >= 4 && root <= (1ull << 57));
    if (ns == 1) return (mp_size_t)root + 3;  // r^root 幂缓冲（pow_ + 高位清零，+3 进位写富余）
    if (ns == 2) return 1;                // nthroot_2_ 内部 TEMP，不使用 tp
    return (mp_size_t)(3 * root + 2) * (ns + 1) + 2 * (mp_size_t)root + 32;
}

void lmmp_nthroot_divide_(
    mp_ptr restrict  dst,
    mp_ptr restrict numa,
    mp_size_t         ns,
    ulong           root,
    mp_ptr restrict   tp,
    int             calr
) {
    lmmp_param_assert(ns > 0);
    lmmp_param_assert(root >= 4 && root <= (1ull << 57));
    lmmp_param_assert(numa != NULL && dst != NULL && tp != NULL);
    lmmp_param_assert(calr == 0 || calr == 1);
    // min_ 含 pow 调用，须提出断言外（LMMP_ASSUME 不可含副作用表达式）
    mp_limb_t minl = lmmp_nthroot_divide_min_(root);
    lmmp_param_assert(numa[root * ns - 1] >= minl);

    if (ns == 1) {
        dst[0] = lmmp_nthroot_1_(numa, root);
        if (calr) {
            // [numa,root] = A - r^root < root*r^(root-1)+... < B^root
            // tp 按 size_(ns==1) = root+3 供给（pow 试探性进位写富余）
            mp_size_t tn = lmmp_pow_(tp, (mp_size_t)root + 3, dst, 1, root);
            lmmp_zero(tp + tn, (mp_size_t)root - tn);  // sub 按满 root 长度取值
            mp_limb_t bo = lmmp_sub_n_(numa, numa, tp, (mp_size_t)root);
            lmmp_debug_assert(bo == 0);
        }
        return;
    }
    if (ns == 2) {
        // est 路径（nthroot_2_）不依赖顶 limb 归一化，且天然维护余数
        lmmp_nthroot_2_(dst, numa, 2 * (mp_size_t)root, root, calr);
        return;
    }

    mp_size_t nsp = ns + (calr ? 0 : 1);  // calr=0 对 A*B^root 求根（根多 1 limb）
    mp_size_t wcap = (mp_size_t)(root - 1) * nsp + 8;
#define W     (tp)                        // x^(root-1)，恰 dn limb 且 MSB=1
#define Xk    (tp + wcap)                 // x^root = W*x，满 root*c limb
#define Mb    (Xk + (mp_size_t)root * nsp + 8)   // 牛顿步被除数场
#define q     (Mb + (mp_size_t)(root + 1) * nsp + 8)  // 商 δ（nq limb）
#define xseg  (q + nsp + 4)               // 根缓冲，x 居顶 [xseg+nsp-c, nsp)
#define win2  (xseg + nsp + 2)            // nthroot_2_ 的顶窗副本

    // 精度栈自顶向下折叠：nsp, nsp/2+1, ..., 5, 3, 2（invcbrt_newton_ 的
    // 尺寸栈同款）。逐级升回时 b = np-c 满足 b <= c-1（m -> (m>>1)+1 的
    // 间距恰为 ceil(m/2)-1 <= (m>>1)+1-1），W/D<2 的分离条件逐级成立；
    // 末级入口精度 ~nsp/2 使最后一条 W 链落在半尺寸（GMP 位级日程同型，
    // 自底向上 2^k+1 型梯子的末级入口达 0.78*nsp，实测 root=100 慢 1.5x）
    mp_size_t sizes[80];  // 深度 log2(nsp)+2（契约内存域内 nsp << 2^63）
    int nk = 0;
    for (mp_size_t m = nsp; m > 2; m = (m >> 1) + 1) sizes[nk++] = m;
    lmmp_debug_assert(nk < 80);

    // 种子：x_2 = A 顶 2root limb 的精确根（其单调修正循环完整收敛，精确
    // 性由 stage 1 起的证书链承接；恒以 calr=0 调用——种子无需余数）
    lmmp_copy(win2, numa + root * (ns - 2), 2 * root);
    lmmp_nthroot_2_(xseg + nsp - 2, win2, 2 * root, root, 0);

    mp_size_t c = 2;
    for (int si = nk; si-- > 0;) {
        mp_size_t np = sizes[si];     // b <= c-1：W/D<2 分离条件（见上）
        mp_size_t b = np - c, nq = b + 1;
        mp_size_t dn = (mp_size_t)(root - 1) * c;
        mp_size_t z = nsp + 1 - c;         // M 场底偏移：[Mb+z, Mb+z+dn+nq)
        mp_size_t ft = z + dn + nq;        // 场顶（恰 dn+nq limb，同截 s 的锚点）
        mp_ptr xp = xseg + nsp - c;        // 当前 x（c limb）
        mp_srcptr aw = numa + root * (ns - c);  // A_c 顶窗（c <= nsp-1 <= ns）

        for (;;) {
            // 证书：x^root <= A_c；W 兼作下方除法除数（免独立验证幂的关键复用）
            mp_size_t rn = lmmp_pow_(W, wcap, xp, c, root - 1);
            lmmp_debug_assert(rn == dn);  // 归一化保证恰占满且 MSB=1
            lmmp_mul_(Xk, W, dn, xp, c);
            lmmp_debug_assert(Xk[root * c - 1] != 0);
            if (lmmp_cmp_(Xk, aw, root * (mp_size_t)c) <= 0) break;
            lmmp_dec(xp);  // 高估（仅牛顿 +1），整段重算，至多 1 次
        }

        // M = r*B^b + a 的场装配（r = A_c - Xk，证书后借位恒 0）：a 的
        // b limb 居场底，r 底对齐于其上——sub 的 root*c limb 容器高出
        // dn+1 的 c-2 limb 恒为零（值界 r < B^(dn+1)），溢出场顶无害。
        // 场长恰为 dn+nq 使 M 右对齐于场顶，与 W 顶窗构成同截
        // s = dn-nq-3（商尺度不变的关键前提）
        lmmp_copy(Mb + z, numa + root * np - b - root, b);
        mp_limb_t bo = lmmp_sub_(Mb + z + b, aw, root * c, Xk, root * c);
        lmmp_debug_assert(bo == 0);
        lmmp_div_1_(Mb + z, Mb + z, dn + nq, root);  // M' = M/root，原地
        // 截断除法（k=3 护卫，Qh ∈ {Q, Q+1}）
        mp_limb_t qh = lmmp_div_s_(q, Mb + ft - (2 * nq + 3), 2 * nq + 3, W + dn - (nq + 3), nq + 3);
        lmmp_debug_assert(qh == 0);  // δ <= B^b + 1 < B^(b+1)（同截无低估 + k=3 排除 +2）
        if (Mb[ft - nq - 1] == 0) {
            // R_t 顶 limb 证书失效（随机 ~2^-64，定向构造可触发）：全量乘
            // Qh*W 与 M' 比较以强制 θ=0。div_s 已就地破坏 M' 顶段，须先
            // 重建（输入 aw/Xk 均未损）；过大则减一（Qh ∈ {Q, Q+1}）
            lmmp_copy(Mb + z, numa + root * np - b - root, b);
            mp_limb_t bo2 = lmmp_sub_(Mb + z + b, aw, root * c, Xk, root * c);
            lmmp_debug_assert(bo2 == 0);
            lmmp_div_1_(Mb + z, Mb + z, dn + nq, root);
            lmmp_mul_(Xk, W, dn, q, nq);  // Xk 至此已死，长操作数在前
            if (lmmp_cmp_(Xk, Mb + z, dn + nq) > 0) lmmp_dec(q);
        }
        // x' = [δ | x] 装配（δ = B^b 的 adj 进位形态截为 B^b-1，恰为 t）
        if (q[nq - 1] != 0) {
            lmmp_fill_n(xseg + nsp - np, b, LIMB_MAX);
        } else {
            lmmp_copy(xseg + nsp - np, q, b);
        }
        c = np;
    }

    if (calr) {
        // 终层证书 + 余数：直接二进制幂到 x^root（终层无除法不需要 W，
        // 免去 W 链之后再乘 x 的额外一次全尺寸乘法）
        for (;;) {
            mp_size_t rn = lmmp_pow_(Xk, root * nsp + 8, xseg, ns, root);
            lmmp_debug_assert(rn == root * ns);
            if (lmmp_cmp_(Xk, numa, root * ns) <= 0) break;
            lmmp_dec(xseg);
        }
        lmmp_sub_(numa, numa, root * ns, Xk, root * ns);  // 高 ns-1 limb 恒 0
    } else if (xseg[0] <= 1) {
        // 低 limb 守卫失效（e ∈ {0,1} 或 +1 进位）：全量精确证书定夺减一
        // 与否（对照虚拟 A_pad = [0(root) | numa] 两段比较，同样直接幂）
        for (;;) {
            mp_size_t rn = lmmp_pow_(Xk, root * nsp + 8, xseg, nsp, root);
            lmmp_debug_assert(rn == root * nsp);
            int cm = lmmp_cmp_(Xk + root, numa, root * ns);
            if (cm < 0) break;
            if (cm == 0) {
                int low_zero = 1;
                for (mp_size_t i = 0; i < (mp_size_t)root; ++i) low_zero &= (Xk[i] == 0);
                if (low_zero) break;
            }
            lmmp_dec(xseg);
        }
    }
    lmmp_copy(dst, xseg + (nsp - ns), ns);
#undef W
#undef Xk
#undef Mb
#undef q
#undef xseg
#undef win2
}

/*
    窗口截断幂链的截断+归一化：v（*plen limb）截至顶 wlimbs limb 并把
    顶 bit 左移到 limb 顶（MSB=1），丢弃位数（bit 计）累入 *dr。
    语义不变量：v_stored·2^dr ≈ 真值。
*/
static void nthroot_chop_(mp_ptr v, mp_size_t *plen, int64_t *dr, mp_size_t wlimbs) {
    mp_size_t l = *plen;
    if (l > wlimbs) {
        *dr += 64 * (int64_t)(l - wlimbs);
        mp_size_t d = l - wlimbs;
        for (mp_size_t i = 0; i < wlimbs; ++i) v[i] = v[d + i];  // 前向安全
        l = wlimbs;
    }
    while (l > 0 && v[l - 1] == 0) --l;
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
    返回 dst 长度。要求 exp ≥ 2、base 归一化。
*/
static mp_size_t nthroot_pow_trunc_(
    mp_ptr dst, int64_t *drop, mp_size_t wlimbs,
    mp_srcptr base, mp_size_t bn, ulong exp, mp_ptr bufa, mp_ptr bufb
) {
    mp_ptr P = bufa;
    mp_size_t pl = bn < wlimbs ? bn : wlimbs;
    lmmp_copy(P, base, pl);
    int64_t dr = 0;
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

        值 = floor(A / 2^s)，s = bl - 64·nlimbs（s < 0 时为 A·2^(-s))

    即把 A 视作左端补零到恰 nlimbs limb 的整体；值域恰 [2^(64·nlimbs-1),
    2^(64·nlimbs)) 的截断情形顶 bit 为 A 的顶 bit。要求 dst 容量
    nlimbs + 1（s>0 且 bit 非零时值可暂占 nlimbs+1 limb，顶 limb 恒 0）。
*/
static void nthroot_top_limbs_(
    mp_ptr dst, mp_size_t nlimbs, mp_srcptr numa, mp_size_t na, uint64_t bl
) {
    int64_t s = (int64_t)bl - 64 * (int64_t)nlimbs;
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

// 零扩展比较 [a,an] vs [b,bn]（高位段非零者大，再比较公共低位段）
static int nthroot_cmp_ext_(mp_srcptr a, mp_size_t an, mp_srcptr b, mp_size_t bn) {
    for (mp_size_t i = an; i-- > bn;) if (a[i]) return 1;
    for (mp_size_t i = bn; i-- > an;) if (b[i]) return -1;
    for (mp_size_t i = an < bn ? an : bn; i-- > 0;)
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return 0;
}

// 根候选幂：dsts 经 dec 调整后可能带高位零 limb（lmmp_pow_ 契约要求归一化），
// trim 后统一走 lmmp_pow_（单 limb 底数大 exp 分派至 pow_1_ 家族）
static mp_size_t pow_small_(mp_ptr dst, mp_size_t cap, mp_srcptr base, mp_size_t n, ulong exp) {
    while (base[n - 1] == 0) --n;
    return lmmp_pow_(dst, cap, base, n, exp);
}

// p[n] += c0 + c1·B（n >= 2；调用方值域保证无 p[n] 溢出，debug 断言绊线）
static void nthroot_add_small_(mp_ptr p, mp_size_t n, mp_limb_t c0, mp_limb_t c1) {
    mp_limb_t cc = c0 ? lmmp_add_1_(p, p, n, c0) : 0;
    lmmp_debug_assert(cc == 0);
    mp_limb_t cc2 = lmmp_add_1_(p + 1, p + 1, n - 1, c1 + cc);
    lmmp_debug_assert(cc2 == 0);
}

// p[n] -= c0 + c1·B（n >= 2；调用方值域保证无下借，debug 断言绊线）
static void nthroot_sub_small_(mp_ptr p, mp_size_t n, mp_limb_t c0, mp_limb_t c1) {
    mp_limb_t bb = c0 ? lmmp_sub_1_(p, p, n, c0) : 0;
    lmmp_debug_assert(bb == 0);
    mp_limb_t bb2 = lmmp_sub_1_(p + 1, p + 1, n - 1, c1);
    lmmp_debug_assert(bb2 == 0);
}

/*
    tp 需求（nr = ceil(na/root)，nq = nr+1 ≥ ceil(L'/64)）：

        Y, Y2   终点幂/试探幂   2·(root·nr + 8)
        work    余数中转        na + 4
        xb/asmb/qb/a1/t2 精化   5·(nq + 8)
        Db      除数            2·nq + 18
        bufa/bufb 幂链乒乓      2·(2·nq + 16)
        Nseg    被除数段/移位   2·nq + 13

    合计 2·root·nr + na + 15·nq + 95。
*/
mp_size_t lmmp_nthroot_size_(mp_size_t na, ulong root) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(root >= 4 && root <= (1ull << 56));
    mp_size_t nr = (na + (mp_size_t)root - 1) / (mp_size_t)root;
    mp_size_t nq = nr + 1;  // ceil(L'/64) ≤ ceil((64na+root)/(64root)) + 0 ≤ nr+1
    return 2 * ((mp_size_t)root * nr + 8) + na + 4 + 15 * nq + 95;
}

void lmmp_nthroot_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa, mp_size_t na, ulong root, mp_ptr tp) {
    lmmp_param_assert(na > 0 && numa != NULL && numa[na - 1] != 0);
    lmmp_param_assert(root >= 4 && root <= (1ull << 56));
    lmmp_param_assert(dsts != NULL && tp != NULL);
    lmmp_param_assert(dstr == NULL || (dstr != dsts && dstr != tp));
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

    /*
        root=4 专用路线：sqrt∘sqrt 嵌套 floor 恒等式（与顶入口的
        Ã = A·2^root 恢复同族定理）

            floor(sqrt(floor(sqrt(A)))) = floor(A^(1/4))

        双向夹逼：S = floor(A^(1/4)) ⟹ S² ≤ floor(sqrt(A)) ⟹ S⁴ ≤ A；
        反向 (S+1)² > floor(sqrt(A)) ⟹ (S+1)² ≥ floor(sqrt(A))+1 ⟹
        (S+1)⁴ > A。余数由两级 sqrtrem 展开：
            A = q1² + r1 = (S²+r2)² + r1 ⟹ A - S⁴ = r2·(q1+S²) + r1
        （q1 = floor(sqrt A)，(S,r2) = sqrtrem(q1)）。对 2 的幂指数
        的推广（root=2^j 的 j 级 sqrt 链）与 root=8 的 sqrt∘nthroot(4)
        组合同理成立，未实现（现行 root≥8 已与 GMP 持平或反超）。
        实测 rem/root-only 全尺寸较通用路径快 15-40%（sqrt 自身的
        优化空间在 sqrt.c，不在本文件）。

        tp 分段（值域 na ≤ 4·nr 保证 ≤ size_ 域 2·root·nr+na+15·nq+95）：
        q1/r1/r2 各 na/2+2，sum na/2+3，S nr+2，prd 3·nr+4。
    */
    if (root == 4) {
        mp_ptr q1 = tp, r1 = q1 + na / 2 + 2, S = r1 + na / 2 + 2, r2v = S + nr + 2,
               sum = r2v + na / 2 + 2, prd = sum + na / 2 + 3;
        lmmp_sqrt_(q1, dstr ? r1 : NULL, numa, na, 0);
        mp_size_t nq1 = na / 2 + 1;
        while (nq1 > 0 && q1[nq1 - 1] == 0) --nq1;
        lmmp_sqrt_(S, dstr ? r2v : NULL, q1, nq1, 0);
        mp_size_t sn = nq1 / 2 + 1;
        while (sn > 0 && S[sn - 1] == 0) --sn;
        if (dstr) {
            // rem = r2·(q1 + S²) + r1
            lmmp_sqr_(sum, S, sn);
            mp_size_t s2n = 2 * sn;
            if (nq1 + 1 > s2n) lmmp_zero(sum + s2n, nq1 + 1 - s2n);
            mp_size_t un = nq1 > s2n ? nq1 : s2n;
            mp_limb_t cy = lmmp_add_(sum, sum, un, q1, nq1);
            if (cy) {
                sum[un] = cy;
                ++un;
            }
            mp_size_t r2n = nq1 / 2 + 1;
            while (r2n > 0 && r2v[r2n - 1] == 0) --r2n;
            lmmp_mul_(prd, sum, un, r2v, r2n ? r2n : 1);
            mp_size_t pn = un + (r2n ? r2n : 1);
            while (pn > 0 && prd[pn - 1] == 0) --pn;
            mp_size_t r1n = na / 2 + 1;
            while (r1n > 0 && r1[r1n - 1] == 0) --r1n;
            {  // prd += r1：先加公共低位段（进位经 prd 高段传播），再补拷超出段
                mp_size_t common = r1n < pn ? r1n : pn;
                if (common) {
                    mp_limb_t cy2 = lmmp_add_(prd, prd, pn, r1, common);
                    if (cy2) {
                        prd[pn] = cy2;
                        ++pn;
                    }
                }
                if (r1n > pn) {
                    lmmp_copy(prd + pn, r1 + pn, r1n - pn);
                    pn = r1n;
                }
            }
            mp_size_t outr = (root - 1) * nr + 1;
            while (pn > 0 && prd[pn - 1] == 0) --pn;
            if (pn > outr) pn = outr;  // 值界 rem < 2^(192·nr+3)，段顶恒零
            lmmp_copy(dstr, prd, pn);
            lmmp_zero(dstr + pn, outr - pn);
        }
        lmmp_zero(dsts, nr);
        lmmp_copy(dsts, S, sn < nr ? sn : nr);  // sn ≤ nr（S < 2^(16·na)）
        return;
    }

    uint64_t bl = LIMB_BITS * (uint64_t)(na - 1) + (uint64_t)lmmp_limb_bits_(numa[na - 1]);
    uint64_t B = bl + root;              // bitlen(Ã)
    uint64_t Lp = (B + root - 1) / root; // L' = bitlen(T)
    int64_t shx;                         // x ≈ T·2^-shx（scale 记账）

    // ---- scratch 布局（单块手动分段，生命周期接力）----
    mp_size_t nqf = nr + 1;
    mp_size_t Ycap = (mp_size_t)root * nr + 8;
    mp_ptr Y = tp;                            // Ycap
    mp_ptr Y2 = Y + Ycap;                     // Ycap（+1 试探幂）
    mp_ptr work = Y2 + Ycap;                  // na + 4（余数中转）
    mp_ptr xb = work + na + 4;                // nqf + 8（精化状态 x）
    mp_ptr asmb = xb + nqf + 8;               // nqf + 8（商/装配累加器）
    mp_ptr qb = asmb + nqf + 8;               // nqf + 8（Dseg / y+1 候选）
    mp_ptr a1 = qb + nqf + 8;                 // nqf + 8（(root-1)·x）
    mp_ptr t2 = a1 + nqf + 8;                 // nqf + 8（q 段移位）
    mp_ptr Db = t2 + nqf + 8;                 // 2·nqf + 18（截断幂链结果/除数）
    mp_ptr bufa = Db + 2 * nqf + 18;          // 2·(nqf + 4) + 8
    mp_ptr bufb = bufa + 2 * (nqf + 4) + 8;   // 同上
    mp_ptr Nseg = bufb + 2 * (nqf + 4) + 8;   // 2·nqf + 13（被除数段/移位暂存）

    // ---- 1) 种子：x0 ≈ T·2^(127-I)，shx = I-127 ----
    {
        // y = log2(Ã) = (B-1) + log2(1+f)，f 取 A 顶 129bit 去前导 1 的
        // 128bit 小数（Ã 顶 bit 同 A）。y/root = I + (rem+f)/root，
        // rem = (B-1) mod root（< 2^56 独占第 3 limb）：
        // 3 limb 定点 [f | rem] 除 root 得 128bit 商，exp2 后取 128bit
        // 归一化尾数。
        uint64_t flo, fhi;
        if (bl >= 129) {
            uint64_t s0 = bl - 129;
            flo = lmmp_shr64_(numa, na, s0);
            fhi = lmmp_shr64_(numa, na, s0 + 64);
        } else {
            mp_limb_t w[3] = {0, 0, 0};
            mp_size_t m = na < 3 ? na : 3;  // bl < 129 ⟹ na ≤ 3
            lmmp_copy(w, numa, m);
            unsigned sh = (unsigned)(129 - bl);
            mp_limb_t cy = lmmp_shl_(w, w, 3, sh);
            (void)cy;  // 高于 129bit 的进位即前导 1 之上，弃
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
        lmmp_zero(xb + 2, nqf + 6);
        shx = (int64_t)((B - 1) / root) - 127;
    }

    // ---- 2) 位窗牛顿精化：e_eff = L'-shx → L' ----
    int64_t lambda = (int64_t)lmmp_limb_bits_(root - 1) + 12;
    for (;;) {
        int64_t e_eff = (int64_t)Lp - shx;
        if (e_eff >= (int64_t)Lp) break;
        int64_t e2 = 2 * e_eff - lambda;   // e_eff ≥ 128 > λ ⟹ e2 > e_eff
        int final_stage = 0;
        if (e2 >= (int64_t)Lp) {
            e2 = (int64_t)Lp;
            final_stage = 1;  // 终层后无条件退出（renorm ±1 不再依赖 e_eff 判定）
        }
        int64_t shx2 = (int64_t)Lp - e2;
        int64_t delta = e2 - e_eff;

        mp_size_t xn = nqf + 8;
        while (xn > 0 && xb[xn - 1] == 0) --xn;
        mp_size_t nq = (mp_size_t)((e2 + 63) / 64) + 1;
        mp_size_t Lseg = 2 * nq + 3;
        mp_size_t w = nq + 4;

        // D = root·P，P = x^(root-1) 截断至 w limb（drop 为丢弃 bit 数）
        int64_t drop;
        mp_size_t pl = nthroot_pow_trunc_(Db, &drop, w, xb, xn, root - 1, bufa, bufb);
        mp_limb_t cy = lmmp_mul_1_(Db, Db, pl, root);
        mp_size_t dl = pl;
        if (cy) {
            Db[pl] = cy;
            ++dl;
        }
        // Dseg = floor(D / 2^dOff)（bitlen 恰 64·(nq+3)，MSB=1 满足 div_s 契约）：
        // 取顶 nq+3 limb 后左移 (64-dtop)
        unsigned dtop = (unsigned)lmmp_limb_bits_(Db[dl - 1]);
        int64_t dbits = 64 * (int64_t)(dl - 1) + dtop;
        int64_t dOff = dbits - 64 * (int64_t)(nq + 3);
        lmmp_copy(qb, Db + (dl - (nq + 3)), nq + 3);
        if (dtop < 64) lmmp_shl_(qb, qb, nq + 3, 64 - dtop);

        // Nseg = A 的顶 Lseg limb（左补零语义）
        nthroot_top_limbs_(Nseg, Lseg, numa, na, bl);

        // q_seg = floor(Nseg/Dseg)（div_s 就地破坏 Nseg——scratch 无碍；
        // 商可达 nq+1 limb（顶 limb 即返回值 qh）——必须回收，否则偶发
        // 丢顶 bit 使 t2 腰斩、牛顿步精度崩塌）
        mp_limb_t qh = lmmp_div_s_(asmb, Nseg, Lseg, qb, nq + 3);
        mp_size_t ql = nq;
        if (qh) {
            asmb[nq] = qh;
            ql = nq + 1;
        }
        while (ql > 0 && asmb[ql - 1] == 0) --ql;

        /*
            尺度恒等式（层核心）：q_true := N_{e2}/y^(root-1) ≈ q_seg·root·2^-X，

            X = dOff + 64·Lseg - bl + root·shx2 - root + (root-1)·delta + drop

            推导：Nseg = A·2^(64Lseg-bl)·(1±2^-64Lseg)，Dseg = root·P·2^-dOff，
            P = x^(root-1)·2^-drop，y^(root-1) = x^(root-1)·2^((root-1)·delta)，
            N_{e2} = A·2^(root-root·shx2)；代入相消即得（各 floor 误差
            ≤ 1 ulp 由守卫吸收）。
        */
        int64_t X = dOff + 64 * (int64_t)Lseg - (int64_t)bl + (int64_t)root * shx2
                  - (int64_t)root + (int64_t)(root - 1) * delta + drop;

        // t2 = (q_seg·root)·2^-X
        mp_size_t tl = ql;
        if (tl) {
            mp_limb_t cy2 = lmmp_mul_1_(t2, asmb, tl, root);
            if (cy2) {
                t2[tl] = cy2;
                ++tl;
            }
        }
        if (X > 0) {
            mp_size_t loff = (mp_size_t)(X >> 6);
            unsigned bit = (unsigned)(X & 63);
            if (loff >= tl) {
                tl = 0;
            } else {
                if (bit) lmmp_shr_(t2, t2 + loff, tl - loff, bit);
                else {
                    for (mp_size_t i = 0; i < tl - loff; ++i) t2[i] = t2[loff + i];
                }
                tl -= loff;
            }
        } else if (X < 0) {
            mp_size_t loff = (mp_size_t)((-X) >> 6);
            unsigned bit = (unsigned)((-X) & 63);
            lmmp_zero(Nseg, 2 * nqf + 13);
            if (bit) {
                mp_limb_t cy3 = lmmp_shl_(Nseg + loff, t2, tl, bit);
                if (loff + tl < 2 * nqf + 13) Nseg[loff + tl] = cy3;
                tl = loff + tl + (cy3 != 0);
            } else {
                lmmp_copy(Nseg + loff, t2, tl);
                tl += loff;
            }
            lmmp_copy(t2, Nseg, tl);
        }

        // num = (root-1)·x·2^delta + t2（Nseg 已死，复用为移位暂存）
        mp_size_t al = xn;
        mp_limb_t cy4 = lmmp_mul_1_(a1, xb, xn, root - 1);
        if (cy4) {
            a1[xn] = cy4;
            al = xn + 1;
        }
        {
            mp_size_t loff = (mp_size_t)(delta >> 6);
            unsigned bit = (unsigned)(delta & 63);
            lmmp_zero(Nseg, 2 * nqf + 13);
            if (bit) {
                mp_limb_t cy5 = lmmp_shl_(Nseg + loff, a1, al, bit);
                if (loff + al < 2 * nqf + 13) Nseg[loff + al] = cy5;
            } else {
                lmmp_copy(Nseg + loff, a1, al);
            }
            mp_size_t nl = loff + al + 1;
            if (nl <= tl) nl = tl + 1;
            lmmp_zero(asmb, nqf + 8);
            lmmp_copy(asmb, Nseg, nl);
            lmmp_add_(asmb, asmb, nl, t2, tl);
            while (nl > 0 && asmb[nl - 1] == 0) --nl;
            lmmp_div_1_(xb, asmb, nl, root);  // x' = floor(num/root)
            lmmp_zero(xb + nl, nqf + 8 - nl); // 清高位残留（plen 扫描的正确性前提）
        }
        // 不做 mantissa 归一化：shx ← shx2 保持排程精度语义（x' 的 bitlen
        // 允许略低于 64·nq——牛顿准确度由 e2 = 2·e_eff-λ 的排程保证，
        // 若按 limb 归一化上移会把"scale"虚标为"精度"，下层排程基于虚标
        // 精度直接 e2 → L' 会越过牛顿的二次收敛保证，终值误差 ~2^(L'-2e+λ)）
        shx = shx2;
        if (final_stage) break;
    }

    // ---- 3) 终点：y0 = x·2^-shx ≈ T；S_cand = y0 >> 1，A 域先降后升 ----
    mp_ptr y = a1;  // nqf+8 limb
    lmmp_zero(y, nqf + 8);
    {
        mp_size_t xn = nqf + 8;
        while (xn > 0 && xb[xn - 1] == 0) --xn;
        if (shx >= 0) {
            mp_size_t loff = (mp_size_t)(shx >> 6);
            unsigned bit = (unsigned)(shx & 63);
            if (bit) {
                mp_limb_t cy = lmmp_shl_(y + loff, xb, xn, bit);
                if (loff + xn < nqf + 8) y[loff + xn] = cy;
            } else {
                lmmp_copy(y + loff, xb, xn);
            }
        } else {
            mp_size_t loff = (mp_size_t)((-shx) >> 6);
            unsigned bit = (unsigned)((-shx) & 63);
            if (bit) lmmp_shr_(y, xb + loff, xn - loff, bit);
            else     lmmp_copy(y, xb + loff, xn - loff);
        }
    }
    mp_size_t yn = nqf + 8;  // S_cand = y0 >> 1（原地）
    while (yn > 0 && y[yn - 1] == 0) --yn;
    for (mp_size_t i = 0; i < yn; ++i)
        y[i] = (y[i] >> 1) | (i + 1 < yn ? y[i + 1] << 63 : 0);
    while (yn > 0 && y[yn - 1] == 0) --yn;

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
    */
    int settled = 0;
    for (int iter = 0; iter < 4 && !settled; ++iter) {
        if (yn < 4 || 64 * (yn - 1) + lmmp_limb_bits_(y[yn - 1]) < 256) break;
        unsigned hb = lmmp_limb_bits_(root) - 1;  // root <= 2^56 ⟹ hb <= 55
        mp_limb_t d0hi = 1ULL << (hb + 4);        // δ0 = 2^(hb+68) 的 limb 对 {0, d0hi}
        int64_t dr;
        mp_ptr V = xb;                            // 链结果（pl limb，MSB=1）
        mp_size_t pl = nthroot_pow_trunc_(V, &dr, nr + 5, y, yn, root, bufa, bufb);
        lmmp_debug_assert(pl >= 8);
        int64_t shift = (int64_t)bl - dr - 64 * (int64_t)pl;
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
            lmmp_shr_(Wc, Wc, pl, 1);
        }
        // slack = |V - Wc|（SPL limb 容器，高位清零），方向 vgtw
        mp_size_t SPL = pl + 1;
        mp_ptr slk = qb;
        lmmp_zero(slk + wl, SPL - wl);
        int vgtw = nthroot_cmp_ext_(V, pl, Wc, wl) >= 0;
        if (vgtw) lmmp_sub_(slk, V, pl, Wc, wl);
        else      lmmp_sub_(slk, Wc, wl, V, pl);
        mp_size_t sn = SPL;
        while (sn > 0 && slk[sn - 1] == 0) --sn;
        // 守卫带宽判定：slack >= 2^(hb+69) ⟺ bitlen(slack) >= hb+70
        if (sn < 2 || lmmp_limb_bits_(slk[sn - 1]) + 64 * (unsigned)(sn - 1) < hb + 70)
            break;  // 不确定带 → 回退全幂端点
        if (vgtw) {
            lmmp_dec(y);  // dec-cert：P > A（y 过高，牛顿 +1 残余）
            continue;
        }
        // le-cert 成立：P <= A。done-cert / inc-cert 判定
        unsigned tb = lmmp_limb_bits_(y[yn - 1]);
        mp_size_t s = 64 * (yn - 1) + (mp_size_t)tb - 128;
        mp_limb_t alo = lmmp_shr64_(y, yn, (uint64_t)s);
        mp_limb_t ahi = lmmp_shr64_(y, yn, (uint64_t)s + 64);
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
        nthroot_sub_small_(slk, SPL, 0, d0hi);  // slack-δ0 >= 0（le-cert 守卫）
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
            lmmp_inc(y);          // y 缓冲上方恒零：全 1 形态安全停在 y[yn]
            if (y[yn] != 0) ++yn;
            continue;  // inc-cert：(y+1)^root <= A，y+1 成为新候选
        }
        break;  // 不确定带 → 回退全幂端点
    }

    if (!settled) {
    // 全幂回退端点（不确定带 / 小根 / bitlen(y) < 256 / 预算耗尽）：
    // 先降后升单调修正（nthroot_1_ 同款协议），从任意起点收敛于真值；
    // Y 恒持有 y^root 供余数。注意与上方探针的区别：早轮被否决的
    // wcmp 以未认证的截断比较直接作循环条件（无尺度恒等式、无双侧
    // 界、误判即失控）；现行探针的每次判决都是单侧定理 + 精确回退。
    int dec_cnt = 0, inc_cnt = 0;
    for (;;) {
        mp_size_t tn = pow_small_(Y, Ycap, y, nqf + 8, root);
        lmmp_zero(Y + tn, Ycap - tn);
        if (nthroot_cmp_ext_(Y, Ycap, numa, na) <= 0) break;
        lmmp_dec(y);
        ++dec_cnt;
    }
    for (;;) {
        lmmp_copy(qb, y, nqf + 8);
        lmmp_inc(qb);
        mp_size_t tn = pow_small_(Y2, Ycap, qb, nqf + 8, root);
        lmmp_zero(Y2 + tn, Ycap - tn);
        if (nthroot_cmp_ext_(Y2, Ycap, numa, na) > 0) break;
        lmmp_copy(y, qb, nqf + 8);
        mp_ptr sw = Y;
        Y = Y2;
        Y2 = sw;  // Y 恒持有 y^root（供余数）
        ++inc_cnt;
    }
    lmmp_debug_assert(dec_cnt <= 8 && inc_cnt <= 8);  // 误差预算绊线（定理 ≤ ~4）
    } else if (dstr) {
        // y = S 已由定理认证；余数需要精确 y^root，付一次全幂（root-only 免）
        mp_size_t tn = pow_small_(Y, Ycap, y, nqf + 8, root);
        lmmp_zero(Y + tn, Ycap - tn);
        int yle = nthroot_cmp_ext_(Y, Ycap, numa, na);  // le-cert 绊线（minl 先算局部变量）
        lmmp_debug_assert(yle <= 0);
    }

    lmmp_zero(dsts, nr);
    {
        mp_size_t yo = nqf + 8;
        while (yo > 0 && y[yo - 1] == 0) --yo;
        lmmp_copy(dsts, y, yo < nr ? yo : nr);
    }
    if (dstr) {
        lmmp_copy(work, numa, na);
        lmmp_zero(work + na, 4);
        mp_size_t yl = Ycap;
        while (yl > 0 && Y[yl - 1] == 0) --yl;  // Y = y^root 实际长度
        if (yl > na) yl = na;                    // Y ≤ A，高位段恒零
        mp_limb_t bo = lmmp_sub_(work, numa, na, Y, yl);
        lmmp_debug_assert(bo == 0);
        if ((root - 1) * nr + 1 > na) lmmp_zero(work + na, (root - 1) * nr + 1 - na);
        lmmp_copy(dstr, work, (root - 1) * nr + 1);
    }
}
