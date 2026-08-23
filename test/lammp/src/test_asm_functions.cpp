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

#include "lammp/lmmpn.h"
#include "lammp/impl/mparam.h"
#include "lammp_test.hpp"
#include "lammp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lammp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed, bool top_bit = true) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0 && top_bit) p[n - 1] |= (u64)1 << 63;
}

BigInt pow_B(size_t n) {
    return BigInt::shl_bits(BigInt(1), n * 64);
}

void write_qdr_limbs(mp_ptr p, size_t n, const BigInt& q, const BigInt& d, const BigInt& r) {
    BigInt v = BigInt::mul_school(q, d);
    v = BigInt::add_abs(v, r);
    for (size_t i = 0; i < n; ++i) p[i] = (i < v.d.size()) ? v.d[i] : 0;
}

}  // namespace

TEST_CASE("asm/addsub", add_nc_sub_nc_random) {
    u64 seed = 0xa11ce01234567890ull;
    for (mp_size_t n = 1; n <= 65; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr b = alloc_limbs(n);
            mp_ptr d = alloc_limbs(n);
            random_limbs(a, n, seed, false);
            random_limbs(b, n, seed, false);
            BigInt ba(a, n), bb(b, n);

            for (u64 c : {0u, 1u}) {
                u64 cy = lmmp_add_nc_(d, a, b, n, c);
                BigInt sum = BigInt::add_abs(BigInt::add_abs(ba, bb), BigInt(c));
                TEST_CHECK_EQ(cy, sum.d.size() > (size_t)n ? sum.d[n] : 0u);
                TEST_CHECK(limb_vec_eq(BigInt(sum.d.data(), (size_t)n), d, n));

                u64 bw = lmmp_sub_nc_(d, a, b, n, c);
                BigInt rhs = BigInt::add_small(bb, c);
                bool borrow = ba < rhs;
                TEST_CHECK_EQ(bw, borrow ? 1u : 0u);
                if (!borrow) {
                    BigInt diff = BigInt::sub_abs(ba, rhs);
                    TEST_CHECK(limb_vec_eq(diff, d, n));
                } else {
                    BigInt diff = BigInt::sub_abs(pow_B(n), BigInt::sub_abs(rhs, ba));
                    TEST_CHECK(limb_vec_eq(diff, d, n));
                }
            }

            lmmp_free(a); lmmp_free(b); lmmp_free(d);
        }
    }
}

