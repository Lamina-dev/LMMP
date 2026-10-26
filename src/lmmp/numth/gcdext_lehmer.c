/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

/*
扩展欧几里得（Lehmer 算法，单行共因子跟踪）。

一、骨架（与本库 gcd_lehmer.c 同构，窗口提取与步进矩阵经 impl/gcd.h 共享）

窗口提取（顶端 63 位对齐）、步进矩阵（小规模欧几里得，break-before 停机于
2^32）、幅度数对更新与除法回退均取自 gcd_lehmer.c 的流程；bn==1 时经
lmmp_gcdext_1_ 尾合并——归约路径与库内 gcd 完全一致。

二、单行共因子跟踪（GMP mpn_gcdext 的行向量思想）

值序归一化 U >= V 后全程维护精确恒等式

    V = u0*a + u1*b

其中 (a;b) 为当前数对（幅度语义——本库 Lehmer 步进矩阵为带符号收敛矩阵，
新值组合可能为负，幅度对 gcd 无害但共因子须携带符号），行元素 (u0,u1) 为
符号-幅度数。记累积变换 (U;V) = M*(a;b)，行即 M 的第 1 行，
detsign = det(M) = [1|-1]：

  - 除法步（a = q*b + r，数对变 (b;r)）：(a;b) = E*(b;r)，E = /q 1; 1 0/
    （det = -1）。行 <- 行*E：(u0,u1) <- (q*u0 + u1, u0)，detsign 变号。
    商为单 limb 时经 lc1_ 标量线性组合（同号项熔合，见 lc1_ 注）；
  - 矩阵步（(new_a;new_b) = N*(a;b)，N 按 lehmer 消费序为 /C D; A B/，即
    new_a = C*a + D*b、new_b = A*a + B*b）：行 <- 行*N_applied^{-1}。幅度
    语义下实际生效矩阵为 N_applied = diag(sa,sb)*N（sa/sb 为新值组合的
    符号，即 lc1_ 结果的 neg 位），
    N_applied^{-1} = det_lin*adj(N_applied)*diag(sa,sb) 展开的四元素为带
    符号标量（|A..D| < 2^31，停机界保证），行更新经 lc1_ 两次标量线性组合
    完成（行自身的符号折入标量）；detsign <- detsign*sa*sb*det_lin。
    数对值序翻转即数对左乘列交换 P（det = -1）：行分量对调、detsign 变号。

三、终态贝祖系数（s 为值序较大者的系数，单模变换保持极小性）

  - 数对第二分量归零（(g;0)）：V = u0*g，解间隔 V/g = u0，候选
    {detsign*u1 + k*u0}，取距 0 近者：2*u1 < u0 取 detsign*u1；2*u1 > u0
    取 detsign*(u1-u0)（幅度 u0-u1 < u0/2，符号与 detsign 相反）；
    2*u1 == u0 平局恒取 +u1——该侧配对系数满足 |t| <= U/(2g)，另一侧
    |g + u1*U| > u1*U 必然越界（GMP 同此选择）；
  - bn == 1 尾合并：g = sigma*a + tau*b（lmmp_gcdext_1_ 给极小系数），
    s = detsign*(sigma*u1 - tau*u0)（一次全乘 + 一次标量乘）。

t 不跟踪，输出相位按 t = (g - s*A)/B（A/B 为值序大/小原对）经
lmmp_divexact_（B 偶时剥 2^k）导出，dstt==NULL 时跳过；发射映射（sw 换序
与导出次序）经 impl/gcd.h 的 lmmp_gcdext_emit_ 与 hgcd 驱动共享。

四、界与内存

|u0|,|u1| <= V/g（收敛子界），行槽 un+2 覆盖乘进位。单块 TEMP：数对池
4 槽（矩阵步取两空闲槽轮换）、行槽 4（u0/u1/k0/k1 指针轮换）、lc1_ 临时
2、除法回退商/余/行积、尾合并与发射缓冲。
*/

#include "../../../include/lmmp/impl/gcd.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

/* 符号-幅度系数（n 归一化；n==0 即零，neg 恒 0） */
typedef struct {
    mp_ptr p;
    mp_size_t n;
    int neg;
} sco_t;

