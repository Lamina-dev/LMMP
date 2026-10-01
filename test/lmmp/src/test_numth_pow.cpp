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
#include <climits>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0) p[n - 1] |= (u64)1 << 63;
}

size_t bigint_bits(const BigInt& x) {
    u64 top = x.d.back();
    return (x.d.size() - 1) * 64 + (64 - __builtin_clzll(top == 0 ? 1 : top));
}

BigInt mod_school(const BigInt& x, const BigInt& m) {
    BigInt q, r;
    q = BigInt::divmod_school(x, m, r);
    (void)q;
    return r;
}

BigInt powmod_school(const BigInt& b, const BigInt& e, const BigInt& m) {
    BigInt r(1), base = mod_school(b, m);
    size_t ebits = bigint_bits(e);
    for (size_t i = 0; i < ebits; ++i) {
        if ((e.d[i / 64] >> (i % 64)) & 1)
            r = mod_school(BigInt::mul_school(r, base), m);
        if (i + 1 < ebits)
            base = mod_school(BigInt::sqr_school(base), m);
    }
    return r;
}

// 生成 n limb 奇模数（顶位置 1），b 归约到 < m 后写入 bp
void make_mod_and_base(mp_ptr mp, mp_ptr bp, size_t n, u64& seed) {
    random_limbs(mp, n, seed);
    mp[0] |= 1;
    random_limbs(bp, n, seed);
    BigInt b(bp, n), m(mp, n);
    to_limbs(mod_school(b, m), bp, (mp_size_t)n);
}

// powlo oracle：模 B^n 逐位梯子（每步截断低 n limb）
BigInt powlo_school(const BigInt& b, const BigInt& e, size_t n) {
    auto mask = [&](BigInt& x) {
        if (x.d.size() > n) x.d.resize(n);
        x.trim();
    };
    BigInt r(1), base = b;
    mask(base);
    size_t ebits = bigint_bits(e);
    for (size_t i = 0; i < ebits; ++i) {
        if ((e.d[i / 64] >> (i % 64)) & 1) {
            r = BigInt::mul_school(r, base);
            mask(r);
        }
        if (i + 1 < ebits) {
            base = BigInt::sqr_school(base);
            mask(base);
        }
    }
    return r;
}

}  // namespace

TEST_CASE("numth/pow", pow_1_variants) {
    struct { ulong base; ulong exp; } cases[] = {
        {1, 1}, {2, 63}, {3, 20}, {10, 18}, {0xf, 20},
        {0xff, 20}, {0xffff, 8}, {0xffffffffull, 5},
        {0x8000000000000000ull, 3}, {UINT64_MAX, 2}
    };
    for (auto c : cases) {
        BigInt expect = BigInt::pow(BigInt(c.base), c.exp);
        mp_size_t need = lmmp_pow_1_size_(c.base, c.exp);
        TEST_CHECK_MSG(need >= (mp_size_t)expect.d.size(), "pow_1_size enough");
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = lmmp_pow_1_(dst, need, c.base, c.exp);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "pow_1 value");
        lmmp_free(dst);
    }

    // 分派函数 u4/u8/u16/u32/u64
    auto check_dispatch = [&](ulong base, ulong exp, mp_size_t (*fn)(mp_ptr, mp_size_t, ulong, ulong)) {
        BigInt expect = BigInt::pow(BigInt(base), exp);
        mp_size_t need = lmmp_pow_1_size_(base, exp);
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = fn(dst, need, base, exp);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "u*_pow_1 value");
        lmmp_free(dst);
    };
    check_dispatch(3, 20, lmmp_u4_pow_1_);
    check_dispatch(0xab, 20, lmmp_u8_pow_1_);
    check_dispatch(0xabcd, 10, lmmp_u16_pow_1_);
    check_dispatch(0xabcdef01ull, 7, lmmp_u32_pow_1_);
    check_dispatch(0x8000000000000000ull, 3, lmmp_u64_pow_1_);
}