TEST_CASE("asm/mul1", mul_1_addmul_submul_random_inplace) {
    u64 seed = 0xb0b0c0d0e0f01020ull;
    for (mp_size_t n = 1; n <= 65; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr d = alloc_limbs(n + 1);
            random_limbs(a, n, seed, false);
            BigInt ba(a, n);
            u64 x = xorshift64(seed) | 1;

            u64 cy = lmmp_mul_1_(d, a, n, x);
            BigInt prod = BigInt::mul_school(ba, BigInt(x));
            TEST_CHECK(limb_vec_eq(BigInt(prod.d.data(), (size_t)n), d, n));
            TEST_CHECK_EQ(cy, prod.d.size() > (size_t)n ? prod.d[n] : 0u);

            mp_ptr a2 = alloc_limbs(n);
            std::memcpy(a2, a, n * 8);
            u64 cy2 = lmmp_mul_1_(a2, a2, n, x);
            TEST_CHECK_EQ(cy2, cy);
            TEST_CHECK(limb_vec_eq(BigInt(prod.d.data(), (size_t)n), a2, n));

            mp_ptr acc = alloc_limbs(n + 1);
            random_limbs(acc, n, seed, false);
            BigInt bacc(acc, n);
            BigInt sum = BigInt::add_abs(bacc, prod);
            mp_ptr acc2 = alloc_limbs(n + 1);
            std::memcpy(acc2, acc, n * 8); acc2[n] = 0;
            u64 c2 = lmmp_addmul_1_(acc2, a, n, x);
            TEST_CHECK_EQ(c2, sum.d.size() > (size_t)n ? sum.d[n] : 0u);
            TEST_CHECK(limb_vec_eq(BigInt(sum.d.data(), (size_t)n), acc2, n));

            std::memcpy(acc2, acc, n * 8); acc2[n] = 0;
            u64 c2b = lmmp_addmul_1_(acc2, acc2, n, x);
            BigInt prod_ap = BigInt::mul_school(bacc, BigInt(x));
            BigInt sum_ap = BigInt::add_abs(bacc, prod_ap);
            TEST_CHECK_EQ(c2b, sum_ap.d.size() > (size_t)n ? sum_ap.d[n] : 0u);
            TEST_CHECK(limb_vec_eq(BigInt(sum_ap.d.data(), (size_t)n), acc2, n));

            BigInt prod_low;
            size_t pl = prod.d.size() < (size_t)n ? prod.d.size() : (size_t)n;
            prod_low.d.assign(pl, 0);
            for (size_t i = 0; i < pl; ++i) prod_low.d[i] = prod.d[i];
            prod_low.trim();
            u64 prod_hi = prod.d.size() > (size_t)n ? prod.d[n] : 0;
            u64 expect_borrow = prod_hi + (bacc < prod_low ? 1u : 0u);

            std::memcpy(acc2, acc, n * 8); acc2[n] = 0;
            u64 c3 = lmmp_submul_1_(acc2, a, n, x);
            TEST_CHECK_EQ(c3, expect_borrow);
            if (bacc >= prod) {
                BigInt diff3 = BigInt::sub_abs(bacc, prod);
                TEST_CHECK(limb_vec_eq(diff3, acc2, n));
            }

            std::memcpy(acc2, acc, n * 8); acc2[n] = 0;
            u64 c3b = lmmp_submul_1_(acc2, acc2, n, x);
            BigInt prod_ip = BigInt::mul_school(bacc, BigInt(x));
            BigInt prod_ip_low;
            size_t pl2 = prod_ip.d.size() < (size_t)n ? prod_ip.d.size() : (size_t)n;
            prod_ip_low.d.assign(pl2, 0);
            for (size_t i = 0; i < pl2; ++i) prod_ip_low.d[i] = prod_ip.d[i];
            prod_ip_low.trim();
            u64 expect_borrow_ip = (prod_ip.d.size() > (size_t)n ? prod_ip.d[n] : 0u)
                                    + (bacc < prod_ip_low ? 1u : 0u);
            TEST_CHECK_EQ(c3b, expect_borrow_ip);
            if (bacc >= prod_ip) {
                BigInt diff_ip = BigInt::sub_abs(bacc, prod_ip);
                TEST_CHECK(limb_vec_eq(diff_ip, acc2, n));
            }

            lmmp_free(a); lmmp_free(a2); lmmp_free(d);
            lmmp_free(acc); lmmp_free(acc2);
        }
    }
}

