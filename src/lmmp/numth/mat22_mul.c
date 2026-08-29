/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/*
2x2 非负矩阵乘法与平方（逐元素显式长度，无零填充不变量）。

长度维护纪律（见 mat22_mul.h）：本文件内所有实际长度均由计算过程
显式推导——
- 乘法：lmmp_mul_ 写 na+nb limbs 且归一化操作数之积 >= B^(na+nb-2)，
  故积长 ∈ {na+nb-1, na+nb}，顶 limb 单次判断即可；
- 加法：lmmp_add_/lmmp_add_n_ 的进位（同号时落盘于高 limb）；
- 减法：先比较大小定符号；符号-绝对值中间量的差可抵消至任意短，
  长度信息不存在，须归一化扫描（矩阵中间量唯一扫描处，同号加法与
  乘积均无此需求）；
- 平方：lmmp_sqr_ 写 2n limbs 且积 >= B^(2n-2)，同乘法。
全程不做逐 limb 的前导零扫描。

实现路径（按元素最大长度 mx 分派）：
- mx < MAT22_MUL_STRASSEN_THRESHOLD：乘法 basecase（8 乘）；
  平方对称 basecase（5 乘）；
- 否则：Winograd-Strassen。乘法 7 乘；平方利用 A*A 组合量对称
  （t_i = s_i），7 乘中 4 次为平方（lmmp_sqr_ 快于 lmmp_mul_）。
组合量与乘积安置在 9 个等宽槽位（宽 W = 2*mx+4）：每个组合量恰被
一个乘积消耗，乘积依次回填释放的槽位，全程无额外缓冲。

历史注记：FFT 缓存乘法路径（8 乘 + lmmp_mul_fft_cache_ 复用短侧
前向变换）曾在此实现并经实测证伪——省 4 次前向变换不敌第 8 次乘法
的点积与逆变换开销，且前向变换份额随规模衰减（34%@2000 ->
17%@16000），渐进劣于 Strassen，已删除。详见 mat22_mul.h 头注。

另一族曾探索的方案（本轮）：Strassen 的 7 个乘积共享前向变换
（各因子一次前向 + 逐积一次点乘与逆变换，梅森域）。经核算证伪：
7 个乘积的 14 个因子两两互异（Strassen 正是以不共享操作数换取
乘法次数），无变换可复用，与逐积 lmmp_mul_（CRT 拆分：两半尺寸
模乘的变换总量与之等价）无净差异，已回退。hgcd 侧 apply_mod 的
fa/fb 复用（每因子参与两次乘法）是变换共享的真实机会，既有
lmmp_mul_mersenne_cache_ 系列已实现；频域线性组合（两积之差的
系数域模减，省一次逆变换）因系数域 ±1 单位簿记与装配校正不兼容
（负真值固定偏差）未采用。
*/

#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/mat22_mul.h"

/* ==================== 带符号助手（仅本文件） ====================
   值以 (指针, 带符号长度) 表示：负长度表示负值，|长度| 为真实长度
   （归一化，顶 limb 非零；0 表示零值）。输入矩阵元素恒为非负且长度
   归一化（接口契约），符号仅出现在 Strassen 中间量；中间量绝对值
   不超过 B^(mx+1)。 */

/* 带符号加：r = (x,xs) + (y,ys)，返回带符号长度；r 可与任一输入别名。
   长度推导：同号加法进位落盘于 r[m]；异号相减先比较定序（归一化
   表示下长者必大，等长才需比较），差经抵消可至任意短、长度信息不
   存在，须归一化扫描（矩阵中间量唯一扫描处）*/
