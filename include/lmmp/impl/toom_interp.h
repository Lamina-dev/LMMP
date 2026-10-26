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

#ifndef __LMMP_TOOM_INTERP_H__
#define __LMMP_TOOM_INTERP_H__

#include "tmp_alloc.h"
#include "../lmmpn.h"

enum toom6_flags { toom6_all_pos = 0, toom6_vm1_neg = 1, toom6_vm2_neg = 2 };

enum toom7_flags { toom7_w1_neg = 1, toom7_w3_neg = 2 };

/**
 * @brief Toom插值计算（5点插值），用于Toom-33和Toom-42乘法算法
 * @param dst 输出结果缓冲区，存储插值计算结果
 *        v(0)储存在[dst,2n]，v(1)储存在[dst+2n,2n]
 * @param v2 v(2)插值点值，长度为 2n+1 个limb
 * @param vm1 v(-1)插值点值，长度为 2n+1 个limb
 * @param n 操作数的 limb 长度
 * @param spt 系数r4的长度
 * @param vm1_neg 符号标记：v(-1)是否为负数（1表示负，0表示正）
 * @param vinf0 无穷远点插值的低64位值
 * @warning dst!=NULL, v2!=NULL, vm1!=NULL, n>0
 */
void lmmp_toom_interp5_(mp_ptr dst, mp_ptr v2, mp_ptr vm1, mp_size_t n, mp_size_t spt, int vm1_neg, mp_limb_t vinf0);

/**
 * @brief Toom插值计算（6点插值）：用于Toom-43和Toom-52 乘法算法
 * @param dst 最终乘积结果缓冲区（5n + w0n limbs）
 *            w5 储存在[dst,2n], w3 储存在[dst+2n,2n+1], w0 储存在[dst+5n,w0n].
 * @param n Toom-6 拆分后每段标准 limb 长度
 * @param flags 插值符号标志位
 * @param w4 输入/临时：点值 W4 缓冲区（2n+1 limbs）
 * @param w2 输入/临时：点值 W2 缓冲区（2n+1 limbs）
 * @param w1 输入/临时：点值 W1 缓冲区（2n+1 limbs）
 * @param w0n 输入：最低位段 W0 的实际 limb 长度（0 < w0n <= 2n）
 * @note w5=f(0), w4=f(-1), w3=f(1), w2=f(-2), w1=f(2), w0=f(inf)
 * @warning n>0, 0<w0n<=2n, dst!=NULL, w4!=NULL, w2!=NULL, w1!=NULL
 */
void lmmp_toom_interp6_(mp_ptr dst, mp_size_t n, enum toom6_flags flags, mp_ptr w4, mp_ptr w2, mp_ptr w1, mp_size_t w0n);

/**
 * @brief Toom插值计算（7点插值）：用于Toom-44、Toom-53、Toom-62 乘法算法
 * @param dst 输出结果缓冲区，存储插值计算结果（6*n + w6n limbs）
 *            w0 储存在[dst,2n], w2 储存在[dst+2n,2n+1], w6 储存在[dst+6n,w6n].
 * @param n Toom-7 拆分后每段标准 limb 长度
 * @param flags Toom-7 符号标志位，控制插值中的正负号运算
 * @param w1 输入/临时：点值 W1 缓冲区（2n+1 limbs）
 * @param w3 输入/临时：点值 W3 缓冲区（2n+1 limbs）
 * @param w4 输入/临时：点值 W4 缓冲区（2n+1 limbs）
 * @param w5 输入/临时：点值 W5 缓冲区（2n+1 limbs）
 * @param w6n 输入：最低位段 W6 的实际 limb 长度 (0 < w6n <= 2n)
 * @param tp 临时缓存空间（2*n+1 limbs）
 * @note w0=f(0), w1=f(-2), w2=f(1), w3=f(-1), w4=f(2), w5=64*f(1/2), w6=f(inf),
 * @warning n>0, 0<w6n<=2n, dst!=NULL, w1!=NULL, w3!=NULL, w4!=NULL, w5!=NULL, tp!=NULL
 */