TEST_CASE("asm/shift", shl_shr_carry_random) {
    u64 seed = 0xc1c2c3c4c5c6c7c8ull;
    for (mp_size_t n = 1; n <= 17; ++n) {
        for (int iter = 0; iter < 5; ++iter) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr d = alloc_limbs(n);
            random_limbs(a, n, seed, false);
            BigInt ba(a, n);

            for (mp_size_t s = 0; s < 64; ++s) {
                BigInt expected_shl = BigInt::shl_bits(ba, s);
                BigInt expected_shr = BigInt::shr_bits(ba, s);

                u64 r1 = lmmp_shl_(d, a, n, s);
                TEST_CHECK(limb_vec_eq(BigInt(expected_shl.d.data(), expected_shl.d.size() > (size_t)n ? (size_t)n : expected_shl.d.size()), d, n));
                u64 expect_ret1 = 0;
                if (s > 0) {
                    BigInt high = BigInt::shr_bits(expected_shl, n * 64);
                    expect_ret1 = (high.d.size() >= 1) ? high.d[0] : 0;
                }
                TEST_CHECK_EQ(r1, expect_ret1);

                u64 r2 = lmmp_shr_(d, a, n, s);
                TEST_CHECK(limb_vec_eq(expected_shr, d, n));
                u64 expect_ret2 = 0;
                for (size_t k = 0; k < (size_t)s; ++k) {
                    u64 bit = (ba.d[k / 64] >> (k % 64)) & 1;
                    expect_ret2 |= bit << (64 - s + k);
                }
                TEST_CHECK_EQ(r2, expect_ret2);

                u64 c_shl = (s == 0) ? 0 : (xorshift64(seed) & (((u64)1 << s) - 1));
                u64 r3 = lmmp_shl_c_(d, a, n, s, c_shl);
                BigInt expected_shlc = expected_shl;
                if (s > 0) expected_shlc.d[0] |= c_shl;
                TEST_CHECK(limb_vec_eq(BigInt(expected_shlc.d.data(), (size_t)n), d, n));
                TEST_CHECK_EQ(r3, expect_ret1);

                u64 c_shr = (s == 0) ? 0 : (xorshift64(seed) << (64 - s));
                u64 r4 = lmmp_shr_c_(d, a, n, s, c_shr);
                BigInt expected_shrc = expected_shr;
                if (expected_shrc.d.size() < (size_t)n) expected_shrc.d.resize(n, 0);
                if (s > 0) expected_shrc.d[n - 1] |= c_shr;
                TEST_CHECK(limb_vec_eq(expected_shrc, d, n));
                TEST_CHECK_EQ(r4, expect_ret2);
            }

            lmmp_free(a); lmmp_free(d);
        }
    }
}

TEST_CASE("asm/shift", shlnot_not_addshl1_subshl1_random) {
    u64 seed = 0xd1d2d3d4d5d6d7d8ull;
    for (mp_size_t n = 1; n <= 65; ++n) {
        for (int iter = 0; iter < 10; ++iter) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr b = alloc_limbs(n);
            mp_ptr d = alloc_limbs(n + 1);
            random_limbs(a, n, seed, false);
            random_limbs(b, n, seed, false);
            BigInt ba(a, n), bb(b, n);

            lmmp_not_(d, a, n);
            for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(d[i], ~a[i]);

            for (mp_size_t s = 0; s < 64; ++s) {
                BigInt shifted = BigInt::shl_bits(ba, s);
                u64 ret = lmmp_shlnot_(d, a, n, s);
                for (mp_size_t i = 0; i < n; ++i) {
                    u64 expected = (i < (mp_size_t)shifted.d.size()) ? ~shifted.d[i] : ~0ull;
                    TEST_CHECK_EQ(d[i], expected);
                }
                u64 expect_ret = 0;
                if (s > 0) {
                    BigInt high = BigInt::shr_bits(shifted, n * 64);
                    expect_ret = (high.d.size() >= 1) ? high.d[0] : 0;
                }
                TEST_CHECK_EQ(ret, expect_ret);
            }

            BigInt bshl1 = BigInt::shl_bits(bb, 1);
            BigInt sum = BigInt::add_abs(ba, bshl1);
            u64 cy = lmmp_addshl1_n_(d, a, b, n);
            TEST_CHECK_EQ(cy, sum.d.size() > (size_t)n ? sum.d[n] : 0u);
            TEST_CHECK(limb_vec_eq(BigInt(sum.d.data(), (size_t)n), d, n));

            BigInt bshl1_low;
            if (bshl1.d.size() > (size_t)n) bshl1_low = BigInt(bshl1.d.data(), (size_t)n);
            else bshl1_low = bshl1;
            u64 mb = bshl1.d.size() > (size_t)n ? bshl1.d[n] : 0u;
            bool borrow = ba < bshl1_low;
            u64 bw = lmmp_subshl1_n_(d, a, b, n);
            TEST_CHECK_EQ(bw, mb + (borrow ? 1u : 0u));
            if (!borrow) {
                BigInt diff = BigInt::sub_abs(ba, bshl1_low);
                TEST_CHECK(limb_vec_eq(diff, d, n));
            } else {
                BigInt diff = BigInt::sub_abs(pow_B(n), BigInt::sub_abs(bshl1_low, ba));
                TEST_CHECK(limb_vec_eq(diff, d, n));
            }

            lmmp_free(a); lmmp_free(b); lmmp_free(d);
        }
    }
}

