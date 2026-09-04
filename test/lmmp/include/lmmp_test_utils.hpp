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

#ifndef LMMP_TEST_UTILS_HPP
#define LMMP_TEST_UTILS_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "lmmp/lmmp.h"
#include "lmmp/lmmpn.h"
#include "u128.hpp"

namespace lmmp_test_utils {

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u128 = uint128_t;

constexpr u64 B_LO_MASK = UINT64_MAX;

inline u64 hi128(u128 x) { return (u64)(x >> 64); }
inline u64 lo128(u128 x) { return (u64)x; }

// 简单确定性 xorshift64 随机源，避免测试输入与库 RNG 互相影响。
inline u64 xorshift64(u64& s) {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
}

struct BigInt {
    std::vector<u64> d;  // little-endian，高位无零（0 表示为 {0}）

    BigInt() : d(1, 0) {}
    BigInt(u64 v) : d(1, v) { trim(); }
    BigInt(const u64* p, size_t n) : d(p, p + n) { trim(); }

    void trim() {
        while (d.size() > 1 && d.back() == 0) d.pop_back();
        if (d.empty()) d.push_back(0);
    }

    size_t size() const { return d.size(); }
    bool is_zero() const { return d.size() == 1 && d[0] == 0; }
    u64& operator[](size_t i) { return d[i]; }
    const u64& operator[](size_t i) const { return d[i]; }

    static int cmp(const BigInt& a, const BigInt& b) {
        if (a.d.size() != b.d.size())
            return a.d.size() < b.d.size() ? -1 : 1;
        for (size_t i = a.d.size(); i-- > 0;) {
            if (a.d[i] != b.d[i]) return a.d[i] < b.d[i] ? -1 : 1;
        }
        return 0;
    }

    bool operator<(const BigInt& o) const { return cmp(*this, o) < 0; }
    bool operator<=(const BigInt& o) const { return cmp(*this, o) <= 0; }
    bool operator>(const BigInt& o) const { return cmp(*this, o) > 0; }
    bool operator>=(const BigInt& o) const { return cmp(*this, o) >= 0; }
    bool operator==(const BigInt& o) const { return cmp(*this, o) == 0; }
    bool operator!=(const BigInt& o) const { return cmp(*this, o) != 0; }

    // 要求 a >= b
    static BigInt sub_abs(const BigInt& a, const BigInt& b) {
        BigInt r;
        r.d.assign(a.d.size(), 0);
        u64 borrow = 0;
        for (size_t i = 0; i < b.d.size(); ++i) {
            u64 x = a.d[i];
            u128 diff = (u128)x - (u128)b.d[i] - borrow;
            r.d[i] = (u64)diff;
            borrow = (diff >> 64) ? 1 : 0;
        }
        for (size_t i = b.d.size(); i < a.d.size(); ++i) {
            u64 x = a.d[i];
            u128 diff = (u128)x - borrow;
            r.d[i] = (u64)diff;
            borrow = (diff >> 64) ? 1 : 0;
        }
        r.trim();
        return r;
    }

    static BigInt add_abs(const BigInt& a, const BigInt& b) {
        const BigInt& big = a.d.size() >= b.d.size() ? a : b;
        const BigInt& small = a.d.size() >= b.d.size() ? b : a;
        BigInt r;
        r.d.assign(big.d.size() + 1, 0);
        u64 carry = 0;
        for (size_t i = 0; i < big.d.size(); ++i) {
            u128 s = (u128)big.d[i] + (i < small.d.size() ? small.d[i] : 0) + carry;
            r.d[i] = lo128(s);
            carry = hi128(s);
        }
        r.d[big.d.size()] = carry;
        r.trim();
        return r;
    }

    static BigInt add_small(const BigInt& a, u64 v) {
        BigInt r = a;
        u128 s = (u128)r.d[0] + v;
        r.d[0] = lo128(s);
        u64 carry = hi128(s);
        size_t i = 1;
        while (carry) {
            if (i >= r.d.size()) r.d.push_back(0);
            u128 s2 = (u128)r.d[i] + carry;
            r.d[i] = lo128(s2);
            carry = hi128(s2);
            ++i;
        }
        r.trim();
        return r;
    }

    static BigInt sub_small(const BigInt& a, u64 v) {
        BigInt r = a;
        u64 x = r.d[0];
        u64 z = x - v;
        r.d[0] = z;
        u64 borrow = (x < v) ? 1 : (z > x ? 1 : 0);
        size_t i = 1;
        while (borrow) {
            if (i >= r.d.size()) r.d.push_back(0);
            x = r.d[i];
            z = x - borrow;
            r.d[i] = z;
            borrow = z > x ? 1 : 0;
            ++i;
        }
        r.trim();
        return r;
    }

