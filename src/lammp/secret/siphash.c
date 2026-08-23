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

#include "../../../include/lammp/secret.h"

static inline uint64_t rotl64(uint64_t x, int b) {
    b &= 63;
    return (x << b) | (x >> (64 - b));
}

uint64_t lmmp_siphash24_(mp_srcptr in, mp_size_t inlen, srckey128_t key) {
#define SIPROUND             \
    do {                     \
        v0 += v1;            \
        v1 = rotl64(v1, 13); \
        v1 ^= v0;            \
        v0 = rotl64(v0, 32); \
        v2 += v3;            \
        v3 = rotl64(v3, 16); \
        v3 ^= v2;            \
        v0 += v3;            \
        v3 = rotl64(v3, 21); \
        v3 ^= v0;            \
        v2 += v1;            \
        v1 = rotl64(v1, 17); \
        v1 ^= v2;            \
        v2 = rotl64(v2, 32); \
    } while (0)

    uint64_t k0;
    uint64_t k1;
    if (key == NULL) {
        k0 = 0;
        k1 = 0;
    } else {
        k0 = key[0];
        k1 = key[1];
    }

    uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
    uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
    uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
    uint64_t v3 = 0x7465646279746573ULL ^ k1;

    const uint64_t* data = (const uint64_t*)in;
    const uint64_t* end = data + inlen;

    const uint64_t* limit = data + (inlen & ~3ULL);

    while (data < limit) {
        uint64_t m0 = data[0];
        uint64_t m1 = data[1];
        uint64_t m2 = data[2];
        uint64_t m3 = data[3];
        data += 4;

        v3 ^= m0;
        SIPROUND;
        SIPROUND;
        v0 ^= m0;

        v3 ^= m1;
        SIPROUND;
        SIPROUND;
        v0 ^= m1;

        v3 ^= m2;
        SIPROUND;
        SIPROUND;
        v0 ^= m2;

        v3 ^= m3;
        SIPROUND;
        SIPROUND;
        v0 ^= m3;
    }

    while (data < end) {
        uint64_t m = *data++;
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }

    uint64_t b = ((uint64_t)(inlen * LIMB_BYTES)) << 56;
    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;

    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;

    return v0 ^ v1 ^ v2 ^ v3;
}