TEST_CASE("numth/pow", pow_basecase_win2_pow) {
    u64 seed = 0xf0f0f0f0c3c3c3c3ull;
    for (mp_size_t n : {1, 2, 5, 10, 30, 100}) {
        for (int iter = 0; iter < 4; ++iter) {
            mp_ptr base = alloc_limbs(n);
            random_limbs(base, n, seed);
            BigInt bbase(base, n);
            ulong exps[] = {1, 2, 3, 5, 7, 17, 31, 63};
            for (ulong exp : exps) {
                BigInt expect = BigInt::pow(bbase, exp);
                mp_size_t need = lmmp_pow_size_(base, n, exp);
                TEST_CHECK_MSG(need >= (mp_size_t)expect.d.size(), "pow_size enough");
                mp_ptr dst = alloc_limbs((size_t)need + 2);

                mp_size_t rn = 0;
                if (exp == 1) {
                    rn = lmmp_pow_win2_(dst, need, base, n, exp);
                } else if (exp % 2 == 1 && exp >= 3 && exp <= 17) {
                    // basecase 只接受奇数次且底数>1；win2 均可
                    mp_ptr dst2 = alloc_limbs((size_t)need + 2);
                    mp_size_t rn2 = lmmp_pow_basecase_(dst2, need, base, n, exp);
                    TEST_CHECK_MSG(from_limbs(dst2, rn2) == expect, "pow_basecase value");
                    lmmp_free(dst2);
                    rn = lmmp_pow_win2_(dst, need, base, n, exp);
                } else {
                    rn = lmmp_pow_win2_(dst, need, base, n, exp);
                }
                TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "pow_win2 value");

                mp_ptr dst3 = alloc_limbs((size_t)need + 2);
                mp_size_t rn3 = lmmp_pow_(dst3, need, base, n, exp);
                TEST_CHECK_MSG(from_limbs(dst3, rn3) == expect, "pow value");
                lmmp_free(dst); lmmp_free(dst3);
            }
            lmmp_free(base);
        }
    }
}

TEST_CASE("numth/powmod", redc_value) {
    u64 seed = 0x9e3779b97f4a7c15ull;
    // 覆盖 REDC 两条路径：小尺寸全积取高半，>= REDC_MERSENNE_THRESHOLD(401) 的梅森折叠
    for (size_t n : {1u, 2u, 3u, 5u, 9u, 17u, 33u, 47u, 48u, 49u, 50u, 52u, 402u, 405u, 430u}) {
        for (int iter = 0; iter < 3; ++iter) {
            mp_ptr m = alloc_limbs(n);
            mp_ptr a = alloc_limbs(n);
            mp_ptr b = alloc_limbs(n);
            mp_ptr t = alloc_limbs(2 * n);
            mp_ptr dst = alloc_limbs(n + 1);
            mp_ptr ninv = alloc_limbs(n);
            make_mod_and_base(m, a, n, seed);
            make_mod_and_base(m, b, n, seed);
            lmmp_mul_(t, a, (mp_size_t)n, b, (mp_size_t)n);

            lmmp_binvert_(ninv, m, (mp_size_t)n, (mp_size_t)n);
            // ninv = -m^(-1) mod B^n（两补码取负；逆元为奇，不会越顶）
            u64 carry = 1;
            for (size_t i = 0; i < n; ++i) {
                u64 x = ~ninv[i];
                u64 s = x + carry;
                carry = (s == 0) ? 1u : 0u;
                ninv[i] = s;
            }

            mp_limb_t cy = lmmp_redc_(dst, t, ninv, m, (mp_size_t)n);

            BigInt mm(m, n);
            BigInt u = from_limbs(dst, (mp_size_t)n);
            if (cy) u = BigInt::add_abs(u, BigInt::shl_bits(BigInt(1), n * 64));
            // u < 2m
            TEST_CHECK_MSG(BigInt::cmp(u, BigInt::add_abs(mm, mm)) < 0, "redc < 2m");
            // u * B^n mod m == t mod m
            BigInt un = BigInt::shl_bits(u, n * 64);
            BigInt tm = mod_school(from_limbs(t, (mp_size_t)(2 * n)), mm);
            TEST_CHECK_MSG(mod_school(un, mm) == tm, "redc congruence");
            TEST_CHECK_MSG(cy <= 1, "redc carry range");
            lmmp_free(m); lmmp_free(a); lmmp_free(b); lmmp_free(t); lmmp_free(dst); lmmp_free(ninv);
        }
    }
}

