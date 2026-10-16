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
 *  This program is distributed in WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/*
扩展欧几里得：hgcd 分治驱动 + 通用分发 + 模逆元。

一、hgcd 驱动（lmmp_gcdext_hgcd_）

值序归一化 U >= V 后全程维护精确恒等式

    V = u0*a + u1*b

其中 (a;b) 为当前归约数对（零填充公共长度表示），行元素 (u0,u1) 非负。
记累积变换 (U;V) = M*(a;b)，行即 M 的第 1 行，detsign = det(M) = [1|-1]。

  - 初始归约：U 长于 V 时做一次除法 a = U mod V（商丢弃——除法矩阵
    E_div = /q 1;1 0/ 的行 1 = (1,0) 不含 q，q 乘零行元素无需计算，共
    因子信息无损，免旧实现的迭代预归约及其行乘开销；行取 (1,0)、
    detsign = -1）；整除则 g = V，系数 (0,1) 直接发射；
  - 主循环（公共长度 n > HGCD_RECURSE_THRESHOLD）：取 p = n/2（首轮，
    行为单位行 (0,1) 或 (1,0)，成功后直接取 M1 的第 1/0 行为行
    ((0,1)*M1 = 行1、(1,0)*M1 = 行0)，免一次行乘）或 p = n/3（后续轮），
    对高位片 (a+p; b+p) 调 lmmp_hgcd_core_（hgcd.c 共享入口，零改动），
    经 lmmp_hgcd_adjust_ 修补低位合入；行 <- 行*M1 经 gcdext_row_mul_
    原位完成；
  - 除法步（core 失败/修补被拒）：先试一次减法 r = a - b——core 失败
    多为近邻对（顶端过近），商为 1，减法 + 行纯加（gcdext_row_qscalar_
    原位 addmul + 指针对调）即完成一步且免除法；r >= b（商 >= 2）时做
    一次除法 a = q*b + r，商为标量走原位 addmul，多 limb 商走全乘。
    余数为 0 即终态 (g;0)；
  - 尾段（n <= 阈值）：转 lmmp_gcdext_lehmer_ 取当前对的极小系数
    (sigma,tau)，组合 S_raw = detsign*(sigma*u1 - tau*u0)（两次全乘），
    极小化快测 2*g*|S| <= V（一次 sn*gn 乘 + 比较，多数直接通过，免算
    D = V/g）；未过时按 D 向 0 平移（S_raw 与真值同格类
    S_true ≡ S_raw (mod V/g)，至多偏离数步；平局恒取正——该侧配对系数
    满足 |t| <= U/(2g)，另一侧 |g + D*U/2| > D*U/2 必越界）；
  - (g;0) 终态（除法步整除）：解间隔 u0 = V/g，候选 detsign*u1 + k*u0
    取距 0 近者（平局恒取正候选，与 gcdext_lehmer.c 同一选点规则）。

s 为 U（值序大者）的极小系数（单模变换保持极小性）；t 不跟踪，输出相位
按 t = (g - s*A)/B（A/B 为值序大/小原对）经 lmmp_divexact_（B 偶时剥
2^k）导出（lmmp_gcdext_derive_/lmmp_gcdext_emit_，与 lehmer 层经
impl/gcd.h 共享），dstt==NULL 时跳过（modinv 路径）。

矩阵元素全程 <= max(U,V)（收敛子界），行缓冲 n0+2 覆盖；行乘产品经
hgcd scratch 池（写长可达 u0n+M1 元素长，和归一化后 <= n0+1）。

二、分发（lmmp_gcdext_）

v==1 转 lmmp_gcdext_1_、vn==2（un>2）转 lmmp_gcdext_2_；两侧均超
GCD_HGCD_THRESHOLD 转 hgcd 驱动（不满足时初始归约后公共长不足，hgcd
无递归空间，Lehmer 更优），否则 lmmp_gcdext_lehmer_。

三、模逆元（lmmp_modinv_ulong_ / lmmp_modinv_）

经 gcdext 的 dstt=NULL 路径（全程不计算 t 侧），s<0 时逆元 = m-|s|。
*/

#include "../../../include/lmmp/impl/gcd.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


/* ---------------- 行向量原位更新 ---------------- */