static int mag_cmp_(mp_srcptr x, mp_size_t xn, mp_srcptr y, mp_size_t yn) {
    if (xn != yn)
        return xn < yn ? -1 : 1;
    return xn == 0 ? 0 : lmmp_cmp_(x, y, xn);
}

/**
 * @brief dst = al*x + be*y（x,y 为非负幅度；al,be 为带符号标量，|al|,|be| < 2^63）
 * @param dst 输出槽（容量 >= max(xn,yn)+2；与 x,y 缓冲分离）
 * @note 两条快路均为两遍扫描（GMP gcdext 的 mul_1+addmul_1 / mul_1+submul_1 结构）：
 *       - 同号（行更新常态，收敛子符号交替定理保证两项幅度无抵消）：
 *         mul_1 + addmul_1 直加；
 *       - 异号（数对更新常态，收敛矩阵行异号）：长项 mul_1 后 submul_1 减短项，
 *         借位经 sub_1 传播；借位穿底（真值为负，实测约 2% 步）换向重算。
 *       逐次运行时核验借位/进位，正确性不依赖符号性质或值序假设
 */
static void lc1_(sco_t* dst, slong al, mp_srcptr x, mp_size_t xn, slong be, mp_srcptr y,
                 mp_size_t yn) {
    ulong am = al < 0 ? (ulong)-al : (ulong)al;
    ulong bm = be < 0 ? (ulong)-be : (ulong)be;
    if (am == 0 || xn == 0) {
        /* 项1 为零：dst = be*y */
        mp_size_t tn = 0;
        if (bm != 0 && yn != 0) {
            mp_limb_t c = lmmp_mul_1_(dst->p, y, yn, bm);
            tn = yn;
            if (c != 0)
                dst->p[tn++] = c;
        }
        dst->n = tn;
        dst->neg = tn != 0 && be < 0;
        return;
    }
    if (bm == 0 || yn == 0) {
        /* 项2 为零：dst = al*x */
        mp_limb_t c = lmmp_mul_1_(dst->p, x, xn, am);
        mp_size_t tn = xn;
        if (c != 0)
            dst->p[tn++] = c;
        dst->n = tn;
        dst->neg = al < 0;
        return;
    }
    if ((al < 0) == (be < 0)) {
        /* 同号熔合：长项 mul_1 直写（含进位槽），短项 addmul_1 叠加，
           其进位经 add_1 向顶传播（值界保证不越 xn+1，防御写 xn+2） */
        if (xn < yn) {
            LMMP_SWAP(x, y, mp_srcptr);
            LMMP_SWAP(xn, yn, mp_size_t);
            LMMP_SWAP(am, bm, ulong);
            LMMP_SWAP(al, be, slong);
        }
        mp_limb_t h1 = lmmp_mul_1_(dst->p, x, xn, am);
        dst->p[xn] = h1;
        mp_size_t tn = xn + 1;
        mp_limb_t h2 = lmmp_addmul_1_(dst->p, y, yn, bm);
        if (h2 != 0) {
            mp_limb_t c = lmmp_add_1_(dst->p + yn, dst->p + yn, xn + 1 - yn, h2);
            if (c != 0) {
                dst->p[tn] = c;
                ++tn;
            }
        }
        while (tn > 0 && dst->p[tn - 1] == 0)
            --tn;
        dst->n = tn;
        dst->neg = (al < 0) && tn != 0;
        return;
    }
    /*
    异号：先按长项为被减数试减。长项 mul_1 直写（含进位槽），短项 submul_1
    减去，借位经 sub_1 向顶传播；借位穿底即真值为负——换向重算（|be|·y 为
    被减数，此时必有 |be|*y > |al|*x，借位必清）
    */
    if (xn < yn) {
        LMMP_SWAP(x, y, mp_srcptr);
        LMMP_SWAP(xn, yn, mp_size_t);
        LMMP_SWAP(am, bm, ulong);
        LMMP_SWAP(al, be, slong);
    }
    for (int dir = 0;; ++dir) {
        mp_limb_t h1 = lmmp_mul_1_(dst->p, x, xn, am);
        dst->p[xn] = h1;
        if (yn > xn + 1)
            lmmp_zero(dst->p + xn + 1, yn - xn - 1); /* 换向后短被减数的顶部缺口 */
        mp_limb_t bw = lmmp_submul_1_(dst->p, y, yn, bm);
        if (bw != 0)
            bw = lmmp_sub_1_(dst->p + yn, dst->p + yn, xn + 1 - yn, bw);
        if (bw == 0) {
            mp_size_t tn = xn + 1;
            while (tn > 0 && dst->p[tn - 1] == 0)
                --tn;
            dst->n = tn;
            dst->neg = (al < 0) && tn != 0;
            return;
        }
        /* 真值为负：换向（一次即出，第二轮被减数必大） */
        LMMP_SWAP(x, y, mp_srcptr);
        LMMP_SWAP(xn, yn, mp_size_t);
        LMMP_SWAP(am, bm, ulong);
        LMMP_SWAP(al, be, slong);
        lmmp_debug_assert(dir == 0);
    }
}

