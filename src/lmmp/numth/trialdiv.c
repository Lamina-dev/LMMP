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

#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/impl/prime_table.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/ele_mul.h"

#define MAX_T 0xffffffffffffull

static ushortp lmmp_trialdiv_short_(mp_srcptr restrict np, mp_size_t nn, ushort N, ushort* rn) {
    if (np == NULL || nn == 0) {
        *rn = 0;
        return NULL;
    }
    ulong t = 1;
    ushort idx[20];
    uint idx_cnt = 0;
    ushort retn_max = 10;
    ushortp ret = ALLOC_TYPE(retn_max, ushort);
    ushort retn = 0;
    if (!(np[0] & 1)) {
        ret[retn++] = 2;
    }
    for (mp_size_t i = 1;; i++) {
        ushort p = prime_short_table[i];
        if (p > N || i >= PRIME_SHORT_TABLE_SIZE - 1) break;
        t *= p;
        idx[idx_cnt++] = p;
        if (t > MAX_T || idx_cnt == 20) {
            mp_limb_t r = lmmp_mod_1_(np, nn, t);
            for (uint j = 0; j < idx_cnt; j++) {
                if (r == 0 || r % idx[j] == 0) {
                    ret[retn++] = idx[j];
                    if (retn == retn_max) {
                        retn_max = retn_max * 12 / 10;
                        ret = REALLOC_TYPE(ret, retn_max, ushort);
                    }
                }
            }
            idx_cnt = 0;
            t = 1;
        }
    }
    if (idx_cnt > 0) {
        mp_limb_t r = lmmp_mod_1_(np, nn, t);
        for (uint j = 0; j < idx_cnt; j++) {
            if (r == 0 || r % idx[j] == 0) {
                ret[retn++] = idx[j];
                if (retn == retn_max) {
                    retn_max += 10;
                    ret = REALLOC_TYPE(ret, retn_max, ushort);
                }
            }
        }
    }
    if (retn == 0) {
        lmmp_free(ret);
        *rn = 0;
        return NULL;
    } else {
        *rn = retn;
        return ret;
    }
}

/*
    素数积缓存：乘积路径每次调用需将 ≤N 全体素数之积构建一遍（高强度
    大尺寸下 ~百 us/次），而 nextprime/素性批量检验以同一上界反复调用。
    单槽缓存命中即免重建；积由常量表构建、构建后只读，跨线程共享安全
    （dec_pow 表同例）。惰性构建，lmmp_global_deinit 注册释放；上界
    变化时原槽重建（退化为旧路径的一次构建成本）。
*/
static struct {
    ushort N;     /* 缓存上界（0 = 空） */
    mp_size_t pn; /* 积的 limb 长 */
    mp_ptr prod;  /* [pn] ≤N 素数积（堆，跨调用持久） */
} td_prod_cache_g = {0, 0, NULL};

void lmmp_trialdiv_cache_free_(void) {
    if (td_prod_cache_g.prod != NULL) {
        lmmp_free(td_prod_cache_g.prod);
        td_prod_cache_g.prod = NULL;
        td_prod_cache_g.N = 0;
        td_prod_cache_g.pn = 0;
    }
}

/* 取 ≤N 素数积（未命中或换界时重建缓存），[pn] 回写积长度 */
static mp_srcptr td_prod_get_(ushort N, ushort primen, mp_size_t* pn) {
    if (td_prod_cache_g.N == N) {
        *pn = td_prod_cache_g.pn;
        return td_prod_cache_g.prod;
    }
    lmmp_trialdiv_cache_free_();

    TEMP_S_DECL;
    /* 至少4个质数的乘积才可能填满一个limb */
    ulongp restrict pp = SALLOC_TYPE(primen / 4 + 1, ulong);
    ulong t = 1;
    mp_size_t pnw = 0;
    for (ushort i = 0; i < primen; i++) {
        t *= prime_short_table[i];
        if (t > MAX_T) {
            pp[pnw++] = t;
            t = 1;
        }
    }
    if (t > 1) {
        pp[pnw++] = t;
    }
    mp_ptr restrict prod = SALLOC_TYPE(pnw * 2, mp_limb_t);
    pnw = lmmp_elem_mul_ulong_(prod, pp, pnw, prod + pnw);

    td_prod_cache_g.prod = ALLOC_TYPE(pnw, mp_limb_t);
    lmmp_copy(td_prod_cache_g.prod, prod, pnw);
    td_prod_cache_g.N = N;
    td_prod_cache_g.pn = pnw;
    TEMP_S_FREE;

    *pn = pnw;
    return td_prod_cache_g.prod;
}

ushortp lmmp_trialdiv_(mp_srcptr restrict np, mp_size_t nn, ushort N, ushort* rn) {
    ushort primen = lmmp_prime_cnt16_(N);
    if (primen > 4 * nn) {
        return lmmp_trialdiv_short_(np, nn, N, rn);
    } else {
    /*
    if [np,nn] is too large, we calculate the product of primes up to N,
    and divide [np,nn] by the product to get remainder.
    */
        mp_size_t pn;
        mp_srcptr prod = td_prod_get_(N, primen, &pn);
        TEMP_S_DECL;
        /* 余数独立缓冲（缓存积只读，不得如旧路径 eqsep 原地覆写） */
        mp_ptr restrict rem = SALLOC_TYPE(pn, mp_limb_t);
        lmmp_div_(NULL, rem, np, nn, prod, pn);
        while (pn > 0 && rem[pn - 1] == 0) --pn;
        ushortp restrict ret;
        if (pn == 0) {
            ret = ALLOC_TYPE(primen, ushort);
            for (ushort i = 0; i < primen; i++) {
                ret[i] = prime_short_table[i];
            }
            *rn = primen;
        } else {
            ret = lmmp_trialdiv_short_(rem, pn, N, rn);
        }
        TEMP_S_FREE;
        return ret;
    }
}
