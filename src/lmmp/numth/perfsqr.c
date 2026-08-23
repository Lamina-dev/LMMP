/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"


#define MASK48 (0xFFFFFFFFFFFF)

#define B1 (LIMB_BITS / 4)
#define B2 (B1 * 2)
#define B3 (B1 * 3)

#define M1 ((1ULL << B1) - 1)
#define M2 ((1ULL << B2) - 1)
#define M3 ((1ULL << B3) - 1)

#define LOW0(n) ((n) & M3)
#define HIGH0(n) ((n) >> B3)

#define LOW1(n) (((n) & M2) << B1)
#define HIGH1(n) ((n) >> B2)

#define LOW2(n) (((n) & M1) << B2)
#define HIGH2(n) ((n) >> B1)

#define PARTS0(n) (LOW0(n) + HIGH0(n))
#define PARTS1(n) (LOW1(n) + HIGH1(n))
#define PARTS2(n) (LOW2(n) + HIGH2(n))

// a += val, if carry, add 1 to c
#define ADD(c, a, val)                 \
    do {                               \
        mp_limb_t new_a = (a) + (val); \
        (c) += new_a < (a);            \
        (a) = new_a;                   \
    } while (0)

mp_limb_t lmmp_mod_2p48sub1_(mp_srcptr p, mp_size_t n) {
    lmmp_param_assert(n > 0 && p != NULL);

    mp_limb_t c0 = 0, c1 = 0, c2 = 0;
    mp_limb_t a0 = 0, a1 = 0, a2 = 0;

    while (n >= 3) {
        ADD(c0, a0, p[0]);
        ADD(c1, a1, p[1]);
        ADD(c2, a2, p[2]);
        p += 3;
        n -= 3;
    }
    if (n == 2) {
        ADD(c0, a0, p[0]);
        ADD(c1, a1, p[1]);
    } else if (n == 1) {
        ADD(c0, a0, p[0]);
    }

    mp_limb_t res = PARTS0(a0) + PARTS1(a1) + PARTS2(a2) 
                  + PARTS1(c0) + PARTS2(c1) + PARTS0(c2);

    res = (res & MASK48) + (res >> B3);
    if (res >= MASK48)
        res -= MASK48;
    return res;
}

#undef B1
#undef B2
#undef B3
#undef M1
#undef M2
#undef M3
#undef LOW0
#undef HIGH0
#undef LOW1
#undef HIGH1
#undef LOW2
#undef HIGH2
#undef PARTS0
#undef PARTS1
#undef PARTS2
#undef ADD

/*
    我们选择2^48-1作为模数只是因为其计算可以非常迅速，并且拥有一组非常好的因数分解
        2^48-1 = 9 * 5 * 7 * 13 * 17 * 97 * 241 * 257 * 673
    而下面的每个函数，都直接硬编码了完全平方数才可能具有的模数。当然也不需要担心这
    么多的判断会导致分支预测效率很低，因为编译器通常可以很好的将其优化为位图，并且
    由于位图很小，可能直接变成立即数。对于更长的数，我们手动计算了位图，直接计算地址，
    取出对应的bit，可以避免编译器将其优化为多分支结构。

    只有完全平方数以及部分非完全平方数可以通过，大部分非完全平方数都无法通过。
    在模2^48-1下，完全平方数可能的结果仅占到大约 0.277%，
    在模256下，完全平方数可能的结果仅占到大约 17.2%
*/

static inline bool is_perfsqr_p9(uchar r) {
    return r == 0 || r == 1 || r == 4 || r == 7;
}

static inline bool is_perfsqr_p5(uchar r) {
    return r == 0 || r == 1 || r == 4;
}

static inline bool is_perfsqr_p7(uchar r) {
    return r == 0 || r == 1 || r == 2 || r == 4;
}

static inline bool is_perfsqr_p13(uchar r) {
    return r == 0 || r == 1 || r == 3 || r == 4 || r == 9 || r == 10 || r == 12;
}

static inline bool is_perfsqr_p17(uchar r) {
    return r == 0 || r == 1 || r == 2 || r == 4 || r == 8 || r == 9 || r == 13 || r == 15 || r == 16;
}

