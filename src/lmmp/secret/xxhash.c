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

#include "../../../include/lmmp/secret.h"

#define PRIME64_1 0x9E3779B185EBCA87ULL
#define PRIME64_2 0xC2B2AE3D27D4EB4FULL
#define PRIME64_3 0x165667B19E3779F9ULL
#define PRIME64_4 0x85EBCA77C2B2AE63ULL
#define PRIME64_5 0x27D4EB2F165667C5ULL

static inline uint64_t rotl64(uint64_t x, int b) {
    b &= 63;
    return (x << b) | (x >> (64 - b));
}

uint64_t lmmp_xxhash_(mp_srcptr in, mp_size_t inlen, srckey64_t key) {
    uint64_t seed = key == NULL ? 0 : key[0];
    if (in == NULL || inlen == 0) {
        seed += PRIME64_5;
        seed += 0;  // len = 0
        seed ^= seed >> 33;
        seed *= PRIME64_2;
        seed ^= seed >> 29;
        seed *= PRIME64_3;
        seed ^= seed >> 32;
        return seed;
    }

    const uint64_t* p = in;
    const uint64_t* const end = in + inlen;
    uint64_t h64;

    if (inlen >= 4) {
        const uint64_t* const limit = end - 4;
        uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t v2 = seed + PRIME64_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - PRIME64_1;

        do {
            v1 += *p * PRIME64_2;
            v1 = rotl64(v1, 31);
            v1 *= PRIME64_1;
            p++;

            v2 += *p * PRIME64_2;
            v2 = rotl64(v2, 31);
            v2 *= PRIME64_1;
            p++;

            v3 += *p * PRIME64_2;
            v3 = rotl64(v3, 31);
            v3 *= PRIME64_1;
            p++;

            v4 += *p * PRIME64_2;
            v4 = rotl64(v4, 31);
            v4 *= PRIME64_1;
            p++;
        } while (p <= limit);

        h64 = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);

        v1 *= PRIME64_2;
        v1 = rotl64(v1, 31);
        v1 *= PRIME64_1;
        h64 ^= v1;
        h64 = h64 * PRIME64_1 + PRIME64_4;

        v2 *= PRIME64_2;
        v2 = rotl64(v2, 31);
        v2 *= PRIME64_1;
        h64 ^= v2;
        h64 = h64 * PRIME64_1 + PRIME64_4;

        v3 *= PRIME64_2;
        v3 = rotl64(v3, 31);
        v3 *= PRIME64_1;
        h64 ^= v3;
        h64 = h64 * PRIME64_1 + PRIME64_4;

        v4 *= PRIME64_2;
        v4 = rotl64(v4, 31);
        v4 *= PRIME64_1;
        h64 ^= v4;
        h64 = h64 * PRIME64_1 + PRIME64_4;
    } else {
        h64 = seed + PRIME64_5;
    }

    h64 += (uint64_t)(inlen * sizeof(mp_limb_t));

    while (p < end) {
        uint64_t k1 = *p;
        k1 *= PRIME64_2;
        k1 = rotl64(k1, 31);
        k1 *= PRIME64_1;
        h64 ^= k1;
        h64 = rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}