TEST_CASE("numth/powmod", powmod_odd_random) {
    u64 seed = 0x1234567890abcdefull;
    for (size_t n : {1u, 2u, 3u, 4u, 7u, 12u, 33u, 70u}) {
        for (int iter = 0; iter < 3; ++iter) {
            mp_ptr m = alloc_limbs(n);
            mp_ptr b = alloc_limbs(n);
            make_mod_and_base(m, b, n, seed);
            BigInt mm(m, n), bb(b, n);

            // 多种指数：边界短指数、单 limb、双 limb、三 limb（触发更大窗口）
            for (ulong e : {1ull, 2ull, 3ull, 5ull, 17ull, 64ull, 1000ull, 0xdeadbeefull, (ulong)ULONG_MAX}) {
                mp_ptr ep = alloc_limbs(1);
                mp_size_t en = 1;
                ep[0] = e;
                mp_ptr dst = alloc_limbs(n);
                lmmp_powmod_odd_(dst, b, ep, en, m, (mp_size_t)n);
                TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(bb, BigInt(e), mm), "powmod ulong exp");
                lmmp_free(dst); lmmp_free(ep);
            }

            mp_ptr ep = alloc_limbs(3);
            for (mp_size_t i = 0; i < 3; ++i) ep[i] = xorshift64(seed);
            ep[2] |= (u64)1 << 63;
            mp_ptr dst = alloc_limbs(n);
            lmmp_powmod_odd_(dst, b, ep, 3, m, (mp_size_t)n);
            TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(bb, BigInt(ep, 3), mm), "powmod 3-limb exp");
            lmmp_free(dst); lmmp_free(ep);

            lmmp_free(m); lmmp_free(b);
        }
    }
}

TEST_CASE("numth/powmod", powmod_corner_cases) {
    u64 seed = 0xfedcba9876543210ull;
    // 角点模数与底数：b=0、b=m-1、m=B^n-1（折叠提取的拼接二义点仅在此模数族可达）、小 m
    for (size_t n : {2u, 5u, 17u, 402u}) {
        mp_ptr m = alloc_limbs(n);
        mp_ptr b = alloc_limbs(n);
        mp_ptr dst = alloc_limbs(n);
        mp_ptr ep = alloc_limbs(2);
        BigInt mm;

        // m = B^n - 1（全 1）
        for (size_t i = 0; i < n; ++i) m[i] = ~(u64)0;
        mm = BigInt(m, n);
        random_limbs(b, n, seed);
        {
            BigInt bb = mod_school(BigInt(b, n), mm);
            to_limbs(bb, b, (mp_size_t)n);
        }
        ep[0] = 0x24f1a7e3c5590b8dull;
        ep[1] = 0x1;
        lmmp_powmod_odd_(dst, b, ep, 2, m, (mp_size_t)n);
        TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(BigInt(b, n), BigInt(ep, 2), mm), "powmod m=B^n-1");

        // b = m - 1 与 b = 0
        to_limbs(BigInt::sub_small(mm, 1), b, (mp_size_t)n);
        lmmp_powmod_odd_(dst, b, ep, 2, m, (mp_size_t)n);
        TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(BigInt(b, n), BigInt(ep, 2), mm), "powmod b=m-1");

        lmmp_zero(b, (mp_size_t)n);
        lmmp_powmod_odd_(dst, b, ep, 2, m, (mp_size_t)n);
        TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == BigInt(0), "powmod b=0");

        // 随机模数 + b = m-1
        random_limbs(m, n, seed);
        m[0] |= 1;
        mm = BigInt(m, n);
        to_limbs(BigInt::sub_small(mm, 1), b, (mp_size_t)n);
        lmmp_powmod_odd_(dst, b, ep, 2, m, (mp_size_t)n);
        TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(BigInt(b, n), BigInt(ep, 2), mm), "powmod random m b=m-1");

        lmmp_free(m); lmmp_free(b); lmmp_free(dst); lmmp_free(ep);
    }
}

