/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published by
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#ifndef __LMMP_IMPL_MAT22_MUL_H__
#define __LMMP_IMPL_MAT22_MUL_H__

#include "../lmmpn.h"

/*
2x2 非负矩阵（hgcd 变换矩阵的通用载体）。

约定：显式存储每个元素的真实长度 n[i][j]（归一化，顶 limb 非零；
n[i][j]==0 表示该元素值为零），不做零填充——各元素缓冲 [0, n[i][j])
之外的内容无效，一切访问以长度为准。输入矩阵的长度由调用方保证已
归一化（本模块按此契约直接使用，仅以调试断言核查顶 limb）。

长度维护纪律：乘积与和差的实际长度一律由计算过程显式推导——乘法的
na+nb limbs 顶 limb 单次判断、加法的进位、减法的大小比较与最高位，
不做逐 limb 的前导零扫描。

本布局与 lmmp_hgcd_matrix_t（numth.h）公共前缀同构（m[2][2] +
n[2][2]，后者多一个 alloc 字段），hgcd 侧可经指针强转无缝复用，
无转换开销。

历史注记：曾实现过"FFT 缓存乘法"路径（8 乘 + 短侧 4 元素经
lmmp_mul_fft_cache_ 系列复用前向变换，阈值 MAT22_MUL_FFT_THRESHOLD），
经理论核算与实测（i7-14 代，2000-16000 limb 元素）证伪：其变换量
24f+16i+16p 对 Strassen 的 28f+14i+14p 仅省 4 次前向变换，却多付第
8 次乘法的点积与逆变换；而 SSA 中前向变换占总成本的份额随规模衰减
（实测 34%@2000 -> 17%@16000），点积占比上升，故小尺寸打平、渐进
劣于 Strassen 且差距扩大。该路径已删除；真正有渐进收益的是频域
共享变换的 Strassen（~8 前向 + 4 逆向 + 7 点积，省约四成变换量），
留待后续。
*/
typedef struct {
    mp_ptr p[2][2];   /* 元素指针 */
    mp_size_t n[2][2]; /* 元素真实长度（归一化；0 表示零元素） */
} lmmp_mat22_t;

/**
 * @brief 计算 D <- A*B（2x2 非负矩阵乘法）
 * @param dst 结果矩阵：写入各元素 [0, dn[i][j]) 与 dn[i][j]
 *            （dn[i][j] <= a->n[i][k]+b->n[k][j]+1，k 为求和下标）
 * @param a 左矩阵（读各元素 [0,a->n[i][j])，长度已归一化；
 *          可与 dst 别名，即支持 D <- D*B）
 * @param b 右矩阵（读各元素 [0,b->n[i][j])，长度已归一化；不可与 dst 别名）
 * @param tp 临时空间（9*(2*mx+4) 个limb，mx 为 a/b 全部元素长度最大值）
 * @warning dst!=NULL, a!=NULL, b!=NULL, tp!=NULL, sep([dst|a|b],tp),
 *          dst 元素缓冲容量 >= 新长度（由调用方保证）
 * @note 实现分派：元素最大长度 < MAT22_MUL_STRASSEN_THRESHOLD 走
 *       basecase（8 次乘法），否则走 Winograd-Strassen（7 次乘法，
 *       中间量带符号，符号-绝对值表示）
 */
void lmmp_mat22_mul_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, const lmmp_mat22_t* b, mp_ptr tp);

/**
 * @brief 计算 D <- A*A（2x2 非负矩阵平方）
 * @param dst 结果矩阵：写入各元素 [0, dn[i][j]) 与 dn[i][j]
 * @param a 源矩阵（读各元素 [0,a->n[i][j])，长度已归一化；可与 dst 别名）
 * @param tp 临时空间（9*(2*mx+4) 个limb，mx 为 a 全部元素长度最大值）
 * @warning dst!=NULL, a!=NULL, tp!=NULL, sep([dst|a],tp),
 *          dst 元素缓冲容量 >= 新长度（由调用方保证）
 * @note 实现分派：元素最大长度 < MAT22_SQR_STRASSEN_THRESHOLD 走对称
 *       basecase（5 次乘法：a00^2、a11^2、a01*a10 与
 *       a01*(a00+a11)、a10*(a00+a11)），否则走 Strassen 平方
 *       （7 次乘法中 4 次为平方，利用 A*A 的组合量对称性 t_i = s_i）
 */
void lmmp_mat22_sqr_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, mp_ptr tp);

#endif // __LMMP_IMPL_MAT22_MUL_H__