/*
行除法步原位更新（q >= 1 标量）：(u0,u1) <- (q*u0 + u1, u0)。
u1 += q*u0 经补零 + addmul_1 原位完成，随后 u0/u1 指针对调（新 u0 落在
原 u1 槽）；u0 == 0 时纯对调。u1 槽容量 >= max(u0n,u1n)+1（行槽 n0+2 覆盖）
*/
static void gcdext_row_qscalar_(mp_ptr* u0p, mp_ptr* u1p, mp_size_t* u0n, mp_size_t* u1n,
                                ulong q) {
    if (*u0n == 0) {
        LMMP_SWAP(*u0p, *u1p, mp_ptr);
        LMMP_SWAP(*u0n, *u1n, mp_size_t);
        return;
    }
    if (*u1n < *u0n)
        lmmp_zero(*u1p + *u1n, *u0n - *u1n);
    mp_limb_t cy = lmmp_addmul_1_(*u1p, *u0p, *u0n, q);
    mp_size_t un = *u0n > *u1n ? *u0n : *u1n;
    (*u1p)[un] = cy;
    un += (cy != 0);
    while (un > 1 && (*u1p)[un - 1] == 0)
        --un;
    LMMP_SWAP(*u0p, *u1p, mp_ptr);
    *u1n = *u0n;
    *u0n = un;
}

/*
行矩阵步原位更新：
    (u0,u1) <- (u0,u1)*M1，即 u0' = u0*m00 + u1*m10、u1' = u0*m01 + u1*m11
全部元素非负（驱动行与 M1 均非负，无符号分支）。t0 <- u0 副本后：
u0' 的 m10 项直写 u0 槽、m00 项入 t1 相加；u1' 的 m11 项先入 t1（u1 尚
存活），m01 项覆写 u1 槽后相加——4 乘 2 加 1 拷贝，无中间行拷贝。
产品按写长（含高位零）整段相加，末端一次归一化。t0/t1 容量
max(u0n,u1n)+maxlen(M1)+2；行槽容量 n0+2 覆盖（各项非负，单项 <= 和 <=
收敛子界 n0+1）
*/
static void gcdext_row_mul_(mp_ptr u0p, mp_ptr u1p, mp_size_t* u0n, mp_size_t* u1n,
                            const lmmp_hgcd_matrix_t* M1, lmmp_hgcd_scratch_t* sc) {
    mp_size_t un0 = *u0n, un1 = *u1n;
    mp_size_t mark = sc->used;
    mp_size_t sz = (un0 > un1 ? un0 : un1) + lmmp_gcd_matrix_maxlen_(M1) + 2;
    mp_ptr t0 = lmmp_hgcd_salloc_(sc, sz);
    mp_ptr t1 = lmmp_hgcd_salloc_(sc, sz);
    lmmp_copy(t0, u0p, un0);

    /* u0' = t0*m00 + u1*m10 -> u0p */
    mp_size_t l0 = 0, l1 = 0;
    if (un0 != 0 && M1->n[0][0] != 0) {
        lmmp_gcd_mul_(t1, t0, un0, M1->m[0][0], M1->n[0][0]);
        l0 = un0 + M1->n[0][0];
    }
    if (un1 != 0 && M1->n[1][0] != 0) {
        lmmp_gcd_mul_(u0p, u1p, un1, M1->m[1][0], M1->n[1][0]);
        l1 = un1 + M1->n[1][0];
    }
    mp_size_t n0n;
    if (l0 != 0 && l1 != 0) {
        mp_limb_t c = (l1 >= l0) ? lmmp_add_(u0p, u0p, l1, t1, l0) : lmmp_add_(u0p, t1, l0, u0p, l1);
        n0n = l1 > l0 ? l1 : l0;
        if (c != 0)
            u0p[n0n++] = c;
    } else if (l0 != 0) {
        lmmp_copy(u0p, t1, l0);
        n0n = l0;
    } else {
        n0n = l1; /* 两项皆零即新 u0 为零 */
    }
    while (n0n > 1 && u0p[n0n - 1] == 0)
        --n0n;

    /* u1' = u1*m11 + t0*m01 -> u1p（m11 项先入 t1，u1 覆写后相加） */
    mp_size_t l2 = 0, l3 = 0;
    if (un1 != 0 && M1->n[1][1] != 0) {
        lmmp_gcd_mul_(t1, u1p, un1, M1->m[1][1], M1->n[1][1]);
        l2 = un1 + M1->n[1][1];
    }
    if (un0 != 0 && M1->n[0][1] != 0) {
        lmmp_gcd_mul_(u1p, t0, un0, M1->m[0][1], M1->n[0][1]);
        l3 = un0 + M1->n[0][1];
    }
    mp_size_t n1n;
    if (l2 != 0 && l3 != 0) {
        mp_limb_t c = (l3 >= l2) ? lmmp_add_(u1p, u1p, l3, t1, l2) : lmmp_add_(u1p, t1, l2, u1p, l3);
        n1n = l2 > l3 ? l2 : l3;
        if (c != 0)
            u1p[n1n++] = c;
    } else if (l2 != 0) {
        lmmp_copy(u1p, t1, l2);
        n1n = l2;
    } else {
        n1n = l3;
    }
    while (n1n > 1 && u1p[n1n - 1] == 0)
        --n1n;

    *u0n = n0n;
    *u1n = n1n;
    sc->used = mark;
}