TEST_CASE("numth/powmod", powmod_fold_large) {
    u64 seed = 0x51ed270b9a3c8f11ull;
    // n >= REDC_MERSENNE_THRESHOLD(401)：主路径走梅森折叠（m 侧变换缓存）
    for (size_t n : {402u, 410u}) {
        mp_ptr m = alloc_limbs(n);
        mp_ptr b = alloc_limbs(n);
        mp_ptr dst = alloc_limbs(n);
        make_mod_and_base(m, b, n, seed);
        BigInt mm(m, n), bb(b, n);

        // 短指数控制 oracle（schoolbook mul + Knuth-D div）耗时
        for (ulong e : {3ull, 0x9e3779b9ull}) {
            mp_ptr ep = alloc_limbs(1);
            ep[0] = e;
            lmmp_powmod_odd_(dst, b, ep, 1, m, (mp_size_t)n);
            TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(bb, BigInt(e), mm), "powmod fold short exp");
            lmmp_free(ep);
        }

        // 中等指数（96 bit，窗口 3~4）：折叠路径与滑窗联合覆盖
        mp_ptr ep = alloc_limbs(2);
        ep[0] = xorshift64(seed);
        ep[1] = xorshift64(seed) & 0xffffffffull;
        if (ep[1] == 0) ep[1] = 1;
        lmmp_powmod_odd_(dst, b, ep, 2, m, (mp_size_t)n);
        TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(bb, BigInt(ep, 2), mm), "powmod fold 96-bit exp");
        lmmp_free(ep);

        lmmp_free(m); lmmp_free(b); lmmp_free(dst);
    }
}

TEST_CASE("numth/pow", powlo_specialized_and_dispatch) {
    u64 seed = 0x1b873593f00dbeebull;
    for (int iter = 0; iter < 4; ++iter) {
        mp_ptr b1 = alloc_limbs(1);
        mp_ptr b2 = alloc_limbs(2);
        random_limbs(b1, 1, seed);
        random_limbs(b2, 2, seed);
        BigInt bb1(b1, 1), bb2(b2, 2);

        for (ulong e : {1ull, 2ull, 3ull, 5ull, 17ull, 64ull, 1000ull, 0xdeadbeefull, (ulong)ULONG_MAX}) {
            mp_ptr ep = alloc_limbs(1);
            mp_ptr d1 = alloc_limbs(1);
            mp_ptr d2 = alloc_limbs(2);
            ep[0] = e;

            lmmp_powlo_1_(d1, b1, ep, 1);
            TEST_CHECK_MSG(from_limbs(d1, 1) == powlo_school(bb1, BigInt(e), 1), "powlo_1 value");
            lmmp_powlo_2_(d2, b2, ep, 1);
            TEST_CHECK_MSG(from_limbs(d2, 2) == powlo_school(bb2, BigInt(e), 2), "powlo_2 value");

            // 泛型分派必须与特化一致
            lmmp_powlo_(d1, b1, 1, ep, 1);
            TEST_CHECK_MSG(from_limbs(d1, 1) == powlo_school(bb1, BigInt(e), 1), "powlo dispatch n=1");
            lmmp_powlo_(d2, b2, 2, ep, 1);
            TEST_CHECK_MSG(from_limbs(d2, 2) == powlo_school(bb2, BigInt(e), 2), "powlo dispatch n=2");

            lmmp_free(ep); lmmp_free(d1); lmmp_free(d2);
        }

        // 多 limb 指数（触发更大窗口）
        mp_ptr ep = alloc_limbs(3);
        for (mp_size_t i = 0; i < 3; ++i) ep[i] = xorshift64(seed);
        ep[2] |= (u64)1 << 63;
        BigInt ee(ep, 3);
        {
            mp_ptr d1 = alloc_limbs(1);
            mp_ptr d2 = alloc_limbs(2);
            lmmp_powlo_1_(d1, b1, ep, 3);
            TEST_CHECK_MSG(from_limbs(d1, 1) == powlo_school(bb1, ee, 1), "powlo_1 3-limb exp");
            lmmp_powlo_2_(d2, b2, ep, 3);
            TEST_CHECK_MSG(from_limbs(d2, 2) == powlo_school(bb2, ee, 2), "powlo_2 3-limb exp");
            lmmp_free(d1); lmmp_free(d2);
        }
        lmmp_free(ep);
        lmmp_free(b1); lmmp_free(b2);
    }

    // 泛型 n>=3：e=1 截断快路径（回归保护：旧实现此处越界读）
    for (size_t n : {3u, 5u, 17u}) {
        mp_ptr b = alloc_limbs(n);
        mp_ptr dst = alloc_limbs(n);
        mp_ptr ep = alloc_limbs(1);
        u64 s2 = 0xa5a5f00dull ^ n;
        random_limbs(b, n, s2);
        ep[0] = 1;
        lmmp_powlo_(dst, b, n, ep, 1);
        TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == BigInt(b, n), "powlo generic e=1");
        // 泛型中尺寸基本对拍（首次覆盖 powlo 主路径）
        ep[0] = 0xc0ffee123456789ull;
        lmmp_powlo_(dst, b, n, ep, 1);
        TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powlo_school(BigInt(b, n), BigInt(ep[0]), n), "powlo generic value");
        lmmp_free(b); lmmp_free(dst); lmmp_free(ep);
    }
}