static mp_ssize_t mat22_addsg_(mp_ptr r, mp_srcptr x, mp_ssize_t xs, mp_srcptr y, mp_ssize_t ys) {
    int xneg = xs < 0;
    int yneg = ys < 0;
    mp_size_t xa = xs < 0 ? (mp_size_t)(-xs) : (mp_size_t)xs;
    mp_size_t ya = ys < 0 ? (mp_size_t)(-ys) : (mp_size_t)ys;
    if (xa == 0) {
        if (r != y) {
            lmmp_copy(r, y, ya);
        }
        return ys;
    }
    if (ya == 0) {
        if (r != x) {
            lmmp_copy(r, x, xa);
        }
        return xs;
    }
    if (xneg == yneg) {
        /* 同号：较长春在前相加，进位落盘 */
        mp_srcptr lp = xa >= ya ? x : y;
        mp_srcptr sp = xa >= ya ? y : x;
        mp_size_t m = xa >= ya ? xa : ya;
        mp_limb_t c = lmmp_add_(r, lp, m, sp, xa >= ya ? ya : xa);
        if (c != 0) {
            r[m] = c;
            ++m;
        }
        return xneg ? -(mp_ssize_t)m : (mp_ssize_t)m;
    }
    /* 异号：绝对值相减（归一化：长者必大，等长才需比较） */
    int xbig = xa > ya || (xa == ya && lmmp_cmp_(x, y, xa) >= 0);
    mp_srcptr big = xbig ? x : y;
    mp_srcptr sml = xbig ? y : x;
    mp_size_t ba = xbig ? xa : ya;
    mp_size_t sa = xbig ? ya : xa;
    lmmp_sub_(r, big, ba, sml, sa);
    /*
    符号-绝对值中间量的差可以抵消至任意短（如 Winograd 的 U2 = p1-U1
    逼近抵消），长度信息在此不存在，必须逐 limb 归一化——这是矩阵
    中间量唯一允许扫描之处（同号加法经进位、乘积经顶 limb 单判断均
    无需扫描）
    */
    while (ba > 0 && r[ba - 1] == 0) {
        --ba;
    }
    int rneg = xbig ? xneg : yneg; /* 结果与绝对值较大者同号 */
    return rneg ? -(mp_ssize_t)ba : (mp_ssize_t)ba;
}

/* 带符号乘：r = (x,xs)*(y,ys)，返回带符号长度；sep(r,[x|y])。
   长度推导：归一化操作数之积 >= B^(xa+ya-2)，积长 xa+ya 或 xa+ya-1，
   顶 limb 单次判断 */
static mp_ssize_t mat22_mulsg_(mp_ptr r, mp_srcptr x, mp_ssize_t xs, mp_srcptr y, mp_ssize_t ys) {
    int neg = (xs < 0) != (ys < 0);
    mp_size_t xa = xs < 0 ? (mp_size_t)(-xs) : (mp_size_t)xs;
    mp_size_t ya = ys < 0 ? (mp_size_t)(-ys) : (mp_size_t)ys;
    if (xa == 0 || ya == 0) {
        return 0;
    }
    if (xa >= ya) {
        lmmp_mul_(r, x, xa, y, ya);
    } else {
        lmmp_mul_(r, y, ya, x, xa);
    }
    mp_size_t l = xa + ya;
    l -= (r[l - 1] == 0);
    return neg ? -(mp_ssize_t)l : (mp_ssize_t)l;
}

/* 带符号平方：r = (x,xs)^2，恒非负，返回长度；sep(r,x)。
   积 >= B^(2xa-2)，长 2xa 或 2xa-1，顶 limb 单次判断 */
static mp_ssize_t mat22_sqrsg_(mp_ptr r, mp_srcptr x, mp_ssize_t xs) {
    mp_size_t xa = xs < 0 ? (mp_size_t)(-xs) : (mp_size_t)xs;
    if (xa == 0) {
        return 0;
    }
    lmmp_sqr_(r, x, xa);
    mp_size_t l = 2 * xa;
    l -= (r[l - 1] == 0);
    return (mp_ssize_t)l;
}