/*
(g;0) 终态选点：解间隔 u0 = V/g，候选 detsign*u1 + k*u0 取距 0 近者
（平局恒取 +u1：该侧配对系数满足界，另一侧 |g+u1*U| > u1*U 必越界）。
幅度写入 sp（容量 >= u0n），符号 *sneg；t1 为 2*u1 暂存（容量 >= u1n+1）
*/
static mp_size_t gcdext_pick_s_(mp_ptr sp, int* sneg, mp_srcptr u0p, mp_size_t u0n,
                                mp_srcptr u1p, mp_size_t u1n, int det, mp_ptr t1) {
    if (u1n == 0) {
        *sneg = 0;
        return 0;
    }
    mp_limb_t c2 = lmmp_mul_1_(t1, u1p, u1n, 2);
    mp_size_t twon = u1n + (c2 != 0);
    int cmp2;
    if (twon != u0n) {
        cmp2 = twon < u0n ? -1 : 1;
    } else {
        cmp2 = lmmp_cmp_(t1, u0p, u0n);
    }
    if (cmp2 <= 0) {
        lmmp_copy(sp, u1p, u1n);
        *sneg = cmp2 == 0 ? 0 : det < 0;
        return u1n;
    }
    /* S = detsign*(u1-u0)：幅度 u0-u1（u1 < u0；u1==u0 时 det 强制皆 1，幅度 0） */
    lmmp_sub_(sp, u0p, u0n, u1p, u1n);
    mp_size_t rn = u0n;
    while (rn > 0 && sp[rn - 1] == 0)
        --rn;
    *sneg = (det > 0 && rn != 0);
    return rn;
}

mp_size_t lmmp_gcdext_derive_(mp_ptr dst, int* tneg, mp_srcptr gp, mp_size_t gn, mp_srcptr sp,
                              mp_size_t sn, int sneg, mp_srcptr xp, mp_size_t xn, mp_srcptr yp,
                              mp_size_t yn, mp_ptr nb, mp_ptr yt) {
    if (sn == 0) {
        dst[0] = 1; /* g == Y */
        *tneg = 0;
        return 1;
    }
    /* P = |S|*X（P >= X >= g，写长 sn+xn） */
    if (sn >= xn)
        lmmp_mul_(nb, sp, sn, xp, xn);
    else
        lmmp_mul_(nb, xp, xn, sp, sn);
    mp_size_t pn = sn + xn;
    while (pn > 0 && nb[pn - 1] == 0)
        --pn;
    lmmp_debug_assert(pn > 0);
    /* N = |g - S*X| */
    if (sneg) {
        mp_limb_t c = lmmp_add_(nb, nb, pn, gp, gn);
        if (c != 0)
            nb[pn++] = c;
        *tneg = 0;
    } else {
        mp_limb_t bw = lmmp_sub_(nb, nb, pn, gp, gn);
        lmmp_debug_assert(bw == 0);
        while (pn > 0 && nb[pn - 1] == 0)
            --pn;
        *tneg = pn != 0;
    }
    if (pn == 0) {
        /* 理论不可达：N==0 ⟺ |S|*X == g，仅 X==g 且 |S|==1，即入口 U==V（已早退） */
        dst[0] = 0;
        *tneg = 0;
        return 0;
    }
    /* 剥 Y 的 2^k（Y 归一化故 j 扫描必终止） */
    mp_size_t j = 0;
    while (yp[j] == 0)
        ++j;
    ulong y0; /* 仅需 cnt，奇化值不使用 */
    int sb;
    ctz_shr_u64(y0, yp[j], sb);
    (void)y0;
    /* N >>= k（整 limb 段 j + 位段 sb，低位必为零） */
    mp_ptr np = nb + j;
    mp_size_t nn = pn - j;
    if (sb != 0) {
        mp_limb_t out = lmmp_shr_(np, np, nn, sb);
        lmmp_debug_assert(out == 0);
        while (nn > 0 && np[nn - 1] == 0)
            --nn;
    }
    /* Y >>= k 奇化到 yt（归一化） */
    mp_size_t yn2 = yn - j;
    if (sb != 0) {
        lmmp_shr_(yt, yp + j, yn2, sb);
        while (yn2 > 0 && yt[yn2 - 1] == 0)
            --yn2;
    } else {
        lmmp_copy(yt, yp + j, yn2);
    }
    lmmp_debug_assert(yn2 > 0 && (yt[0] & 1) == 1);
    lmmp_debug_assert(nn >= yn2);
    /* tv = N/Y（|tv| >= 1 故 nn >= yn2） */
    lmmp_divexact_(dst, np, nn, yt, yn2);
    mp_size_t rn = nn - yn2 + 1;
    while (rn > 0 && dst[rn - 1] == 0)
        --rn;
    lmmp_debug_assert(rn > 0);
    return rn;
}