TEST_CASE("numth/powmod", powmod_specialized_and_dispatch) {
    u64 seed = 0x9e3779b97f4a7c21ull;
    for (int iter = 0; iter < 4; ++iter) {
        // 1 limb 特化
        mp_ptr m1 = alloc_limbs(1);
        mp_ptr b1 = alloc_limbs(1);
        make_mod_and_base(m1, b1, 1, seed);
        BigInt mm1(m1, 1), bb1(b1, 1);

        // 2 limb 特化（模数高 limb 非零）
        mp_ptr m2 = alloc_limbs(2);
        mp_ptr b2 = alloc_limbs(2);
        make_mod_and_base(m2, b2, 2, seed);
        BigInt mm2(m2, 2), bb2(b2, 2);

        for (ulong e : {1ull, 2ull, 3ull, 5ull, 17ull, 64ull, 1000ull, 0xdeadbeefull, (ulong)ULONG_MAX}) {
            mp_ptr ep = alloc_limbs(1);
            ep[0] = e;
            mp_ptr d1 = alloc_limbs(1);
            mp_ptr d2 = alloc_limbs(2);
            lmmp_powmod_1_(d1, b1, ep, 1, m1[0]);
            TEST_CHECK_MSG(from_limbs(d1, 1) == powmod_school(bb1, BigInt(e), mm1), "powmod_1 value");
            lmmp_powmod_2_(d2, b2, ep, 1, m2);
            TEST_CHECK_MSG(from_limbs(d2, 2) == powmod_school(bb2, BigInt(e), mm2), "powmod_2 value");

            // 泛型分派
            lmmp_powmod_odd_(d1, b1, ep, 1, m1, 1);
            TEST_CHECK_MSG(from_limbs(d1, 1) == powmod_school(bb1, BigInt(e), mm1), "powmod dispatch n=1");
            lmmp_powmod_odd_(d2, b2, ep, 1, m2, 2);
            TEST_CHECK_MSG(from_limbs(d2, 2) == powmod_school(bb2, BigInt(e), mm2), "powmod dispatch n=2");

            lmmp_free(ep); lmmp_free(d1); lmmp_free(d2);
        }

        // 多 limb 指数
        mp_ptr ep = alloc_limbs(3);
        for (mp_size_t i = 0; i < 3; ++i) ep[i] = xorshift64(seed);
        ep[2] |= (u64)1 << 63;
        BigInt ee(ep, 3);
        {
            mp_ptr d1 = alloc_limbs(1);
            mp_ptr d2 = alloc_limbs(2);
            lmmp_powmod_1_(d1, b1, ep, 3, m1[0]);
            TEST_CHECK_MSG(from_limbs(d1, 1) == powmod_school(bb1, ee, mm1), "powmod_1 3-limb exp");
            lmmp_powmod_2_(d2, b2, ep, 3, m2);
            TEST_CHECK_MSG(from_limbs(d2, 2) == powmod_school(bb2, ee, mm2), "powmod_2 3-limb exp");
            lmmp_free(d1); lmmp_free(d2);
        }
        lmmp_free(ep);

        // b = 0 与 b = m-1
        mp_ptr z1 = alloc_limbs(1);
        mp_ptr z2 = alloc_limbs(2);
        mp_ptr d1 = alloc_limbs(1);
        mp_ptr d2 = alloc_limbs(2);
        mp_ptr e1 = alloc_limbs(1);
        e1[0] = 0x24f1a7e3c5590b8dull;
        lmmp_zero(z1, 1); lmmp_zero(z2, 2);
        lmmp_powmod_1_(d1, z1, e1, 1, m1[0]);
        TEST_CHECK_MSG(from_limbs(d1, 1) == BigInt(0), "powmod_1 b=0");
        lmmp_powmod_2_(d2, z2, e1, 1, m2);
        TEST_CHECK_MSG(from_limbs(d2, 2) == BigInt(0), "powmod_2 b=0");
        to_limbs(BigInt::sub_small(mm1, 1), z1, 1);
        to_limbs(BigInt::sub_small(mm2, 1), z2, 2);
        lmmp_powmod_1_(d1, z1, e1, 1, m1[0]);
        TEST_CHECK_MSG(from_limbs(d1, 1) == powmod_school(BigInt(z1, 1), BigInt(e1[0]), mm1), "powmod_1 b=m-1");
        lmmp_powmod_2_(d2, z2, e1, 1, m2);
        TEST_CHECK_MSG(from_limbs(d2, 2) == powmod_school(BigInt(z2, 2), BigInt(e1[0]), mm2), "powmod_2 b=m-1");
        lmmp_free(z1); lmmp_free(z2); lmmp_free(d1); lmmp_free(d2); lmmp_free(e1);

        lmmp_free(m1); lmmp_free(b1); lmmp_free(m2); lmmp_free(b2);
    }

    // 角点：m = B^2-1（2 limb 全 1）
    {
        mp_ptr m2 = alloc_limbs(2);
        mp_ptr b2 = alloc_limbs(2);
        mp_ptr d2 = alloc_limbs(2);
        mp_ptr ep = alloc_limbs(2);
        m2[0] = ~(u64)0; m2[1] = ~(u64)0;
        BigInt mm2(m2, 2);
        u64 s3 = 0xabcdef0123456789ull;
        random_limbs(b2, 2, s3);
        to_limbs(mod_school(BigInt(b2, 2), mm2), b2, 2);
        ep[0] = 0x51ed270b9a3c8f11ull;
        ep[1] = 0x1234;
        lmmp_powmod_2_(d2, b2, ep, 2, m2);
        TEST_CHECK_MSG(from_limbs(d2, 2) == powmod_school(BigInt(b2, 2), BigInt(ep, 2), mm2), "powmod_2 m=B^2-1");
        lmmp_powmod_odd_(d2, b2, ep, 2, m2, 2);
        TEST_CHECK_MSG(from_limbs(d2, 2) == powmod_school(BigInt(b2, 2), BigInt(ep, 2), mm2), "dispatch m=B^2-1");
        lmmp_free(m2); lmmp_free(b2); lmmp_free(d2); lmmp_free(ep);
    }

    // 泛型 e=1 快路径（n=5：拷贝即取模）
    {
        mp_ptr m = alloc_limbs(5);
        mp_ptr b = alloc_limbs(5);
        mp_ptr dst = alloc_limbs(5);
        mp_ptr ep = alloc_limbs(1);
        u64 s4 = 0x51ce25c0ff3351ceull;
        make_mod_and_base(m, b, 5, s4);
        ep[0] = 1;
        lmmp_powmod_odd_(dst, b, ep, 1, m, 5);
        TEST_CHECK_MSG(from_limbs(dst, 5) == BigInt(b, 5), "powmod generic e=1");
        lmmp_free(m); lmmp_free(b); lmmp_free(dst); lmmp_free(ep);
    }
}

