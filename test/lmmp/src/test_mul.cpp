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
#include "lmmp/impl/mul_hard.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0) p[n - 1] |= (u64)1 << 63;
}

void check_mul_result(const BigInt& expected, const mp_limb_t* got, mp_size_t na, mp_size_t nb) {
    size_t total = na + nb;
    size_t got_size = total;
    while (got_size > 0 && got[got_size - 1] == 0) --got_size;
    if (got_size == 0) got_size = 1;
    if (expected.d.size() != got_size) {
        TEST_CHECK_MSG(false, "mul result size mismatch");
        return;
    }
    for (size_t i = 0; i < got_size; ++i) {
        if (expected.d[i] != got[i]) {
            TEST_CHECK_MSG(false, "mul result limb mismatch");
            return;
        }
    }
    TEST_CHECK(true);
}

}  // namespace

TEST_CASE("mul/basecase", mul_basecase_vs_school) {
    u64 seed = 0x13579bdf2468ace0ull;
    for (size_t na = 1; na <= 40; ++na) {
        for (size_t nb = 1; nb <= na; ++nb) {
            mp_ptr a = alloc_limbs(na);
            mp_ptr b = alloc_limbs(nb);
            mp_ptr dst = alloc_limbs(na + nb + 1);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
            BigInt ba(a, na), bb(b, nb);
            BigInt expect = BigInt::mul_school(ba, bb);
            lmmp_mul_basecase_(dst, a, na, b, nb);
            check_mul_result(expect, dst, na, nb);
            lmmp_free(a); lmmp_free(b); lmmp_free(dst);
        }
    }
}

TEST_CASE("mul/sqr_basecase", sqr_basecase_vs_school) {
    u64 seed = 0x2468ace013579bdfull;
    for (size_t na = 1; na <= 40; ++na) {
        mp_ptr a = alloc_limbs(na);
        mp_ptr dst = alloc_limbs(2 * na + 1);
        random_limbs(a, na, seed);
        BigInt ba(a, na);
        BigInt expect = BigInt::sqr_school(ba);
        lmmp_sqr_basecase_(dst, a, na);
        check_mul_result(expect, dst, na, na);
        lmmp_free(a); lmmp_free(dst);
    }
}

TEST_CASE("mul/dispatch", mul_dispatch_vs_school) {
    u64 seed = 0x0123456789abcdefull;
    // 覆盖 basecase / toom22 / toom32 / toom33 / toom42 / toom43 / toom44 等阈值
    const mp_size_t sizes[] = {
        1, 2, 5, 19, 20, 21, 30, 40, 64, 65, 66, 80, 100, 120,
        150, 180, 220, 300, 400, 580, 581, 582, 700, 900, 1200, 1600, 2316
    };
    for (mp_size_t na : sizes) {
        for (int t = 0; t < 3; ++t) {
            mp_size_t nb = (mp_size_t)((u64)na * (t == 0 ? 1 : (t == 1 ? 2 : 3)) / (t == 0 ? 1 : (t == 1 ? 3 : 4)));
            if (nb < 1) nb = 1;
            if (nb > na) nb = na;
            // 学校乘法在过大尺寸下较慢，限制总规模。
            if ((u64)na * nb > 1200000ull) continue;

            mp_ptr a = alloc_limbs(na);
            mp_ptr b = alloc_limbs(nb);
            mp_ptr dst = alloc_limbs(na + nb + 2);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
            BigInt ba(a, na), bb(b, nb);
            BigInt expect = BigInt::mul_school(ba, bb);

            lmmp_mul_(dst, a, na, b, nb);
            check_mul_result(expect, dst, na, nb);
            lmmp_free(a); lmmp_free(b); lmmp_free(dst);
        }
    }
}

