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

#include "../../../../include/lmmp/impl/longlong.h"
#include "../../../../include/lmmp/lmmpn.h"
#include "../../../../include/lmmp/numth.h"


int lmmp_limb_bits_(mp_limb_t x) {
    int k = 0;
    while (x) {
        x >>= 1;
        k++;
    }
    return k;
}

int lmmp_limb_popcnt_(mp_limb_t x) {
#if defined(__GNUC__) || defined(__clang__)
    mp_limb_t count;
#if defined(LMMP_ASM_X64)
    __asm__ volatile("popcnt %1, %0" : "=r"(count) : "r"(x) : "cc");
#else
    count = __builtin_popcountll(x);
#endif
    return count;
#else
    int k = 0;
    while (x) {
        k += x & 1;
        x >>= 1;
    }
    return k;
#endif
}

int lmmp_leading_zeros_(mp_limb_t x) {
    if (x == 0) return 64;
#ifdef __GNUC__
    return __builtin_clzll(x);
#else
    int n = 0;
    if (x <= 0x00000000FFFFFFFF) { n += 32; x <<= 32; }
    if (x <= 0x0000FFFFFFFFFFFF) { n += 16; x <<= 16; }
    if (x <= 0x00FFFFFFFFFFFFFF) { n += 8;  x <<= 8; }
    if (x <= 0x0FFFFFFFFFFFFFFF) { n += 4;  x <<= 4; }
    if (x <= 0x3FFFFFFFFFFFFFFF) { n += 2;  x <<= 2; }
    if (x <= 0x7FFFFFFFFFFFFFFF) { n += 1;  x <<= 1; }
    return n;
#endif
}

int lmmp_tailing_zeros_(mp_limb_t x) {
    if (x == 0) return 64;
#ifdef __GNUC__
    return __builtin_ctzll(x);
#else
    int n = 0;
    if ((x & 0x00000000FFFFFFFF) == 0) { n += 32; x >>= 32; }
    if ((x & 0x000000000000FFFF) == 0) { n += 16; x >>= 16; }
    if ((x & 0x00000000000000FF) == 0) { n += 8;  x >>= 8; }
    if ((x & 0x000000000000000F) == 0) { n += 4;  x >>= 4; }
    if ((x & 0x0000000000000003) == 0) { n += 2;  x >>= 2; }
    if ((x & 0x0000000000000001) == 0) { n += 1;  x >>= 1; }
    return n;
#endif
}

mp_limb_t lmmp_mulh_(mp_limb_t a, mp_limb_t b) {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
    __uint128_t t = (__uint128_t)a * (__uint128_t)b;
    return (mp_limb_t)(t >> 64);
#else
    uint64_t ah = a >> 32, bh = b >> 32;
    a = (uint32_t)a, b = (uint32_t)b;
    uint64_t r0 = a * b, r1 = a * bh, r2 = ah * b, r3 = ah * bh;
    r3 += (r1 >> 32) + (r2 >> 32);
    r1 = (uint32_t)r1, r2 = (uint32_t)r2;
    r1 += r2;
    r1 += (r0 >> 32);
    return r3 + (r1 >> 32);
#endif
}

void lmmp_mullh_(mp_limb_t a, mp_limb_t b, mp_ptr restrict dst) {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
    __uint128_t prod = (__uint128_t)a * b;
    dst[0] = (mp_limb_t)prod;        
    dst[1] = (mp_limb_t)(prod >> 64);
#else
    uint64_t ah = a >> 32, bh = b >> 32;
    a = (uint32_t)a, b = (uint32_t)b;
    uint64_t r0 = a * b, r1 = a * bh, r2 = ah * b, r3 = ah * bh;
    r3 += (r1 >> 32) + (r2 >> 32);
    r1 = (uint32_t)r1, r2 = (uint32_t)r2;
    r1 += r2;
    r1 += (r0 >> 32);
    dst[1] = r3 + (r1 >> 32);
    dst[0] = (r1 << 32) | (uint32_t)r0;
#endif
}

ulong lmmp_mulmod_ulong_(ulong a, ulong b, ulong mod, ulongp restrict q) {
    int shl = lmmp_leading_zeros_(mod);
    mod <<= shl;
    ulong inv = lmmp_inv_1_(mod);
    ulong ab[2];
    ulong r;
    _umul64to128_(a, b, ab, ab + 1);
    if (shl > 0)
        _u128lshl(ab, ab, shl);
    _udiv_qrnnd_preinv(*q, r, ab[1], ab[0], mod, inv);
    return r >> shl;
}
