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

#ifndef __LMMP_IMPL_MAT22_MUL_H__
#define __LMMP_IMPL_MAT22_MUL_H__

#include "../lmmpn.h"

/*
    2x2 非负矩阵（hgcd 变换矩阵的通用载体）。

    约定：显式存储每个元素的真实长度 n[i][j]（归一化，顶 limb 非零；
    n[i][j]==0 表示该元素值为零），不做零填充——各元素缓冲 [0, n[i][j])
    之外的内容无效，一切访问以长度为准。输入矩阵的长度由调用方保证已
    归一化（本模块按此契约直接使用，仅以调试断言核查顶 limb）。

    约定：mx(A) 即 A 矩阵中，最大的n[i][j]，也即四个指针中，最长的元素的
    limb 长度，mx(A,B) 指 A和B矩阵中，最大的n[i][j]

    本布局与 lmmp_hgcd_matrix_t（numth.h）公共前缀同构（m[2][2] +
    n[2][2]，后者多一个 alloc 字段），hgcd 侧可经指针强转无缝复用，
    无转换开销。
*/

typedef struct {
    mp_ptr p[2][2];   /* 元素指针 */
    mp_size_t n[2][2]; /* 元素真实长度（归一化；0 表示零元素） */
} lmmp_mat22_t;

/**
 * @brief 计算 D <- A*B（2x2 非负矩阵乘法）
 * @param dst 结果矩阵，各个元素结果写入 dst->p 中的数组，对应实际长度被写入到 dst->n 中
 * @param a 左矩阵（元素长度为 a->n[i][j]，可与 dst 别名，即支持 D <- D*B）
 * @param b 右矩阵（元素长度为 b->n[i][j]，不可与 dst 别名）
 * @param tp 临时空间（8*(mx(a)+mx(b))+16 个limb）
 * @warning dst!=NULL, a!=NULL, b!=NULL, tp!=NULL, sep(dst,b), eqsep(dst,a),
 *          dst 各元素缓冲容量至少为 mx(a)+mx(b)+4
 */
void lmmp_mat22_mul_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, const lmmp_mat22_t* b, mp_ptr tp);

/**
 * @brief 计算 D <- A*A（2x2 非负矩阵平方）
 * @param dst 结果矩阵，各个元素结果写入 dst->p 中的数组，对应实际长度被写入到 dst->n 中
 * @param a 源矩阵（元素长度为 a->n[i][j]，可与 dst 别名）
 * @param tp 临时空间（15*mx(a)+14 个limb）
 * @warning dst!=NULL, a!=NULL, tp!=NULL, eqsep(dst,a),
 *          dst 元素缓冲容量 >= 2*mx(a)+4
 */
void lmmp_mat22_sqr_(lmmp_mat22_t* dst, const lmmp_mat22_t* a, mp_ptr tp);

#endif // __LMMP_IMPL_MAT22_MUL_H__
