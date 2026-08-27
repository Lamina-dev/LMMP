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

#ifndef __LMMP_RAND_STATE_H__
#define __LMMP_RAND_STATE_H__

#include "../lmmp.h"
#include "../impl/longlong.h"

typedef struct {
    mp_limb_t state[2];
    mp_limb_t inc[2];  // 必须为奇数
} pcg64_128_state;

typedef struct {
    mp_limb_t s[4];  // 256位状态，必须初始化为非零值
} xoshiro256pp_state;

#define PCG128_DEFAULT_MULTIPLIER_HI 0x2360ED051FC65DA4ULL
#define PCG128_DEFAULT_MULTIPLIER_LO 0x4385DF649FCCF645ULL

#ifndef INLINE_
#define INLINE_ static inline
#endif

INLINE_ mp_limb_t rotl(const mp_limb_t x, int k) {
    const int shift = k & 63;
    return (x << shift) | (x >> ((-shift) & 63));
}

/**
 * @brief 种子生成器
 * @param seed 低熵种子
 * @return 高熵种子
 */
INLINE_ mp_limb_t lmmp_seed_generator(mp_limb_t seed) {
    mp_limb_t z = (seed += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

INLINE_ void pcg64_128_action(mp_limb_t state[2], const mp_limb_t inc[2]) {
    // state = (state * PCG64_MULT + inc) mod 2^128
    // state * M = s0*m0 + ((s0*m1 + s1*m0) mod 2^64) << 64  (mod 2^128)
    // 交叉项 s0*m1 与 s1*m0 只有低 64 位落在 2^128 以内，高 64 位模掉
    mp_limb_t tmp[2] = {0, 0};
    _umul64to128_(state[0], PCG128_DEFAULT_MULTIPLIER_LO, tmp, tmp + 1);
    tmp[1] += state[1] * PCG128_DEFAULT_MULTIPLIER_LO;
    tmp[1] += state[0] * PCG128_DEFAULT_MULTIPLIER_HI;
    _u128add(state, tmp, inc);
}

INLINE_ void lmmp_pcg64_128_srandom(pcg64_128_state* rng, mp_limb_t seed) {
    lmmp_param_assert(rng != NULL);

    rng->state[0] = seed * 0x24069528d54bbaa4ULL;
    rng->state[1] = (seed << 17) + 0xf98bc019ecd71a28ULL;
    rng->inc[0] = rotl(seed, 31) * 0xb5b2943a321cdf10ULL;
    rng->inc[0] |= 1ull;
    rng->inc[1] = (seed << 21) ^ seed;

    // warm up
    pcg64_128_action(rng->state, rng->inc);
    pcg64_128_action(rng->state, rng->inc);
}

// （PCG-XSL-RR-128/64）
INLINE_ mp_limb_t lmmp_pcg64_128_random(pcg64_128_state* rng) {
    lmmp_param_assert(rng != NULL);
    mp_limb_t oldstate[2] = {rng->state[0], rng->state[1]};
    pcg64_128_action(rng->state, rng->inc);

    // XSL-RR
    mp_limb_t xsl = ((oldstate[1]) ^ oldstate[0]);

    mp_byte_t rot = (mp_byte_t)(oldstate[1] >> 58); 
    return (xsl >> rot) | (xsl << ((-rot) & 63));
}

INLINE_ mp_limb_t lmmp_xoshiro256pp_random(xoshiro256pp_state* rng) {
    lmmp_param_assert(rng != NULL);
    const mp_limb_t r = rotl(rng->s[0] + rng->s[3], 23) + rng->s[0];
    const mp_limb_t t = rng->s[1] << 17;

    rng->s[2] ^= rng->s[0];
    rng->s[3] ^= rng->s[1];
    rng->s[1] ^= rng->s[2];
    rng->s[0] ^= rng->s[3];
    rng->s[2] ^= t;
    rng->s[3] = rotl(rng->s[3], 45);

    return r;
}

INLINE_ void lmmp_xoshiro256pp_srandom(xoshiro256pp_state* rng, mp_limb_t seed) {
    lmmp_param_assert(rng != NULL);

    // 不可能为全零状态
    rng->s[0] = seed ^ 0x9e37b91f8a5d7c19ULL;
    rng->s[1] = (seed << 17) + 0xf98bc01ecdc71a28ULL;
    rng->s[2] = rotl(seed, 37) ^ (seed << 21);
    rng->s[3] = 0x1b30964ec95c4069ULL * seed;

    // warm up
    lmmp_xoshiro256pp_random(rng);
    lmmp_xoshiro256pp_random(rng);
}

#define PCG64_LE_MULTIPLIER 6364136223846793005ULL
#define PCG64_LE_INCREMENT 1442695040888963407ULL

typedef struct {
    mp_size_t k;
    mp_limb_t* restrict state;
} pcg64_le_seq_t;

INLINE_ void pcg64_le_seq_init(pcg64_le_seq_t* rng, mp_size_t i, mp_limb_t seed) {
    lmmp_param_assert(rng != NULL);
    lmmp_param_assert(rng->k > 0);
    lmmp_param_assert(rng->state != NULL);

#define PRIME64_0 0x9E3779B185EBCA87ULL
#define PRIME64_1 0xC2B2AE3D27D4EB4FULL
#define PRIME64_2 0x165667B19E3779F9ULL
#define PRIME64_3 0x85EBCA77C2B2AE63ULL
#define PRIME64_4 0x27D4EB2F165667C5ULL

    mp_limb_t s0, s1, s2, s3;

    for (; i + 3 < rng->k; i += 4) {
        s0 = rotl(seed + i + 0, 41);
        s1 = rotl(seed + i + 1, 29);
        s2 = rotl(seed + i + 2, 23);
        s3 = rotl(seed + i + 3, 7);
        s0 *= PRIME64_0;
        s1 *= PRIME64_1;
        s2 *= PRIME64_2;
        s3 *= PRIME64_3;
        rng->state[i + 0] = lmmp_seed_generator(s0 ^ rotl(s0, 17));
        rng->state[i + 1] = lmmp_seed_generator(s1 ^ rotl(s1, 21));
        rng->state[i + 2] = lmmp_seed_generator(s2 ^ rotl(s2, 13));
        rng->state[i + 3] = lmmp_seed_generator(s3 ^ rotl(s3, 33));
    }
    for (; i < rng->k; i++) {
        s0 = rotl(seed + i, 31);
        s0 *= PRIME64_4;
        rng->state[i] = lmmp_seed_generator(s0 ^ rotl(s0, 27));
    }

#undef PRIME64_0
#undef PRIME64_1
#undef PRIME64_2
#undef PRIME64_3
#undef PRIME64_4
}

INLINE_ mp_limb_t pcg64_le_action(mp_limb_t* restrict state) {
    mp_limb_t old_state = *state;
    *state = old_state * PCG64_LE_MULTIPLIER + PCG64_LE_INCREMENT;

    // RXS-M-XS
    mp_limb_t x = old_state;
    int count = x >> 59;
    x ^= x >> (5 + count);
    x *= 12605985483714917081ULL;
    x ^= x >> 43;

    return x;
}

INLINE_ void pcg64_le_seq_next(mp_ptr restrict dst, mp_size_t n, pcg64_le_seq_t* rng) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(rng != NULL);
    lmmp_param_assert(n <= rng->k);
    mp_size_t i;
    mp_limb_t mixn = lmmp_seed_generator(n * 0xb9ce52b55c72d585ULL);
    for (i = 0; i + 3 < n; i += 4) {
        dst[i + 0] = pcg64_le_action(&rng->state[i + 0]) ^ mixn;
        dst[i + 1] = pcg64_le_action(&rng->state[i + 1]) ^ mixn;
        dst[i + 2] = pcg64_le_action(&rng->state[i + 2]) ^ mixn;
        dst[i + 3] = pcg64_le_action(&rng->state[i + 3]) ^ mixn;
    }
    for (; i < n; i++) {
        dst[i] = pcg64_le_action(&rng->state[i]) ^ mixn;
    }
    for (; i < rng->k; i++) {
        mp_limb_t old_state = rng->state[i];
        rng->state[i] = old_state * PCG64_LE_MULTIPLIER + PCG64_LE_INCREMENT;
    }
}

#undef INLINE_

#endif // __LMMP_RAND_STATE_H__