/* 元素长度最大值 */
static mp_size_t mat22_maxlen_(const lmmp_mat22_t* a) {
    mp_size_t mx = 0;
    for (int i = 0; i < 4; ++i) {
        if (a->n[i / 2][i % 2] > mx) {
            mx = a->n[i / 2][i % 2];
        }
    }
    return mx;
}

/* ==================== 结果写回 ====================
   四个结果元素（缓冲 c[i]，带符号长度 cl[i]）取非负值写回 dst 并
   记录各自长度；dst 元素缓冲容量由调用方保证 */
static void mat22_store_(lmmp_mat22_t* dst, mp_ptr* c, const mp_ssize_t* cl) {
    for (int i = 0; i < 4; ++i) {
        mp_size_t l = cl[i] < 0 ? (mp_size_t)(-cl[i]) : (mp_size_t)cl[i];
        lmmp_copy(dst->p[i / 2][i % 2], c[i], l);
        dst->n[i / 2][i % 2] = l;
    }
}

/* ==================== 乘法 basecase：8 次乘法 ====================
   C_ij = A_i0*B_0j + A_i1*B_1j */
static void mat22_mul_basecase_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, const lmmp_mat22_t* b, mp_ptr tp) {
    mp_size_t pn = mat22_maxlen_(a) + mat22_maxlen_(b) + 2;
    mp_ptr c[4];
    mp_ssize_t cl[4];
    for (int i = 0; i < 4; ++i) {
        c[i] = tp + (mp_size_t)i * pn;
    }
    mp_ptr p = tp + 4 * pn; /* 第二乘积 ping-pong 缓冲 */
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            mp_ptr cij = c[2 * i + j];
            mp_ssize_t l1 = mat22_mulsg_(cij, a->p[i][0], (mp_ssize_t)a->n[i][0], b->p[0][j], (mp_ssize_t)b->n[0][j]);
            mp_ssize_t l2 = mat22_mulsg_(p, a->p[i][1], (mp_ssize_t)a->n[i][1], b->p[1][j], (mp_ssize_t)b->n[1][j]);
            cl[2 * i + j] = mat22_addsg_(cij, cij, l1, p, l2);
        }
    }
    mat22_store_(dst, c, cl);
}

/* ==================== Winograd-Strassen 乘法：7 次乘法 ====================
   记 A = /a00 a01\，B 同。组合量与乘积安置在 9 个等宽槽位（宽
           \a10 a11/
   W = 2*mx+4，容纳任意中间量 <= mx+1 与乘积 <= 2*mx+2）：
   s1..s4 与 t1..t4 各占一槽；每个组合量恰被一个乘积消耗，乘积依次
   回填已释放的槽位（见行内注释），全程无额外缓冲。
     s1 = a11+a01   t1 = b11+b01    p1 = s1*t1   p2 = s2*t2   p3 = s3*t3
     s2 = a11-a10   t2 = b11-b10    p4 = a00*b00 p5 = a01*b10
     s3 = s2+a01    t3 = t2+b01     p6 = s4*b01  p7 = a10*t4
     s4 = s3-a00    t4 = t3-b00
     U1 = p3+p5；U2 = p1-U1；U3 = U1-p2
     C00 = p4+p5；C01 = U3-p6；C10 = U2-p7；C11 = p2+U2 */