// ---------------------------------------------------------------------------
// lmmp_powmod_：任意模数（含偶模数），CRT 合成路径
// ---------------------------------------------------------------------------

// 构造 2^k * odd 形状的偶模数写入 [mp,容量n]，mm 接收其值，返回实际 limb 长度
static size_t make_even_mod(size_t k, const BigInt& odd, mp_ptr mp, BigInt& mm) {
    mm = BigInt::shl_bits(odd, k);
    size_t n = mm.d.size();
    to_limbs(mm, mp, (mp_size_t)n);
    return n;
}

TEST_CASE("numth/powmod", powmod_two_adic_corners) {
    u64 seed = 0x243f6a8885a308d3ull;
    const size_t cap = 12;
    mp_ptr mp = alloc_limbs(cap);
    mp_ptr b = alloc_limbs(cap);
    mp_ptr dst = alloc_limbs(cap);
    mp_ptr ep = alloc_limbs(3);

    /* 2-adic 赋值 k：覆盖 63/64/65 跨 limb 边界、limb 对齐值（免掩码分支）
       与跨多 limb 的深移位；odd 部：纯 2 幂（m=2^k 与 B^n 族）、单 limb 奇、
       双 limb 随机奇（触发 nodd < nb2 的借位传播形态） */
    size_t ks[] = {1, 2, 3, 31, 62, 63, 64, 65, 66, 100, 127, 128, 129, 130,
                   191, 192, 193, 256, 257, 320, 383, 384, 447, 448, 511, 512, 576};
    for (size_t k : ks) {
        BigInt odds[4];
        odds[0] = BigInt(1);
        odds[1] = BigInt(3);
        odds[2] = BigInt(xorshift64(seed) | 1);
        {
            u64 t[2] = {xorshift64(seed) | 1, xorshift64(seed)};
            odds[3] = BigInt(t, 2);
        }

        for (const BigInt& odd : odds) {
            BigInt mm;
            size_t n = make_even_mod(k, odd, mp, mm);
            TEST_CHECK_MSG(n <= cap, "corner modulus fits buffer");

            struct { mp_size_t en; u64 w[3]; } exps[] = {
                {1, {1, 0, 0}},                 /* e=1 拷贝快路径 */
                {1, {2, 0, 0}},
                {1, {3, 0, 0}},
                {1, {17, 0, 0}},
                {1, {0xdeadbeefull, 0, 0}},
                {3, {0, 0, 1}},                 /* e=B^2：偶底数 en>1 短路 */
                {2, {0x9e3779b97f4a7c15ull, 0x1234, 0}},
            };
            for (const auto& ex : exps) {
                for (mp_size_t i = 0; i < ex.en; ++i) ep[i] = ex.w[i];
                BigInt ee(ex.w, ex.en);

                /* 底数角点：随机归约、强制偶（触发 2^k 部短路）、m-1、0 */
                BigInt bases[4];
                {
                    u64 s5 = seed ^ (u64)k * 0x9e3779b97f4a7c15ull ^ ex.w[0];
                    random_limbs(b, n, s5);
                    bases[0] = mod_school(BigInt(b, n), mm);
                    bases[1] = BigInt::sub_small(bases[0], bases[0].d[0] & 1); /* 清最低位（奇偶两种随机形态） */
                    bases[2] = BigInt::sub_small(mm, 1);
                    bases[3] = BigInt(0);
                }

                for (int bi = 0; bi < 4; ++bi) {
                    to_limbs(bases[bi], b, (mp_size_t)n);
                    lmmp_powmod_(dst, b, ep, ex.en, mp, (mp_size_t)n);
                    TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(bases[bi], ee, mm),
                                   "powmod even corner");
                }
            }
        }
    }
    lmmp_free(mp); lmmp_free(b); lmmp_free(dst); lmmp_free(ep);
}

