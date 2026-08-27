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

#include "../../../include/lmmp/mprand.h"
#include "../../../include/lmmp/impl/rand_state.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"

typedef struct {
    mp_limb_t state;
    int seed_type;
} lmmp_global_rng_t;

#define GLOBAL_RNG_INIT_STATE 10451216379200822465ULL
#define GLOBAL_RNG_INIT_SEED_TYPE 1
static LMMP_THREAD_LOCAL lmmp_global_rng_t lmmp_global_rng = {GLOBAL_RNG_INIT_STATE, GLOBAL_RNG_INIT_SEED_TYPE};

void lmmp_global_rng_init_(int seed, int seed_type) {
    lmmp_global_rng.state = lmmp_seed_generator(seed + seed_type);
    lmmp_global_rng.seed_type = seed_type % 2;
}

mp_size_t lmmp_seed_random_(mp_ptr restrict dst, mp_size_t n, mp_limb_t seed, int seed_type) {
    seed_type %= 2;
    if (seed_type == 0) {
        pcg64_128_state rng;
        lmmp_pcg64_128_srandom(&rng, seed);
        mp_size_t i = 0;
        for (; i + 3 < n; i += 4) {
            dst[i + 0] = lmmp_pcg64_128_random(&rng);
            dst[i + 1] = lmmp_pcg64_128_random(&rng);
            dst[i + 2] = lmmp_pcg64_128_random(&rng);
            dst[i + 3] = lmmp_pcg64_128_random(&rng);
        }
        for (; i < n; ++i) {
            dst[i] = lmmp_pcg64_128_random(&rng);
        }
    } else {
        xoshiro256pp_state rng;
        lmmp_xoshiro256pp_srandom(&rng, seed);
        mp_size_t i = 0;
        for (; i + 3 < n; i += 4) {
            dst[i + 0] = lmmp_xoshiro256pp_random(&rng);
            dst[i + 1] = lmmp_xoshiro256pp_random(&rng);
            dst[i + 2] = lmmp_xoshiro256pp_random(&rng);
            dst[i + 3] = lmmp_xoshiro256pp_random(&rng);
        }
        for (; i < n; ++i) {
            dst[i] = lmmp_xoshiro256pp_random(&rng);
        }
    }
    while (n > 0 && dst[n - 1] == 0) {
        --n;
    }
    return n;
}

mp_size_t lmmp_random_(mp_ptr restrict dst, mp_size_t n) {
    if (n == 0 || dst == NULL) {
        return 0;
    }
    mp_limb_t seed = lmmp_global_rng.state + n;
    seed ^= rotl(lmmp_seed_generator(n), 23);
    lmmp_global_rng.state = lmmp_seed_generator(seed);
    return lmmp_seed_random_(dst, n, seed, lmmp_global_rng.seed_type);
}

typedef struct lmmp_strong_rng_t {
    pcg64_le_seq_t stream;
} lmmp_strong_rng_t;

lmmp_strong_rng_t* lmmp_strong_rng_init_(mp_size_t k, int seed) {
    lmmp_param_assert(k > 0);

    lmmp_strong_rng_t* rng = (lmmp_strong_rng_t*)lmmp_alloc(sizeof(lmmp_strong_rng_t));
    rng->stream.k = k;
    rng->stream.state = ALLOC_TYPE(k, mp_limb_t);

    mp_limb_t new_seed = lmmp_seed_generator(seed);
    new_seed = rotl(new_seed, 17) ^ lmmp_seed_generator(k);
    pcg64_le_seq_init(&rng->stream, 0, new_seed);
    return rng;
}

void lmmp_strong_rng_extern_(lmmp_strong_rng_t* rng, mp_size_t k) {
    lmmp_param_assert(rng != NULL);
    lmmp_param_assert(k > 0);
    if (k <= rng->stream.k) return;
    mp_size_t old_k = rng->stream.k;
    rng->stream.state = (mp_limb_t*)lmmp_realloc(rng->stream.state, k * sizeof(mp_limb_t));

    mp_limb_t new_seed = lmmp_seed_generator(rng->stream.k);
    new_seed = rotl(new_seed, 37) ^ lmmp_seed_generator(k);
    rng->stream.k = k;
    pcg64_le_seq_init(&rng->stream, old_k, new_seed);
}

void lmmp_strong_rng_free_(lmmp_strong_rng_t* rng) {
    if (rng != NULL) {
        if (rng->stream.state != NULL) {
            lmmp_free(rng->stream.state);
        }
        lmmp_free(rng);
    }
}

mp_size_t lmmp_strong_random_(mp_ptr restrict dst, mp_size_t n, lmmp_strong_rng_t* rng) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(rng != NULL);
    lmmp_param_assert(rng->stream.state != NULL);
    lmmp_param_assert(n > 0);
    lmmp_param_assert(n <= rng->stream.k);
    pcg64_le_seq_next(dst, n, &rng->stream);
    while (n > 0 && dst[n - 1] == 0) {
        --n;
    }
    return n;
}
