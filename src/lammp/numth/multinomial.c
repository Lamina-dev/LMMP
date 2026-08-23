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

#include "../../../include/lammp/impl/ele_mul.h"
#include "../../../include/lammp/impl/inlines.h"
#include "../../../include/lammp/impl/lglg.h"
#include "../../../include/lammp/impl/longlong.h"
#include "../../../include/lammp/impl/mparam.h"
#include "../../../include/lammp/impl/prime_table.h"


static inline mp_size_t fac_size_lower(uint n) {
    if (n < 20) {
        return 0;
    } else {
        return log2_fac_floor(n);
    }
}

static inline mp_size_t fac_size_bigger(uint n) {
    if (n < 20) {
        return 64;
    } else {
        return log2_fac_ceil(n);
    }
}

mp_size_t lmmp_multinomial_size_(const uintp r, uint m, ulong* restrict n) {
    ulong n_ret = 0;
    uint i = 0;
    for (; i < m; ++i) n_ret += r[i];

    lmmp_param_assert(n_ret <= MP_UINT_MAX);
    lmmp_param_assert(n_ret > 0);
    *n = n_ret;
    mp_size_t rn = fac_size_bigger(n_ret);

    for (i = 0; i < m; ++i) {
        rn -= fac_size_lower(r[i]);
    }
    rn = (rn + LIMB_BITS - 1) / LIMB_BITS;
    return rn + 2;
}

static inline uint count_factors(fac_ptr fac, uint nfactors, uint n, const uintp r, uint m, uint p) {
    uint pn = n;
    uint e = 0;
    ulong inv = MP_ULONG_MAX / p + 1;
    while (pn > 0) {
        _udiv32by32_q_preinv(pn, pn, inv);
        e += pn;
    }
    for (uint i = 0; i < m; ++i) {
        pn = r[i];
        while (pn > 0) {
            _udiv32by32_q_preinv(pn, pn, inv);
            e -= pn;
        }
    }
    if (e > 0) {
        fac[nfactors].f = p;
        fac[nfactors++].j = e;
    }
    return nfactors;
}

static inline uint factor_size_int(mp_size_t rn, uint n) {
    /*
     使用类似组合数的思路来估计缓冲区大小。
    */
    ulong approx1 = rn * 8;
    lmmp_debug_assert(rn * 8 <= MP_UINT_MAX);
    ulong approx2 = lmmp_prime_size_(n);
    return approx1 < approx2 ? approx1 : approx2;
}

static inline ushort factor_size_short(mp_size_t rn) {
    ushort approx1 = rn * 8;
    lmmp_debug_assert(rn * 8 <= MP_USHORT_MAX);
    return approx1;
}

static mp_size_t lmmp_odd_multinomial_ushort_(
          mp_ptr    restrict dst, 
          mp_size_t           rn, 
          uint                 n, 
    const uintp     restrict   r, 
          uint                 m
) {
    if (n < ODD_FACTORIAL_SIZE) {
        lmmp_odd_nPr_ushort_(dst, rn, n, n);
        mp_limb_t t = 0;
        for (uint i = 0; i < m; ++i) {
            lmmp_odd_nPr_ushort_(&t, 1, r[i], r[i]);
            dst[0] /= t;
        }
        return 1;
    } else {
        TEMP_DECL;
        ushort primen = lmmp_prime_cnt16_(n);
        ushort nfactors = factor_size_short(rn);
        nfactors = primen < nfactors ? primen : nfactors;
        fac_ptr restrict fac = TALLOC_TYPE(nfactors, fac_t);
        nfactors = 0;
        for (ushort i = 1; i < primen; ++i) {
            ushort p = prime_short_table[i];
            nfactors = count_factors(fac, nfactors, n, r, m, p);
        }

        rn = lmmp_factors_mul_ushort_(dst, rn, fac, nfactors);

        TEMP_FREE;
        return rn;
    }
}

static mp_size_t lmmp_odd_multinomial_uint_(mp_ptr restrict dst, mp_size_t rn, uint n, const uintp restrict r, uint m) {
    lmmp_prime_int_table_init_(n);
    TEMP_B_DECL;
    uint nfactors = factor_size_int(rn, n);
    fac_ptr restrict fac = BALLOC_TYPE(nfactors, fac_t);

    nfactors = 0;
    prime_cache_t cache;
    lmmp_prime_cache_init_(&cache, n);
    while (cache.is_end == 0) {
        lmmp_prime_cache_next_(&cache);
        for (uint i = 0; i < cache.size; ++i) {
            nfactors = count_factors(fac, nfactors, n, r, m, cache.pp[i]);
        }
    }
    lmmp_prime_cache_free_(&cache);

    rn = lmmp_factors_mul_(dst, rn, fac, nfactors);

    TEMP_B_FREE;
    return rn;
}

#define MULTINOMIAL_SHORT_LIMIT (0xffff)
#define MULTINOMIAL_INT_LIMIT (0xffffffff)

mp_size_t lmmp_multinomial_(mp_ptr restrict dst, mp_size_t rn, uint n, const uintp restrict r, uint m) {
    mp_size_t shl = n - lmmp_limb_popcnt_(n);
    for (uint j = 0; j < m; ++j) {
        shl += lmmp_limb_popcnt_(r[j]);
        shl -= r[j];
    }
    mp_size_t shw = shl / LIMB_BITS;
    shl %= LIMB_BITS;
    lmmp_zero(dst, shw);
    if (n <= MULTINOMIAL_SHORT_LIMIT) {
        rn = lmmp_odd_multinomial_ushort_(dst + shw, rn - shw, n, r, m);
    } else {
        rn = lmmp_odd_multinomial_uint_(dst + shw, rn - shw, n, r, m);
    }

    if (shl > 0) {
        dst[shw + rn] = lmmp_shl_(dst + shw, dst + shw, rn, shl);
        rn += shw + 1;
        rn -= dst[rn - 1] == 0 ? 1 : 0;
    } else {
        rn += shw;
    }
    return rn;
}