void lmmp_gcdext_emit_(mp_ptr dstg, mp_ptr dsts, mp_ssize_t* sn, mp_ptr dstt, mp_ssize_t* tn,
                       mp_srcptr gp, mp_size_t gn, mp_srcptr sp, mp_size_t sun, int suneg, int sw,
                       mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn, mp_ptr nb, mp_ptr yt) {
    if (!sw) {
        /* s = S（up 为大者），t = (g - S*up)/vp */
        if (dstt != NULL) {
            int tneg;
            mp_size_t rn = lmmp_gcdext_derive_(dstt, &tneg, gp, gn, sp, sun, suneg, up, un, vp, vn,
                                               nb, yt);
            if (rn == 0)
                dstt[0] = 0;
            *tn = (rn != 0 && tneg) ? -(mp_ssize_t)rn : (mp_ssize_t)rn;
        }
        lmmp_copy(dstg, gp, gn);
        lmmp_copy(dsts, sp, sun);
        if (sun == 0)
            dsts[0] = 0;
        *sn = (sun != 0 && suneg) ? -(mp_ssize_t)sun : (mp_ssize_t)sun;
    } else {
        /* t = S（vp 为大者），s = (g - S*vp)/up（导出 X 用值序大者 up） */
        int tneg;
        mp_size_t rn = lmmp_gcdext_derive_(dsts, &tneg, gp, gn, sp, sun, suneg, up, un, vp, vn, nb,
                                           yt);
        lmmp_copy(dstg, gp, gn);
        if (dstt != NULL) {
            lmmp_copy(dstt, sp, sun);
            if (sun == 0)
                dstt[0] = 0;
            *tn = (sun != 0 && suneg) ? -(mp_ssize_t)sun : (mp_ssize_t)sun;
        }
        if (rn == 0)
            dsts[0] = 0;
        *sn = (rn != 0 && tneg) ? -(mp_ssize_t)rn : (mp_ssize_t)rn;
    }
}

/* ---------------- hgcd 分治驱动 ---------------- */

LMMP_VECT_CONSERVATIVE

