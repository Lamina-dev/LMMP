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

#ifndef __LMMP_IMPL_MAT22_MUL_H__
#define __LMMP_IMPL_MAT22_MUL_H__

#include "../lmmp.h"


#ifndef INLINE_
#define INLINE_ static inline
#endif

typedef struct {
    mp_ptr a00;
    mp_ptr a01;
    mp_ptr a10;
    mp_ptr a11;
    mp_ssize_t n00;
    mp_ssize_t n01;
    mp_ssize_t n10;
    mp_ssize_t n11;
} lmmp_mat22_t;

/**
 * @brief 计算2x2矩阵和2x2矩阵的乘积需要分配的内存
 * @param dst 结果矩阵，dst中的n将会被覆盖为对应位置需要的limb长度，此函数不分配内存。
 * @param matA 矩阵A
 * @param matB 矩阵B
 * @param tn 输出参数，将会被覆盖为缓冲区需要的limb长度，正数
 * @param maxa 如果被覆盖，即matA中最大的元素的limb长度+1，此参数只有当确认使用STRASSEN算法时才需要
 * @warning dst!=NULL, [matA|matB]!=NULL, nonull([matA|matB]), sep(dst,[matA|matB]), tn!=NULL, maxa!=NULL
 * @note 如果你可以确认一定不使用STRASSEN算法，则不需要maxa参数，其可以为NULL。
 * @return 0表示选择basecase算法，1表示选择STRASSEN算法。
 */
int lmmp_mat22_mul_size_(lmmp_mat22_t* dst, const lmmp_mat22_t* matA, const lmmp_mat22_t* matB, mp_size_t* tn,
                         mp_size_t* maxa);

/**
 * @brief 计算2x2矩阵和2x2矩阵的乘积
 * @param dst 结果矩阵。
 * @param matA 矩阵A
 * @param matB 矩阵B
 * @param tp 临时缓冲区，用于存储中间结果，需要分配2*tn个limb，若为NULL，则会自动分配。
 * @param tn 缓冲区的limb长度
 * @warning dst!=NULL, nonull(dst), [matA|matB]!=NULL, nonull([matA|matB]), sep(dst,[matA|matB]), tn>0
 */
void lmmp_mat22_mul_basecase_(lmmp_mat22_t* dst, const lmmp_mat22_t* matA, const lmmp_mat22_t* matB, mp_ptr tp,
                              mp_size_t tn);

/**
 * @brief 计算2x2矩阵平方
 * @param dst 结果矩阵。
 * @param matA 矩阵A
 * @param tp 临时缓冲区，用于存储中间结果，需要分配2*tn个limb，若为NULL，则会自动分配。
 * @param tn 缓冲区的limb长度
 * @warning dst!=NULL, nonull(dst), matA!=NULL, nonull(matA), sep(dst,matA), tn>0
 */
void lmmp_mat22_sqr_basecase_(lmmp_mat22_t* dst, const lmmp_mat22_t* matA, mp_ptr tp, mp_size_t tn);

/**
 * @brief 计算（稠密）2x2矩阵和（稠密）2x2矩阵的乘积（STRASSEN算法）
 * @param dst 结果矩阵。
 * @param matA 矩阵A
 * @param matB 矩阵B
 * @param tp 临时缓冲区，用于存储中间结果，需要分配7*(tn+1)个limb，若为NULL，则会自动分配。
 * @param tn 缓冲区的limb长度
 * @param maxa matA中最大的元素的limb长度+1，建议由lmmp_mat22_mul_size_确定
 * @warning dst!=NULL, nonull(dst), [matA|matB]!=NULL, nonull([matA|matB]), sep(dst,[matA|matB]), tn>0
 */
void lmmp_mat22_mul_strassen_(lmmp_mat22_t* dst, const lmmp_mat22_t* matA, const lmmp_mat22_t* matB, mp_ptr tp,
                              mp_size_t tn, mp_size_t maxa);

/**
 * @brief 计算（稠密）2x2矩阵平方（STRASSEN算法）
 * @param dst 结果矩阵。
 * @param matA 矩阵A
 * @param tp 临时缓冲区，用于存储中间结果，需要分配7*(tn+1)个limb，若为NULL，则会自动分配。
 * @param tn 缓冲区的limb长度
 * @warning dst!=NULL, nonull(dst), matA!=NULL, nonull(matA), sep(dst,matA), tn>0
 */
void lmmp_mat22_sqr_strassen_(lmmp_mat22_t* dst, const lmmp_mat22_t* matA, mp_ptr tp, mp_size_t tn);

/**
 * @brief 计算2x2矩阵和2x2矩阵的乘积
 * @param dst 结果矩阵。
 * @param matA 矩阵A
 * @param matB 矩阵B
 * @param choose 选择算法，0表示basecase算法，1表示STRASSEN算法
 * @param tn 缓冲区的limb长度，建议由lmmp_mat22_mul_size_确定
 * @param maxa matA中最大的元素的limb长度+1，建议由lmmp_mat22_mul_size_确定
 * @warning dst!=NULL, nonull(dst), [matA|matB]!=NULL, nonull([matA|matB]), sep(dst,[matA|matB]), choose==[0|1]
 */
INLINE_ void 
lmmp_mat22_mul_(lmmp_mat22_t* dst, const lmmp_mat22_t* matA, const lmmp_mat22_t* matB, int choose, mp_size_t tn,
                mp_size_t maxa) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(matA != NULL);
    lmmp_param_assert(matB != NULL);
    lmmp_param_assert(choose == 0 || choose == 1);
    if (choose == 0) {
        lmmp_mat22_mul_basecase_(dst, matA, matB, NULL, tn);
    } else {
        lmmp_mat22_mul_strassen_(dst, matA, matB, NULL, tn, maxa);
    }
}

/**
 * @brief 计算（稠密）2x2矩阵平方
 * @param dst 结果矩阵。
 * @param matA 矩阵A
 * @param tn 缓冲区的limb长度，建议由lmmp_mat22_mul_size_确定
 * @param choose 选择算法，0表示basecase算法，1表示STRASSEN算法
 * @warning dst!=NULL, nonull(dst), matA!=NULL, nonull(matA), sep(dst,matA), tn>0
 */
INLINE_ void 
lmmp_mat22_sqr_(lmmp_mat22_t* dst, const lmmp_mat22_t* mat, int choose, mp_size_t tn) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(mat != NULL);
    lmmp_param_assert(tn > 0);
    lmmp_param_assert(choose == 0 || choose == 1);
    if (choose == 0) {
        lmmp_mat22_sqr_basecase_(dst, mat, NULL, tn);
    } else {
        lmmp_mat22_sqr_strassen_(dst, mat, NULL, tn);
    }
}

#ifdef INLINE_
#undef INLINE_
#endif

#endif // __LMMP_IMPL_MAT22_MUL_H__