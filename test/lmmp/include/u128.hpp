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

namespace u128_detail {

inline int clz64(std::uint64_t x) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(x);
#else
    int n = 0;
    if ((x >> 32) == 0) { n += 32; x <<= 32; }
    if ((x >> 48) == 0) { n += 16; x <<= 16; }
    if ((x >> 56) == 0) { n += 8; x <<= 8; }
    if ((x >> 60) == 0) { n += 4; x <<= 4; }
    if ((x >> 62) == 0) { n += 2; x <<= 2; }
    if ((x >> 63) == 0) { n += 1; }
    return n;
#endif
}

}  // namespace u128_detail

#if defined(__SIZEOF_INT128__)

// 编译器内置 128 位整数（GCC/Clang，x64/arm64 原生）。
// 加减乘移位比较直接映射硬件指令；除法/取模刻意不使用内置的 / 与 %
// （那会展开为 __udivti3/__umodti3 编译器运行时库调用），统一走本类的
// 软件除法：divmod_by_u64 半字 Knuth-D 核心 + 位级回退长除法。
__extension__ typedef unsigned __int128 u128_native;

class uint128_t {
   private:
    u128_native v;

   public:
    // 构造
    uint128_t() noexcept : v(0) {}
    uint128_t(u128_native x) noexcept : v(x) {}
    uint128_t(std::uint64_t low) noexcept : v(low) {}
    uint128_t(std::uint64_t high, std::uint64_t low) noexcept
        : v(((u128_native)high << 64) | low) {}
    // 限 <= 64 位整型：避免有符号 128 位落到此模板被截断为低 64 位
    //（无符号 128 位由上面的非模板构造精确接住）。
    template <typename T,
              typename = std::enable_if_t<std::is_integral_v<T> && (sizeof(T) <= 8)>>
    uint128_t(T val) noexcept : v((u128_native)(std::uint64_t)val) {}

    // 显式转换
    explicit operator std::uint64_t() const noexcept { return (std::uint64_t)v; }
    explicit operator bool() const noexcept { return v != 0; }
    explicit operator u128_native() const noexcept { return v; }

    // 比较
    bool operator==(const uint128_t& rhs) const noexcept { return v == rhs.v; }
    bool operator!=(const uint128_t& rhs) const noexcept { return v != rhs.v; }
    bool operator<(const uint128_t& rhs) const noexcept { return v < rhs.v; }
    bool operator>(const uint128_t& rhs) const noexcept { return v > rhs.v; }
    bool operator<=(const uint128_t& rhs) const noexcept { return v <= rhs.v; }
    bool operator>=(const uint128_t& rhs) const noexcept { return v >= rhs.v; }

    // 位运算
    uint128_t operator~() const noexcept { return uint128_t(~v); }
    uint128_t operator&(const uint128_t& rhs) const noexcept { return uint128_t(v & rhs.v); }
    uint128_t operator|(const uint128_t& rhs) const noexcept { return uint128_t(v | rhs.v); }
    uint128_t operator^(const uint128_t& rhs) const noexcept { return uint128_t(v ^ rhs.v); }
    uint128_t& operator&=(const uint128_t& rhs) noexcept {
        v &= rhs.v;
        return *this;
    }
    uint128_t& operator|=(const uint128_t& rhs) noexcept {
        v |= rhs.v;
        return *this;
    }
    uint128_t& operator^=(const uint128_t& rhs) noexcept {
        v ^= rhs.v;
        return *this;
    }

    // 移位（逻辑）。内置 128 位对 >= 128 的移位计数是 UB，必须先拦截。
    uint128_t operator<<(unsigned shift) const noexcept {
        if (shift >= 128) return uint128_t(0);
        return uint128_t(v << shift);
    }
    uint128_t operator>>(unsigned shift) const noexcept {
        if (shift >= 128) return uint128_t(0);
        return uint128_t(v >> shift);
    }
    uint128_t& operator<<=(unsigned shift) noexcept {
        *this = *this << shift;
        return *this;
    }
    uint128_t& operator>>=(unsigned shift) noexcept {
        *this = *this >> shift;
        return *this;
    }