TEST_CASE("asm/rsh1", shr1add_shr1sub_random) {
    u64 seed = 0xe1e2e3e4e5e6e7e8ull;
    for (mp_size_t n = 1; n <= 65; ++n) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr b = alloc_limbs(n);
            mp_ptr d = alloc_limbs(n);
            random_limbs(a, n, seed, false);
            random_limbs(b, n, seed, false);
            BigInt ba(a, n), bb(b, n);

            for (u64 c : {0u, 1u}) {
                BigInt sum = BigInt::add_abs(BigInt::add_abs(ba, bb), BigInt(c));
                u64 l = lmmp_shr1add_nc_(d, a, b, n, c);
                BigInt sum_low = BigInt(sum.d.data(), (size_t)n);
                BigInt shifted = BigInt::shr_bits(sum_low, 1);
                if (shifted.d.size() < (size_t)n) shifted.d.resize(n, 0);
                if (sum.d.size() > (size_t)n) shifted.d[n - 1] |= (u64)1 << 63;
                TEST_CHECK_EQ(l, (sum_low.d[0] & 1));
                TEST_CHECK(limb_vec_eq(shifted, d, n));

                BigInt rhs = BigInt::add_small(bb, c);
                bool borrow = ba < rhs;
                u64 l2 = lmmp_shr1sub_nc_(d, a, b, n, c);
                BigInt diff;
                if (!borrow) {
                    diff = BigInt::sub_abs(ba, rhs);
                } else {
                    diff = BigInt::sub_abs(pow_B(n), BigInt::sub_abs(rhs, ba));
                }
                BigInt shifted2 = BigInt::shr_bits(diff, 1);
                if (shifted2.d.size() < (size_t)n) shifted2.d.resize(n, 0);
                if (borrow) shifted2.d[n - 1] |= (u64)1 << 63;
                TEST_CHECK_EQ(l2, (diff.d[0] & 1));
                TEST_CHECK(limb_vec_eq(shifted2, d, n));
            }

            lmmp_free(a); lmmp_free(b); lmmp_free(d);
        }
    }
}

TEST_CASE("asm/div", div_3_2_random) {
    u64 seed = 0xf1f2f3f4f5f6f7f8ull;
    for (int iter = 0; iter < 500; ++iter) {
        mp_ptr np = alloc_limbs(3);
        mp_ptr dp = alloc_limbs(2);
        dp[1] = xorshift64(seed) | LIMB_B_2;
        dp[0] = xorshift64(seed);
        BigInt d(dp, 2);
        u64 q = xorshift64(seed);
        BigInt r(0);
        r.d[0] = dp[0] - 1;
        if (BigInt::cmp(r, d) >= 0) { r.d[0] = dp[0] - 1; }
        write_qdr_limbs(np, 3, BigInt(q), d, r);

        mp_limb_t inv21 = lmmp_inv_2_1_(dp[1], dp[0]);
        mp_limb_t qq = lmmp_div_3_2_(np, dp, inv21);
        TEST_CHECK_EQ(qq, q);
        TEST_CHECK_EQ(np[0], r.d[0]);
        TEST_CHECK_EQ(np[1], r.d.size() > 1 ? r.d[1] : 0u);

        lmmp_free(np); lmmp_free(dp);
    }
}

