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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/fft_ssa.h"
#include "../../../include/lmmp/lmmpn.h"


/**
 * @brief 判断 n 是否为费马变换可行尺寸（64n 能被 2^best_k 整除）
 * @param n 输入的 limb 长度
 * @return 1 可行 / 0 不可行
 */
static int mrs_fermat_usable_(mp_size_t n) {
    mp_size_t k = lmmp_fft_best_k_(n) - LOG2_LIMB_BITS;
    return (n & (((mp_size_t)1 << k) - 1)) == 0;
}

/**
 * @brief 取负 [dst,n] = B^n - [numa,n]，返回进位（= [numa,n] != 0）
 * @param dst 输出缓冲区，长度至少为 n
 * @param numa 输入操作数，长度为 n
 * @param n 操作数的 limb 长度
 * @warning n>0, sep/eqsep(dst,numa)
 */
static mp_limb_t mrs_neg_(mp_ptr dst, mp_srcptr numa, mp_size_t n) {
    if (lmmp_zero_q_(numa, n)) {
        lmmp_zero(dst, n);
        return 0;
    }
    // B^n - x = ~x + 1；x!=0 保证 ~x+1 不越过 n limb
    lmmp_not_(dst, numa, n);
    lmmp_inc(dst);
    return 1;
}

/**
 * @brief mod B^n+1 基例乘法 [rp,n+1] = [ap,n+1]*[bp,n+1] mod B^n+1（规范形 [0,B^n]）
 * @param rp 输出缓冲区，长度至少为 n+1
 * @param ap 第一个操作数（高半域形，值 < B^n+1），长度为 n+1
 * @param bp 第二个操作数，长度为 n+1
 * @param n 模运算的阶数
 * @param tp 工作区，长度至少为 2n（允许 tp==rp）
 * @warning n>0, sep(tp,[ap|bp]) 或 tp==rp
 * @note 顶位特例：ap/bp 顶 limb 为 1（值 = B^n ≡ -1）时以取反代乘
 */
static void mrs_mul_bnp1_(mp_ptr rp, mp_srcptr ap, mp_srcptr bp, mp_size_t n, mp_ptr tp) {
    mp_limb_t cy;
    if (ap[n] | bp[n]) {
        if (ap[n])
            cy = bp[n] + mrs_neg_(rp, bp, n);
        else  // ap[n]==0
            cy = mrs_neg_(rp, ap, n);
    } else {
        lmmp_mul_n_(tp, ap, bp, n);
        cy = lmmp_sub_n_(rp, tp, tp + n, n);
    }
    rp[n] = 0;
    // cy==1 时 rp = lo-hi+B^n <= B^n-1，再 +1 不会越过 rp[n]
    lmmp_inc_1(rp, cy);
}

/**
 * @brief mod B^n+1 基例平方 [rp,n+1] = [ap,n+1]^2 mod B^n+1（规范形 [0,B^n]）
 * @param rp 输出缓冲区，长度至少为 n+1
 * @param ap 操作数（高半域形，值 < B^n+1），长度为 n+1
 * @param n 模运算的阶数
 * @param tp 工作区，长度至少为 2n（允许 tp==rp）
 * @warning n>0, sep(tp,ap) 或 tp==rp
 * @note 顶位特例：ap 顶 limb 为 1（值 ≡ -1）时结果为 1
 */
static void mrs_sqr_bnp1_(mp_ptr rp, mp_srcptr ap, mp_size_t n, mp_ptr tp) {
    mp_limb_t cy;
    if (ap[n]) {  // (-1)^2 = 1
        rp[0] = 1;
        lmmp_zero(rp + 1, n);
        return;
    }
    lmmp_sqr_(tp, ap, n);
    cy = lmmp_sub_n_(rp, tp, tp + n, n);
    rp[n] = 0;
    lmmp_inc_1(rp, cy);
}

/**
 * @brief 分治梅森乘法核心（半归一化内部约定：零类 = B^n-1）
 * @param rp 输出 [rp,rn] = [ap,an]*[bp,bn] mod B^rn-1（半归一化，写入完整 rn limb）
 * @param rn 模运算的阶数
 * @param ap 第一个操作数
 * @param an 第一个操作数的 limb 长度
 * @param bp 第二个操作数
 * @param bn 第二个操作数的 limb 长度
 * @param tp 工作区，长度至少为 2rn+4
 * @warning 0<bn<=an<=rn, an+bn>rn/2, sep(rp,[ap|bp]), sep(tp,[rp|ap|bp])
 */
