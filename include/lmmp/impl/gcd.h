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

#ifndef __LMMP_IMPL_GCD_H__
#define __LMMP_IMPL_GCD_H__

#include "../lmmpn.h"
#include "../numth.h"

/*
hgcd 递归阈值：公共长度超过该值时启用分治路径（hgcd.c 的核与 gcdext.c
的驱动循环共用；待接入 tune 体系）
*/
#define HGCD_RECURSE_THRESHOLD 64

typedef struct {
    mp_ptr base;
    mp_size_t cap;  /* 总容量 */
    mp_size_t used; /* 当前游标 */
} lmmp_hgcd_scratch_t;

/* 从递增分配器取 n limbs（出口用 sc->used = mark 复位） */
static inline mp_ptr lmmp_hgcd_salloc_(lmmp_hgcd_scratch_t* sc, mp_size_t n) {
    lmmp_debug_assert(sc->used + n <= sc->cap);
    mp_ptr p = sc->base + sc->used;
    sc->used += n;
    return p;
}

static inline mp_size_t lmmp_gcd_norm_(mp_srcptr p, mp_size_t n) {
    while (n > 0 && p[n - 1] == 0) {
        --n;
    }
    return n;
}

static inline mp_size_t lmmp_gcd_matrix_maxlen_(const lmmp_hgcd_matrix_t* M) {
    mp_size_t mx = 0;
    if (M->n[0][0] > mx)
        mx = M->n[0][0];
    if (M->n[0][1] > mx)
        mx = M->n[0][1];
    if (M->n[1][0] > mx)
        mx = M->n[1][0];
    if (M->n[1][1] > mx)
        mx = M->n[1][1];
    return mx;
}

/* 矩阵列交换（指针交换）：对应右乘 P（M <- M*P），数对翻转簿记用 */
static inline void lmmp_gcd_matrix_swap_cols_(lmmp_hgcd_matrix_t* M) {
    LMMP_SWAP(M->m[0][0], M->m[0][1], mp_ptr);
    LMMP_SWAP(M->m[1][0], M->m[1][1], mp_ptr);
    LMMP_SWAP(M->n[0][0], M->n[0][1], mp_size_t);
    LMMP_SWAP(M->n[1][0], M->n[1][1], mp_size_t);
}

static inline void lmmp_gcd_matrix_init_buf_(lmmp_hgcd_matrix_t* M, mp_ptr buf, mp_size_t alloc) {
    lmmp_debug_assert(M != NULL && buf != NULL && alloc > 0);
    M->m[0][0] = buf;
    M->m[0][1] = buf + alloc;
    M->m[1][0] = buf + 2 * alloc;
    M->m[1][1] = buf + 3 * alloc;
    M->n[0][0] = 1;
    M->n[0][1] = 0;
    M->n[1][0] = 0;
    M->n[1][1] = 1;
    M->alloc = alloc;
    M->m[0][0][0] = 1;
    M->m[1][1][0] = 1;
}

static inline void lmmp_gcd_mul_(mp_ptr dst, mp_srcptr a, mp_size_t an, mp_srcptr b, mp_size_t bn) {
    if (an >= bn) {
        lmmp_mul_(dst, a, an, b, bn);
    } else {
        lmmp_mul_(dst, b, bn, a, an);
    }
}

#define GCD_LEHMER_MIN_V 0x100000000ll /* 停机界：窗口余数将跌破 2^32 时停止 */
#define GCD_LEHMER_EXACT_BITS 63       /* 顶端对齐窗口位宽 */

typedef struct {
    slong m11, m12;
    slong m21, m22;
} lmmp_gcd_lehmer_t;

/*
步进矩阵：在窗口 (u,v) 上运行小规模欧几里得。
消费序 new_a = C*a + D*b、new_b = A*a + B*b（C=m21、D=m22、A=m11、B=m12），
元素带符号，|A..D| < 2^31（v >= 2^32 停机界：元素 <= x/2^32 < 2^63/2^32）
*/
static inline void lmmp_gcd_lehmer_step_(slong u, slong v, lmmp_gcd_lehmer_t* M) {
#define A (M->m11)
#define B (M->m12)
#define C (M->m21)
#define D (M->m22)
    A = 1;
    B = 0;
    C = 0;
    D = 1;
    while (v != 0) {
        slong q = u / v;
        slong t = u % v;
        u = v;
        v = t;
        t = A - q * C;
        A = C;
        C = t;
        t = B - q * D;
        B = D;
        D = t;
        if (v < (slong)GCD_LEHMER_MIN_V)
            break;
    }
#undef A
#undef B
#undef C
#undef D
}

