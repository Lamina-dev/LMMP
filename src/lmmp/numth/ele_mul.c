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

#include "../../../include/lmmp/impl/ele_mul.h"
#include "../../../include/lmmp/impl/mparam.h"

#define INSERTION_SORT_THRESHOLD 16

static inline void swap_huff_node(huff_node* a, huff_node* b) {
    mp_srcptr tp = a->np;
    mp_size_t tn = a->nn;
    a->np = b->np;
    a->nn = b->nn;
    b->np = tp;
    b->nn = tn;
}

/**
 * @brief 在闭区间 [low, high] 上进行插入排序
 */
static void insertion_sort(huff_node arr[], sint low, sint high) {
    for (sint i = low + 1; i <= high; ++i) {
        mp_srcptr np = arr[i].np;
        mp_size_t nn = arr[i].nn;
        sint j = i - 1;
        while (j >= low && arr[j].nn > nn) {
            arr[j + 1].np = arr[j].np;
            arr[j + 1].nn = arr[j].nn;
            --j;
        }
        arr[j + 1].np = np;
        arr[j + 1].nn = nn;
    }
}

/** 
 * @brief 返回 arr[a]、arr[b]、arr[c] 三个索引中对应值的中位数的索引
 */
static inline sint median_of_three(const huff_node arr[], sint a, sint b, sint c) {
    mp_size_t va = arr[a].nn, vb = arr[b].nn, vc = arr[c].nn;
    if (va < vb) {
        if (vb < vc)
            return b;
        else if (va < vc)
            return c;
        else
            return a;
    } else {
        if (va < vc)
            return a;
        else if (vb < vc)
            return c;
        else
            return b;
    }
}

/**
 * @brief 递归的三路快速排序，对闭区间 [low, high] 排序
 */
static void quicksort_rec(huff_node arr[], sint low, sint high) {
    if (high - low + 1 <= INSERTION_SORT_THRESHOLD) {
        insertion_sort(arr, low, high);
        return;
    }

    sint mid = low + (high - low) / 2;
    sint pivot_index = median_of_three(arr, low, mid, high);
    if (pivot_index != low) {
        swap_huff_node(&arr[low], &arr[pivot_index]);
    }
    mp_size_t pivot = arr[low].nn;

    sint lt = low;  // 小于区域的右边界（开）
    sint gt = high; // 大于区域的左边界（开）
    sint i = low + 1;

    while (i <= gt) {
        if (arr[i].nn < pivot) {
            swap_huff_node(&arr[lt], &arr[i]);
            ++lt;
            ++i;
        } else if (arr[i].nn > pivot) {
            swap_huff_node(&arr[i], &arr[gt]);
            --gt;
        } else {
            ++i;
        }
    }

    quicksort_rec(arr, low, lt - 1);
    quicksort_rec(arr, gt + 1, high);
}

sint lmmp_huff_tree_build_(huff_tree* restrict ht) {
    lmmp_param_assert(ht != NULL);
    lmmp_param_assert(ht->root != NULL && ht->size > 0);
    sint n = ht->size;
    huff_node* arr = ht->root;

    if (n == 1) {
        return 0;
    }

    quicksort_rec(arr, 0, n - 1);

    sint i = 0, j = n, k = n;

    while (k < (2 * n - 1)) {
        sint idx1, idx2;

        if (i < n && (j >= k || arr[i].nn <= arr[j].nn)) {
            idx1 = i++;
        } else {
            idx1 = j++;
        }

        if (i < n && (j >= k || arr[i].nn <= arr[j].nn)) {
            idx2 = i++;
        } else {
            idx2 = j++;
        }
        // 非叶子节点的np不会被访问，此赋值不会影响结果
        //arr[k].np = NULL;
        arr[k].nn = arr[idx1].nn + arr[idx2].nn;
        arr[k].left = idx1;
        arr[k].right = idx2;
        k++;
    }

    return 2 * n - 2;
}

mp_size_t lmmp_huff_tree_mul_(huff_tree* restrict ht, sint ridx, mp_ptr restrict dst, mp_ptr restrict tp) {
    if (ht->root[ridx].left == -1) {
        lmmp_copy(dst, ht->root[ridx].np, ht->root[ridx].nn);
        return ht->root[ridx].nn;
    } else {
        sint idx = ht->root[ridx].left;
        mp_size_t halfn = ht->root[idx].nn; // 左子树的limb缓冲区长度
        mp_size_t n1 = lmmp_huff_tree_mul_(ht, ht->root[ridx].left, tp, dst);
        mp_size_t n2 = lmmp_huff_tree_mul_(ht, ht->root[ridx].right, tp + halfn, dst + halfn);
        if (n1 > n2)
            lmmp_mul_(dst, tp, n1, tp + halfn, n2);
        else
            lmmp_mul_(dst, tp + halfn, n2, tp, n1);
        n1 += n2;
        n1 -= dst[n1 - 1] == 0 ? 1 : 0;
        return n1;
    }
}

mp_size_t lmmp_elem_mul_ulong_(mp_ptr restrict dst, const ulongp restrict limbs, mp_size_t n, mp_ptr restrict tp) {
    if (n < ELEM_MUL_BASECASE_THRESHOLD) {
        lmmp_param_assert(n > 0);
        dst[0] = limbs[0];
        mp_size_t rn = 1;
        for (mp_size_t i = 1; i < n; i++) {
            dst[rn] = lmmp_mul_1_(dst, dst, rn, limbs[i]);
            rn++;
            rn -= dst[rn - 1] == 0 ? 1 : 0;
        }
        return rn;
    }
    mp_size_t halfn = n / 2;
    mp_size_t n1 = lmmp_elem_mul_ulong_(tp, limbs, halfn, dst);
    mp_size_t n2 = lmmp_elem_mul_ulong_(tp + halfn, limbs + halfn, n - halfn, dst + halfn);
    if (n1 > n2)
        lmmp_mul_(dst, tp, n1, tp + halfn, n2);
    else
        lmmp_mul_(dst, tp + halfn, n2, tp, n1);
    n = n1 + n2;
    n -= dst[n - 1] == 0 ? 1 : 0;
    return n;
}
