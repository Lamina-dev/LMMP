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

#include "../../../include/lammp/impl/inlines.h"
#include "../../../include/lammp/impl/mparam.h"
#include "../../../include/lammp/impl/tmp_alloc.h"
#include "../../../include/lammp/lmmpn.h"


/*
funtion: bninv
output:
    dstq := B^(2*(na+ni)) / ([numa,na] * B^ni)

           numa
             |
    |---ni---|------na-------|
    +--------+---------------+
    | 000000 | aaaaaaaaaaaaa |
    +--------+---------------+
    <---remn---->|<--a_hatn->|
                 |
               a_hat

    N = na + ni
    a_hatn = N / 2 + 1
    remn = N - a_hatn
    a_hat = numa + na - a_hatn

    if remn > ni:
        ni_hat = 0
    else
        ni_hat = ni - remn
    q_hat = bninv(a_hat, a_hatn, ni_hat)

    q_hatn = a_hatn + ni_hat + 1
           = N_half + 1

    qrn = N + 1 - q_hatn
        = N + 1 - (N_half + 1)
        = N - N_half

            q_hat
               |
    |---qrn----|--q_hatn--|
    +----------+----------+
    | 00000000 | qqqqqqqq |
    +----------+----------+
    <---------N+1--------->


    2*q_hatn + 2*qrn + an + ni = 2*(N+1) + N = 3*N + 2

                                q_hat_sqr_a
                                     |
    |---qrn---|---qrn---|-----ni-----|------2 * q_hatn + an-------|
    +---------+---------+------------+----------------------------+
    | 0000000 | 0000000 | 0000000000 | ssssssssssssssssssss |  0  |
    +---------+---------+----------+-+---------+------------+-----+
    |            delete            | 000000000 | qqqqqqqqqq |  ca |
    +-------------- 2*N -----------+----qrn----+-- q_hatn --+-----+
                        | 2*N_half |<------- N+1 ---------->|<-1->|
                                   |
                                  dstq
    and assert(ca == 0)
*/

/**
 * @brief 牛顿法求精确逆元（至多产生1的误差）
 * @note dstq := B^(2*(na+ni)) / ([numa,na] * B^ni) + [0|1]
 * @warning sep(dstq,numa), dstq!=NULL, numa!=NULL, na>=3, MSB(numa)=1
 */
static void lmmp_bninv_appr_newton_(mp_ptr restrict dstq, mp_srcptr restrict numa, mp_size_t na, mp_size_t ni) {
    lmmp_param_assert(na >= 3);
    lmmp_param_assert(dstq != NULL && numa != NULL);
    lmmp_param_assert(numa[na - 1] > LIMB_B_2);
    TEMP_DECL;
    if (na < BNINV_NEWTON_THRESHOLD) {
        mp_ptr restrict bnp = TALLOC_TYPE(2 * na + ni + 1, mp_limb_t);
        lmmp_zero(bnp, 2 * na + ni + 1);
        bnp[2 * na + ni] = 1;
        mp_limb_t inv21 = lmmp_inv_2_1_(numa[na - 1], numa[na - 2]);
        lmmp_div_basecase_(dstq, bnp, 2 * na + ni + 1, numa, na, inv21);
    } else {
        mp_srcptr restrict a_hat;

        mp_size_t N = na + ni;
        mp_size_t a_hatn = N / 2 + 1;
        mp_size_t remn = N - a_hatn;
        mp_size_t ni_hat;
        if (remn > ni) {
            ni_hat = 0;
            a_hat = numa + na - a_hatn;
        } else {
            ni_hat = ni - remn;
            a_hat = numa;
            a_hatn = na;
        }
        mp_size_t q_hatn = a_hatn + ni_hat + 1;
        mp_size_t qrn = N + 1 - q_hatn;
        mp_ptr restrict q_hat = dstq + qrn;

        mp_ptr restrict q_hat_sqr = TALLOC_TYPE(2 * q_hatn, mp_limb_t);
        mp_ptr restrict q_hat_sqr_a = TALLOC_TYPE(2 * q_hatn + na, mp_limb_t);

        lmmp_bninv_appr_newton_(q_hat, a_hat, a_hatn, ni_hat);
        lmmp_sqr_(q_hat_sqr, q_hat, q_hatn);
        lmmp_mul_(q_hat_sqr_a, q_hat_sqr, 2 * q_hatn, numa, na);
        // we can assert q_hat_sqr_a[2*q_hatn+na-1] == 0
        lmmp_zero(dstq, qrn);
        if (2 * qrn + ni > 2 * N) {
            mp_size_t start = 2 * qrn + ni - 2 * N;
            lmmp_shl_(q_hat, q_hat, q_hatn, 1);  // assert no carry
            lmmp_sub_n_(dstq + start, dstq + start, q_hat_sqr_a, N + 1 - start);
        } else {
            mp_size_t start = 2 * N - 2 * qrn - ni;
            lmmp_shl_(q_hat, q_hat, q_hatn, 1);  // assert no carry
            lmmp_sub_n_(dstq, dstq, q_hat_sqr_a + start, N + 1);
        }
    }
    TEMP_FREE;
}

void lmmp_bninv_(mp_ptr restrict dstq, mp_srcptr restrict numa, mp_size_t na, mp_size_t ni) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(dstq != NULL && numa != NULL);
    lmmp_param_assert(numa[na - 1] > 0);
    TEMP_DECL;
    if (na == 1) {
        lmmp_zero(dstq, 2 + ni);
        dstq[2 + ni] = 1;
        lmmp_div_1_(dstq, dstq, 3 + ni, numa[0]);
    } else if (na == 2) {
        mp_size_t bn = 2 * 2 + ni + 1;
        mp_ptr restrict bnp = TALLOC_TYPE(bn, mp_limb_t);
        lmmp_zero(bnp, bn - 1);
        bnp[bn - 1] = 1;
        mp_limb_t d[2] = {numa[0], numa[1]};
        lmmp_div_2_(dstq, bnp, bn, d);
    } else if (ni > na) {
        mp_ptr restrict B = TALLOC_TYPE(2 * na + ni + 1, mp_limb_t);
        lmmp_zero(B, 2 * na + ni);
        B[2 * na + ni] = 1;
        lmmp_div_(dstq, NULL, B, 2 * na + ni + 1, numa, na);
    } else {
        int shift = lmmp_leading_zeros_(numa[na - 1]);
        if (shift > 0) {
            mp_ptr restrict numa_shift = TALLOC_TYPE(na, mp_limb_t);
            lmmp_shl_(numa_shift, numa, na, shift);
            lmmp_bninv_appr_newton_(dstq, numa_shift, na, ni + 1);
            lmmp_shr_(dstq, dstq, na + ni + 2, LIMB_BITS - shift);
        } else {
            lmmp_bninv_appr_newton_(dstq, numa, na, ni + 1);
            lmmp_copy(dstq, dstq + 1, na + ni + 1);
            dstq[na + ni + 1] = 0;
        }
    }
    TEMP_FREE;
    return;
}