TEST_CASE("numth/powmod", powmod_even_random) {
    u64 seed = 0x13198a2e03707344ull;
    for (size_t n : {1u, 2u, 3u, 5u, 9u, 17u, 33u, 70u}) {
        for (int iter = 0; iter < 6; ++iter) {
            mp_ptr m = alloc_limbs(n);
            mp_ptr b = alloc_limbs(n);
            mp_ptr dst = alloc_limbs(n);
            random_limbs(m, n, seed);
            /* 低 limb 形状轮换：随机偶 / 低 limb 全零（z>=1）/ ctz=1（m=2*odd） */
            switch (iter % 3) {
            case 0: m[0] &= ~(u64)1; break;
            case 1: if (n > 1) m[0] = 0; else m[0] = 0xdeadbee0ull; break;
            default: m[0] = (m[0] << 1) | 2; break;
            }
            BigInt mm(m, n);

            random_limbs(b, n, seed);
            BigInt bb = mod_school(BigInt(b, n), mm);
            if (iter % 2 == 1 && bb > BigInt(1))
                bb = BigInt::sub_small(bb, bb.d[0] & 1); /* 偶化，保持 >0 */
            to_limbs(bb, b, (mp_size_t)n);

            for (ulong e : {1ull, 2ull, 3ull, 5ull, 17ull, 64ull, 1000ull, 0xdeadbeefull, (ulong)ULONG_MAX}) {
                mp_ptr ep = alloc_limbs(1);
                ep[0] = e;
                lmmp_powmod_(dst, b, ep, 1, m, (mp_size_t)n);
                TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(bb, BigInt(e), mm), "powmod even random");
                lmmp_free(ep);
            }

            mp_ptr ep = alloc_limbs(3);
            for (mp_size_t i = 0; i < 3; ++i) ep[i] = xorshift64(seed);
            ep[2] |= (u64)1 << 63;
            lmmp_powmod_(dst, b, ep, 3, m, (mp_size_t)n);
            TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(bb, BigInt(ep, 3), mm), "powmod even 3-limb exp");
            lmmp_free(ep);

            lmmp_free(m); lmmp_free(b); lmmp_free(dst);
        }
    }

    /* 奇模数经 lmmp_powmod_ 分派须与 lmmp_powmod_odd_ 同值 */
    {
        u64 seed2 = 0xa4093822299f31d0ull;
        for (size_t n : {1u, 5u, 17u}) {
            mp_ptr m = alloc_limbs(n);
            mp_ptr b = alloc_limbs(n);
            mp_ptr dst = alloc_limbs(n);
            make_mod_and_base(m, b, n, seed2);
            BigInt mm(m, n), bb(b, n);
            mp_ptr ep = alloc_limbs(1);
            ep[0] = 0x9e3779b9ull;
            lmmp_powmod_(dst, b, ep, 1, m, (mp_size_t)n);
            TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(bb, BigInt(ep[0]), mm), "powmod odd dispatch");
            lmmp_free(m); lmmp_free(b); lmmp_free(dst); lmmp_free(ep);
        }
    }
}