mp_size_t lmmp_gcdext_hgcd_(mp_ptr dstg, mp_ptr dsts, mp_ssize_t* sn, mp_ptr dstt,
                            mp_ssize_t* tn, mp_srcptr up, mp_size_t un, mp_srcptr vp,
                            mp_size_t vn) {
    lmmp_param_assert(un > 0 && vn > 0);
    lmmp_param_assert(up != NULL && vp != NULL && dstg != NULL && dsts != NULL && sn != NULL);
    lmmp_param_assert(up[un - 1] != 0 && vp[vn - 1] != 0);

    /* 值序归一化（sw 记录交换；s 恒为原始 up 的系数） */
    int sw = 0;
    if (un < vn) {
        LMMP_SWAP(up, vp, mp_srcptr);
        LMMP_SWAP(un, vn, mp_size_t);
        sw = 1;
    } else if (un == vn) {
        int c = lmmp_cmp_(up, vp, un);
        if (c == 0) {
            lmmp_copy(dstg, up, un);
            dsts[0] = 0;
            *sn = 0;
            if (dstt != NULL) {
                dstt[0] = 1;
                *tn = 1;
            }
            return un;
        }
        if (c < 0) {
            LMMP_SWAP(up, vp, mp_srcptr);
            sw = 1;
        }
    }
    // U > V > 0（un >= vn），up/vp 为值序大/小者

    mp_size_t n0 = un;
    /* 一次分配：数对/修补对/矩阵/行/scratch/S/导出与尾组合暂存 */
    mp_size_t mcap = 2 * n0 / 3 + 4; /* M1 每元素容量（p >= n/3 时片长 <= 2n/3） */
    mp_size_t scsz = lmmp_hgcd_scratch_size_(2 * n0 / 3 + 2) + 4 * n0 + 64;
    mp_size_t wsz = (n0 + 1) * 4      /* wa/wb/sa/sb */
                    + 4 * mcap        /* M1 矩阵（尾段 gb 复用） */
                    + (n0 + 2) * 2    /* 行 u0/u1 */
                    + scsz            /* hgcd scratch 池 */
                    + (n0 + 2)        /* S 暂存 */
                    + (2 * n0 + 4) * 2 /* 导出 nb/yt */
                    + (2 * n0 + 8) * 2; /* 尾组合积 p1/p2 */
    mp_ptr wbuf = ALLOC_TYPE(wsz, mp_limb_t);
    mp_ptr wa = wbuf;
    mp_ptr wb = wbuf + (n0 + 1);
    mp_ptr sa = wbuf + 2 * (n0 + 1);
    mp_ptr sb = wbuf + 3 * (n0 + 1);
    mp_ptr mbuf1 = wbuf + 4 * (n0 + 1);
    mp_ptr u0p = mbuf1 + 4 * mcap;
    mp_ptr u1p = u0p + (n0 + 2);
    mp_ptr scbuf = u1p + (n0 + 2);
    mp_ptr sp = scbuf + scsz; /* S 暂存 */
    mp_ptr nb = sp + (n0 + 2);
    mp_ptr yt = nb + (2 * n0 + 4);
    mp_ptr p1b = yt + (2 * n0 + 4); /* 尾组合积 sigma*u1 */
    mp_ptr p2b = p1b + (2 * n0 + 8); /* 尾组合积 tau*u0 */
    lmmp_hgcd_scratch_t sc = {scbuf, scsz, 0};

    /* 初始归约：U 长于 V 时一次除法（商丢弃——q 乘零行元素无需计算） */
    mp_size_t n;
    int rowz = 0; /* 初始归约后行为 (1,0)（首轮捷径取 M1 第 0 行、det 反号） */
    if (un > vn) {
        lmmp_div_(NULL, wb, up, un, vp, vn); /* a = U mod V 直写 wb（<= vn limbs） */
        mp_size_t rn = lmmp_gcd_norm_(wb, vn);
        if (rn == 0) {
            /* V | U：g = V，系数 (S,T) = (0,1)，按 sw 映射 */
            lmmp_copy(dstg, vp, vn);
            if (!sw) {
                dsts[0] = 0;
                *sn = 0;
                if (dstt != NULL) {
                    dstt[0] = 1;
                    *tn = 1;
                }
            } else {
                dsts[0] = 1;
                *sn = 1;
                if (dstt != NULL) {
                    dstt[0] = 0;
                    *tn = 0;
                }
            }
            lmmp_free(wbuf);
            return vn;
        }
        lmmp_copy(wa, vp, vn);
        lmmp_zero(wb + rn, vn - rn);
        n = vn;
        rowz = 1; /* 归约已发生：行 (1,0)、det=-1（E_div 行 1） */
    } else {
        lmmp_copy(wa, up, un);
        lmmp_copy(wb, vp, vn);
        n = un;
    }

    /*
    行 (u0,u1)：未经归约时初值 (0,1)（V = 0*U + 1*V）；初始归约后为
    (1,0)（V = 1*V + 0*r，E_div = /q 1;1 0/ 的行 1，det=-1）。首轮 hgcd
    成功后直接取 M1 的对应行（(0,1)*M1 = 行1、(1,0)*M1 = 行0），免一次行乘
    */
    mp_size_t u0n = 0, u1n = 0;
    int det = 1;
    if (rowz) {
        u0p[0] = 1;
        u0n = 1;
        det = -1;
    } else {
        u1p[0] = 1;
        u1n = 1;
    }
    int rowinit = 0; /* 行是否已经历首轮 M1 初始化 */
    int first = 1;
    int term_g0 = 0; /* (g;0) 终态（g 在 wa，长度 n） */
    while (n > HGCD_RECURSE_THRESHOLD) {
        mp_size_t p = first ? n / 2 : n / 3;
        first = 0;
        lmmp_hgcd_matrix_t M1;
        lmmp_gcd_matrix_init_buf_(&M1, mbuf1, n - p + 2);
        int dsub = 1;
        sc.used = 0;
        mp_size_t nn = lmmp_hgcd_core_(&M1, wa + p, wb + p, n - p, &sc, &dsub, 0);
        if (nn > 0) {
            sc.used = 0;
            mp_size_t blen = 0;
            mp_size_t rn = lmmp_hgcd_adjust_(&M1, dsub, sa, sb, &blen, wa + p, wb + p, nn, wa, wb,
                                             n, p, &sc);
            sc.used = 0;
            mp_size_t maxlen = lmmp_gcd_matrix_maxlen_(&M1);
            if (rn > 0 && rn < n && blen > maxlen) {
                lmmp_copy(wa, sa, rn);
                lmmp_copy(wb, sb, rn);
                if (lmmp_cmp_(wa, wb, rn) < 0) {
                    LMMP_SWAP(wa, wb, mp_ptr); /* 指针交换后基址偏离 wbuf，仅长度域内使用 */
                    lmmp_gcd_matrix_swap_cols_(&M1);
                    dsub = -dsub;
                }
                if (!rowinit) {
                    /* 行 <- M1 对应行（(0,1)*M1 = 行1；归约后 (1,0)*M1 = 行0），
                       M1 缓冲下一轮被重初始化，须拷出 */
                    int rr = rowz ? 0 : 1;
                    lmmp_copy(u0p, M1.m[rr][0], M1.n[rr][0]);
                    u0n = M1.n[rr][0];
                    lmmp_copy(u1p, M1.m[rr][1], M1.n[rr][1]);
                    u1n = M1.n[rr][1];
                    det = rowz ? -dsub : dsub; /* E_div 的 det=-1 已并入 */
                    rowinit = 1;
                } else {
                            gcdext_row_mul_(u0p, u1p, &u0n, &u1n, &M1, &sc);
                    det *= dsub;
                }
                n = rn;
                continue;
            }
        }
        /*
        core 失败或修补被拒：除法步。core 失败多为近邻对（商 1）——先做
        一次减法 r = a - b，行为纯加（原位 addmul+对调）；r >= b 时商 >= 2，
        做一次除法 a = q*b + r（商标量走原位 addmul，多 limb 走全乘）。
        行 <- (q*u0 + u1, u0)，detsign 变号；余数为 0 即终态 (g;0)
        */
        {
            mp_size_t nb_cur = lmmp_gcd_norm_(wb, n);
            mp_size_t mark = sc.used;
            /* 试减 r1 = a - b（a > b 无借位；r1 < b ⟺ 商为 1） */
            lmmp_sub_(sa, wa, n, wb, nb_cur);
            mp_size_t r1n = lmmp_gcd_norm_(sa, n);
            if (r1n < nb_cur || (r1n == nb_cur && lmmp_cmp_(sa, wb, r1n) < 0)) {
                /* 商为 1（含 r1 == 0 终态）：数对 (b; r1) */
                gcdext_row_qscalar_(&u0p, &u1p, &u0n, &u1n, 1);
                det = -det;
                rowinit = 1;
                sc.used = mark;
                if (r1n == 0) {
                    LMMP_SWAP(wa, wb, mp_ptr); /* g = wb（除数），移入 wa 统一终态 */
                    n = nb_cur;
                    term_g0 = 1;
                        break;
                }
                lmmp_copy(wa, wb, nb_cur);
                lmmp_zero(wa + nb_cur, n - nb_cur);
                lmmp_copy(wb, sa, r1n);
                lmmp_zero(wb + r1n, nb_cur - r1n);
                n = nb_cur;
                continue;
            }
            /* 商 >= 2：全除法 a = q*b + r，余数直写 sa（eqsep 契约） */
            mp_ptr q = lmmp_hgcd_salloc_(&sc, n + 2);
            lmmp_div_(q, sa, wa, n, wb, nb_cur);
            mp_size_t qn = n - nb_cur + 1 - (q[n - nb_cur] == 0);
            mp_size_t rn = lmmp_gcd_norm_(sa, nb_cur);
            if (qn == 1 && q[0] < (1ull << 63)) {
                /* 商为标量：行原位 addmul + 对调 */
                gcdext_row_qscalar_(&u0p, &u1p, &u0n, &u1n, q[0]);
            } else if (u0n == 0) {
                /* u0 为零：new_u0 = u1，new_u1 = 0（指针对调即可） */
                LMMP_SWAP(u0p, u1p, mp_ptr);
                LMMP_SWAP(u0n, u1n, mp_size_t);
            } else {
                /* 多 limb 商：new_u0 = q*u0 + u1（积 <= V/g <= n0+1 limb） */
                mp_ptr t1 = lmmp_hgcd_salloc_(&sc, qn + u0n + 2);
                lmmp_gcd_mul_(t1, q, qn, u0p, u0n);
                mp_size_t pn = qn + u0n;
                while (pn > 0 && t1[pn - 1] == 0)
                    --pn;
                mp_ptr r0 = lmmp_hgcd_salloc_(&sc, n0 + 2);
                mp_size_t r0n = 0;
                if (u1n == 0) {
                    lmmp_copy(r0, t1, pn);
                    r0n = pn;
                } else {
                    mp_limb_t c = (pn >= u1n) ? lmmp_add_(r0, t1, pn, u1p, u1n)
                                              : lmmp_add_(r0, u1p, u1n, t1, pn);
                    r0n = pn > u1n ? pn : u1n;
                    if (c != 0)
                        r0[r0n++] = c;
                }
                lmmp_copy(u1p, u0p, u0n); /* new_u1 = 旧 u0 */
                u1n = u0n;
                lmmp_copy(u0p, r0, r0n);
                u0n = r0n;
            }
            rowinit = 1;
            det = -det;
            sc.used = mark;
            if (rn == 0) {
                /* g = wb（除数），移入 wa 统一终态 */
                LMMP_SWAP(wa, wb, mp_ptr);
                n = nb_cur;
                term_g0 = 1;
                break;
            }
            /* 数对 (b; r)：b 入 wa，r（sa）入 wb，公共长 nb_cur */
            lmmp_copy(wa, wb, nb_cur);
            lmmp_zero(wa + nb_cur, n - nb_cur);
            lmmp_copy(wb, sa, rn);
            lmmp_zero(wb + rn, nb_cur - rn);
            n = nb_cur;
        }
    }

    /* ---------------- 尾段与发射 ---------------- */
    mp_size_t gn = 0;
    mp_size_t sun = 0; /* S（值序大者系数）幅度长 */
    int suneg = 0;
    if (term_g0) {
        gn = n;
        sun = gcdext_pick_s_(sp, &suneg, u0p, u0n, u1p, u1n, det, sa);
        lmmp_gcdext_emit_(dstg, dsts, sn, dstt, tn, wa, gn, sp, sun, suneg, sw, up, un, vp, vn, nb,
                          yt);
    } else {
        /* Lehmer 尾：当前对 (wa,wb) 取极小系数 (sigma,tau)，
           S = det*(sigma*u1 - tau*u0)（两次乘 + 符号-幅度组合） */
        mp_size_t ra = lmmp_gcd_norm_(wa, n);
        mp_size_t rb = lmmp_gcd_norm_(wb, n);
        mp_ptr gb = mbuf1; /* <= n <= 64 limb（循环已降至阈值下），mcap 足够 */
        mp_ptr sgp = sa;
        mp_ptr taup = sb;
        mp_ssize_t snp, tnp;
        gn = lmmp_gcdext_lehmer_(gb, sgp, &snp, taup, &tnp, wa, ra, wb, rb);
        mp_size_t snabs = snp < 0 ? -snp : snp;
        mp_size_t tnabs = tnp < 0 ? -tnp : tnp;
        mp_size_t p1n = 0, p2n = 0;
        if (snabs != 0 && u1n != 0) {
            lmmp_gcd_mul_(p1b, sgp, snabs, u1p, u1n);
            p1n = snabs + u1n;
            while (p1n > 0 && p1b[p1n - 1] == 0)
                --p1n;
        }
        if (tnabs != 0 && u0n != 0) {
            lmmp_gcd_mul_(p2b, taup, tnabs, u0p, u0n);
            p2n = tnabs + u0n;
            while (p2n > 0 && p2b[p2n - 1] == 0)
                --p2n;
        }
        /* S = sigma*u1 + (-tau)*u0（第二项符号 = τ 取负），符号-幅度组合 */
        lmmp_gcdext_smag_(sp, &sun, &suneg, p1b, p1n, snp < 0, p2b, p2n, !(tnp < 0));
        if (det < 0 && sun != 0)
            suneg = !suneg;
        /*
        极小化：S 落在正确的格类（det*(sigma*m11-tau*m10) ≡ S_true (mod V/g)），
        但可能偏离极小代表元至多若干步（|K| <= V/(2g)*(1+o(1))，步数有界）。
        快测 2*g*|S| <= V（⟺ |S| <= D/2，D = V/g）：一次 sn*gn 乘 + 比较，
        多数输入直接通过，免算 D 的整除；未过时按 D 向 0 平移至 |S| <= D/2；
        平局（2|S| == D）恒取正（该侧配对系数满足 |t| <= U/(2g)，另一侧
        |g + D*U/2| > D*U/2 必越界）
        */
        if (sun != 0) {
            lmmp_gcd_mul_(p1b, sp, sun, gb, gn); /* |S|*g */
            mp_size_t pn = sun + gn;
            while (pn > 0 && p1b[pn - 1] == 0)
                --pn;
            mp_limb_t c2 = lmmp_mul_1_(p2b, p1b, pn, 2); /* 2*|S|*g */
            mp_size_t twon = pn + (c2 != 0);
            int cmp2 = (twon != vn) ? (twon < vn ? -1 : 1) : lmmp_cmp_(p2b, vp, vn);
            if (cmp2 > 0) {
                mp_ptr db = p1b; /* D = V/g（恒等式保证整除，商长度 vn-gn+1） */
                lmmp_div_(db, NULL, vp, vn, gb, gn);
                mp_size_t dnn = vn - gn + 1;
                while (dnn > 0 && db[dnn - 1] == 0)
                    --dnn;
                for (;;) {
                    mp_limb_t c2 = lmmp_mul_1_(p2b, sp, sun, 2);
                    mp_size_t twon = sun + (c2 != 0);
                    int cmp2 = (twon != dnn) ? (twon < dnn ? -1 : 1) : lmmp_cmp_(p2b, db, dnn);
                    if (cmp2 < 0)
                        break;
                    if (cmp2 == 0) {
                        suneg = 0; /* 平局恒取正 */
                        break;
                    }
                    int cD = (sun != dnn) ? (sun < dnn ? -1 : 1) : lmmp_cmp_(sp, db, dnn);
                    if (cD >= 0) {
                        lmmp_sub_(sp, sp, sun, db, dnn);
                        while (sun > 0 && sp[sun - 1] == 0)
                            --sun;
                    } else {
                        lmmp_sub_(sp, db, dnn, sp, sun);
                        sun = dnn;
                        while (sun > 0 && sp[sun - 1] == 0)
                            --sun;
                        suneg = !suneg;
                    }
                    if (sun == 0) {
                        suneg = 0;
                        break;
                    }
                }
            } else if (cmp2 == 0) {
                suneg = 0; /* 平局恒取正 */
            }
        }
        lmmp_gcdext_emit_(dstg, dsts, sn, dstt, tn, gb, gn, sp, sun, suneg, sw, up, un, vp, vn, nb,
                          yt);
    }
    lmmp_free(wbuf);
    return gn;
}

