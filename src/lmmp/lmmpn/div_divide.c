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

#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


/**
 * @brief 分治除法
 * @param dstq 结果商指针（[dstq,n]=[numa,2*n] div [numb,n]）
 * @param numa 被除数指针（[numa,n]=[numa,2*n] mod [numb,n]）
 * @param numb 除数指针
 * @param n 除数长度
 * @param inv21 除数高128位的逆
 * @param tp 临时工作区指针（需要 n 个 limb 的空间）
 * @warning n>=6, MSB(numb)=1, inv21=(2^192-1)/[numb+n-2,2]-2^64, sep(dstq,numa,numb,tp)
 * @return 除法结果最高位商
 */
static mp_limb_t lmmp_div_divide_n_(
    mp_ptr    restrict dstq,
    mp_ptr    restrict numa,
    mp_srcptr restrict numb,
    mp_size_t             n,
    mp_limb_t         inv21,
    mp_ptr    restrict   tp
) {
    lmmp_param_assert(n >= 6);
    lmmp_param_assert(dstq != NULL && numa != NULL && numb != NULL);
    lmmp_param_assert(numb[n - 1] >= LIMB_B_2);
    mp_size_t lo = n >> 1, hi = n - lo;
    mp_limb_t cy, qh, ql;

    if (hi < DIV_DIVIDE_THRESHOLD) {
        qh = lmmp_div_basecase_(dstq + lo, numa + 2 * lo, 2 * hi, numb + lo, hi, inv21);
    } else {
        qh = lmmp_div_divide_n_(dstq + lo, numa + 2 * lo, numb + lo, hi, inv21, tp);
    }
    lmmp_mul_(tp, dstq + lo, hi, numb, lo);

    cy = lmmp_sub_n_(numa + lo, numa + lo, tp, n);
    if (qh)
        cy += lmmp_sub_n_(numa + n, numa + n, numb, lo);

    while (cy) {
        qh -= lmmp_sub_1_(dstq + lo, dstq + lo, hi, 1);
        cy -= lmmp_add_n_(numa + lo, numa + lo, numb, n);
    }

    if (lo < DIV_DIVIDE_THRESHOLD) {
        ql = lmmp_div_basecase_(dstq, numa + hi, 2 * lo, numb + hi, lo, inv21);
    } else {
        ql = lmmp_div_divide_n_(dstq, numa + hi, numb + hi, lo, inv21, tp);
    }
    lmmp_mul_(tp, numb, hi, dstq, lo);

    cy = lmmp_sub_n_(numa, numa, tp, n);
    if (ql)
        cy += lmmp_sub_n_(numa + lo, numa + lo, numb, hi);

    while (cy) {
        lmmp_sub_1_(dstq, dstq, lo, 1);
        cy -= lmmp_add_n_(numa, numa, numb, n);
    }
    return qh;
}

mp_limb_t lmmp_div_divide_(
    mp_ptr    restrict dstq,
    mp_ptr    restrict numa,
    mp_size_t            na,
    mp_srcptr restrict numb,
    mp_size_t            nb,
    mp_limb_t         inv21
) {
    lmmp_param_assert(na >= 2 * nb);
    lmmp_param_assert(nb >= 6);
    lmmp_param_assert(numb[nb - 1] >= LIMB_B_2);
    mp_size_t nq = na - nb;

    dstq += nq;
    numa += nq;

    do {
        nq -= nb;
    } while (nq >= nb);

    dstq -= nq;
    numa -= nq;

    /* Perform the typically smaller block first. */
    mp_limb_t qh = lmmp_div_s_(dstq, numa, nb + nq, numb, nb);

    TEMP_DECL;
    mp_ptr restrict tp = TALLOC_TYPE(nb, mp_limb_t);
    nq = na - nb - nq;

    do {
        dstq -= nb;
        numa -= nb;
        lmmp_div_divide_n_(dstq, numa, numb, nb, inv21, tp);
        nq -= nb;
    } while (nq > 0);

    TEMP_FREE;
    return qh;
}