    static BigInt mul_school(const BigInt& a, const BigInt& b) {
        BigInt r;
        r.d.assign(a.d.size() + b.d.size(), 0);
        for (size_t j = 0; j < b.d.size(); ++j) {
            u64 carry = 0;
            u64 y = b.d[j];
            for (size_t i = 0; i < a.d.size(); ++i) {
                u128 t = (u128)a.d[i] * y + r.d[i + j] + carry;
                r.d[i + j] = lo128(t);
                carry = hi128(t);
            }
            size_t k = j + a.d.size();
            while (carry) {
                if (k >= r.d.size()) r.d.push_back(0);
                u128 t = (u128)r.d[k] + carry;
                r.d[k] = lo128(t);
                carry = hi128(t);
                ++k;
            }
        }
        r.trim();
        return r;
    }

    static BigInt sqr_school(const BigInt& a) { return mul_school(a, a); }

    static BigInt shl_bits(const BigInt& a, size_t bits) {
        BigInt r;
        size_t limbs = bits / 64;
        size_t rem = bits % 64;
        r.d.assign(a.d.size() + limbs + (rem ? 1 : 0), 0);
        for (size_t i = 0; i < a.d.size(); ++i) {
            u128 t = (u128)a.d[i] << rem;
            r.d[i + limbs] |= lo128(t);
            if (rem) r.d[i + limbs + 1] |= hi128(t);
        }
        r.trim();
        return r;
    }

    static BigInt shr_bits(const BigInt& a, size_t bits) {
        size_t limbs = bits / 64;
        size_t rem = bits % 64;
        if (limbs >= a.d.size()) return BigInt(0);
        BigInt r;
        r.d.assign(a.d.size() - limbs, 0);
        if (rem == 0) {
            std::memcpy(r.d.data(), a.d.data() + limbs, r.d.size() * 8);
        } else {
            for (size_t i = 0; i < r.d.size(); ++i) {
                u64 lo = a.d[i + limbs] >> rem;
                u64 hi = (i + limbs + 1 < a.d.size()) ? (a.d[i + limbs + 1] << (64 - rem)) : 0;
                r.d[i] = lo | hi;
            }
        }
        r.trim();
        return r;
    }

    static BigInt div_small(const BigInt& a, u64 v, u64& rem) {
        if (v == 0) return BigInt(0);
        BigInt q;
        q.d.assign(a.d.size(), 0);
        u64 r = 0;
        for (size_t i = a.d.size(); i-- > 0;) {
            u128 cur = ((u128)r << 64) | a.d[i];
            q.d[i] = (u64)(cur / v);
            r = (u64)(cur % v);
        }
        q.trim();
        rem = r;
        return q;
    }

    static u64 mod_small(const BigInt& a, u64 v) {
        u64 rem = 0;
        div_small(a, v, rem);
        return rem;
    }

