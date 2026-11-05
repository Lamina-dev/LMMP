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

#include "lmmp/lmmpn.h"
#include "lmmp/impl/mparam.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

/*
    梅森域乘法（B^rn - 1 卷绕）验证：oracle = 全积（lmmp_mul_，大尺寸走
    FFT，与被测分治/数论路径算法族独立）+ 循环折叠 + 全 1 零类归一化
    （B^rn-1 ≡ 0）——全库内路径，无自建算法。
*/

namespace {

/* oracle：全积 + 循环折叠 + 零类归一化（B^rn-1 -> 0）——全库内路径 */
void mersenne_oracle(mp_ptr r, mp_size_t rn, mp_srcptr a, mp_size_t na, mp_srcptr b, mp_size_t nb,
                     mp_ptr full) {
    lmmp_zero(full, 2 * rn + 2);
    if (na >= nb)
        lmmp_mul_(full, a, na, b, nb);
    else
        lmmp_mul_(full, b, nb, a, na);
    mp_size_t pn = na + nb;
    if (pn <= rn) {
        lmmp_copy(r, full, pn);
        lmmp_zero(r + pn, rn - pn);
    } else {
        mp_limb_t cy = lmmp_add_(r, full, rn, full + rn, pn - rn);
        lmmp_inc_1(r, cy);
    }
    mp_size_t i = 0;
    while (i < rn && r[i] == ~(mp_limb_t)0) ++i;
    if (i == rn) lmmp_zero(r, rn);
}

bool limbs_same(const mp_limb_t* x, const mp_limb_t* y, mp_size_t n) {
    return std::memcmp(x, y, (size_t)n * sizeof(mp_limb_t)) == 0;
}

}  // namespace

TEST_CASE("mul/mersenne_dc", mersenne_dc_vs_oracle) {
    u64 seed = 0x51ce5eed1a2b3c4dull;
    const mp_size_t sizes[] = {2,  3,  5,  8,  13, 14, 16, 17, 31, 32, 64, 100, 256,
                               511, 512, 768, 1000, 1024, 1721, 2048, 3442};
    for (mp_size_t rn : sizes) {
        mp_size_t shapes[6][2] = {{rn, rn}, {rn, rn / 2 + 1}, {rn, 1},
                                  {rn / 2 + 1, rn / 2 + 1}, {3 * rn / 4 + 1, rn / 4 + 1},
                                  {rn / 2, rn / 4 + 1}};  // 末形 na+nb<=rn：无卷绕路径
        for (auto& sh : shapes) {
            mp_size_t na = sh[0], nb = sh[1];
            if (nb > na || na > rn || na + nb <= rn / 2) continue;
            for (int mode = 0; mode < 3; ++mode) {  // 0 随机 / 1 全 1（零类卷绕）/ 2 全 0
                mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
                if (mode == 0) {
                    random_limbs(a, na, seed);
                    random_limbs(b, nb, seed);
                } else if (mode == 1) {
                    for (mp_size_t i = 0; i < na; ++i) a[i] = ~(mp_limb_t)0;
                    for (mp_size_t i = 0; i < nb; ++i) b[i] = ~(mp_limb_t)0;
                } else {
                    lmmp_zero(a, na);
                    lmmp_zero(b, nb);
                }
                mp_ptr r = alloc_limbs(rn), ro = alloc_limbs(rn), full = alloc_limbs(2 * rn + 2);
                lmmp_mul_mersenne_dc_(r, rn, a, na, b, nb);
                mersenne_oracle(ro, rn, a, na, b, nb, full);
                TEST_CHECK(limbs_same(ro, r, rn));

                lmmp_free(a); lmmp_free(b); lmmp_free(r); lmmp_free(ro); lmmp_free(full);
            }
        }

        /* 平方：满长 / 3/4 长（2an>rn 路径）/ rn/4+1（2an<rn 路径，须 4an>rn）
           / rn/2（2an==rn 边界）；满长另走分派入口（fft 分支仅 an==rn 满足
           rn = fft_next_size((2an+1)>>1) 前置，rn 不可行时回落 dc 亦被覆盖） */
        mp_size_t ans[4] = {rn, 3 * rn / 4 + 1, rn > 2 ? rn / 4 + 1 : rn, rn / 2};
        for (mp_size_t an : ans) {
            if (an > rn || 4 * an <= rn) continue;
            for (int mode = 0; mode < 3; ++mode) {  // 0 随机 / 1 全 1 / 2 全 0
                mp_ptr a = alloc_limbs(an);
                if (mode == 0)
                    random_limbs(a, an, seed);
                else if (mode == 1)
                    for (mp_size_t i = 0; i < an; ++i) a[i] = ~(mp_limb_t)0;
                else
                    lmmp_zero(a, an);
                mp_ptr r = alloc_limbs(rn), ro = alloc_limbs(rn), full = alloc_limbs(2 * rn + 2);
                lmmp_sqr_mersenne_dc_(r, rn, a, an);
                mersenne_oracle(ro, rn, a, an, a, an, full);
                TEST_CHECK(limbs_same(ro, r, rn));
                if (an == rn) {
                    lmmp_sqr_mersenne_(r, rn, a, an);
                    TEST_CHECK(limbs_same(ro, r, rn));
                }
                lmmp_free(a); lmmp_free(r); lmmp_free(ro); lmmp_free(full);
            }
        }
    }
}

TEST_CASE("mul/mersenne_next_size", mersenne_next_size_props) {
    for (mp_size_t n = 1; n <= 5000; ++n) {
        mp_size_t m = lmmp_mersenne_next_size_(n);
        TEST_CHECK(m >= n);
    }
    /* fft 档（nh 越过 MODF 阈值）输出 = 2*fft_next_size(nh)：钉住对齐意图，
       若退化为直接返回 n，分发入口在 rn>=FFT 阈值时回落 dc（性能回归
       不可见于正确性用例）。不断言 m 本身 fft 可行——倍乘不保可行性
       （nh=2171 可行而 2*2171=4342 不可行，fns(4342)=4344） */
    for (mp_size_t n = 8 * (MUL_MERSENNE_DC_THRESHOLD - 1) + 1; n <= 5000; n += 97) {
        if ((n + 1) >> 1 < MUL_FFT_MODF_THRESHOLD) continue;
        TEST_CHECK(lmmp_mersenne_next_size_(n) == 2 * lmmp_fft_next_size_((n + 1) >> 1));
    }
    /* 分发入口与 next_size 尺寸下的正确性 */
    u64 seed = 0x2f6e1d5a9c83b701ull;
    for (int t = 0; t < 16; ++t) {
        mp_size_t n = 3 + (mp_size_t)(xorshift64(seed) % 900);
        mp_size_t rn = lmmp_mersenne_next_size_((n + n + 1) >> 1);
        mp_ptr a = alloc_limbs(n), b = alloc_limbs(n);
        mp_ptr r = alloc_limbs(rn), ro = alloc_limbs(rn), full = alloc_limbs(2 * rn + 2);
        random_limbs(a, n, seed);
        random_limbs(b, n, seed);
        if (n + n > rn / 2) {
            lmmp_mul_mersenne_(r, rn, a, n, b, n);
            mersenne_oracle(ro, rn, a, n, b, n, full);
            TEST_CHECK(limbs_same(ro, r, rn));
        }
        lmmp_free(a); lmmp_free(b); lmmp_free(r); lmmp_free(ro); lmmp_free(full);
    }
}