static inline bool is_perfsqr_p97(uchar r) {
    return r ==  0 || r ==  1 || r ==  2 || r ==  3 || r ==  4 || r ==  6 || r ==  8 || r ==  9 || r == 11 || r == 12 ||
           r == 16 || r == 18 || r == 22 || r == 24 || r == 25 || r == 27 || r == 31 || r == 32 || r == 33 || r == 35 ||
           r == 36 || r == 43 || r == 44 || r == 47 || r == 48 || r == 49 || r == 50 || r == 53 || r == 54 || r == 61 ||
           r == 62 || r == 64 || r == 65 || r == 66 || r == 70 || r == 72 || r == 73 || r == 75 || r == 79 || r == 81 ||
           r == 85 || r == 86 || r == 88 || r == 89 || r == 91 || r == 93 || r == 94 || r == 95 || r == 96;
}

static inline bool is_perfsqr_p241(ushort r) {
    static const uint64_t p241[] = {0x3C67A3116B15977F, 0x2FD21C174C8FA909, 0x98F24257C4CBA0E1, 0x0001FBA6A35A2317};
    ushort elem = r / 64;
    lmmp_param_assert(elem < 4);
    ushort bit = r % 64;
    return (p241[elem] >> bit) & 1ULL;
}

static inline bool is_perfsqr_p257(ushort r) {
    static const uint64_t p257[] = {0x7E16541DE6E7AB17, 0x1F76811C93128359, 0x6B052324E205BBE3, 0xA3579D9EE0A9A1FA,
                                     0x0000000000000001};
    ushort elem = r / 64;
    lmmp_param_assert(elem < 5);
    ushort bit = r % 64;
    return (p257[elem] >> bit) & 1ULL;
}

static inline bool is_perfsqr_p673(ushort r) {
    static const uint64_t p673[] = {0x85F744B13FA573DF, 0xC231D5979ABA4F21, 0xE944C76E98DD0C01, 0xD20E0F2BD993E915,
                                     0x616259FB225208AB, 0x7E691A18F8B7B47C, 0x53C1C12F54412913, 0xDB8C8A5EA25F266F,
                                     0xA6AE310E00C2EC65, 0x348BBE8613C97567, 0x00000001EF3A97F2};
    ushort elem = r / 64;
    lmmp_param_assert(elem < 11);
    ushort bit = r % 64;
    return (p673[elem] >> bit) & 1ULL;
}

static inline bool is_perfsqr_p256(uchar r) {
    static const uint64_t p256[] = {0x0202021202030213, 0x0202021202020213, 0x0202021202030212, 0x0202021202020212};
    ushort elem = r / 64;
    lmmp_param_assert(elem < 4);
    ushort bit = r % 64;
    return (p256[elem] >> bit) & 1ULL;
}

bool lmmp_perfsqr_filter_1_(mp_limb_t p) {
    lmmp_param_assert(p > 0);
    mp_limb_t a = p % 256;
    if (!is_perfsqr_p256(a)) return false;
    p = p % MASK48;
    return is_perfsqr_p9(p % 9) && is_perfsqr_p5(p % 5) && is_perfsqr_p7(p % 7) && is_perfsqr_p13(p % 13) &&
           is_perfsqr_p17(p % 17) && is_perfsqr_p97(p % 97) && is_perfsqr_p241(p % 241) &&
           is_perfsqr_p257(p % 257) && is_perfsqr_p673(p % 673);
}

bool lmmp_perfsqr_filter_(mp_srcptr p, mp_size_t n) {
    lmmp_param_assert(n > 0 && p != NULL);
    lmmp_param_assert(p[n - 1] > 0);
    if (n == 1) return lmmp_perfsqr_filter_1_(p[0]);
    mp_limb_t a = p[0] % 256;
    if (!is_perfsqr_p256(a))
        return false;
    mp_limb_t r = lmmp_mod_2p48sub1_(p, n);
    return is_perfsqr_p9(r % 9) && is_perfsqr_p5(r % 5) && is_perfsqr_p7(r % 7) && is_perfsqr_p13(r % 13) &&
           is_perfsqr_p17(r % 17) && is_perfsqr_p97(r % 97) && is_perfsqr_p241(r % 241) &&
           is_perfsqr_p257(r % 257) && is_perfsqr_p673(r % 673);
}

bool lmmp_perfsqr_(mp_srcptr p, mp_size_t n) {
    lmmp_param_assert(n > 0 && p != NULL);
    lmmp_param_assert(p[n - 1] > 0);
    bool filter, ret;

    filter = lmmp_perfsqr_filter_(p, n);
    if (filter == false) return false;

    TEMP_DECL;
    mp_ptr restrict tp = TALLOC_TYPE(n + 2, mp_limb_t);
    mp_ptr restrict dstr = tp + n / 2 + 1;
    lmmp_sqrt_(tp, dstr, p, n, 0);
    ret = lmmp_zero_q_(dstr, n / 2 + 1);
    TEMP_FREE;
    return ret;
}