static void mat22_mul_strassen_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, const lmmp_mat22_t* b, mp_ptr tp) {
    mp_size_t mx = mat22_maxlen_(a);
    mp_size_t mxb = mat22_maxlen_(b);
    if (mxb > mx) {
        mx = mxb;
    }
    mp_size_t W = 2 * mx + 4;
#define SLOT(i) (tp + (mp_size_t)(i) * W)
    mp_ptr s1 = SLOT(0), s2 = SLOT(1), s3 = SLOT(2), s4 = SLOT(3);
    mp_ptr t1 = SLOT(4), t2 = SLOT(5), t3 = SLOT(6), t4 = SLOT(7);
    mp_ptr spare = SLOT(8);
    mp_ssize_t ns1, ns2, ns3, ns4, nt1, nt2, nt3, nt4;
    ns1 = mat22_addsg_(s1, a->p[1][1], (mp_ssize_t)a->n[1][1], a->p[0][1], (mp_ssize_t)a->n[0][1]);
    ns2 = mat22_addsg_(s2, a->p[1][1], (mp_ssize_t)a->n[1][1], a->p[1][0], -(mp_ssize_t)a->n[1][0]);
    ns3 = mat22_addsg_(s3, s2, ns2, a->p[0][1], (mp_ssize_t)a->n[0][1]);
    ns4 = mat22_addsg_(s4, s3, ns3, a->p[0][0], -(mp_ssize_t)a->n[0][0]);
    nt1 = mat22_addsg_(t1, b->p[1][1], (mp_ssize_t)b->n[1][1], b->p[0][1], (mp_ssize_t)b->n[0][1]);
    nt2 = mat22_addsg_(t2, b->p[1][1], (mp_ssize_t)b->n[1][1], b->p[1][0], -(mp_ssize_t)b->n[1][0]);
    nt3 = mat22_addsg_(t3, t2, nt2, b->p[0][1], (mp_ssize_t)b->n[0][1]);
    nt4 = mat22_addsg_(t4, t3, nt3, b->p[0][0], -(mp_ssize_t)b->n[0][0]);

    /* 乘积槽位：p1 用空槽；p2/p3/p6 回填已消耗的 s 槽；p4/p5/p7 占 t 槽 */
    mp_ptr p1 = spare, p2 = s1, p3 = s2, p4 = t1, p5 = t2, p6 = s3, p7 = t3;
    mp_ssize_t np1 = mat22_mulsg_(p1, s1, ns1, t1, nt1);
    mp_ssize_t np2 = mat22_mulsg_(p2, s2, ns2, t2, nt2); /* s2、t2 已消耗 */
    mp_ssize_t np3 = mat22_mulsg_(p3, s3, ns3, t3, nt3); /* s3、t3 已消耗 */
    mp_ssize_t np4 = mat22_mulsg_(p4, a->p[0][0], (mp_ssize_t)a->n[0][0], b->p[0][0], (mp_ssize_t)b->n[0][0]);
    mp_ssize_t np5 = mat22_mulsg_(p5, a->p[0][1], (mp_ssize_t)a->n[0][1], b->p[1][0], (mp_ssize_t)b->n[1][0]);
    mp_ssize_t np6 = mat22_mulsg_(p6, s4, ns4, b->p[0][1], (mp_ssize_t)b->n[0][1]); /* s4 已消耗 */
    mp_ssize_t np7 = mat22_mulsg_(p7, a->p[1][0], (mp_ssize_t)a->n[1][0], t4, nt4); /* t4 已消耗 */

    /* U 链与四个结果元素均安置在已消耗的槽位内 */
    mp_ssize_t nu1 = mat22_addsg_(p3, p3, np3, p5, np5);    /* U1 = p3+p5，存 p3 槽 */
    mp_ssize_t nu2 = mat22_addsg_(p1, p1, np1, p3, -nu1);   /* U2 = p1-U1，存 p1 槽 */
    mp_ssize_t nu3 = mat22_addsg_(p3, p3, nu1, p2, -np2);   /* U3 = U1-p2，存 p3 槽 */
    mp_ptr c[4];
    mp_ssize_t cl[4];
    c[0] = p4;
    cl[0] = mat22_addsg_(p4, p4, np4, p5, np5);             /* C00 = p4+p5 */
    c[1] = p6;
    cl[1] = mat22_addsg_(p6, p3, nu3, p6, -np6);            /* C01 = U3-p6 */
    c[2] = p7;
    cl[2] = mat22_addsg_(p7, p1, nu2, p7, -np7);            /* C10 = U2-p7 */
    c[3] = p2;
    cl[3] = mat22_addsg_(p2, p2, np2, p1, nu2);             /* C11 = p2+U2 */
    mat22_store_(dst, c, cl);
#undef SLOT
}