static void mrs_mul_core_(mp_ptr rp, mp_size_t rn, mp_srcptr ap, mp_size_t an, mp_srcptr bp, mp_size_t bn, mp_ptr tp) {
    if ((rn & 1) != 0 || rn < MUL_MERSENNE_DC_THRESHOLD) {
        if (bn < rn) {
            if (an + bn <= rn) {
                // an+bn<=rn 蕴含 (B^an-1)(B^bn-1) < B^rn-1，积即精确残差
                lmmp_mul_(rp, ap, an, bp, bn);
                lmmp_zero(rp + an + bn, rn - (an + bn));
            } else {
                lmmp_mul_(tp, ap, an, bp, bn);
                mp_limb_t cy = lmmp_add_(rp, tp, rn, tp + rn, an + bn - rn);
                lmmp_inc_1(rp, cy);
            }
        } else {
            lmmp_mul_n_(tp, ap, bp, rn);
            mp_limb_t cy = lmmp_add_n_(rp, tp, tp + rn, rn);
            // cy==1 时 rp <= B^rn-2，+1 不会溢出
            lmmp_inc_1(rp, cy);
        }
        return;
    }

    mp_size_t n = rn >> 1;
    mp_limb_t cy;
    lmmp_debug_assert(an + bn > n);

    /*
      x = -xp * B^n + (B^n + 1) * [ (xp + xm)/2 mod (B^n-1)]
    */
#define a0 ap
#define a1 (ap + n)
#define b0 bp
#define b1 (bp + n)
#define xp tp /* 2n+2 */
    /* am1  maybe in {xp, n} */
    /* bm1  maybe in {xp + n, n} */
#define sp1 (tp + 2 * n + 2)
    /* ap1  maybe in {sp1, n + 1} */
    /* bp1  maybe in {sp1 + n + 1, n + 1} */

    {
        // xm = a*b mod (B^n-1)
        mp_srcptr am1, bm1;
        mp_size_t anm, bnm;
        mp_ptr so;

        bm1 = b0;
        bnm = bn;
        if (an > n) {
            am1 = xp;
            cy = lmmp_add_(xp, a0, n, a1, an - n);
            lmmp_inc_1(xp, cy);
            anm = n;
            so = xp + n;
            if (bn > n) {
                bm1 = so;
                cy = lmmp_add_(so, b0, n, b1, bn - n);
                lmmp_inc_1(so, cy);
                bnm = n;
                so += n;
            }
        } else {
            so = xp;
            am1 = a0;
            anm = an;
        }

        mrs_mul_core_(rp, n, am1, anm, bm1, bnm, so);
    }

    {
        // xp = a*b mod (B^n+1)，规范形 [xp,n+1] in [0,B^n]
        mp_srcptr ap1, bp1;
        mp_size_t anp, bnp;

        bp1 = b0;
        bnp = bn;
        if (an > n) {
            ap1 = sp1;
            cy = lmmp_sub_(sp1, a0, n, a1, an - n);
            sp1[n] = 0;
            lmmp_inc_1(sp1, cy);
            anp = n + sp1[n];
            if (bn > n) {
                bp1 = sp1 + n + 1;
                cy = lmmp_sub_(sp1 + n + 1, b0, n, b1, bn - n);
                sp1[2 * n + 1] = 0;
                lmmp_inc_1(sp1 + n + 1, cy);
                bnp = n + bp1[n];
            }
        } else {
            ap1 = a0;
            anp = an;
        }

        if (n >= MUL_MERSENNE_FERMAT_THRESHOLD && mrs_fermat_usable_(n)) {
            lmmp_mul_fermat_(xp, n, ap1, anp, bp1, bnp);
        } else if (bp1 == b0) {
            // bn<=n：直乘 + 反向折叠
            lmmp_mul_(xp, ap1, anp, bp1, bnp);
            anp = anp + bnp - n;
            lmmp_debug_assert(anp <= n || xp[2 * n] == 0);
            anp -= anp > n;
            cy = lmmp_sub_(xp, xp, n, xp + n, anp);
            xp[n] = 0;
            lmmp_inc_1(xp, cy);
        } else {
            mrs_mul_bnp1_(xp, ap1, bp1, n, xp);
        }
    }

    // xm <- (xp + xm)/2 = (xp + xm)*B^n/2 mod (B^n-1)，除2即循环右移（B^n=1）
    // [xp,n+1] 已规范形，[rp,n] 半归一化；零类（B^n-1）在半域表示下依旧成立
    cy = lmmp_shr1add_nc_(rp, rp, xp, n, xp[n]);
    cy <<= LIMB_BITS - 1;
    rp[n - 1] += cy;
    if (rp[n - 1] < cy)  // 仅当 rp[n-1] 溢出时进位，此时 [rp,n) 不可能全 B-1
        lmmp_inc(rp);

    // 高半段 = ([(xp+xm)/2 mod B^n-1] - xp) * B^n
    if (an + bn < rn) {
        // 此情形下结果为零当且仅当某操作数为零，且两路递归与 CRT 输出均为 0 而非 B^rn-1
        // 值只占 [rp,an+bn)；GMP 同构实现仅保证写 MIN(rn,an+bn) limb，此处补零至 rn
        cy = lmmp_sub_n_(rp + n, rp, xp, an + bn - n);
        // 高段相减只为取得借位链，值本身无意义
        cy = xp[n] + lmmp_sub_nc_(xp + an + bn - n, rp + an + bn - n, xp + an + bn - n, rn - (an + bn), cy);
        cy = lmmp_sub_1_(rp, rp, an + bn, cy);
        lmmp_zero(rp + an + bn, rn - (an + bn));
    } else {
        cy = xp[n] + lmmp_sub_n_(rp + n, rp, xp, n);
        // cy==2 不可能：xp[n]==1 蕴含 [xp,n]==0，即 [rp,n]!=0 时减法必不借位
        // cy==1 时 DECR 至多波及最低 n limb
        lmmp_dec_1(rp, cy);
    }

#undef a0
#undef a1
#undef b0
#undef b1
#undef xp
#undef sp1
}

