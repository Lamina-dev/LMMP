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

/*
 * 算法阈值边界确定性测试。
 *
 * 对 mparam.h 中当前静态阈值，在 T-1 / T / T+1 处用独立参考实现或
 * 等价的库内关系交叉验证，确保分派点两侧都正确。
 */

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void fill_deterministic(mp_ptr p, mp_size_t n, u64 seed) {
    for (mp_size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0) p[n - 1] |= (u64)1 << 63;
}

void check_mul_n(mp_size_t n, u64 seed) {
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(2 * n + 2);
    fill_deterministic(a, n, seed);
    fill_deterministic(b, n, seed ^ 0x9e3779b97f4a7c15ull);
    const BigInt ba(a, n);
    const BigInt bb(b, n);
    const BigInt expect = BigInt::mul_school(ba, bb);

    lmmp_mul_n_(d, a, b, n);
    TEST_CHECK_MSG(limb_vec_eq(expect, d, 2 * n), "mul_n at threshold boundary");

    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
}

}  // namespace

TEST_CASE("threshold/mul", mul_n_toom_boundaries) {
    check_mul_n(MUL_TOOM22_THRESHOLD - 1, 0x1111222233334444ull);
    check_mul_n(MUL_TOOM22_THRESHOLD,     0x2222333344445555ull);
    check_mul_n(MUL_TOOM22_THRESHOLD + 1, 0x3333444455556666ull);

    check_mul_n(MUL_TOOM33_THRESHOLD - 1, 0x4444555566667777ull);
    check_mul_n(MUL_TOOM33_THRESHOLD,     0x5555666677778888ull);
    check_mul_n(MUL_TOOM33_THRESHOLD + 1, 0x6666777788889999ull);

    check_mul_n(MUL_TOOM44_THRESHOLD - 1, 0x777788889999aaaall);
    check_mul_n(MUL_TOOM44_THRESHOLD,     0x88889999aaaabbbbull);
    check_mul_n(MUL_TOOM44_THRESHOLD + 1, 0x9999aaaabbbbccccull);
}

TEST_CASE("threshold/mullo", mullo_dc_boundary) {
    const mp_size_t sizes[] = {
        MULLO_DC_THRESHOLD - 1,
        MULLO_DC_THRESHOLD,
        MULLO_DC_THRESHOLD + 1
    };
    u64 seed = 0xabcddcba12344321ull;
    for (mp_size_t n : sizes) {
        mp_ptr a = alloc_limbs(n);
        mp_ptr b = alloc_limbs(n);
        mp_ptr lo = alloc_limbs(n);
        mp_ptr full = alloc_limbs(2 * n + 2);
        fill_deterministic(a, n, seed);
        fill_deterministic(b, n, seed ^ 0x13579bdf2468ace0ull);

        lmmp_mul_(full, a, n, b, n);
        lmmp_mullo_(lo, a, b, n);
        TEST_CHECK_MSG(std::memcmp(lo, full, (size_t)n * sizeof(mp_limb_t)) == 0,
                       "mullo low limbs match full product at threshold");

        lmmp_free(a);
        lmmp_free(b);
        lmmp_free(lo);
        lmmp_free(full);
    }
}

TEST_CASE("threshold/div", divide_threshold_boundary) {
    const mp_size_t ns[] = {
        DIV_DIVIDE_THRESHOLD - 1,
        DIV_DIVIDE_THRESHOLD,
        DIV_DIVIDE_THRESHOLD + 1
    };
    u64 seed = 0x0123456789abcdeful;
    for (mp_size_t n : ns) {
        const mp_size_t na = 2 * n;
        mp_ptr num = alloc_limbs(na);
        mp_ptr den = alloc_limbs(n);
        mp_ptr q = alloc_limbs(n + 2);
        mp_ptr r = alloc_limbs(n + 2);
        fill_deterministic(num, na, seed);
        fill_deterministic(den, n, seed ^ 0xfedcba9876543210ull);
        num[na - 1] &= ~((u64)1 << 63); /* 保证 q 不超过 n+1 limbs */

        const BigInt bn(num, na);
        const BigInt bd(den, n);
        BigInt rem_ref;
        const BigInt q_ref = BigInt::divmod_school(bn, bd, rem_ref);

        lmmp_div_(q, r, num, na, den, n);
        TEST_CHECK_MSG(from_limbs(q, n + 2) == q_ref, "div quotient at boundary");
        TEST_CHECK_MSG(from_limbs(r, n) == rem_ref, "div remainder at boundary");

        lmmp_free(num);
        lmmp_free(den);
        lmmp_free(q);
        lmmp_free(r);
    }
}

TEST_CASE("threshold/divexact", divexact_two_threshold_boundaries) {
    u64 seed = 0x0fedcba987654321ull;
    struct Case { mp_size_t nn; mp_size_t dn; };
    const Case cases[] = {
        {DIVEXACT_NN_THRESHOLD - 1, DIVEXACT_BASECASE_THRESHOLD - 1},
        {DIVEXACT_NN_THRESHOLD - 1, DIVEXACT_BASECASE_THRESHOLD},
        {DIVEXACT_NN_THRESHOLD,     DIVEXACT_BASECASE_THRESHOLD - 1},
        {DIVEXACT_NN_THRESHOLD,     DIVEXACT_BASECASE_THRESHOLD},
        {DIVEXACT_NN_THRESHOLD + 1, DIVEXACT_BASECASE_THRESHOLD + 1}
    };
    for (const auto& c : cases) {
        const mp_size_t qn = c.nn - c.dn + 1;
        mp_ptr q0 = alloc_limbs(qn);
        mp_ptr dp = alloc_limbs(c.dn);
        mp_ptr np = alloc_limbs(c.nn + 2);
        mp_ptr dst = alloc_limbs(c.nn + 2);
        fill_deterministic(q0, qn, seed);
        fill_deterministic(dp, c.dn, seed ^ 0xa5a5a5a55a5a5a5aull);
        dp[0] |= 1u;
        dp[c.dn - 1] |= (u64)1 << 63;

        if (qn >= c.dn)
            lmmp_mul_(np, q0, qn, dp, c.dn);
        else
            lmmp_mul_(np, dp, c.dn, q0, qn);
        np[c.nn] = 0;

        lmmp_divexact_(dst, np, c.nn, dp, c.dn);
        TEST_CHECK_MSG(from_limbs(dst, qn) == from_limbs(q0, qn),
                       "divexact boundary recovers original quotient");

        lmmp_free(q0);
        lmmp_free(dp);
        lmmp_free(np);
        lmmp_free(dst);
    }
}

TEST_CASE("threshold/pow", pow_1_exp_boundary) {
    const mp_limb_t base = 3;
    mp_ptr b = alloc_limbs(1);
    b[0] = base;
    const ulong exps[] = {(ulong)POW_1_EXP_THRESHOLD - 1, (ulong)POW_1_EXP_THRESHOLD,
                         (ulong)POW_1_EXP_THRESHOLD + 1, 1ul};
    for (ulong e : exps) {
        const mp_size_t rn = lmmp_pow_size_(b, 1, e);
        mp_ptr dst = alloc_limbs(rn + 2);
        const BigInt expect = BigInt::pow(BigInt(base), e);
        const mp_size_t got = lmmp_pow_(dst, rn, b, 1, e);
        TEST_CHECK_MSG(from_limbs(dst, got) == expect, "pow_1 boundary");
        lmmp_free(dst);
    }
    lmmp_free(b);
}