TEST_CASE("mul/hard_threshold_boundary", mul_hard_boundary_vs_school) {
    /* 覆盖硬编码函数(hardcoded n=1..19)经 toom 递归的边界尺寸:
       toom22 递归子规模 n/2 恰好落在 19/20 两侧 */
    u64 seed = 0x0f1e2d3c4b5a6978ull;
    const mp_size_t sizes[] = {
        19, 20, 21, 22, 37, 38, 39, 40, 41, 42, 63, 64, 65, 66
    };
    for (mp_size_t n : sizes) {
        for (int t = 0; t < 8; t++) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr b = alloc_limbs(n);
            mp_ptr dst = alloc_limbs(2 * n + 2);
            random_limbs(a, n, seed);
            random_limbs(b, n, seed);
            BigInt ba(a, n), bb(b, n);
            BigInt expect = BigInt::mul_school(ba, bb);
            lmmp_mul_n_(dst, a, b, n);
            check_mul_result(expect, dst, n, n);

            BigInt es = BigInt::sqr_school(ba);
            lmmp_sqr_(dst, a, n);
            check_mul_result(es, dst, n, n);
            lmmp_free(a); lmmp_free(b); lmmp_free(dst);
        }
    }
}

TEST_CASE("mul/hard_direct_all_sizes", mul_hard_direct_n1_to_n19) {
    /* 硬编码路径全规模直测: n=1..19 经顶层 lmmp_mul_n_/lmmp_sqr_ (阈值下
       必走硬编码调度) 对照学校乘法, 覆盖随机/全 1/全 0xFF/最高位等
       触发进位链极端的输入模式 */
    u64 seed = 0x0d1c2b3a49586775ull;
    for (mp_size_t n = 1; n <= LMMP_MUL_HARD_MAX_N; n++) {
        for (int pat = 0; pat < 6; pat++) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr b = alloc_limbs(n);
            mp_ptr dst = alloc_limbs(2 * n + 2);
            for (mp_size_t i = 0; i < n; i++) {
                switch (pat) {
                    case 0: a[i] = b[i] = xorshift64(seed); break;
                    case 1: a[i] = b[i] = ~(u64)0; break;
                    case 2: a[i] = b[i] = 1; break;
                    case 3: a[i] = b[i] = (u64)1 << 63; break;
                    case 4: a[i] = b[i] = (~(u64)0) - (u64)(i & 1); break;
                    default: a[i] = b[i] = xorshift64(seed) >> (i & 63); break;
                }
            }
            BigInt ba(a, n), bb(b, n);
            lmmp_mul_n_(dst, a, b, n);
            check_mul_result(BigInt::mul_school(ba, bb), dst, n, n);
            lmmp_sqr_(dst, a, n);
            check_mul_result(BigInt::sqr_school(ba), dst, n, n);
            lmmp_free(a); lmmp_free(b); lmmp_free(dst);
        }
    }
}

TEST_CASE("mul/unbalanced", mul_unbalanced_vs_school) {
    u64 seed = 0xfedcba9876543210ull;
    struct Case { mp_size_t na; mp_size_t nb; };
    const Case cases[] = {
        {600, 3}, {600, 5}, {600, 7}, {600, 10}, {600, 30}, {600, 100},
        {700, 100}, {800, 200}, {900, 250}, {1200, 300}, {1600, 400},
        {2316, 400}, {2316, 800}, {2400, 900}, {3116, 180}, {4200, 1033}
    };
    for (auto c : cases) {
        if ((u64)c.na * c.nb > 1500000ull) continue;
        mp_ptr a = alloc_limbs(c.na);
        mp_ptr b = alloc_limbs(c.nb);
        mp_ptr dst = alloc_limbs(c.na + c.nb + 2);
        random_limbs(a, c.na, seed);
        random_limbs(b, c.nb, seed);
        BigInt ba(a, c.na), bb(b, c.nb);
        BigInt expect = BigInt::mul_school(ba, bb);
        lmmp_mul_(dst, a, c.na, b, c.nb);
        check_mul_result(expect, dst, c.na, c.nb);
        lmmp_free(a); lmmp_free(b); lmmp_free(dst);
    }
}

TEST_CASE("mul/mullo", mullo_vs_school) {
    u64 seed = 0xa53a591286afcbb5ull;
    for (mp_size_t n : {1, 2, 10, 50, 65, 83, 122, 201, 737, 1262}) {
        mp_ptr a = alloc_limbs(n);
        mp_ptr b = alloc_limbs(n);
        mp_ptr dst = alloc_limbs(n);
        random_limbs(a, n, seed);
        random_limbs(b, n, seed);
        BigInt ba(a, n), bb(b, n);
        BigInt prod = BigInt::mul_school(ba, bb);
        if (prod.d.size() > (size_t)n) prod.d.resize(n);
        prod.trim();

        lmmp_mullo_(dst, a, b, n);
        TEST_CHECK(limb_vec_eq(prod, dst, n));
        lmmp_free(a); lmmp_free(b); lmmp_free(dst);
    }
}

