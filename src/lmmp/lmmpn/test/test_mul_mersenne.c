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

#include "lmmp/lmmpn.h"
#include "impl/mul_cache.h"
#include "mparam.h"
#include "cases.h"
#include "support/lmmp_test_utils.h"

#include <string.h>

/*
    梅森域乘法（B^rn - 1 卷绕）验证：oracle = 全积（lmmp_mul_，大尺寸走
    FFT，与被测分治/数论路径算法族独立）+ 梅森折叠 + 全 1 零类归一化
    （B^rn-1 ≡ 0）——全库内路径，无自建算法。
*/

/* oracle：全积 + 梅森折叠 + 零类归一化（B^rn-1 -> 0）——全库内路径 */
static void mersenne_oracle(mp_ptr r, mp_size_t rn, mp_srcptr a, mp_size_t na, mp_srcptr b, mp_size_t nb,
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

static bool limbs_same(const mp_limb_t* x, const mp_limb_t* y, mp_size_t n) {
    return memcmp(x, y, (size_t)n * sizeof(mp_limb_t)) == 0;
}

TEST_CASE("mul/mersenne_dc", mersenne_dc_vs_oracle) {
    u64 seed = 0x51ce5eed1a2b3c4dull;
    const mp_size_t sizes[] = {2,  3,  5,  8,  13, 14, 16, 17, 31, 32, 64, 100, 256,
                               511, 512, 768, 1000, 1024, 1721, 2048, 3442};
    for (size_t rn_index = 0; rn_index < ARRAY_SIZE(sizes); ++rn_index) {
        mp_size_t rn = sizes[rn_index];
        mp_size_t shapes[6][2] = {{rn, rn}, {rn, rn / 2 + 1}, {rn, 1},
                                  {rn / 2 + 1, rn / 2 + 1}, {3 * rn / 4 + 1, rn / 4 + 1},
                                  {rn / 2, rn / 4 + 1}};  // 末形 na+nb<=rn：无卷绕路径
        for (size_t sh_index = 0; sh_index < ARRAY_SIZE(shapes); ++sh_index) {
            const mp_size_t *sh = shapes[sh_index];
            mp_size_t na = sh[0], nb = sh[1];
            if (nb > na || na > rn || na + nb <= rn / 2) continue;
            for (int mode = 0; mode < 3; ++mode) {  // 0 随机 / 1 全 1（零类卷绕）/ 2 全 0
                mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
                if (mode == 0) {
                    random_limbs(a, na, &seed);
                    random_limbs(b, nb, &seed);
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
        for (size_t an_index = 0; an_index < ARRAY_SIZE(ans); ++an_index) {
            mp_size_t an = ans[an_index];
            if (an > rn || 4 * an <= rn) continue;
            for (int mode = 0; mode < 3; ++mode) {  // 0 随机 / 1 全 1 / 2 全 0
                mp_ptr a = alloc_limbs(an);
                if (mode == 0)
                    random_limbs(a, an, &seed);
                else if (mode == 1)
                    for (mp_size_t i = 0; i < an; ++i) a[i] = ~(mp_limb_t)0;
                else
                    lmmp_zero(a, an);
                mp_ptr r = alloc_limbs(rn), ro = alloc_limbs(rn), full = alloc_limbs(2 * rn + 2);
                lmmp_sqr_mersenne_dc_(r, rn, a, an);
                mersenne_oracle(ro, rn, a, an, a, an, full);
                TEST_CHECK(limbs_same(ro, r, rn));
                if (an == rn) {
                    /* 毒化 r：防分派入口未写/部分写时沿用 dc 结果通过 */
                    for (mp_size_t i = 0; i < rn; ++i) r[i] = ~ro[i];
                    lmmp_sqr_mersenne_(r, rn, a, an);
                    TEST_CHECK(limbs_same(ro, r, rn));
                }
                lmmp_free(a); lmmp_free(r); lmmp_free(ro); lmmp_free(full);
            }
        }
    }
}

TEST_CASE("mul/mersenne_dispatch_small", mersenne_dispatch_small_operands) {
    u64 seed = 0x7c3a5f19b2d4e681ull;
    const mp_size_t sizes[] = {8, 72, 100, 300, 511, 512, 1024, 2048, 3442};
    for (size_t rn_index = 0; rn_index < ARRAY_SIZE(sizes); ++rn_index) {
        mp_size_t rn = sizes[rn_index];
        /* 分派入口小操作数直乘路径（na+nb<=rn/2）：rn/2 边界、rn/2-1（dc 前置
           违例区）、rn/4；两形 {对半, 长短}，后者含 nb>na（分派内部交换后短路） */
        const mp_size_t sums[] = {rn / 2, rn / 2 - 1, rn / 4};
        for (size_t si = 0; si < ARRAY_SIZE(sums); ++si) {
            mp_size_t sum = sums[si];
            if (sum < 2) continue;
            const mp_size_t sh[2][2] = {{sum / 2, sum - sum / 2}, {sum - 1, 1}};
            for (size_t sh_index = 0; sh_index < ARRAY_SIZE(sh); ++sh_index) {
                mp_size_t na = sh[sh_index][0], nb = sh[sh_index][1];
                for (int mode = 0; mode < 3; ++mode) {  // 0 随机 / 1 全 1 / 2 全 0
                    mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
                    if (mode == 0) {
                        random_limbs(a, na, &seed);
                        random_limbs(b, nb, &seed);
                    } else if (mode == 1) {
                        for (mp_size_t i = 0; i < na; ++i) a[i] = ~(mp_limb_t)0;
                        for (mp_size_t i = 0; i < nb; ++i) b[i] = ~(mp_limb_t)0;
                    } else {
                        lmmp_zero(a, na);
                        lmmp_zero(b, nb);
                    }
                    mp_ptr r = alloc_limbs(rn), ro = alloc_limbs(rn), full = alloc_limbs(2 * rn + 2);
                    mersenne_oracle(ro, rn, a, na, b, nb, full);
                    for (mp_size_t i = 0; i < rn; ++i) r[i] = ~ro[i];  // 毒化：直乘+补零须写满 rn
                    lmmp_mul_mersenne_(r, rn, a, na, b, nb);
                    TEST_CHECK(limbs_same(ro, r, rn));
                    lmmp_free(a); lmmp_free(b); lmmp_free(r); lmmp_free(ro); lmmp_free(full);
                }
            }
        }
        /* 平方小操作数直乘路径（4*na<=rn）：rn/4 边界、rn/4-1、rn/8 */
        const mp_size_t ans[] = {rn / 4, rn / 4 - 1, rn / 8};
        for (size_t an_index = 0; an_index < ARRAY_SIZE(ans); ++an_index) {
            mp_size_t an = ans[an_index];
            if (an < 1) continue;
            for (int mode = 0; mode < 3; ++mode) {
                mp_ptr a = alloc_limbs(an);
                if (mode == 0)
                    random_limbs(a, an, &seed);
                else if (mode == 1)
                    for (mp_size_t i = 0; i < an; ++i) a[i] = ~(mp_limb_t)0;
                else
                    lmmp_zero(a, an);
                mp_ptr r = alloc_limbs(rn), ro = alloc_limbs(rn), full = alloc_limbs(2 * rn + 2);
                mersenne_oracle(ro, rn, a, an, a, an, full);
                for (mp_size_t i = 0; i < rn; ++i) r[i] = ~ro[i];
                lmmp_sqr_mersenne_(r, rn, a, an);
                TEST_CHECK(limbs_same(ro, r, rn));
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
    /* fft 档（nh 越过 MODF 阈值）输出 = 2*fft_next_size(nh)：固定对齐意图，
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
        mp_size_t n = 3 + (mp_size_t)(xorshift64(&seed) % 900);
        mp_size_t rn = lmmp_mersenne_next_size_((n + n + 1) >> 1);
        mp_ptr a = alloc_limbs(n), b = alloc_limbs(n);
        mp_ptr r = alloc_limbs(rn), ro = alloc_limbs(rn), full = alloc_limbs(2 * rn + 2);
        random_limbs(a, n, &seed);
        random_limbs(b, n, &seed);
        if (n + n > rn / 2) {
            lmmp_mul_mersenne_(r, rn, a, n, b, n);
            mersenne_oracle(ro, rn, a, n, b, n, full);
            TEST_CHECK(limbs_same(ro, r, rn));
        }
        lmmp_free(a); lmmp_free(b); lmmp_free(r); lmmp_free(ro); lmmp_free(full);
    }
}

TEST_CASE("mul/mersenne_fft", mersenne_fft_single_limb) {
    const mp_limb_t a[] = {LIMB_MAX, 0, 1, 0x8123456789abcdefull};
    const mp_limb_t b = 0xfedcba9876543210ull;
    mp_limb_t dst[2] = {0, LIMB_MAX}, ref, full[4];
    fft_gr_cache cache;
    lmmp_mul_mersenne_cache_init_(dst, 1, a, 1, &b, 1, &cache);
    mersenne_oracle(&ref, 1, a, 1, &b, 1, full);
    TEST_CHECK(dst[0] == ref && dst[1] == LIMB_MAX);

    for (size_t i = 0; i < ARRAY_SIZE(a); ++i) {
        mersenne_oracle(&ref, 1, a + i, 1, &b, 1, full);
        dst[0] = ~ref;
        lmmp_mul_mersenne_cache_(dst, a + i, &cache);
        TEST_CHECK(dst[0] == ref && dst[1] == LIMB_MAX);
        dst[0] = ~ref;
        lmmp_mul_mersenne_fft_(dst, 1, a + i, 1, &b, 1);
        TEST_CHECK(dst[0] == ref && dst[1] == LIMB_MAX);
        mersenne_oracle(&ref, 1, a + i, 1, a + i, 1, full);
        dst[0] = ~ref;
        lmmp_sqr_mersenne_fft_(dst, 1, a + i, 1);
        TEST_CHECK(dst[0] == ref && dst[1] == LIMB_MAX);
    }
    lmmp_fft_gr_cache_free_(&cache);
}

TEST_CASE("mul/fermat_fft", fermat_cache_single_limb) {
    const mp_limb_t a = 2, b = LIMB_B_2, next = 1;
    mp_limb_t dst[3] = {LIMB_MAX, LIMB_MAX, LIMB_MAX};
    fft_gr_cache cache;
    lmmp_mul_fermat_cache_init_(dst, 1, &a, 1, &b, 1, &cache);
    TEST_CHECK(dst[0] == 0 && dst[1] == 1 && dst[2] == LIMB_MAX);
    lmmp_mul_fermat_cache_(dst, &next, &cache);
    TEST_CHECK(dst[0] == b && dst[1] == 0 && dst[2] == LIMB_MAX);
    lmmp_fft_gr_cache_free_(&cache);
}

void test_mul_mersenne(void) {
    TEST_RUN("mul/mersenne_dc", mersenne_dc_vs_oracle);
    TEST_RUN("mul/mersenne_dispatch_small", mersenne_dispatch_small_operands);
    TEST_RUN("mul/mersenne_next_size", mersenne_next_size_props);
    TEST_RUN("mul/mersenne_fft", mersenne_fft_single_limb);
    TEST_RUN("mul/fermat_fft", fermat_cache_single_limb);
}