static inline void lmmp_gcd_lehmer_extract_(mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn,
                                            slong* restrict a, slong* restrict b) {
    int kz = lmmp_limb_bits_(up[un - 1]);
    if (kz >= GCD_LEHMER_EXACT_BITS) {
        *a = up[un - 1] >> (kz - GCD_LEHMER_EXACT_BITS);
        if (vn == un)
            *b = vp[vn - 1] >> (kz - GCD_LEHMER_EXACT_BITS);
        else
            *b = 0;
    } else {
        *a = up[un - 1] << (GCD_LEHMER_EXACT_BITS - kz);
        *a |= up[un - 2] >> (LIMB_BITS - (GCD_LEHMER_EXACT_BITS - kz));
        if (un > vn + 1) {
            *b = 0;
        } else if (un == vn + 1) {
            *b = vp[vn - 1] >> (LIMB_BITS - (GCD_LEHMER_EXACT_BITS - kz));
        } else {
            *b = vp[vn - 1] << (GCD_LEHMER_EXACT_BITS - kz);
            *b |= vp[vn - 2] >> (LIMB_BITS - (GCD_LEHMER_EXACT_BITS - kz));
        }
    }
}

/*
dst = (x,xneg) ± (y,yneg)：按符号分派和/差；dst 与 x,y 缓冲分离。
零向量约定 n==0（neg 无意义）
*/
static inline void lmmp_gcdext_smag_(mp_ptr dst, mp_size_t* n, int* neg, mp_srcptr x, mp_size_t xn,
                                     int xneg, mp_srcptr y, mp_size_t yn, int yneg) {
    if (xn == 0) {
        lmmp_copy(dst, y, yn);
        *n = yn;
        *neg = yneg;
        return;
    }
    if (yn == 0) {
        lmmp_copy(dst, x, xn);
        *n = xn;
        *neg = xneg;
        return;
    }
    if (xneg == yneg) {
        mp_limb_t c;
        if (xn >= yn) {
            c = lmmp_add_(dst, x, xn, y, yn);
            *n = xn;
        } else {
            c = lmmp_add_(dst, y, yn, x, xn);
            *n = yn;
        }
        if (c != 0)
            dst[(*n)++] = c;
        *neg = xneg;
        return;
    }
    int c = (xn != yn) ? (xn < yn ? -1 : 1) : lmmp_cmp_(x, y, xn);
    if (c == 0) {
        *n = 0;
        *neg = 0;
        return;
    }
    if (c > 0) {
        lmmp_sub_(dst, x, xn, y, yn);
        *n = xn;
        *neg = xneg;
    } else {
        lmmp_sub_(dst, y, yn, x, xn);
        *n = yn;
        *neg = yneg;
    }
    while (*n > 0 && dst[*n - 1] == 0)
        --*n;
    if (*n == 0)
        *neg = 0;
}

/**
 * @brief hgcd 递归核：将数对归约至约 n/2 规模并累积变换矩阵（详见 hgcd.c 实现注）
 * @param M 变换矩阵：入口应为单位矩阵，出口累积全部归约变换
 * @param ap 较大分量输入兼输出数组（输入读 [ap,n)，ap[n-1]>0；输出写 [ap,rn) 并零填充至 rn）
 * @param bp 较小分量输入兼输出数组（允许高位为零，输出零填充至 rn）
 * @param n ap,bp 的公共输入长度
 * @param sc 临时空间（容量由 lmmp_hgcd_scratch_size_ 给出）
 * @param detsign 出参，返回 det(M) 的符号
 * @param appr 1 表示近似模式（仅矩阵累积，数对停留在约 3n/4；见 hgcd.c 实现注）
 * @return 归约后公共长度 rn；0 表示未做任何归约（M 保持单位矩阵）
 * @warning M!=NULL, sc!=NULL, ap!=NULL, bp!=NULL, sep(ap,bp), a>b>0, n>=3, M->alloc>=n+2
 */
mp_size_t lmmp_hgcd_core_(lmmp_hgcd_matrix_t* M,
                          mp_ptr ap,
                          mp_ptr bp,
                          mp_size_t n,
                          lmmp_hgcd_scratch_t* sc,
                          int* detsign,
                          int appr);

/**
 * @brief 高位直写 + 低位修补：将子递归成果（矩阵 M1 与归约后的高位对）合入完整数对
 * @param M1 子递归累积的矩阵（各元素输入长度为 M1->n 个limb）
 * @param detsign det(M1) 的符号，[1|-1]
 * @param dap 分量 a 输出数组（写入 n 个limb，返回 0 时内容无效）
 * @param dbp 分量 b 输出数组（写入 n 个limb，返回 0 时内容无效）
 * @param blen 出参，分量 b 合入后的真实长度（供上层校验）
 * @param ha 子递归归约后的高位 a（长度为 hn0 个limb，可为 ap+p 原地）
 * @param hb 子递归归约后的高位 b（长度为 hn0 个limb）
 * @param hn0 子递归返回的公共长度，hn0<=n-p
 * @param ap 源完整较大分量（长度为 n 个limb，低 p 位参与修补）
 * @param bp 源完整较小分量（长度为 n 个limb）
 * @param n ap,bp 的公共长度
 * @param p 高低位分裂点
 * @param sc 临时空间（4*(p+M1->n+1) 个limb）
 * @return 合入后的公共长度；0 表示子归约过冲（借位穿底）
 * @warning M1!=NULL, dap!=NULL, dbp!=NULL, sc!=NULL, sep([dap|dbp],[ap|bp|ha|hb]),
 *          n>p>0, ap[n-1]>0
 */