/**
 * @brief 分治梅森平方核心（半归一化内部约定：零类 = B^n-1）
 * @param rp 输出 [rp,rn] = [ap,an]^2 mod B^rn-1（半归一化，写入完整 rn limb）
 * @param rn 模运算的阶数
 * @param ap 操作数
 * @param an 操作数的 limb 长度
 * @param tp 工作区，长度至少为 2rn+4
 * @warning 0<an<=rn, 4*an>rn, sep(rp,ap), sep(tp,[rp|ap])
 */
static void mrs_sqr_core_(mp_ptr rp, mp_size_t rn, mp_srcptr ap, mp_size_t an, mp_ptr tp) {
    if ((rn & 1) != 0 || rn < SQR_MERSENNE_DC_THRESHOLD) {
        if (an < rn) {
            if (2 * an <= rn) {
                // (B^an-1)^2 < B^rn-1，平方即精确残差
                lmmp_sqr_(rp, ap, an);
                lmmp_zero(rp + 2 * an, rn - 2 * an);
            } else {
                lmmp_sqr_(tp, ap, an);
                mp_limb_t cy = lmmp_add_(rp, tp, rn, tp + rn, 2 * an - rn);
                lmmp_inc_1(rp, cy);
            }
        } else {
            lmmp_sqr_(tp, ap, rn);
            mp_limb_t cy = lmmp_add_n_(rp, tp, tp + rn, rn);
            lmmp_inc_1(rp, cy);
        }
        return;
    }

    mp_size_t n = rn >> 1;
    mp_limb_t cy;
    lmmp_debug_assert(2 * an > n);

#define a0 ap
#define a1 (ap + n)
#define xp tp /* 2n+2 */
    /* am1  maybe in {xp, n} */
#define sp1 (tp + 2 * n + 2)
    /* ap1  maybe in {sp1, n + 1} */

    {
        // xm = a^2 mod (B^n-1)
        mp_srcptr am1;
        mp_size_t anm;
        mp_ptr so;

        if (an > n) {
            so = xp + n;
            am1 = xp;
            cy = lmmp_add_(xp, a0, n, a1, an - n);
            lmmp_inc_1(xp, cy);
            anm = n;
        } else {
            so = xp;
            am1 = a0;
            anm = an;
        }

        mrs_sqr_core_(rp, n, am1, anm, so);
    }

    {
        // xp = a^2 mod (B^n+1)，规范形 [xp,n+1] in [0,B^n]
        mp_srcptr ap1;
        mp_size_t anp;

        if (an > n) {
            ap1 = sp1;
            cy = lmmp_sub_(sp1, a0, n, a1, an - n);
            sp1[n] = 0;
            lmmp_inc_1(sp1, cy);
            anp = n + sp1[n];
        } else {
            ap1 = a0;
            anp = an;
        }

        if (n >= SQR_MERSENNE_FERMAT_THRESHOLD && mrs_fermat_usable_(n)) {
            lmmp_sqr_fermat_(xp, n, ap1, anp);
        } else if (ap1 == a0) {
            // an<=n：直乘平方 + 反向折叠
            lmmp_sqr_(xp, a0, an);
            anp = 2 * an - n;
            cy = lmmp_sub_(xp, xp, n, xp + n, anp);
            xp[n] = 0;
            lmmp_inc_1(xp, cy);
        } else {
            mrs_sqr_bnp1_(xp, ap1, n, xp);
        }
    }

    // xm <- (xp + xm)/2 mod (B^n-1)，同乘法版 CRT
    cy = lmmp_shr1add_nc_(rp, rp, xp, n, xp[n]);
    cy <<= LIMB_BITS - 1;
    rp[n - 1] += cy;
    if (rp[n - 1] < cy)
        lmmp_inc(rp);

    if (2 * an < rn) {
        // 值只占 [rp,2an)，补零至 rn（同乘法版注释）
        cy = lmmp_sub_n_(rp + n, rp, xp, 2 * an - n);
        cy = xp[n] + lmmp_sub_nc_(xp + 2 * an - n, rp + 2 * an - n, xp + 2 * an - n, rn - 2 * an, cy);
        cy = lmmp_sub_1_(rp, rp, 2 * an, cy);
        lmmp_zero(rp + 2 * an, rn - 2 * an);
    } else {
        cy = xp[n] + lmmp_sub_n_(rp + n, rp, xp, n);
        lmmp_dec_1(rp, cy);
    }

#undef a0
#undef a1
#undef xp
#undef sp1
}