void lmmp_toom_interp7_(mp_ptr dst, mp_size_t n, enum toom7_flags flags, mp_ptr w1, mp_ptr w3, mp_ptr w4, mp_ptr w5,
                        mp_size_t w6n, mp_ptr tp);

/**
 * @brief Toom-3 专用：3次多项式在 x = +1 和 x = -1 处求值
 * 计算 P(+1) 和 P(-1)，其中 P(x) 是一个3次多项式（4段系数）。
 * @param xp1 输出：P(+1) 的结果（n+1 个 limb）
 * @param xm1 输出：P(-1) 的结果（n+1 个 limb）
 * @param xp 输入：多项式系数数组（共4段，每段 n limbs）
 * @param n 每段完整系数的 limb 长度
 * @param x3n 最后一段系数的实际长度（通常等于 n）
 * @param tp 临时缓存空间（至少 n+1 limbs）
 * @warning 0<x3n<=n, xp1!=NULL, xm1!=NULL, xp!=NULL, tp!=NULL
 * @return 符号位：0=正，~0=负（对应 P(-1)）
 */
int lmmp_toom_eval_dgr3_pm1_(mp_ptr xp1, mp_ptr xm1, mp_srcptr xp, mp_size_t n, mp_size_t x3n, mp_ptr tp);

/**
 * @brief Toom-3 专用：3次多项式在 x = +2 和 x = -2 处求值
 * 计算 P(+2) 和 P(-2)，其中 P(x) 是一个3次多项式（4段系数）。
 * @param xp2 输出：P(+2) 的结果（n+1 个 limb）
 * @param xm2 输出：P(-2) 的结果（n+1 个 limb）
 * @param xp 输入：多项式系数数组（共4段，每段 n limbs）
 * @param n 每段完整系数的 limb 长度
 * @param x3n 最后一段系数的实际长度
 * @param tp 临时缓存空间（至少 n+1 limbs）
 * @warning 0<x3n<=n, xp2!=NULL, xm2!=NULL, xp!=NULL, tp!=NULL
 * @return 符号位：0=正，~0=负（对应 P(-2)）
 */
int lmmp_toom_eval_dgr3_pm2_(mp_ptr xp2, mp_ptr xm2, mp_srcptr xp, mp_size_t n, mp_size_t x3n, mp_ptr tp);

/**
 * @brief 通用高阶 Toom 求值：k次多项式在 x = +1 和 x = -1 处求值
 * @param xp1 输出：P(+1) 的结果（n+1 limbs）
 * @param xm1 输出：P(-1) 的结果（n+1 limbs）
 * @param k 输入：多项式次数（也是完整段的数量）
 * @param xp 输入：多项式系数数组
 * @param n 输入：每段完整系数的 limb 长度
 * @param hn 输入：最后一段系数的实际长度
 * @param tp 临时缓存空间（n+1 limbs）
 * @warning 0<hn<=n, 3<k, xp1!=NULL, xm1!=NULL, xp!=NULL, tp!=NULL
 * @return 符号位：0=正，~0=负
 */
int lmmp_toom_eval_pm1_(mp_ptr xp1, mp_ptr xm1, unsigned k, mp_srcptr xp, mp_size_t n, mp_size_t hn, mp_ptr tp);

/**
 * @brief 通用高阶 Toom 求值：k次多项式在 x = +2 和 x = -2 处求值
 * @param xp2 输出：P(+2) 的结果（n+1 limbs）
 * @param xm2 输出：P(-2) 的结果（n+1 limbs）
 * @param k 输入：多项式次数
 * @param xp 输入：多项式系数数组
 * @param n 输入：每段完整系数的 limb 长度
 * @param hn 输入：最后一段系数的实际长度
 * @param tp 临时缓存空间（n+1 limbs）
 * @warning 0<hn<=n, 3 < k < LIMB_BITS, xp2!=NULL, xm2!=NULL, xp!=NULL, tp!=NULL
 * @return 符号位：0=正，~0=负
 */
