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

#include <cstring>
#include <vector>

using namespace lmmp_test_utils;
using u16 = std::uint16_t;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

BigInt ref_factorial(u32 n) {
    BigInt r(1);
    for (u32 i = 2; i <= n; ++i) r = BigInt::mul_school(r, BigInt(i));
    return r;
}

BigInt ref_product_range(u32 lo, u32 hi) {
    BigInt r(1);
    for (u32 i = lo; i <= hi; ++i) r = BigInt::mul_school(r, BigInt(i));
    return r;
}

bool is_prime_small(u32 n) {
    if (n < 2) return false;
    for (u32 d = 2; (u64)d * d <= n; ++d) if (n % d == 0) return false;
    return true;
}

}  // namespace

TEST_CASE("numth/fac", factorial) {
    for (u32 n : {0, 1, 2, 5, 10, 20, 50, 100, 200}) {
        BigInt expect = ref_factorial(n);
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_factorial_size_(n, &bits);
        TEST_CHECK_MSG(need >= (mp_size_t)expect.d.size(), "factorial_size enough");
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = lmmp_factorial_(dst, bits, need, n);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "factorial value");
        lmmp_free(dst);
    }
}

TEST_CASE("numth/fac_extern", double_hyper_super_primefac) {
    // 双阶乘
    for (u32 n : {0, 1, 2, 3, 10, 21, 50, 107}) {
        BigInt expect(1);
        for (u32 i = (n % 2 == 0 ? 2 : 1); i <= n; i += 2) expect = BigInt::mul_school(expect, BigInt(i));
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_2factorial_size_(n, &bits);
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = lmmp_2factorial_(dst, bits, need, n);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "2factorial value");
        lmmp_free(dst);
    }

    // hyper factorial: prod k^k
    for (u16 n : {1, 3, 5, 8, 20}) {
        BigInt expect(1);
        for (u16 k = 1; k <= n; ++k) expect = BigInt::mul_school(expect, BigInt::pow(BigInt(k), k));
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_hyperfac_size_(n, &bits);
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = lmmp_hyperfac_(dst, bits, need, n);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "hyperfac value");
        lmmp_free(dst);
    }

    // super factorial: prod k!
    for (u16 n : {1, 3, 5, 8, 30}) {
        BigInt expect(1);
        for (u16 k = 1; k <= n; ++k) expect = BigInt::mul_school(expect, ref_factorial(k));
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_superfac_size_(n, &bits);
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = lmmp_superfac_(dst, bits, need, n);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "superfac value");
        lmmp_free(dst);
    }

    // prime factorial: prod primes <= n
    for (u32 n : {2, 3, 10, 20, 50, 700}) {
        BigInt expect(1);
        for (u32 p = 2; p <= n; ++p) if (is_prime_small(p)) expect = BigInt::mul_school(expect, BigInt(p));
        mp_size_t need = lmmp_primefac_size_(n);
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = lmmp_primefac_(dst, need, n);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "primefac value");
        lmmp_free(dst);
    }
}