/* ==================== 平方 basecase：5 次乘法（对称性） ====================
   s = a00+a11；C00 = a00^2 + a01*a10；C01 = a01*s；C10 = a10*s；
   C11 = a11^2 + a01*a10（交叉积 a01*a10 复用，仅 5 次大数乘） */
static void mat22_sqr_basecase_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, mp_ptr tp) {
    mp_size_t mx = mat22_maxlen_(a);
    mp_size_t pn = 2 * mx + 2;
    mp_ptr c0 = tp, c1 = tp + pn, c2 = tp + 2 * pn, c3 = tp + 3 * pn;
    mp_ptr p = tp + 4 * pn;
    mp_ptr s = tp + 5 * pn;
    mp_ssize_t ls = mat22_addsg_(s, a->p[0][0], (mp_ssize_t)a->n[0][0], a->p[1][1], (mp_ssize_t)a->n[1][1]);
    mp_ssize_t lcross = mat22_mulsg_(p, a->p[0][1], (mp_ssize_t)a->n[0][1], a->p[1][0], (mp_ssize_t)a->n[1][0]);
    mp_ptr c[4];
    mp_ssize_t cl[4];
    mp_ssize_t l00 = mat22_sqrsg_(c0, a->p[0][0], (mp_ssize_t)a->n[0][0]);
    c[0] = c0;
    cl[0] = mat22_addsg_(c0, c0, l00, p, lcross);           /* C00 = a00^2+p */
    c[1] = c1;
    cl[1] = mat22_mulsg_(c1, a->p[0][1], (mp_ssize_t)a->n[0][1], s, ls); /* C01 = a01*s */
    c[2] = c2;
    cl[2] = mat22_mulsg_(c2, a->p[1][0], (mp_ssize_t)a->n[1][0], s, ls); /* C10 = a10*s */
    mp_ssize_t l11 = mat22_sqrsg_(c3, a->p[1][1], (mp_ssize_t)a->n[1][1]);
    c[3] = c3;
    cl[3] = mat22_addsg_(c3, c3, l11, p, lcross);           /* C11 = a11^2+p */
    mat22_store_(dst, c, cl);
}

/* ==================== Strassen 平方：7 次乘法（4 次平方） ====================
   B = A 时组合量对称（t1=s1, t2=s2, t3=s3, t4=s4），乘积退化为
   p1=s1^2、p2=s2^2、p3=s3^2、p4=a00^2、p5=a01*a10、p6=s4*a01、
   p7=a10*s4，U 链与 C 公式同乘法。槽位复用同乘法（t 槽让给 p4/p5） */