int lmmp_toom_eval_pm2_(mp_ptr xp2, mp_ptr xm2, unsigned k, mp_srcptr xp, mp_size_t n, mp_size_t hn, mp_ptr tp);

/**
 * @brief 通用高阶 Toom 求值：k次多项式在 x = +2^shift 和 x = -2^shift 处求值
 *        （toom6h 的 ±4 / toom8h 的 ±4、±8 点）
 * @param xp2 输出：P(+2^shift) 的结果（n+1 limbs）
 * @param xm2 输出：P(-2^shift) 的结果（n+1 limbs）；求值过程中亦作暂存
 * @param k 输入：多项式次数（完整段数）
 * @param xp 输入：多项式系数数组（k 个 n 长段 + 1 个 hn 长尾段）
 * @param n 输入：每段完整系数的 limb 长度
 * @param hn 输入：最后一段系数的实际长度
 * @param shift 输入：2 的幂指数（1<=shift，且 k*shift < LIMB_BITS）
 * @param tp 临时缓存空间（n+1 limbs）
 * @warning 0<hn<=n, 3<=k, k*shift<LIMB_BITS, sep(xp2,[xp|tp]), sep(xm2,[xp|tp])
 * @return 符号位：0=正，~0=负（对应 P(-2^shift)）
 */
int lmmp_toom_eval_pm2exp_(mp_ptr xp2, mp_ptr xm2, unsigned k, mp_srcptr xp, mp_size_t n, mp_size_t hn, unsigned shift, mp_ptr tp);

/**
 * @brief 通用高阶 Toom 求值：q次多项式在 x = +1/2^s 和 x = -1/2^s 处求值，
 *        结果放大 2^(s*q)（toom6h 的 ±1/2、±1/4 / toom8h 的 ±1/2、±1/4、±1/8 点）
 * @param rp 输出：2^(s*q)*P(+1/2^s) 的结果（n+1 limbs）
 * @param rm 输出：2^(s*q)*P(-1/2^s) 的结果（n+1 limbs）
 * @param q 输入：多项式次数（完整段数）
 * @param ap 输入：多项式系数数组（q 个 n 长段 + 1 个 t 长尾段）
 * @param n 输入：每段完整系数的 limb 长度
 * @param t 输入：最后一段系数的实际长度
 * @param s 输入：2 的幂指数（s!=0，且 q*s < LIMB_BITS）
 * @param ws 临时缓存空间（n+1 limbs）
 * @warning 0<t<=n, 1<q, sep(rp,[ap|ws]), sep(rm,[ap|ws])
 * @return 符号位：0=正，~0=负（对应 P(-1/2^s)）
 */
int lmmp_toom_eval_pm2rexp_(mp_ptr rp, mp_ptr rm, unsigned q, mp_srcptr ap, mp_size_t n, mp_size_t t, unsigned s, mp_ptr ws);

/**
 * @brief Toom ± 点对配对混合：一次完成和/差半边的右移缩放与重叠拼回
 *        （toom6h/8h 各评估点对的两个 (n+1) 位乘积的后处理）
 * 输入 {pp,n} 与 (nsign ? -{np,n} : {np,n})，计算：
 *   {np,n} <- ({pp,n}+{np,n})/2^(ns+1)（和半边，供奇系数插值）
 *   {pp,n} <- ({pp,n}-{np,n})/2^(ps+1)（差半边，供偶系数）
 * 最后 {pp,n+off} <- {pp,n} + {np,n}*B^off
 * @param pp 输入/输出：正点值乘积（容量 n+off+1 limbs）
 * @param n 有效数据长度
 * @param np 输入/输出：负点值乘积（容量 n limbs）
 * @param nsign np 的符号（非 0 表示 np 存的是负值）
 * @param off 拼回偏移（limb 数，0<=off<n）
 * @param ps 和半边附加右移位数
 * @param ns 差半边附加右移位数
 * @warning 0<=off<n, ps>=0, ns>=0, pp/np 数据低 ps+1/ns+1 位保证为 0
 */