TEST_CASE("numth/powmod", powmod_even_large_fold) {
    u64 seed = 0x9e3779b97f4a7c33ull;
    /* nodd >= REDC_MERSENNE_THRESHOLD(309)：偶模数内部的奇部走梅森折叠 REDC。
       m = 2*odd（k=1，nb2=1）与深移位 m = 2^65*odd 两种形态 */
    for (int shape = 0; shape < 2; ++shape) {
        size_t n = shape == 0 ? 403u : 405u;
        mp_ptr m = alloc_limbs(n);
        mp_ptr b = alloc_limbs(n);
        mp_ptr dst = alloc_limbs(n);
        random_limbs(m, n, seed);
        if (shape == 0) {
            m[0] = (m[0] << 1) | 2; /* k=1 */
        } else {
            m[0] = 0;
            m[1] = 2; /* k=65 */
        }
        BigInt mm(m, n);
        random_limbs(b, n, seed);
        BigInt bb = mod_school(BigInt(b, n), mm);
        to_limbs(bb, b, (mp_size_t)n);

        /* 短指数控制 oracle（schoolbook mul + Knuth-D div）耗时 */
        for (ulong e : {3ull, 0x9e3779b9ull}) {
            mp_ptr ep = alloc_limbs(1);
            ep[0] = e;
            lmmp_powmod_(dst, b, ep, 1, m, (mp_size_t)n);
            TEST_CHECK_MSG(from_limbs(dst, (mp_size_t)n) == powmod_school(bb, BigInt(e), mm), "powmod even fold");
            lmmp_free(ep);
        }
        lmmp_free(m); lmmp_free(b); lmmp_free(dst);
    }
}
