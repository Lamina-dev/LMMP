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
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "mparam.h"
#include "impl/mul_hard.h"
#include "lmmp/lmmpn.h"


/*
        Areas where the different toom algorithms can be called
0    1/6  1/5 1/4  1/3 2/5 9/20 1/2 5/9 3/5 2/3 3/4 4/5  9/10   1  nb/na

                                 (-------------------(xxxxxxxxxx|  toom22
                    (----------------(xxxxxxxxxxxxxxx|----------)  toom32
               (----(xxxxxxxxxxxxxxxx|-------)                     toom42
(xxxxxxxxxxxxxxxxxxx|                                              toom42-unblanced

                                             (-------(xxxxxxxxxx|  toom33
                    (----------------(xxxxxxxxxxxxxxx|----------)  toom32
               (----(xxxxxxxxxxxxxxxx|-------)                     toom42
(xxxxxxxxxxxxxxxxxxx|                                              toom42-unblanced

                                                 (---(xxxxxxxxxx|  toom44
                                 (-------(xxxxxxxxxxx|----------)  toom43
                        (---(xxxxxxxxxxxx|-------)                 toom53
           (--------(xxxxxxx|----)                                 toom52
      (----(xxxxxxxx|---)                                          toom62
(xxxxxxxxxx|                                                       toom62-unblanced

                    (xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx|  toom6h
      (-------------|---)                                          toom62
(xxxxxxxxxxxxxxxxxxx)                                              toom93-unbalance

               |xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx|  toom8h
(xxxxxxxxxxxxxx|----|                                              toom93-unbalance

0    1/6  1/5 1/4  1/3 2/5 9/20 1/2 5/9 3/5 2/3 3/4 4/5  9/10   1  nb/na
*/

void lmmp_mul_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(nb > 0);
    if (na == nb) {
        if (numa == numb)
            lmmp_sqr_(dst, numa, na);
        else
            lmmp_mul_n_(dst, numa, numb, na);
    } else if (nb < MUL_TOOM22_THRESHOLD) {
        lmmp_mul_basecase_(dst, numa, na, numb, nb);
    } else if (((na + nb) >> 1) < MUL_TOOM33_THRESHOLD || 2 * nb < MUL_TOOM33_THRESHOLD) {
        if (4 * na < 5 * nb)
            lmmp_mul_toom22_(dst, numa, na, numb, nb);
        else if (5 * na < 9 * nb)
            lmmp_mul_toom32_(dst, numa, na, numb, nb);
        else if (na < 3 * nb)
            lmmp_mul_toom42_(dst, numa, na, numb, nb);
        else
            lmmp_mul_toom42_unbalance_(dst, numa, na, numb, nb);
    } else if (((na + nb) >> 1) < MUL_TOOM44_THRESHOLD || 2 * nb < MUL_TOOM44_THRESHOLD) {
        if (4 * na < 5 * nb)
            lmmp_mul_toom33_(dst, numa, na, numb, nb);
        else if (5 * na < 9 * nb)
            lmmp_mul_toom32_(dst, numa, na, numb, nb);
        else if (na < 3 * nb)
            lmmp_mul_toom42_(dst, numa, na, numb, nb);
        else
            lmmp_mul_toom42_unbalance_(dst, numa, na, numb, nb);
    } else if (((na + nb) >> 1) < MUL_TOOM6H_THRESHOLD || 3 * nb < MUL_TOOM6H_THRESHOLD) {
        if (4 * na < 5 * nb)
            lmmp_mul_toom44_(dst, numa, na, numb, nb);
        else if (3 * na < 5 * nb)
            lmmp_mul_toom43_(dst, numa, na, numb, nb);
        else if (9 * na < 20 * nb)
            lmmp_mul_toom53_(dst, numa, na, numb, nb);
        else if (na < 3 * nb)
            lmmp_mul_toom52_(dst, numa, na, numb, nb);
        else if (na < 5 * nb)
            lmmp_mul_toom62_(dst, numa, na, numb, nb);
        else
            lmmp_mul_toom62_unbalance_(dst, numa, na, numb, nb);
    } else if (((na + nb) >> 1) < MUL_TOOM8H_THRESHOLD || 3 * nb < MUL_TOOM8H_THRESHOLD) {
        if (na + 9 <= 3 * nb)
            lmmp_mul_toom6h_(dst, numa, na, numb, nb);
        else if (na < 3 * nb)
            lmmp_mul_toom62_(dst, numa, na, numb, nb);
        else
            lmmp_mul_toom93_unbalance_(dst, numa, na, numb, nb);
    } else if (((na + nb) >> 1) < MUL_FFT_THRESHOLD || 4 * nb < MUL_FFT_THRESHOLD) {
        if (na < 4 * nb)
            lmmp_mul_toom8h_(dst, numa, na, numb, nb);
        else
            lmmp_mul_toom93_unbalance_(dst, numa, na, numb, nb);
    } else {
        if (na < 6 * nb)
            lmmp_mul_fft_(dst, numa, na, numb, nb);
        else
            lmmp_mul_fft_unbalance_(dst, numa, na, numb, nb);
    }
}

void lmmp_mul_n_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_srcptr restrict numb, mp_size_t n) {
    if (n < MUL_TOOM22_THRESHOLD)
        lmmp_mul_hard_n_(dst, numa, numb, n);
    else if (n < MUL_TOOM33_THRESHOLD)
        lmmp_mul_toom22_(dst, numa, n, numb, n);
    else if (n < MUL_TOOM44_THRESHOLD)
        lmmp_mul_toom33_(dst, numa, n, numb, n);
    else if (n < MUL_TOOM6H_THRESHOLD)
        lmmp_mul_toom44_(dst, numa, n, numb, n);
    else if (n < MUL_TOOM8H_THRESHOLD)
        lmmp_mul_toom6h_(dst, numa, n, numb, n);
    else if (n < MUL_FFT_THRESHOLD)
        lmmp_mul_toom8h_(dst, numa, n, numb, n);
    else
        lmmp_mul_fft_(dst, numa, n, numb, n);
}

void lmmp_sqr_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    if (na < SQR_TOOM22_THRESHOLD)
        lmmp_sqr_hard_n_(dst, numa, na);
    else if (na < SQR_TOOM33_THRESHOLD)
        lmmp_sqr_toom2_(dst, numa, na);
    else if (na < SQR_TOOM44_THRESHOLD)
        lmmp_sqr_toom3_(dst, numa, na);
    else if (na < SQR_TOOM6_THRESHOLD)
        lmmp_sqr_toom4_(dst, numa, na);
    else if (na < SQR_TOOM8_THRESHOLD)
        lmmp_sqr_toom6_(dst, numa, na);
    else if (na < SQR_FFT_THRESHOLD)
        lmmp_sqr_toom8_(dst, numa, na);
    else
        lmmp_sqr_fft_(dst, numa, na);
}