static void mat22_sqr_strassen_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, mp_ptr tp) {
    mp_size_t mx = mat22_maxlen_(a);
    mp_size_t W = 2 * mx + 4;
#define SLOT(i) (tp + (mp_size_t)(i) * W)
    mp_ptr s1 = SLOT(0), s2 = SLOT(1), s3 = SLOT(2), s4 = SLOT(3);
    mp_ptr spare = SLOT(4), p4s = SLOT(5), p5s = SLOT(6), p7s = SLOT(7);
    mp_ssize_t ns1, ns2, ns3, ns4;
    ns1 = mat22_addsg_(s1, a->p[1][1], (mp_ssize_t)a->n[1][1], a->p[0][1], (mp_ssize_t)a->n[0][1]);
    ns2 = mat22_addsg_(s2, a->p[1][1], (mp_ssize_t)a->n[1][1], a->p[1][0], -(mp_ssize_t)a->n[1][0]);
    ns3 = mat22_addsg_(s3, s2, ns2, a->p[0][1], (mp_ssize_t)a->n[0][1]);
    ns4 = mat22_addsg_(s4, s3, ns3, a->p[0][0], -(mp_ssize_t)a->n[0][0]);

    /* p6 回填已消耗的 s3 槽，其余用 t 槽与空槽 */
    mp_ptr p1 = spare, p2 = s1, p3 = s2, p4 = p4s, p5 = p5s, p6 = s3, p7 = p7s;
    mp_ssize_t np1 = mat22_sqrsg_(p1, s1, ns1);
    mp_ssize_t np2 = mat22_sqrsg_(p2, s2, ns2); /* s2 已消耗 */
    mp_ssize_t np3 = mat22_sqrsg_(p3, s3, ns3); /* s3 已消耗 */
    mp_ssize_t np4 = mat22_sqrsg_(p4, a->p[0][0], (mp_ssize_t)a->n[0][0]);
    mp_ssize_t np5 = mat22_mulsg_(p5, a->p[0][1], (mp_ssize_t)a->n[0][1], a->p[1][0], (mp_ssize_t)a->n[1][0]);
    mp_ssize_t np6 = mat22_mulsg_(p6, s4, ns4, a->p[0][1], (mp_ssize_t)a->n[0][1]); /* s4 尚需用于 p7 */
    mp_ssize_t np7 = mat22_mulsg_(p7, a->p[1][0], (mp_ssize_t)a->n[1][0], s4, ns4); /* 对称 t4=s4，此后 s4 已消耗 */

    mp_ssize_t nu1 = mat22_addsg_(p3, p3, np3, p5, np5);    /* U1 = p3+p5 */
    mp_ssize_t nu2 = mat22_addsg_(p1, p1, np1, p3, -nu1);   /* U2 = p1-U1 */
    mp_ssize_t nu3 = mat22_addsg_(p3, p3, nu1, p2, -np2);   /* U3 = U1-p2 */
    mp_ptr c[4];
    mp_ssize_t cl[4];
    c[0] = p4;
    cl[0] = mat22_addsg_(p4, p4, np4, p5, np5);             /* C00 = p4+p5 */
    c[1] = p6;
    cl[1] = mat22_addsg_(p6, p3, nu3, p6, -np6);            /* C01 = U3-p6 */
    c[2] = p7;
    cl[2] = mat22_addsg_(p7, p1, nu2, p7, -np7);            /* C10 = U2-p7 */
    c[3] = p2;
    cl[3] = mat22_addsg_(p2, p2, np2, p1, nu2);             /* C11 = p2+U2 */
    mat22_store_(dst, c, cl);
#undef SLOT
}

/* 分派判据：任一元素短于阈值即走 basecase（按各元素真实长度相乘，
   混合大小元素时远优于把全部操作数填充到最大元素宽度的 Strassen；
   hgcd 中单位阵×大矩阵的情形即属此类）。仅当全部元素均达到阈值
   才启用 Strassen */
static int mat22_all_big_(const lmmp_mat22_t* x, mp_size_t th) {
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            if (x->n[i][j] < th) {
                return 0;
            }
        }
    }
    return 1;
}

void lmmp_mat22_mul_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, const lmmp_mat22_t* b, mp_ptr tp) {
    lmmp_param_assert(dst != NULL && a != NULL && b != NULL && tp != NULL);
    if (mat22_all_big_(a, MAT22_MUL_STRASSEN_THRESHOLD) && mat22_all_big_(b, MAT22_MUL_STRASSEN_THRESHOLD)) {
        mat22_mul_strassen_(dst, a, b, tp);
    } else {
        mat22_mul_basecase_(dst, a, b, tp);
    }
}

void lmmp_mat22_sqr_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, mp_ptr tp) {
    lmmp_param_assert(dst != NULL && a != NULL && tp != NULL);
    if (mat22_all_big_(a, MAT22_SQR_STRASSEN_THRESHOLD)) {
        mat22_sqr_strassen_(dst, a, tp);
    } else {
        mat22_sqr_basecase_(dst, a, tp);
    }
}