TEST_CASE("asm/div", div_1_mod_1_random_extra) {
    u64 seed = 0x0123456789abcdefull;
    for (mp_size_t n = 1; n <= 70; ++n) {
        for (int iter = 0; iter < 10; ++iter) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr q = alloc_limbs(n + 1);
            random_limbs(a, n, seed, false);
            BigInt ba(a, n);
            u64 x = xorshift64(seed) | 1;

            u64 r_mod = lmmp_mod_1_(a, n, x);
            u64 expect_r = BigInt::mod_small(ba, x);
            TEST_CHECK_EQ(r_mod, expect_r);

            u64 r_div = lmmp_div_1_(q, a, n, x);
            TEST_CHECK_EQ(r_div, expect_r);
            u64 dummy = 0;
            BigInt expect_q = BigInt::div_small(ba, x, dummy);
            TEST_CHECK(limb_vec_eq(expect_q, q, n));

            if (n > 1 && x >= LIMB_B_2) {
                mp_ptr a2 = alloc_limbs(n);
                std::memcpy(a2, a, n * 8);
                u64 qh = lmmp_div_1_s_(q, a2, n, x);
                std::vector<u64> full(q, q + n - 1);
                full.push_back(qh);
                BigInt got_q(full.data(), full.size());
                u64 r_ref = 0;
                BigInt expect_qs = BigInt::div_small(ba, x, r_ref);
                TEST_CHECK_MSG(got_q == expect_qs, "div_1_s quotient");
                TEST_CHECK_EQ(a2[0], r_ref);
                lmmp_free(a2);
            }

            lmmp_free(a); lmmp_free(q);
        }
    }
}

TEST_CASE("asm/div", div_2_mod_2_random_extra) {
    u64 seed = 0x0badf00dcafefaceull;
    for (mp_size_t n = 2; n <= 40; ++n) {
        for (int iter = 0; iter < 10; ++iter) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr q = alloc_limbs(n + 1);
            mp_ptr b = alloc_limbs(2);
            random_limbs(a, n, seed, false);
            b[1] = xorshift64(seed) | LIMB_B_2;
            b[0] = xorshift64(seed);
            BigInt ba(a, n), bd(b, 2);

            BigInt expect_rem;
            BigInt expect_q = BigInt::divmod_school(ba, bd, expect_rem);

            mp_ptr a2 = alloc_limbs(n);
            mp_ptr b2 = alloc_limbs(2);
            std::memcpy(b2, b, 2 * 8);
            std::memcpy(a2, a, n * 8);
            lmmp_mod_2_(a2, n, b2);
            BigInt br(b2, 2);
            TEST_CHECK(br == expect_rem);

            std::memcpy(b2, b, 2 * 8);
            std::memcpy(a2, a, n * 8);
            lmmp_div_2_(q, a2, n, b2);
            BigInt bq(q, n - 1);
            BigInt brm(b2, 2);
            TEST_CHECK_MSG(bq == expect_q, "div_2 quotient");
            TEST_CHECK_MSG(brm == expect_rem, "div_2 remainder");

            if (n > 2) {
                std::memcpy(b2, b, 2 * 8);
                std::memcpy(a2, a, n * 8);
                mp_limb_t qh = lmmp_div_2_s_(q, a2, n, b2);
                std::vector<u64> full(q, q + n - 2);
                full.push_back(qh);
                BigInt got_q(full.data(), full.size());
                BigInt got_r(a2, 2);
                TEST_CHECK_MSG(got_q == expect_q, "div_2_s quotient");
                TEST_CHECK_MSG(got_r == expect_rem, "div_2_s remainder");
            }

            lmmp_free(a); lmmp_free(a2); lmmp_free(b2); lmmp_free(q); lmmp_free(b);
        }
    }
}

TEST_CASE("asm/inv", inv_1_inv_2_1_random) {
    u64 seed = 0x1111222233334444ull;
    const u128 max128(UINT64_MAX, UINT64_MAX);
    const u128 B = (u128)1 << 64;
    for (int i = 0; i < 2000; ++i) {
        u64 x = xorshift64(seed) | LIMB_B_2;
        u64 inv = lmmp_inv_1_(x);
        u128 q = (u128)inv + B;
        u128 prod = q * x;
        TEST_CHECK_MSG(prod <= max128 && prod > max128 - x, "inv_1 property");

        u64 xh = xorshift64(seed) | LIMB_B_2;
        u64 xl = xorshift64(seed);
        mp_limb_t inv21 = lmmp_inv_2_1_(xh, xl);
        TEST_CHECK_MSG(inv21 != 0, "inv_2_1 nonzero");
    }
}