mp_size_t lmmp_hgcd_adjust_(lmmp_hgcd_matrix_t* M1,
                            int detsign,
                            mp_ptr dap,
                            mp_ptr dbp,
                            mp_size_t* blen,
                            mp_srcptr ha,
                            mp_srcptr hb,
                            mp_size_t hn0,
                            mp_srcptr ap,
                            mp_srcptr bp,
                            mp_size_t n,
                            mp_size_t p,
                            lmmp_hgcd_scratch_t* sc);

/**
 * @brief 计算 hgcd 内部临时空间大小（容量上界推导见 hgcd.c 定义处注）
 * @param n 输入长度
 * @warning n>0
 */
mp_size_t lmmp_hgcd_scratch_size_(mp_size_t n);

/**
 * @brief 导出另一侧系数：tv = (g - S·X)/Y（S 为 X 侧系数，恒等式保证整除）
 * @param dst 输出（容量 >= xn+2）
 * @param tneg 出参，tv 的符号
 * @param gp g（归一化）
 * @param gn g 的长度
 * @param sp S 的幅度（归一化）
 * @param sn S 的幅度长
 * @param sneg S 的符号
 * @param xp 值序大者原对（归一化）
 * @param xn 其长度
 * @param yp 值序小者原对（归一化）
 * @param yn 其长度
 * @param nb 全乘积暂存（容量 >= xn+yn+2）
 * @param yt Y 奇化暂存（容量 >= yn+1）
 * @param qt 整商暂存（容量 >= xn+2；lmmp_divexact_ 写入长 nn-yn2+1 可超
 *           出 dst 公开容量，先落此处再按归一化长度拷出）
 * @return tv 写入长度（tv==0 理论不可达时返回 0）
 * @warning sep([dst|nb|yt|qt],[gp|sp|xp|yp])，gn>0，sn<=xn
 * @note S==0 时 Y|X 且 g==Y，tv=1。Y 偶时剥公因子 2^k 后走 lmmp_divexact_
 *       （要求奇除数；N = tv*Y 保证 N 的低 k 位为零）。S 与 tv 反号（极小
 *       系数性质，|S|*X >= X >= g 使 g 落在 0 与 |S|*X 之间）
 */
mp_size_t lmmp_gcdext_derive_(mp_ptr dst, int* tneg, mp_srcptr gp, mp_size_t gn, mp_srcptr sp,
                              mp_size_t sn, int sneg, mp_srcptr xp, mp_size_t xn, mp_srcptr yp,
                              mp_size_t yn, mp_ptr nb, mp_ptr yt, mp_ptr qt);

/**
 * @brief 发射：g 与 S（值序大者系数）按 sw 映射到 (dsts,dstt)，另一系数经恒等式导出
 * @param dstg 结果 g 输出（容量 >= gn）
 * @param dsts u 侧系数输出（容量 >= max(un,vn)+1）
 * @param sn 输出 s 的带符号长度
 * @param dstt v 侧系数输出（容量 >= max(un,vn)+1，可为 NULL 表示不需要）
 * @param tn 输出 t 的带符号长度（dstt==NULL 时忽略，可为 NULL）
 * @param gp g（归一化）
 * @param gn g 的长度
 * @param sp S 的幅度（归一化）
 * @param sun S 的幅度长
 * @param suneg S 的符号
 * @param sw 值序归一化的交换标志（1 表示 up/vp 已交换，S 为 vp 侧系数）
 * @param up 值序大者原对
 * @param un 其长度
 * @param vp 值序小者原对
 * @param vn 其长度
 * @param nb 导出全乘积暂存（容量 >= 2*max(un,vn)+4）
 * @param yt 导出奇化暂存（容量 >= max(un,vn)+2）
 * @param qt 导出整商暂存（容量 >= max(un,vn)+2）
 * @warning sep([dsts|dstt],dstg)，对 up/vp 的全部读取（导出）先于输出写入
 */
void lmmp_gcdext_emit_(mp_ptr dstg, mp_ptr dsts, mp_ssize_t* sn, mp_ptr dstt, mp_ssize_t* tn,
                       mp_srcptr gp, mp_size_t gn, mp_srcptr sp, mp_size_t sun, int suneg, int sw,
                       mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn, mp_ptr nb, mp_ptr yt,
                       mp_ptr qt);

#endif /* __LMMP_IMPL_GCD_H__ */