/**
 * @brief 零类归一化：[p,n] == B^n-1 时清零（内部半归一化 → 库规范 0 表示）
 * @param p 输入输出缓冲区，长度为 n
 * @param n limb 长度
 */
static void mrs_norm_zero_(mp_ptr p, mp_size_t n) {
    mp_size_t i = 0;
    while (i < n && p[i] == ~(mp_limb_t)0) ++i;
    if (i == n) lmmp_zero(p, n);
}

void lmmp_mul_mersenne_dc_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb) {
    lmmp_param_assert(na > 0 && nb > 0);
    lmmp_param_assert(nb <= na && na <= rn);
    lmmp_param_assert(dst != NULL && numa != NULL && numb != NULL);
    lmmp_debug_assert(na + nb > rn / 2);

    mp_ptr tp = ALLOC_TYPE(2 * rn + 4, mp_limb_t);
    mrs_mul_core_(dst, rn, numa, na, numb, nb, tp);
    mrs_norm_zero_(dst, rn);
    lmmp_free(tp);
}

void lmmp_sqr_mersenne_dc_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na) {
    lmmp_param_assert(na > 0 && na <= rn);
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_debug_assert(4 * na > rn);

    mp_ptr tp = ALLOC_TYPE(2 * rn + 4, mp_limb_t);
    mrs_sqr_core_(dst, rn, numa, na, tp);
    mrs_norm_zero_(dst, rn);
    lmmp_free(tp);
}

mp_size_t lmmp_mersenne_next_size_(mp_size_t n) {
    if (n < MUL_MERSENNE_DC_THRESHOLD) return n;
    if (n < 4 * (MUL_MERSENNE_DC_THRESHOLD - 1) + 1) return (n + 1) & ~(mp_size_t)1;
    if (n < 8 * (MUL_MERSENNE_DC_THRESHOLD - 1) + 1) return (n + 3) & ~(mp_size_t)3;
    mp_size_t nh = (n + 1) >> 1;
    if (nh < MUL_FFT_MODF_THRESHOLD) return (n + 7) & ~(mp_size_t)7;
    return 2 * lmmp_fft_next_size_(nh);
}

void lmmp_mul_mersenne_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb) {
    if (na < nb) {
        mp_srcptr tp = numa;
        mp_size_t tn = na;
        numa = numb;
        na = nb;
        numb = tp;
        nb = tn;
    }
    /* fft 分支要求 rn 为 fft 可行尺寸（缓存结构与 mersenne_next_size 的粗粒度
       输出均不保证），未对齐时回落 dc（任意 rn 合法） */
    if (rn >= MUL_MERSENNE_FFT_THRESHOLD && lmmp_fft_next_size_(rn) == rn)
        lmmp_mul_mersenne_fft_(dst, rn, numa, na, numb, nb);
    else
        lmmp_mul_mersenne_dc_(dst, rn, numa, na, numb, nb);
}

void lmmp_sqr_mersenne_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na) {
    if (rn >= SQR_MERSENNE_FFT_THRESHOLD && lmmp_fft_next_size_(rn) == rn)
        lmmp_sqr_mersenne_fft_(dst, rn, numa, na);
    else
        lmmp_sqr_mersenne_dc_(dst, rn, numa, na);
}