mp_size_t lmmp_gcdext_(mp_ptr dstg, mp_ptr dsts, mp_ssize_t* sn, mp_ptr dstt, mp_ssize_t* tn,
                       mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn) {
    lmmp_param_assert(un > 0 && vn > 0);
    lmmp_param_assert(up != NULL && vp != NULL && dstg != NULL && dsts != NULL && sn != NULL);
    lmmp_param_assert(up[un - 1] != 0 && vp[vn - 1] != 0);

    if (vn == 1)
        return lmmp_gcdext_1_(dstg, dsts, sn, dstt, tn, up, un, vp[0]);
    if (vn == 2 && un > 2)
        return lmmp_gcdext_2_(dstg, dsts, sn, dstt, tn, up, un, vp);
    if (vn >= GCD_HGCD_THRESHOLD && un >= GCD_HGCD_THRESHOLD)
        return lmmp_gcdext_hgcd_(dstg, dsts, sn, dstt, tn, up, un, vp, vn);
    return lmmp_gcdext_lehmer_(dstg, dsts, sn, dstt, tn, up, un, vp, vn);
}

ulong lmmp_modinv_ulong_(ulong u, ulong m) {
    lmmp_param_assert(u > 0 && m > 0);

    if (m == 1)
        return 0; // 模 1：逆元存在且为 0
    ulong r = u % m;
    if (r == 0)
        return 0; // m | u 且 m > 1：无逆
    slong s;
    ulong g = lmmp_gcdext_11_(r, m, &s, NULL);
    if (g != 1)
        return 0;
    // s*r = 1 (mod m)，|s| <= m/2 → 归一化到 [0,m)
    ulong sm = s < 0 ? (ulong)-s : (ulong)s;
    return s < 0 ? m - sm : sm;
}