TEST_CASE("mul/sqr_dispatch", sqr_dispatch_vs_school) {
    u64 seed = 0x819821c27be91101ull;
    for (mp_size_t n : {1, 2, 19, 20, 21, 30, 65, 100, 400, 581, 800, 1200, 2318, 2401}) {
        if ((u64)n * n > 1200000ull) continue;
        mp_ptr a = alloc_limbs(n);
        mp_ptr dst = alloc_limbs(2 * n + 2);
        random_limbs(a, n, seed);
        BigInt ba(a, n);
        BigInt expect = BigInt::sqr_school(ba);
        lmmp_sqr_(dst, a, n);
        check_mul_result(expect, dst, n, n);
        lmmp_free(a); lmmp_free(dst);
    }
}

namespace {

bool limbs_same(const mp_limb_t* x, const mp_limb_t* y, mp_size_t n) {
    return std::memcmp(x, y, (size_t)n * sizeof(mp_limb_t)) == 0;
}

/* oracle：全积 + 循环折叠 + 零类归一化（B^rn-1 -> 0） */
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

}  // namespace

TEST_CASE("mul/mersenne_dc", mersenne_dc_vs_oracle) {
    u64 seed = 0x51ce5eed1a2b3c4dull;
    const mp_size_t sizes[] = {2,  3,  5,  8,  13, 14, 16, 17, 31, 32, 64, 100, 256,
                               511, 512, 768, 1000, 1024, 1721, 2048, 3442};
    for (mp_size_t rn : sizes) {
        mp_size_t shapes[5][2] = {{rn, rn}, {rn, rn / 2 + 1}, {rn, 1},
                                  {rn / 2 + 1, rn / 2 + 1}, {3 * rn / 4 + 1, rn / 4 + 1}};
        for (auto& sh : shapes) {
            mp_size_t na = sh[0], nb = sh[1];
            if (nb > na || na > rn || na + nb <= rn / 2) continue;
            for (int mode = 0; mode < 2; ++mode) {  // 0 随机 / 1 全 1（零类卷绕）
                mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
                if (mode == 0) {
                    random_limbs(a, na, seed);
                    random_limbs(b, nb, seed);
                } else {
                    for (mp_size_t i = 0; i < na; ++i) a[i] = ~(mp_limb_t)0;
                    for (mp_size_t i = 0; i < nb; ++i) b[i] = ~(mp_limb_t)0;
                }
                mp_ptr r = alloc_limbs(rn), ro = alloc_limbs(rn), full = alloc_limbs(2 * rn + 2);
                lmmp_mul_mersenne_dc_(r, rn, a, na, b, nb);
                mersenne_oracle(ro, rn, a, na, b, nb, full);
                TEST_CHECK(limbs_same(ro, r, rn));

                lmmp_free(a); lmmp_free(b); lmmp_free(r); lmmp_free(ro); lmmp_free(full);
            }
        }

        /* 平方：满长 / 3/4 长（2an>rn 路径）/ rn/4+1（2an<rn 路径，须 4an>rn） */
        mp_size_t ans[3] = {rn, 3 * rn / 4 + 1, rn > 2 ? rn / 4 + 1 : rn};
        for (mp_size_t an : ans) {
            if (an > rn || 4 * an <= rn) continue;
            for (int mode = 0; mode < 2; ++mode) {
                mp_ptr a = alloc_limbs(an);
                if (mode == 0)
                    random_limbs(a, an, seed);
                else
                    for (mp_size_t i = 0; i < an; ++i) a[i] = ~(mp_limb_t)0;
                mp_ptr r = alloc_limbs(rn), ro = alloc_limbs(rn), full = alloc_limbs(2 * rn + 2);
                lmmp_sqr_mersenne_dc_(r, rn, a, an);
                mersenne_oracle(ro, rn, a, an, a, an, full);
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
