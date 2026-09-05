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

#ifndef __LMMP_ELE_MUL_H__
#define __LMMP_ELE_MUL_H__

#include "../lmmpn.h"
#include "../numth.h"
#include "tmp_alloc.h"


typedef struct {
    mp_srcptr np;
    mp_size_t nn;
    sint left;    // 左节点索引
    sint right;   // 右节点索引
} huff_node;

typedef struct {
    huff_node* root;
    sint cap;
    sint size;
} huff_tree;


/**
 * @brief 初始化哈夫曼乘法树
 * @param ht 哈夫曼乘法树指针
 * @param cap 需要插入的元素的最大数量
 * @warning 0<cap<2^30, ht!=NULL
 */
static inline void lmmp_huff_tree_init_(huff_tree* ht, sint cap) {
    lmmp_param_assert(cap > 0);
    lmmp_param_assert(cap < (1 << 30));
    lmmp_param_assert(ht != NULL);
    ht->root = ALLOC_TYPE(2 * cap - 1, huff_node);
    ht->cap = cap;
    ht->size = 0;
}

/**
 * @brief 释放哈夫曼树
 * @param ht 哈夫曼树指针
 * @warning ht->size==0, ht!=NULL
 */
static inline void lmmp_huff_tree_free_(huff_tree* ht) {
    lmmp_param_assert(ht != NULL);
    lmmp_free(ht->root);
    ht->cap = 0;
    ht->size = 0;
    ht->root = NULL;
}

/**
 * @brief 入队哈夫曼树
 * @param ht 哈夫曼树指针
 * @param np 待入队的元素指针
 * @param nn 元素的 limb 长度
 * @warning ht!=NULL, ht->size<ht->cap, np!=NULL, nn>0
 * @note 传入乘法元素必须为非空元素，可以和其他乘法元素指向完全相同
 */
static inline void lmmp_huff_tree_push_(huff_tree* ht, mp_srcptr np, mp_size_t nn) {
    lmmp_param_assert(ht != NULL);
    lmmp_param_assert(ht->size < ht->cap);
    lmmp_param_assert(np != NULL);
    lmmp_param_assert(nn > 0);
    ht->root[ht->size].np = np;
    ht->root[ht->size].nn = nn;
    ht->root[ht->size].left = -1;
    ht->root[ht->size].right = -1;
    ht->size++;
}

/**
 * @brief 构建哈夫曼乘法树
 * @param ht 哈夫曼乘法树指针
 * @warning ht!=NULL
 * @return 构建后的根节点索引
 */
sint lmmp_huff_tree_build_(huff_tree* ht);

/**
 * @brief 将哈夫曼树中所有元素相乘
 * @param ht 哈夫曼乘法树指针
 * @param ridx 哈夫曼乘法树根节点索引
 * @param dst 结果指针（缓冲区长度为根节点的nn值）
 * @param tp 临时指针（缓冲区长度为根节点的nn值）
 * @warning pq!=NULL, dst!=NULL, tp!=NULL, sep(dst,tp)
 * @return 结果指针的 limb 长度
 */
mp_size_t lmmp_huff_tree_mul_(huff_tree* ht, sint ridx, mp_ptr dst, mp_ptr tp);

/**
 * @brief 计算limbs数组的累乘积
 * @param dst 结果指针（长度为 n 个 limb）
 * @param limbs 数组指针
 * @param n limbs数组长度
 * @param tp 临时指针（长度为 n 个 limb）
 * @warning dst!=NULL, limbs!=NULL, n>0, tp!=NULL
 * @return 结果指针的 limb 长度
 * @note 建议将limbs数组的元素累计至较大，可以减少空间占用和乘法次数
 */
mp_size_t lmmp_elem_mul_ulong_(mp_ptr dst, const ulongp limbs, mp_size_t n, mp_ptr tp);

typedef struct fac_t {
    uint f; // factor
    uint j; // exp
} fac_t;

typedef fac_t* fac_ptr;

/**
 * @brief 计算因子的累乘，并将结果放入dst中
 * @param dst 结果数组
 * @param rn 结果数组的长度
 * @param fac 因子数组（将会被递归覆盖）
 * @param nfactors 因子数组的长度
 * @warning 因子必须要单调递增，可以接受重复因子（但不推荐这样做），因子的贡献必须要大于0。
 *          已假定进入朴素乘法时，因子数组元素都小于等于0xffff，同时假定了，因子数组为小因子大指数形式，
 *          可以存在大的因子有略大的指数，但整体的趋势应是小因子大指数。
 *          在组合数以及由阶乘和幂次构成的有理数中，暂未见不满足这些假设的例子。
 * @return 结果数组的长度
 */
mp_size_t lmmp_factors_mul_(mp_ptr dst, mp_size_t rn, fac_ptr fac, uint nfactors);

/**
 * @brief 计算因子的累乘，并将结果放入dst中
 * @param dst 结果数组
 * @param rn 结果数组的长度
 * @param fac 因子数组（将会被递归覆盖），且因子数组元素都不可超过ushort范围
 * @param nfactors 因子数组的长度
 * @warning 因子必须要单调递增，且因子数组元素都不可超过ushort范围，可以接受重复因子（但不推荐这样做），
 *          已假定了，因子数组为小因子大指数形式，可以存在大的因子有略大的指数，但整体的趋势应是小因子大指数。
 *          在组合数以及由阶乘和幂次构成的有理数中，暂未见不满足这些假设的例子。
 * @return 结果数组的长度
 */
mp_size_t lmmp_factors_mul_ushort_(mp_ptr dst, mp_size_t rn, fac_ptr fac, ushort nfactors);

#endif // __LMMP_ELE_MUL_H__