void lmmp_toom_couple_handling_(mp_ptr pp, mp_size_t n, mp_ptr np, int nsign, mp_size_t off, int ps, int ns);

/**
 * @brief Toom插值计算（12点插值）：用于 Toom-6.5（toom6h/toom6 平方）乘法
 *        评估点：inf(仅 half), ±4, ±2, ±1, ±1/2, ±1/4, 0
 * @param dst 结果缓冲区（half ? 11n+spt : 10n+spt limbs）
 *            r8=f(0) 在 [dst,2n]，r6 在 [dst+3n,3n+1]，
 *            r4 在 [dst+7n,3n+1]，r0 在 [dst+11n,spt]
 * @param r1 输入/临时：±8...±4 点对混合后的奇系数（3n+1 limbs）
 * @param r3 输入/临时：±1 点对（3n+1 limbs）
 * @param r5 输入/临时：±1/2 点对（3n+1 limbs）
 * @param n 每段标准 limb 长度
 * @param spt 顶部系数 r0 的长度（0<spt<=2n）
 * @param half 非零表示乘积多项式次数为奇（需要 inf 点，r0 有效）
 * @param tp 临时缓存（3n+1 limbs）
 * @note r1=f(±4), r2=f(±2), r3=f(±1), r4=f(±1/4), r5=f(±1/2)；
 *       各 ± 点对须先经 lmmp_toom_couple_handling_ 混合；负中间值以
 *       补码存放，输入被破坏
 * @warning n>0, 0<spt<=2n, sep(tp,[dst|r1|r3|r5])
 */
void lmmp_toom_interp12_(mp_ptr dst, mp_ptr r1, mp_ptr r3, mp_ptr r5, mp_size_t n, mp_size_t spt, int half, mp_ptr tp);

/**
 * @brief Toom插值计算（16点插值）：用于 Toom-8.5（toom8h/toom8 平方）乘法
 *        评估点：inf(仅 half), ±8, ±4, ±2, ±1, ±1/2, ±1/4, ±1/8, 0
 * @param dst 结果缓冲区（half ? 15n+spt : 14n+spt limbs）
 *            r8=f(0) 在 [dst,2n]，r6 在 [dst+3n,3n+1]，r4 在 [dst+7n,3n+1]，
 *            r2 在 [dst+11n,3n+1]，r0 在 [dst+15n,spt]
 * @param r1 输入/临时：±8 点对混合后的奇系数（3n+1 limbs）
 * @param r3 输入/临时：±2 点对（3n+1 limbs）
 * @param r5 输入/临时：±1/4 点对（3n+1 limbs）
 * @param r7 输入/临时：±1/8 点对（3n+1 limbs）
 * @param n 每段标准 limb 长度
 * @param spt 顶部系数 r0 的长度（0<spt<=2n）
 * @param half 非零表示乘积多项式次数为奇（需要 inf 点，r0 有效）
 * @param tp 临时缓存（3n+1 limbs）
 * @note r1=f(±8), r2=f(±4), r3=f(±2), r4=f(±1), r5=f(±1/4), r6=f(±1/2),
 *       r7=f(±1/8)；各 ± 点对须先经 lmmp_toom_couple_handling_ 混合；
 *       负中间值以补码存放，输入被破坏
 * @warning n>0, 0<spt<=2n, sep(tp,[dst|r1|r3|r5|r7])
 */
void lmmp_toom_interp16_(mp_ptr dst, mp_ptr r1, mp_ptr r3, mp_ptr r5, mp_ptr r7, mp_size_t n, mp_size_t spt, int half, mp_ptr tp);

#endif // __LMMP_TOOM_INTERP_H__