mp_size_t lmmp_modinv_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr mp, mp_size_t mn) {
    lmmp_param_assert(un > 0 && mn > 0);
    lmmp_param_assert(up != NULL && mp != NULL && dst != NULL);
    lmmp_param_assert(up[un - 1] != 0 && mp[mn - 1] != 0);

    if (mn == 1) {
        if (mp[0] == 1) {
            dst[0] = 0;
            return 1; // 模 1：逆元存在且为 0
        }
        ulong inv = lmmp_modinv_ulong_(lmmp_mod_1_(up, un, mp[0]), mp[0]);
        if (inv == 0)
            return 0;
        dst[0] = inv;
        return 1;
    }

    TEMP_B_DECL;
    mp_ptr r = BALLOC_TYPE(mn, mp_limb_t);
    mp_ptr g = BALLOC_TYPE(mn, mp_limb_t);
    mp_ptr s = BALLOC_TYPE(mn + 1, mp_limb_t);
    lmmp_div_(NULL, r, up, un, mp, mn);
    mp_size_t rn = mn;
    while (rn > 0 && r[rn - 1] == 0)
        --rn;
    if (rn == 0) {
        TEMP_B_FREE;
        return 0; // m | u 且 m > 1：无逆
    }
    mp_ssize_t sn;
    mp_size_t gn = lmmp_gcdext_(g, s, &sn, NULL, NULL, r, rn, mp, mn);
    if (gn != 1 || g[0] != 1) {
        TEMP_B_FREE;
        return 0;
    }
    /* s*r = 1 (mod m) → s*u = 1 (mod m)。s<0 时逆元 = m - |s|（|s| <= m/2
       保证结果落在 (m/2, m)，无借位）；防御性上界复查后归一化 */
    mp_size_t snn = sn < 0 ? -sn : sn;
    lmmp_debug_assert(snn > 0 && snn <= mn);
    if (sn < 0) {
        lmmp_sub_(s, mp, mn, s, snn);
        snn = mn;
    }
    if (snn == mn && lmmp_cmp_(s, mp, mn) >= 0) {
        lmmp_sub_(s, s, mn, mp, mn);
    }
    while (snn > 0 && s[snn - 1] == 0)
        --snn;
    lmmp_debug_assert(snn > 0);
    lmmp_copy(dst, s, snn);
    TEMP_B_FREE;
    return snn;
}