TEST_CASE("numth/number", nPr_nCr_multinomial_arith) {
    // nPr
    for (ulong n : {0, 1, 5, 10, 20, 50, 100}) {
        for (ulong r = 0; r <= n; r += (n > 20 ? 20 : 1)) {
            BigInt expect = ref_product_range((u32)(n - r + 1), (u32)n);
            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_nPr_size_(n, r, &bits);
            mp_ptr dst = alloc_limbs((size_t)need + 2);
            mp_size_t rn = lmmp_nPr_(dst, bits, need, n, r);
            TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "nPr value");
            lmmp_free(dst);
        }
    }

    // nCr（要求 r <= n/2）
    for (u32 n : {0, 1, 5, 10, 20, 50, 100}) {
        for (u32 r = 0; r <= n / 2; ++r) {
            BigInt fn = ref_factorial(n);
            BigInt fr = ref_factorial(r);
            BigInt fnr = ref_factorial(n - r);
            BigInt rem;
            BigInt expect = BigInt::divmod_school(fn, BigInt::mul_school(fr, fnr), rem);
            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_nCr_size_(n, r, &bits);
            mp_ptr dst = alloc_limbs((size_t)need + 2);
            mp_size_t rn = lmmp_nCr_(dst, bits, need, n, r);
            TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "nCr value");
            lmmp_free(dst);
        }
    }

    // multinomial
    {
        u32 r[] = {1, 2, 3};
        u32 n_sum = 6;
        BigInt fn = ref_factorial(n_sum);
        BigInt den = BigInt::mul_school(BigInt::mul_school(ref_factorial(r[0]), ref_factorial(r[1])), ref_factorial(r[2]));
        BigInt rem;
        BigInt expect = BigInt::divmod_school(fn, den, rem);
        ulong n = 0;
        mp_size_t need = lmmp_multinomial_size_(r, 3, &n);
        TEST_CHECK_EQ(n, (ulong)n_sum);
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = lmmp_multinomial_(dst, need, (uint)n, r, 3);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "multinomial value");
        lmmp_free(dst);
    }

    // arith_seqprod: x(x+m)...(x+n*m)
    for (u32 x : {1, 3, 10}) {
        for (u32 m : {2, 5, 10}) {
            for (u32 n : {1, 3, 10}) {
                if ((u64)x + (u64)n * m > 100000) continue;
                BigInt expect(1);
                for (u32 i = 0; i <= n; ++i) expect = BigInt::mul_school(expect, BigInt(x + i * m));
                mp_size_t need = lmmp_arith_seqprod_size_(x, n, m);
                mp_ptr dst = alloc_limbs((size_t)need + 2);
                mp_size_t rn = lmmp_arith_seqprod_(dst, need, x, n, m);
                TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "arith_seqprod value");
                lmmp_free(dst);
            }
        }
    }
}

TEST_CASE("numth/trialdiv", trialdiv_remove) {
    // trialdiv
    for (mp_size_t n : {1, 2, 3, 5}) {
        mp_ptr p = alloc_limbs(n);
        u64 seed = 0x0123456789abcdeful + n;
        for (size_t i = 0; i < (size_t)n; ++i) p[i] = xorshift64(seed);
        if (p[n - 1] == 0) p[n - 1] = 1;
        ushort rn = 0;
        ushortp divs = lmmp_trialdiv_(p, n, 100, &rn);
        // 用参考方法验证：每个返回素数都能整除 p，且无遗漏小素数
        BigInt bp(p, n);
        for (ushort i = 0; i < rn; ++i) {
            u16 d = divs[i];
            TEST_CHECK_MSG(BigInt::mod_small(bp, d) == 0, "trialdiv divisor divides");
            TEST_CHECK_MSG(is_prime_small(d), "trialdiv divisor prime");
        }
        for (u32 d = 2; d <= 100; ++d) {
            if (is_prime_small(d) && BigInt::mod_small(bp, d) == 0) {
                bool found = false;
                for (ushort i = 0; i < rn; ++i) if (divs[i] == d) found = true;
                TEST_CHECK_MSG(found, "trialdiv found small prime divisor");
            }
        }
        if (divs) lmmp_free(divs);
        lmmp_free(p);
    }

    // remove：构造 np = dp^k * rest
    for (u64 dp : {2, 3, 5, 7}) {
        mp_ptr np = alloc_limbs(3);
        mp_ptr d = alloc_limbs(1);
        d[0] = dp;
        np[0] = 1;
        for (int k = 0; k < 4; ++k) {
            mp_size_t nn = 1;
            BigInt bn(1);
            for (int i = 0; i < k; ++i) {
                bn = BigInt::mul_school(bn, BigInt(dp));
            }
            bn = BigInt::mul_school(bn, BigInt(11));  // 不能被 dp 整除
            to_limbs(bn, np, 3);
            // 规范化长度：最高非零 limb 下标 + 1
            nn = 1;
            for (int i = 2; i >= 0; --i) if (np[i]) { nn = i + 1; break; }

            mp_size_t cnt = lmmp_remove_(np, &nn, d, 1);
            TEST_CHECK_EQ(cnt, (mp_size_t)k);
            BigInt rest(11);
            TEST_CHECK_MSG(from_limbs(np, nn) == rest, "remove rest value");
        }
        lmmp_free(np);
        lmmp_free(d);
    }
}