    // 加减乘（原生）
    uint128_t& operator+=(const uint128_t& rhs) noexcept {
        v += rhs.v;
        return *this;
    }
    uint128_t& operator-=(const uint128_t& rhs) noexcept {
        v -= rhs.v;
        return *this;
    }
    uint128_t& operator*=(const uint128_t& rhs) noexcept {
        v *= rhs.v;
        return *this;
    }
    friend uint128_t operator+(uint128_t a, const uint128_t& b) noexcept {
        return uint128_t(a.v + b.v);
    }
    friend uint128_t operator-(uint128_t a, const uint128_t& b) noexcept {
        return uint128_t(a.v - b.v);
    }
    friend uint128_t operator*(uint128_t a, const uint128_t& b) noexcept {
        return uint128_t(a.v * b.v);
    }

    // 除法与取模：软件实现（不触内置 128 位 / 与 %）。
    uint128_t& operator/=(const uint128_t& rhs) {
        uint128_t q, r;
        divmod(*this, rhs, q, r);
        v = q.v;
        return *this;
    }
    uint128_t& operator%=(const uint128_t& rhs) {
        uint128_t q, r;
        divmod(*this, rhs, q, r);
        v = r.v;
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

    // 自增/自减
    uint128_t operator++(int) noexcept {
        uint128_t old = *this;
        v += 1;
        return old;
    }
    uint128_t operator--(int) noexcept {
        uint128_t old = *this;
        v -= 1;
        return old;
    }
    uint128_t& operator++() noexcept {
        v += 1;
        return *this;
    }
    uint128_t& operator--() noexcept {
        v -= 1;
        return *this;
    }

    /**
     * 128÷64 快速核心：求 (n1:n0) / d 与余数。
     * 前置条件：d != 0 且 n1 < d（等价于商与余数都 < 2^64）。
     *
     * 半字（基 2^32）两步 Knuth-D 试商，仅使用 64 位硬件除法与乘法：
     * 规格化后除数为 (v1:v0)（v1 最高位为 1），对 4 个半字被除数依次
     * 求高、低 32 位商。试商 q = 高两部/v1，用 q*v0 > rh*2^32+w1 判定
     * 需回退（与 BigInt::divmod_school 的 64 位 limb 版同构，正确性
     * 论证同 Knuth Algorithm D）；每步修正至多 2 次。
     */
    static void divmod_by_u64(std::uint64_t n1, std::uint64_t n0, std::uint64_t d,
                              std::uint64_t& q_out, std::uint64_t& r_out) noexcept {
        const int s = u128_detail::clz64(d);
        const std::uint64_t dv = d << s;
        // n1 < d 保证 n < d*2^64，规格化左移后仍 < dv*2^64 <= 2^128，不回绕。
        u128_native nn = (((u128_native)n1) << 64) | n0;
        nn <<= (unsigned)s;
        const std::uint64_t a1 = (std::uint64_t)(nn >> 64);  // < dv
        const std::uint64_t a0 = (std::uint64_t)nn;
        const std::uint64_t v1 = dv >> 32;  // >= 2^31
        const std::uint64_t v0 = dv & 0xFFFFFFFFu;
        const std::uint64_t w1 = a0 >> 32;
        const std::uint64_t w0 = a0 & 0xFFFFFFFFu;

        // 高 32 位商：估计 a1/v1，乘法下界检验回退。
        // 循环顶部不变量 rh < 2^32，故 (rh<<32)+w1 与 qh*v0 均不回绕
        //（qh > 0xFFFFFFFF 时短路，不取乘积）。
        std::uint64_t qh = a1 / v1;
        std::uint64_t rh = a1 % v1;
        while (qh > 0xFFFFFFFFu || qh * v0 > (rh << 32) + w1) {
            --qh;
            rh += v1;
            if (rh > 0xFFFFFFFFu) break;  // rh >= 2^32 时乘积比较必不成立
        }
        // t = (a1:w1) - qh*dv，退出条件保证 0 <= t < 2^64。
        const std::uint64_t t =
            (std::uint64_t)(((((u128_native)a1) << 32) | w1) - (u128_native)qh * dv);

        // 低 32 位商：同构。t < dv 保证 t 的半字高位 <= v1，估计定理适用。
        std::uint64_t ql = t / v1;
        std::uint64_t rl = t % v1;
        while (ql > 0xFFFFFFFFu || ql * v0 > (rl << 32) + w0) {
            --ql;
            rl += v1;
            if (rl > 0xFFFFFFFFu) break;
        }
        const u128_native rem = ((((u128_native)t) << 32) | w0) - (u128_native)ql * dv;
        r_out = (std::uint64_t)(rem >> s);
        q_out = (qh << 32) | ql;
    }

    /**
     * 通用 128÷128 除法。除数为 0 抛 domain_error。
     * 除数 < 2^64 时走 divmod_by_u64（必要时先原生除高部拆两段），
     * 否则商必 < 2^64，用位级长除法（冷路径）。
     */
    static void divmod(const uint128_t& dividend, const uint128_t& divisor,
                       uint128_t& quotient, uint128_t& remainder) {
        if (divisor.v == 0) throw std::domain_error("division by zero");
        const u128_native n = dividend.v;
        const u128_native d = divisor.v;
        if ((std::uint64_t)(d >> 64) == 0) {
            const std::uint64_t d64 = (std::uint64_t)d;
            const std::uint64_t n1 = (std::uint64_t)(n >> 64);
            const std::uint64_t n0 = (std::uint64_t)n;
            std::uint64_t q, r;
            if (n1 < d64) {
                divmod_by_u64(n1, n0, d64, q, r);
            } else {
                // 商 >= 2^64：n1 = q1*d + rh 精确分解，余下 (rh:n0)/d 走核心。
                const std::uint64_t q1 = n1 / d64;
                const std::uint64_t rh = n1 % d64;
                std::uint64_t q0;
                divmod_by_u64(rh, n0, d64, q0, r);
                quotient.v = ((u128_native)q1 << 64) | q0;
                remainder.v = r;
                return;
            }
            quotient.v = q;
            remainder.v = r;
            return;
        }
        // 除数 >= 2^64：位级长除法。
        u128_native q = 0;
        u128_native r = 0;
        for (int i = 127; i >= 0; --i) {
            r = (r << 1) | ((n >> i) & 1);
            if (r >= d) {
                r -= d;
                q |= (u128_native)1 << i;
            }
        }
        quotient.v = q;
        remainder.v = r;
    }
};

#else  // !defined(__SIZEOF_INT128__)

// 纯软件 uint128（无编译器内置 128 位时的可移植回退）。
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

        // 将 temp 左移到尽可能接近 rem 但不超过 rem。
        // temp 最高位为 1 时不可再左移（会溢出丢位），此时 2*temp >= 2^128 > rem，
        // temp 已是对齐上限；仅当循环因 temp > rem 退出时才需要回退一次。
        int shift = 0;
        while (temp <= rem && !(temp >> 127)) {
            temp <<= 1;
            ++shift;
        }
        if (temp > rem) {
            temp >>= 1;
            --shift;
        }

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

    /**
     * 128÷64 核心（与内置 128 位分支同名同契约）。
     * 前置条件：d != 0 且 n1 < d（商与余数都 < 2^64）。
     * 回退实现直接走通用 divmod（慢但独立于被测路径）。
     */
    static void divmod_by_u64(uint64_t n1, uint64_t n0, uint64_t d, uint64_t& q_out,
                              uint64_t& r_out) {
        uint128_t q, r;
        divmod(uint128_t(n1, n0), uint128_t(d), q, r);
        q_out = (uint64_t)q;
        r_out = (uint64_t)r;
    }
};

#endif  // defined(__SIZEOF_INT128__)

#endif  // U128_HPP
