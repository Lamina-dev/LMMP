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

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <vector>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

// 朴素迭代参考：F[0..limit]
std::vector<BigInt> fib_ref(ulong limit) {
    std::vector<BigInt> f;
    f.push_back(BigInt(0));
    f.push_back(BigInt(1));
    for (ulong i = 2; i <= limit; ++i) f.push_back(BigInt::add_abs(f[i - 1], f[i - 2]));
    return f;
}

mp_size_t norm_len(const mp_ptr p, mp_size_t n) {
    while (n > 0 && p[n - 1] == 0) --n;
    return n;
}

}  // namespace

TEST_CASE("numth/fib", small_exhaustive) {
    const ulong NLIM = 500;
    auto ref = fib_ref(NLIM);
    for (ulong n = 0; n <= NLIM; n += 5) {
        mp_size_t rn = lmmp_fibonacci_size_(n);
        TEST_CHECK_MSG(rn >= 1, "size lower bound");
        mp_ptr dst = alloc_limbs((size_t)rn);
        mp_size_t len = lmmp_fibonacci_(dst, rn, n);
        TEST_CHECK_MSG(from_limbs(dst, len) == ref[n], "fibonacci_ value");
        lmmp_free(dst);

        mp_size_t rn2 = lmmp_fibonacci_size_(n);
        TEST_CHECK_MSG(rn2 >= 1, "size lower bound");
        mp_ptr fp = alloc_limbs((size_t)rn2);
        mp_ptr f1p = alloc_limbs((size_t)rn2);
        mp_size_t len2 = lmmp_fibonacci2_(fp, f1p, rn2, n);
        TEST_CHECK_MSG(from_limbs(fp, len2) == ref[n], "fibonacci2_ F[n] value");
        mp_size_t len2m1 = len2 - (f1p[len2 - 1] == 0);
        if (n == 0) {
            TEST_CHECK_MSG(from_limbs(f1p, len2m1) == BigInt(1), "F[-1]==1");
        } else {
            TEST_CHECK_MSG(from_limbs(f1p, len2m1) == ref[n - 1], "fibonacci2_ F[n-1] value");
        }
        lmmp_free(fp);
        lmmp_free(f1p);
    }
}

/* 大规模抽查：递推链 F[n]=F[n-1]+F[n-2] 与 Cassini 恒等式 F[n-1]*F[n+1]-F[n]^2=(-1)^n，
   均以库内加/乘法验证，覆盖奇偶、2 的幂、fibonacci_ 合并路径中 2F[k] 溢出的位组合 */

TEST_CASE("numth/fib", large_recurrence_and_cassini) {
    for (ulong n : {94ul, 95ul, 187ul, 371ul, 373ul, 925ul, 1000ul, 4181ul, 65536ul, 65537ul,
                    100001ul, 131072ul, 150000ul}) {
        mp_size_t rn = lmmp_fibonacci_size_(n + 1);
        mp_ptr a = alloc_limbs((size_t)rn);        // F[n-1]
        mp_ptr b = alloc_limbs((size_t)rn);        // F[n]
        mp_ptr c = alloc_limbs((size_t)rn);        // F[n+1]
        mp_ptr t = alloc_limbs((size_t)(2 * rn));  // fibonacci2_ 副输出 / F[n]^2
        mp_ptr s = alloc_limbs((size_t)(2 * rn));

        lmmp_zero(a, rn); lmmp_zero(b, rn); lmmp_zero(c, rn);
        mp_size_t lb = lmmp_fibonacci2_(b, a, rn, n);
        lmmp_zero(b + lb, rn - lb); lmmp_zero(a + lb, rn - lb);
        mp_size_t lc = lmmp_fibonacci2_(c, t, rn, n + 1);
        lmmp_zero(c + lc, rn - lc);

        // 递推链：F[n+1] == F[n] + F[n-1]
        lmmp_zero(s, 2 * rn);
        mp_limb_t carry = lmmp_add_(s, b, rn, a, rn);
        TEST_CHECK_MSG(carry == 0, "recurrence no carry");
        TEST_CHECK_MSG(norm_len(s, 2 * rn) == lc, "recurrence size");
        for (mp_size_t i = 0; i < lc; ++i) {
            if (s[i] != c[i]) {
                TEST_CHECK_MSG(false, "recurrence value");
                break;
            }
        }

        // fibonacci_ 与 fibonacci2_ 一致
        mp_ptr d = alloc_limbs((size_t)lmmp_fibonacci_size_(n));
        mp_size_t ld = lmmp_fibonacci_(d, lmmp_fibonacci_size_(n), n);
        TEST_CHECK_MSG(ld == lb, "consistency size");
        bool same = true;
        for (mp_size_t i = 0; i < ld; ++i) same = same && (d[i] == b[i]);
        TEST_CHECK_MSG(same, "consistency value");

        // Cassini：F[n-1]*F[n+1] - F[n]^2 == (-1)^n
        lmmp_mul_(s, a, rn, c, rn);
        lmmp_sqr_(t, b, rn);
        mp_limb_t borrow = lmmp_sub_(s, s, 2 * rn, t, 2 * rn);
        if (n % 2 == 0) {
            TEST_CHECK_MSG(borrow == 0 && s[0] == 1 && norm_len(s, 2 * rn) == 1, "Cassini +1");
        } else {
            // -1 即 2*rn 个全 F limb
            bool allf = true;
            for (mp_size_t i = 0; i < 2 * rn; ++i) allf = allf && (s[i] == LIMB_MAX);
            TEST_CHECK_MSG(borrow == 1 && allf, "Cassini -1");
        }

        lmmp_free(a); lmmp_free(b); lmmp_free(c); lmmp_free(t); lmmp_free(s); lmmp_free(d);
    }
}