/* 数对池 4 槽中排除 ia/ib 的两个空闲槽 */
static void pool_free2_(int ia, int ib, int* f1, int* f2) {
    *f1 = -1;
    *f2 = -1;
    for (int i = 0; i < 4; ++i) {
        if (i != ia && i != ib) {
            if (*f1 < 0) {
                *f1 = i;
            } else {
                *f2 = i;
                return;
            }
        }
    }
}

mp_size_t lmmp_gcdext_lehmer_(mp_ptr dstg, mp_ptr dsts, mp_ssize_t* sn, mp_ptr dstt,
                              mp_ssize_t* tn, mp_srcptr up, mp_size_t un, mp_srcptr vp,
                              mp_size_t vn) {
    lmmp_param_assert(un > 0 && vn > 0);
    lmmp_param_assert(up != NULL && vp != NULL && dstg != NULL && dsts != NULL && sn != NULL);
    lmmp_param_assert(up[un - 1] != 0 && vp[vn - 1] != 0);

    /* 值序归一化（sw 记录交换；s 恒为值序大者的系数，输出时按 sw 映射回原参数序） */
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
    // U > V > 0（un >= vn，up/vp 为值序大/小者）

    mp_size_t RSZ = un + 2;
    TEMP_B_DECL;
    mp_ptr buf = BALLOC_TYPE(10 * RSZ + (2 * un + 8) + 3 * (un + 2) + 2, mp_limb_t);
    mp_ptr pool = buf; /* 数对池 4 槽，各 RSZ */
    mp_ptr rowp = pool + 4 * RSZ; /* 行槽 4（u0/u1/k0/k1），各 RSZ */
    mp_ptr t1 = rowp + 4 * RSZ;   /* 终态 2*u1 与尾合并组合临时 */
    mp_ptr t2 = t1 + RSZ;
    mp_ptr t2f = t2 + RSZ; /* 全乘积缓冲（除法步行积/尾合并 τ·u0/发射 nb） */
    mp_ptr qt = t2f + (2 * un + 8);
    mp_ptr rt = qt + (un + 2);
    mp_ptr tg = rt + (un + 2); /* 尾合并 g（1）+ σ（1）+ τ/S 暂存（un+2） */
    mp_ptr ts = tg + 1;
    mp_ptr tt = ts + 1;

    sco_t u0 = {rowp, 0, 0}, u1 = {rowp + RSZ, 0, 0};
    sco_t k0 = {rowp + 2 * RSZ, 0, 0}, k1 = {rowp + 3 * RSZ, 0, 0};
    u1.n = 1;
    u1.p[0] = 1; /* V = 0*a + 1*b */
    int det = 1;

    int ia = 0, ib = 1;
    mp_ptr a = pool;
    mp_ptr b = pool + RSZ;
    mp_size_t an = un, bn = vn;
    lmmp_copy(a, up, un);
    lmmp_copy(b, vp, vn);

    mp_size_t sun = 0; /* S（值序大者系数）幅度长 */
    int suneg = 0;
    mp_ptr sp = tt;    /* S 幅度暂存（<= vn+1 <= un+2） */
    mp_ptr gp = NULL;  /* g 暂存 */
    mp_size_t gn = 0;
    lmmp_gcd_lehmer_t M;
    slong x = 0, y = 0;
    for (;;) {
        if (bn == 0) {
            /*
            (g;0) 终态：V = u0*g（强制 u0 > 0），解间隔 u0 = V/g，候选
            det*u1 + k*u0 取距 0 近者（平局恒取正候选 +|u1|，见文件注三）
            */
            gp = a;
            gn = an;
            lmmp_debug_assert(u0.neg == 0); /* V = u0*g 与 V>0 强制 */
            if (u1.n == 0) {
                sun = 0;
                suneg = 0;
            } else {
                mp_limb_t c2 = lmmp_mul_1_(t1, u1.p, u1.n, 2);
                mp_size_t twon = u1.n + (c2 != 0);
                int cmp2 = mag_cmp_(t1, twon, u0.p, u0.n);
                int sn1 = (u1.neg != 0) ^ (det < 0); /* S0 = det*u1 的符号 */
                if (cmp2 < 0) {
                    lmmp_copy(sp, u1.p, u1.n);
                    sun = u1.n;
                    suneg = sn1;
                } else if (cmp2 > 0) {
                    /* S = S0 - sign(S0)*u0：幅度 u0-|u1| (< u0/2)，符号翻转 */
                    lmmp_sub_(sp, u0.p, u0.n, u1.p, u1.n);
                    sun = u0.n;
                    while (sun > 0 && sp[sun - 1] == 0)
                        --sun;
                    suneg = sun != 0 && !sn1;
                } else {
                    lmmp_copy(sp, u1.p, u1.n); /* 平局恒取正候选 */
                    sun = u1.n;
                    suneg = 0;
                }
            }
            break;
        }
        if (bn == 1) {
            /* 尾合并：g = sigma*a + tau*b（lmmp_gcdext_1_ 极小系数），
               s = det*(sigma*u1 - tau*u0)：一次标量乘 + 一次全乘 + 符号-幅度组合 */
            mp_ssize_t sgn, tgn;
            lmmp_gcdext_1_(tg, ts, &sgn, tt, &tgn, a, an, b[0]);
            lmmp_debug_assert(tg[0] != 0);
            gp = tg;
            gn = 1;
            slong sigma = sgn < 0 ? -(slong)ts[0] : (slong)ts[0];
            mp_size_t tqn = tgn < 0 ? -tgn : tgn;
            mp_size_t p1n = 0, p2n = 0;
            int p1neg = 0, p2neg = 0;
            if (sigma != 0 && u1.n != 0) {
                mp_limb_t c = lmmp_mul_1_(k0.p, u1.p, u1.n,
                                          sigma < 0 ? (ulong)-sigma : (ulong)sigma);
                p1n = u1.n;
                if (c != 0)
                    k0.p[p1n++] = c;
                p1neg = (sigma < 0) ^ (u1.neg != 0);
            }
            if (tqn != 0 && u0.n != 0) {
                if (tqn >= u0.n)
                    lmmp_mul_(t2f, tt, tqn, u0.p, u0.n);
                else
                    lmmp_mul_(t2f, u0.p, u0.n, tt, tqn);
                p2n = tqn + u0.n;
                while (p2n > 0 && t2f[p2n - 1] == 0)
                    --p2n;
                p2neg = (tgn < 0) ^ (u0.neg != 0);
            }
            /* s = sigma*u1 - tau*u0 = p1 + (-p2)（第二项符号翻转） */
            lmmp_gcdext_smag_(t1, &sun, &suneg, k0.p, p1n, p1neg, t2f, p2n, !p2neg);
            lmmp_copy(sp, t1, sun);
            if (det < 0 && sun != 0)
                suneg = !suneg;
            break;
        }

        lmmp_gcd_lehmer_extract_(a, an, b, bn, &x, &y);
        lmmp_gcd_lehmer_step_(x, y, &M);
        if (M.m21 == 0) {
            /*
            除法回退：a = q*b + r，数对 (b;r)。行 <- 行*E：(q*u0+u1, u0)，
            det 变号。余数直写旧 a 槽（eqsep 契约），数对槽位交换即可
            */
            lmmp_div_(qt, a, a, an, b, bn);
            mp_size_t qn = an - bn + 1 - (qt[an - bn] == 0);
            mp_size_t rn = bn;
            while (rn > 0 && a[rn - 1] == 0)
                --rn;
            if (qn == 1 && qt[0] < (1ull << 63)) {
                /* 商为标量：行更新走 lc1_（两遍扫描快路） */
                int d0 = u0.neg ? -1 : 1, d1 = u1.neg ? -1 : 1;
                lc1_(&k0, (slong)qt[0] * d0, u0.p, u0.n, d1, u1.p, u1.n);
            } else {
                mp_size_t pn = 0;
                if (qn != 0 && u0.n != 0) {
                    if (qn >= u0.n)
                        lmmp_mul_(t2f, qt, qn, u0.p, u0.n);
                    else
                        lmmp_mul_(t2f, u0.p, u0.n, qt, qn);
                    pn = qn + u0.n;
                    while (pn > 0 && t2f[pn - 1] == 0)
                        --pn;
                }
                lmmp_gcdext_smag_(k0.p, &k0.n, &k0.neg, t2f, pn, u0.neg != 0, u1.p, u1.n,
                                  u1.neg != 0);
            }
            {
                sco_t t_ = u1;
                u1 = u0;
                u0 = k0;
                k0 = t_;
            }
            mp_ptr ta = a;
            a = b;
            b = ta; /* b 槽现持有 r */
            an = bn;
            bn = rn;
            int ti = ia;
            ia = ib;
            ib = ti;
            det = -det;
            continue;
        }
        /*
        矩阵步：值 (new_a;new_b) = (C*a+D*b; A*a+B*b) 落入两空闲槽；
        行 <- 行*N_applied^{-1}，四组合系数 X（见文件注二）为带符号标量
        */
        int f1, f2;
        pool_free2_(ia, ib, &f1, &f2);
        sco_t sa = {pool + f1 * RSZ, 0, 0}, sb = {pool + f2 * RSZ, 0, 0};
        lc1_(&sa, M.m21, a, an, M.m22, b, bn);
        lc1_(&sb, M.m11, a, an, M.m12, b, bn);
        lmmp_debug_assert(sa.n != 0 || sb.n != 0);
        int da = sa.neg ? -1 : 1, db = sb.neg ? -1 : 1;
        slong dlin = M.m21 * M.m12 - M.m22 * M.m11; /* det_lin = C*B - D*A = ±1 */
        slong sda = (slong)da * dlin, sdb = (slong)db * dlin;
        /* 行元素带符号：符号折入组合标量（sda、d0 均为 ±1，|标量| = |矩阵元素| < 2^31） */
        int d0 = u0.neg ? -1 : 1, d1 = u1.neg ? -1 : 1;
        lc1_(&k0, sda * M.m12 * d0, u0.p, u0.n, -sda * M.m11 * d1, u1.p, u1.n);
        lc1_(&k1, -sdb * M.m22 * d0, u0.p, u0.n, sdb * M.m21 * d1, u1.p, u1.n);
        det = (int)(det * da * db * dlin);
        {
            sco_t t_ = u0;
            u0 = k0;
            k0 = t_;
        }
        {
            sco_t t_ = u1;
            u1 = k1;
            k1 = t_;
        }
        /* 数对新槽与值序（值序翻转 = 行分量对调 + det 变号） */
        if (sa.n == 0) {
            sco_t t_ = u0;
            u0 = u1;
            u1 = t_;
            det = -det;
            a = pool + f2 * RSZ;
            an = sb.n;
            b = pool + f1 * RSZ;
            bn = 0;
            ia = f2;
            ib = f1;
        } else if (sb.n == 0) {
            a = pool + f1 * RSZ;
            an = sa.n;
            b = pool + f2 * RSZ;
            bn = 0;
            ia = f1;
            ib = f2;
        } else if (mag_cmp_(sa.p, sa.n, sb.p, sb.n) < 0) {
            sco_t t_ = u0;
            u0 = u1;
            u1 = t_;
            det = -det;
            a = pool + f2 * RSZ;
            an = sb.n;
            b = pool + f1 * RSZ;
            bn = sa.n;
            ia = f2;
            ib = f1;
        } else {
            a = pool + f1 * RSZ;
            an = sa.n;
            b = pool + f2 * RSZ;
            bn = sb.n;
            ia = f1;
            ib = f2;
        }
    }

    /* 发射：g 与 S 按 sw 映射 + 恒等式导出（与 hgcd 驱动共享，见 impl/gcd.h） */
    /* qt 复用 k1 行槽（容量 RSZ=un+2，循环已结束槽位死亡） */
    lmmp_gcdext_emit_(dstg, dsts, sn, dstt, tn, gp, gn, sp, sun, suneg, sw, up, un, vp, vn, t2f, rt,
                      k1.p);
    TEMP_B_FREE;
    return gn;
}
