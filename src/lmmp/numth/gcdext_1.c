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

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

/*
扩展欧几里得（标量核与 n limb x 1 limb 层）。

gcdext_11_ 采用带余除法欧几里得 + 双系数幅度递推。设归约序
r0 = v, r1 = u mod v, r_{k+1} = r_{k-1} mod r_k，各余数的 u/v 系数为
(s_k, t_k)：r_k = s_k*u + t_k*v。系数符号严格交替（连分数收敛子性质），
故幅度递推为纯加法：

    |s_{k+1}| = q_k*|s_k| + |s_{k-1}|,   |t_{k+1}| = q_k*|t_k| + |t_{k-1}|

无溢出论证（u64 乘加安全）：
    - g = gcd(u,v) < v（v|u 已提前返回），g|v 故 v >= 2g；
    - 终态极小系数界 |s| <= v/(2g) < 2^63、|t| <= u/(2g) < 2^63；
    - 幅度沿递推单调不减，任意中间 q_k*|s_k| <= |s_{k+1}| <= 终态界，
      首商 q0 = u/v <= u/(2g) 同界——乘积与和均不越 u64。

gcdext_1_ 将 [up,un] 归约至 r = u mod v 后调标量核：s 对 r 的系数即对 u
的系数（s*u = s*r + s*q*v，模 v 意义一致，且极小界不变）；t = (g - s*u)/v
经一次 mul_1 与一次除法精确导出（整除性由恒等式保证）。
*/

ulong lmmp_gcdext_11_(ulong u, ulong v, slong* s, slong* t) {
    lmmp_param_assert(u > 0 && v > 0);
    lmmp_param_assert(s != NULL);

    int sw = 0;
    if (u < v) {
        LMMP_SWAP(u, v, ulong);
        sw = 1;
    }
    // u >= v

    ulong r1 = u % v;
    if (r1 == 0) {
        // v | u：g = v，系数在较小者
        *s = sw ? 1 : 0;
        if (t != NULL)
            *t = sw ? 0 : 1;
        return v;
    }

    ulong r0 = v;
    ulong su0 = 0, su1 = 1;     // u 系数幅度（符号交替，su1 当前为正）
    ulong tv0 = 1, tv1 = u / v; // v 系数幅度（与 su 相位相反）
    int phase = 1;              // su1 的符号 [+1|-1]
    for (;;) {
        ulong q = r0 / r1;
        ulong r2 = r0 % r1;
        if (r2 == 0)
            break; // g = r1，系数即 (su1, tv1) 当前行
        ulong su2 = q * su1 + su0;
        ulong tv2 = q * tv1 + tv0;
        r0 = r1;
        r1 = r2;
        su0 = su1;
        su1 = su2;
        tv0 = tv1;
        tv1 = tv2;
        phase = -phase;
    }
    slong gu = phase > 0 ? (slong)su1 : -(slong)su1;
    slong gv = phase > 0 ? -(slong)tv1 : (slong)tv1;
    *s = sw ? gv : gu;
    if (t != NULL)
        *t = sw ? gu : gv;
    return r1;
}

mp_size_t lmmp_gcdext_1_(mp_ptr dstg, mp_ptr dsts, mp_ssize_t* sn, mp_ptr dstt, mp_ssize_t* tn,
                         mp_srcptr up, mp_size_t un, ulong v) {
    lmmp_param_assert(un > 0 && v > 0);
    lmmp_param_assert(up != NULL && dstg != NULL && dsts != NULL && sn != NULL);

    ulong r = lmmp_mod_1_(up, un, v);
    if (r == 0) {
        dstg[0] = v;
        dsts[0] = 0;
        *sn = 0;
        if (dstt != NULL) {
            dstt[0] = 1;
            *tn = 1;
        }
        return 1;
    }

    slong sg;
    ulong g = lmmp_gcdext_11_(r, v, &sg, NULL);
    lmmp_debug_assert(sg != 0); // r>0 且 v∤r → g<v → σ≠0

    if (dstt == NULL) {
        dstg[0] = g;
        dsts[0] = sg < 0 ? (ulong)-sg : (ulong)sg;
        *sn = sg < 0 ? -1 : 1; /* 带符号长度（sg!=0 已断言） */
        return 1;
    }

    /* t = (g - sg*u)/v：X = |sg|*u；N = g - sg*u（符号-幅度，X >= u >= g
       保证 sg>0 时差非负方向确定）；|t| = N/v。对 up 的读取（X）先于全部
       输出写入（允许 dstg/dsts/dstt 与 up 别名的契约次序） */
    TEMP_B_DECL;
    mp_ptr x = BALLOC_TYPE(un + 2, mp_limb_t);
    mp_ptr nb = BALLOC_TYPE(un + 2, mp_limb_t);
    mp_ptr tq = BALLOC_TYPE(un + 2, mp_limb_t);
    ulong sm = sg < 0 ? (ulong)-sg : (ulong)sg;
    mp_limb_t c = lmmp_mul_1_(x, up, un, sm);
    mp_size_t xn = un;
    if (c != 0)
        x[xn++] = c;
    mp_size_t nn;
    int tneg = 0;
    if (sg > 0) {
        // N = g - X = -(X - g)，X >= g（|sg|>=1 且 g|u）
        lmmp_sub_(nb, x, xn, &g, 1);
        nn = xn;
        while (nn > 0 && nb[nn - 1] == 0)
            --nn;
        tneg = nn != 0;
    } else {
        // N = g + X > 0
        mp_limb_t cy = lmmp_add_(nb, x, xn, &g, 1);
        nn = xn;
        if (cy != 0)
            nb[nn++] = cy;
    }
    // |t| = N/v（恒等式保证整除；N==0 即 t==0 时跳过除法）
    mp_size_t tqn = 0;
    if (nn != 0) {
        mp_limb_t vv[1] = {v};
        lmmp_div_(tq, NULL, nb, nn, vv, 1);
        tqn = nn;
        while (tqn > 0 && tq[tqn - 1] == 0)
            --tqn;
    }

    dstg[0] = g;
    dsts[0] = sm;
    *sn = sg < 0 ? -1 : 1;
    lmmp_copy(dstt, tq, tqn);
    if (tqn == 0)
        dstt[0] = 0;
    *tn = tneg ? -(mp_ssize_t)tqn : (mp_ssize_t)tqn;
    TEMP_B_FREE;
    return 1;
}
