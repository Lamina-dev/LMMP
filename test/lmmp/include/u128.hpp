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

#ifndef U128_HPP
#define U128_HPP

#include <cstdint>
#include <stdexcept>
#include <type_traits>

class uint128_t {
   private:
    uint64_t lo;  // low 64 bits
    uint64_t hi;  // high 64 bits

    // 64位乘法 → 128位（返回低64位和高64位）
    static inline void mul64_128(uint64_t a, uint64_t b, uint64_t& out_hi, uint64_t& out_lo) {
        uint32_t a0 = static_cast<uint32_t>(a);
        uint32_t a1 = static_cast<uint32_t>(a >> 32);
        uint32_t b0 = static_cast<uint32_t>(b);
        uint32_t b1 = static_cast<uint32_t>(b >> 32);

        uint64_t p0 = static_cast<uint64_t>(a0) * b0;
        uint64_t p1 = static_cast<uint64_t>(a0) * b1;
        uint64_t p2 = static_cast<uint64_t>(a1) * b0;
        uint64_t p3 = static_cast<uint64_t>(a1) * b1;

        uint64_t low = p0;
        uint64_t carry = 0;

        // 加 p1 << 32
        uint64_t p1_lo = static_cast<uint32_t>(p1);
        uint64_t p1_hi = p1 >> 32;
        uint64_t sum = low;
        low += (p1_lo << 32);
        if (low < sum)
            ++carry;

        // 加 p2 << 32
        uint64_t p2_lo = static_cast<uint32_t>(p2);
        uint64_t p2_hi = p2 >> 32;
        sum = low;
        low += (p2_lo << 32);
        if (low < sum)
            ++carry;

        uint64_t high = p3 + p1_hi + p2_hi + carry;
        out_lo = low;
        out_hi = high;
    }

   public:
    // 构造
    uint128_t() noexcept : lo(0), hi(0) {}
    uint128_t(uint64_t low) noexcept : lo(low), hi(0) {}
    uint128_t(uint64_t high, uint64_t low) noexcept : lo(low), hi(high) {}

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    uint128_t(T val) noexcept : lo(static_cast<uint64_t>(val)), hi(0) {}

    // 显式转换
    explicit operator uint64_t() const noexcept { return lo; }
    explicit operator bool() const noexcept { return lo || hi; }

    // 比较
    bool operator==(const uint128_t& rhs) const noexcept { return lo == rhs.lo && hi == rhs.hi; }
    bool operator!=(const uint128_t& rhs) const noexcept { return !(*this == rhs); }
    bool operator<(const uint128_t& rhs) const noexcept {
        if (hi != rhs.hi)
            return hi < rhs.hi;
        return lo < rhs.lo;
    }
    bool operator>(const uint128_t& rhs) const noexcept { return rhs < *this; }
    bool operator<=(const uint128_t& rhs) const noexcept { return !(*this > rhs); }
    bool operator>=(const uint128_t& rhs) const noexcept { return !(*this < rhs); }

    // 位运算
    uint128_t operator~() const noexcept { return uint128_t{~hi, ~lo}; }
    uint128_t operator&(const uint128_t& rhs) const noexcept { return uint128_t{hi & rhs.hi, lo & rhs.lo}; }
    uint128_t operator|(const uint128_t& rhs) const noexcept { return uint128_t{hi | rhs.hi, lo | rhs.lo}; }
    uint128_t operator^(const uint128_t& rhs) const noexcept { return uint128_t{hi ^ rhs.hi, lo ^ rhs.lo}; }
    uint128_t& operator&=(const uint128_t& rhs) noexcept {
        *this = *this & rhs;
        return *this;
    }
    uint128_t& operator|=(const uint128_t& rhs) noexcept {
        *this = *this | rhs;
        return *this;
    }
    uint128_t& operator^=(const uint128_t& rhs) noexcept {
        *this = *this ^ rhs;
        return *this;
    }

    // 移位（逻辑）
    uint128_t operator<<(unsigned shift) const noexcept {
        if (shift >= 128)
            return uint128_t(0);
        if (shift == 0)
            return *this;
        if (shift >= 64) {
            return uint128_t(lo << (shift - 64), 0);
        } else {
            uint64_t new_hi = (hi << shift) | (lo >> (64 - shift));
            uint64_t new_lo = lo << shift;
            return uint128_t(new_hi, new_lo);
        }
    }
    uint128_t operator>>(unsigned shift) const noexcept {
        if (shift >= 128)
            return uint128_t(0);
        if (shift == 0)
            return *this;
        if (shift >= 64) {
            return uint128_t(0, hi >> (shift - 64));
        } else {
            uint64_t new_hi = hi >> shift;
            uint64_t new_lo = (lo >> shift) | (hi << (64 - shift));
            return uint128_t(new_hi, new_lo);
        }
    }
    uint128_t& operator<<=(unsigned shift) noexcept {
        *this = *this << shift;
        return *this;
    }
    uint128_t& operator>>=(unsigned shift) noexcept {
        *this = *this >> shift;
        return *this;
    }