    // Knuth-D 基 2^64 除法。仅用于交叉验证小/中等规模输入。
    static BigInt divmod_school(const BigInt& num, const BigInt& den, BigInt& rem) {
        if (den.is_zero()) return BigInt(0);
        int c = cmp(num, den);
        if (c < 0) {
            rem = num;
            return BigInt(0);
        }
        if (den.d.size() == 1) {
            u64 r = 0;
            BigInt q = div_small(num, den.d[0], r);
            rem = BigInt(r);
            return q;
        }

        int shift = __builtin_clzll(den.d.back());
        BigInt u = shl_bits(num, (size_t)shift);
        BigInt v = shl_bits(den, (size_t)shift);
        size_t n = v.d.size();
        size_t m = u.d.size() - n;  // 商的长度为 m+1

        BigInt q;
        q.d.assign(m + 1, 0);

        // 确保 u 有 j+n+1 个有效访问位（最坏情况下 m+1+n）。
        while (u.d.size() < n + m + 2) u.d.push_back(0);

        for (size_t j = m + 1; j-- > 0;) {
            u64 qhat;
            u64 rhat;
            u64 v1 = v.d[n - 1];
            u64 v0 = v.d[n - 2];

            if (u.d[j + n] == v1) {
                // 标准 Knuth-D 特判，避免 qhat == B 溢出。
                qhat = UINT64_MAX;
                u128 nr = (u128)u.d[j + n - 1] + v1;
                rhat = lo128(nr);
                if (hi128(nr)) {
                    goto skip_adjust;
                }
            } else {
                u128 top = ((u128)u.d[j + n] << 64) | u.d[j + n - 1];
                qhat = (u64)(top / v1);
                rhat = (u64)(top % v1);
            }

            while (true) {
                u128 lhs = (u128)qhat * v0;
                u128 rhs = ((u128)rhat << 64) | u.d[j + n - 2];
                if (lhs <= rhs) break;
                --qhat;
                u128 nr = (u128)rhat + v1;
                rhat = lo128(nr);
                if (hi128(nr)) break;
            }

        skip_adjust:
            // u[j..j+n] -= qhat * v
            // 借位必须独立传播：若把 borrow 折进 lo128(t)（y = lo + borrow），
            // 当 lo == 2^64-1 且 borrow == 1 时 y 回绕为 0，借位静默丢失。
            u64 borrow = 0;
            u64 carry = 0;
            for (size_t i = 0; i < n; ++i) {
                u128 t = (u128)qhat * v.d[i] + carry;
                carry = hi128(t);
                u64 x = u.d[j + i];
                u64 y = lo128(t);
                u64 z = x - y - borrow;
                borrow = (x < y) | ((x == y) & borrow);
                u.d[j + i] = z;
            }
            {
                u64 x = u.d[j + n];
                u64 y = carry + borrow;
                u64 z = x - y;
                u64 new_borrow = (x < y) ? 1 : (z > x ? 1 : 0);
                u.d[j + n] = z;
                borrow = new_borrow;
            }

            if (borrow) {
                --qhat;
                u64 c = 0;
                for (size_t i = 0; i < n; ++i) {
                    u128 t = (u128)u.d[j + i] + v.d[i] + c;
                    u.d[j + i] = lo128(t);
                    c = hi128(t);
                }
                u.d[j + n] += c;
            }

            q.d[j] = qhat;
        }

        rem.d.assign(n, 0);
        for (size_t i = 0; i < n; ++i) rem.d[i] = u.d[i];
        rem = shr_bits(rem, (size_t)shift);
        q.trim();
        return q;
    }

    static BigInt pow(const BigInt& base, u64 exp) {
        BigInt r(1), b = base;
        while (exp) {
            if (exp & 1) r = mul_school(r, b);
            exp >>= 1;
            if (exp) b = sqr_school(b);
        }
        return r;
    }

    static BigInt factorial(u32 n) {
        BigInt r(1);
        for (u32 i = 2; i <= n; ++i) r = mul_school(r, BigInt(i));
        return r;
    }

    static BigInt gcd_euclid(BigInt a, BigInt b) {
        while (!b.is_zero()) {
            BigInt r;
            divmod_school(a, b, r);
            a = b;
            b = r;
        }
        return a;
    }
};

// 把库的 limb 数组复制为 BigInt，并去除前导零。
inline BigInt from_limbs(const mp_limb_t* p, mp_size_t n) {
    if (n <= 0) return BigInt(0);
    return BigInt(p, (size_t)n);
}

inline void to_limbs(const BigInt& x, mp_ptr p, mp_size_t n) {
    std::memset(p, 0, (size_t)n * sizeof(mp_limb_t));
    for (size_t i = 0; i < x.d.size() && i < (size_t)n; ++i) p[i] = x.d[i];
}

inline std::string to_hex(const BigInt& x) {
    std::string s;
    s.reserve(x.d.size() * 16 + 2);
    char buf[24];
    std::snprintf(buf, sizeof(buf), "0x");
    s += buf;
    for (size_t i = x.d.size(); i-- > 0;) {
        std::snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)x.d[i]);
        s += buf;
    }
    return s;
}

// 十进制解析（独立于库 from_str）。
inline BigInt from_decimal(const std::string& s) {
    BigInt r(0);
    for (char ch : s) {
        if (ch < '0' || ch > '9') continue;
        r = BigInt::mul_school(r, BigInt(10));
        r = BigInt::add_small(r, (u64)(ch - '0'));
    }
    return r;
}

// 十进制输出（独立于库 to_str），仅用于小规模输入。
inline std::string to_decimal(const BigInt& x) {
    if (x.is_zero()) return "0";
    std::string out;
    BigInt t = x;
    while (!t.is_zero()) {
        u64 rem = 0;
        t = BigInt::div_small(t, 1000000000ull, rem);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%09llu", (unsigned long long)rem);
        out = std::string(buf) + out;
    }
    // 去掉前导零
    size_t pos = out.find_first_not_of('0');
    return pos == std::string::npos ? "0" : out.substr(pos);
}

inline bool limb_vec_eq(const BigInt& a, const mp_limb_t* p, mp_size_t n) {
    BigInt b = from_limbs(p, n);
    return a == b;
}

}  // namespace lmmp_test_utils

#endif  // LMMP_TEST_UTILS_HPP
