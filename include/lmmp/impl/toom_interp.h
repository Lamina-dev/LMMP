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
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
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
 * @param v2 v(2)插值点值，长度为 2n+1
 * @param vm1 v(-1)插值点值，长度为 2n+1
 * @param n 操作数的 limb 长度
 * @param spt 系数r4的长度
 * @param vm1_neg 符号标记：v(-1)是否为负数（1表示负，0表示正）
 * @param vinf0 无穷远点插值的低64位值
 */
void lmmp_toom_interp5_(mp_ptr dst, mp_ptr v2, mp_ptr vm1, mp_size_t n, mp_size_t spt, int vm1_neg, mp_limb_t vinf0);

/**
 * @brief Toom插值计算（6点插值）：用于Toom-43和Toom-52 乘法算法
 * @param dst    输出：最终乘积结果缓冲区（5n + w0n limbs）
 *               w5 储存在[dst,2n], w3 储存在[dst+2n,2n+1], w0 储存在[dst+5n,w0n].
 * @param n      输入：Toom-6 拆分后每段标准 limb 长度
 * @param flags  输入：Toom-6 插值符号标志位（控制正负号运算）
 *               - toom6_vm2_neg: 对应 x=-2 点值符号
 *               - toom6_vm1_neg: 对应 x=-1 点值符号
 * @param w4     输入/临时：点值 W4 缓冲区（2n+1 limbs）
 * @param w2     输入/临时：点值 W2 缓冲区（2n+1 limbs）
 * @param w1     输入/临时：点值 W1 缓冲区（2n+1 limbs）
 * @param w0n    输入：最低位段 W0 的实际 limb 长度（0 < w0n <= 2n）
 * @note         w5=f(0), w4=f(-1), w3=f(1), w2=f(-2), w1=f(2), w0=f(inf)
 * @warning      n>0, 0<w0n<=2n
 */
void lmmp_toom_interp6_(mp_ptr dst, mp_size_t n, enum toom6_flags flags, mp_ptr w4, mp_ptr w2, mp_ptr w1, mp_size_t w0n);

/**
 * @brief Toom插值计算（7点插值）：用于Toom-44、Toom-53、Toom-62 乘法算法
 * @param dst    输出结果缓冲区，存储插值计算结果（6*n + w6n limbs）
 *               w0 储存在[dst,2n], w2 储存在[dst+2n,2n+1], w6 储存在[dst+6n,w6n].
 * @param n      输入：Toom-7 拆分后每段标准 limb 长度
 * @param flags  输入：Toom-7 符号标志位，控制插值中的正负号运算
 *               - toom7_w1_neg: 点值 W1 符号位
 *               - toom7_w3_neg: 点值 W3 符号位
 * @param w1     输入/临时：点值 W1 缓冲区（2n+1 limbs）
 * @param w3     输入/临时：点值 W3 缓冲区（2n+1 limbs）
 * @param w4     输入/临时：点值 W4 缓冲区（2n+1 limbs）
 * @param w5     输入/临时：点值 W5 缓冲区（2n+1 limbs）
 * @param w6n    输入：最低位段 W6 的实际 limb 长度 (0 < w6n ≤ 2n)
 * @param tp     临时缓存空间（2*n+1 limbs）
 * @note         w0=f(0), w1=f(-2), w2=f(1), w3=f(-1), w4=f(2), w5=64*f(1/2), w6=f(inf),
 * @warning      n>0, 0<w6n<=2n
 */
void lmmp_toom_interp7_(mp_ptr dst, mp_size_t n, enum toom7_flags flags, mp_ptr w1, mp_ptr w3, mp_ptr w4, mp_ptr w5,
                        mp_size_t w6n, mp_ptr tp);

/**
 * @brief Toom-3 专用：3次多项式在 x = +1 和 x = -1 处求值
 * 计算 P(+1) 和 P(-1)，其中 P(x) 是一个3次多项式（4段系数）。
 * @param xp1  输出：P(+1) 的结果（n+1 个 limbs 空间）
 * @param xm1  输出：P(-1) 的结果（n+1 个 limbs 空间）
 * @param xp   输入：多项式系数数组（共4段，每段 n limbs）
 * @param n    输入：每段完整系数的 limb 长度
 * @param x3n  输入：最后一段系数的实际长度（通常等于 n）
 * @param tp   临时缓存空间（至少 n+1 limbs）
 * @warning    0<x3n<=n
 * @return     符号位：0=正，~0=负（对应 P(-1)）
 */
int lmmp_toom_eval_dgr3_pm1_(mp_ptr xp1, mp_ptr xm1, mp_srcptr xp, mp_size_t n, mp_size_t x3n, mp_ptr tp);

/**
 * @brief Toom-3 专用：3次多项式在 x = +2 和 x = -2 处求值
 * 计算 P(+2) 和 P(-2)，其中 P(x) 是一个3次多项式（4段系数）。
 * @param xp2  输出：P(+2) 的结果（n+1 个 limbs 空间）
 * @param xm2  输出：P(-2) 的结果（n+1 个 limbs 空间）
 * @param xp   输入：多项式系数数组（共4段，每段 n limbs）
 * @param n    输入：每段完整系数的 limb 长度
 * @param x3n  输入：最后一段系数的实际长度
 * @param tp   临时缓存空间（至少 n+1 limbs）
 * @warning    0<x3n<=n
 * @return     符号位：0=正，~0=负（对应 P(-2)）
 */
int lmmp_toom_eval_dgr3_pm2_(mp_ptr xp2, mp_ptr xm2, mp_srcptr xp, mp_size_t n, mp_size_t x3n, mp_ptr tp);

/**
 * @brief 通用高阶 Toom 求值：k次多项式在 x = +1 和 x = -1 处求值
 * @param xp1  输出：P(+1) 的结果（n+1 limbs）
 * @param xm1  输出：P(-1) 的结果（n+1 limbs）
 * @param k    输入：多项式次数（也是完整段的数量）
 * @param xp   输入：多项式系数数组
 * @param n    输入：每段完整系数的 limb 长度
 * @param hn   输入：最后一段系数的实际长度
 * @param tp   临时缓存空间（n+1 limbs）
 * @warning    0<hn<=n, 3 < k
 * @return     符号位：0=正，~0=负
 */
int lmmp_toom_eval_pm1_(mp_ptr xp1, mp_ptr xm1, unsigned k, mp_srcptr xp, mp_size_t n, mp_size_t hn, mp_ptr tp);

/**
 * @brief 通用高阶 Toom 求值：k次多项式在 x = +2 和 x = -2 处求值
 * @param xp2  输出：P(+2) 的结果（n+1 limbs）
 * @param xm2  输出：P(-2) 的结果（n+1 limbs）
 * @param k    输入：多项式次数
 * @param xp   输入：多项式系数数组
 * @param n    输入：每段完整系数的 limb 长度
 * @param hn   输入：最后一段系数的实际长度
 * @param tp   临时缓存空间（n+1 limbs）
 * @warning    0<hn<=n, 3 < k < LIMB_BITS
 * @return     符号位：0=正，~0=负
 */
int lmmp_toom_eval_pm2_(mp_ptr xp2, mp_ptr xm2, unsigned k, mp_srcptr xp, mp_size_t n, mp_size_t hn, mp_ptr tp);

#endif // __LMMP_TOOM_INTERP_H__