    // 加减
    uint128_t& operator+=(const uint128_t& rhs) noexcept {
        uint64_t sum_lo = lo + rhs.lo;
        uint64_t carry = (sum_lo < lo) ? 1 : 0;
        hi = hi + rhs.hi + carry;
        lo = sum_lo;
        return *this;
    }
    uint128_t& operator-=(const uint128_t& rhs) noexcept {
        uint64_t diff_lo = lo - rhs.lo;
        uint64_t borrow = (diff_lo > lo) ? 1 : 0;  // 如果被减数小于减数则借位
        hi = hi - rhs.hi - borrow;
        lo = diff_lo;
        return *this;
    }
    friend uint128_t operator+(uint128_t a, const uint128_t& b) noexcept {
        a += b;
        return a;
    }
    friend uint128_t operator-(uint128_t a, const uint128_t& b) noexcept {
        a -= b;
        return a;
    }

    // 乘法
    uint128_t& operator*=(const uint128_t& rhs) noexcept {
        // 拆分为高64位和低64位
        uint64_t a_lo = lo, a_hi = hi;
        uint64_t b_lo = rhs.lo, b_hi = rhs.hi;

        // 计算四个64位乘积
        uint64_t p0_hi, p0_lo;
        mul64_128(a_lo, b_lo, p0_hi, p0_lo);

        uint64_t p1_hi, p1_lo;
        mul64_128(a_lo, b_hi, p1_hi, p1_lo);

        uint64_t p2_hi, p2_lo;
        mul64_128(a_hi, b_lo, p2_hi, p2_lo);

        // 忽略 a_hi * b_hi 因为对低128位无贡献
        // 结果低64位 = p0_lo
        // 结果高64位 = p0_hi + p1_lo + p2_lo  (进位自动截断)
        uint64_t res_lo = p0_lo;
        uint64_t res_hi = p0_hi + p1_lo + p2_lo;

        lo = res_lo;
        hi = res_hi;
        return *this;
    }
    friend uint128_t operator*(uint128_t a, const uint128_t& b) noexcept {
        a *= b;
        return a;
    }

    // 除法与取模（长除法）
    static void divmod(const uint128_t& dividend, const uint128_t& divisor, uint128_t& quotient, uint128_t& remainder) {
        if (divisor == uint128_t(0))
            throw std::domain_error("division by zero");

        if (dividend < divisor) {
            quotient = uint128_t(0);
            remainder = dividend;
            return;
        }

        uint128_t q = 0;
        uint128_t rem = dividend;
        uint128_t temp = divisor;

        // 将 temp 左移到尽可能接近 rem 但不超过 rem
        int shift = 0;
        while (temp <= rem) {
            temp <<= 1;
            ++shift;
        }
        // 回退一次
        temp >>= 1;
        --shift;

        // 从高位到低位试商
        for (int i = shift; i >= 0; --i) {
            if (temp <= rem) {
                rem -= temp;
                q |= (uint128_t(1) << i);
            }
            temp >>= 1;
        }

        quotient = q;
        remainder = rem;
    }

    uint128_t& operator/=(const uint128_t& rhs) {
        uint128_t q, r;
        divmod(*this, rhs, q, r);
        *this = q;
        return *this;
    }
    uint128_t& operator%=(const uint128_t& rhs) {
        uint128_t q, r;
        divmod(*this, rhs, q, r);
        *this = r;
        return *this;
    }
    friend uint128_t operator/(uint128_t a, const uint128_t& b) {
        a /= b;
        return a;
    }
    friend uint128_t operator%(uint128_t a, const uint128_t& b) {
        a %= b;
        return a;
    }

    // 后缀自增/自减（简单实现）
    uint128_t operator++(int) noexcept {
        uint128_t old = *this;
        *this += uint128_t(1);
        return old;
    }
    uint128_t operator--(int) noexcept {
        uint128_t old = *this;
        *this -= uint128_t(1);
        return old;
    }
    uint128_t& operator++() noexcept {
        *this += uint128_t(1);
        return *this;
    }
    uint128_t& operator--() noexcept {
        *this -= uint128_t(1);
        return *this;
    }
};


#endif // U128_HPP