/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/prime_table.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/trialdiv.h"

#define MAX_T ((ulong)0x7fffffffffff)

/* 块数的理论上限：全部奇素数的乘积不到 2^94548（θ(65535) < 65535），而每个
 * 满块的积都不小于 2^47，所以满块至多 2011 个，再加最后一个乘不满的块，总共
 * 不会超过 2012 个。容量翻倍时以它封顶，绝不给出超出这个数的空间 */
#define TRIALDIV_BLK_MAX ((ushort)2012)

/**
 * 块内字段分三组、各按水位 [0,bn_x) 惰性初始化，纯小 nn 负载不付 chunk
 * 组预处理、纯大 nn 负载不付 preinv 组预处理：
 *   打包组   t/p0/pn        建块即得
 *   preinv 组 k/inv         首个 preinv 腿调用覆盖到该块时算
 *   chunk 组  dinv/r2/base/mi  首个 chunk 腿调用覆盖到该块时算
 */
typedef struct trialdiv_blk {
    ulong t;     // 块积，也即除数
    ulong inv;   // preinv 组：lmmp_inv_1_(t<<k)
    ulong dinv;  // chunk 组：t*dinv==1 mod B
    ulong r2;    // chunk 组：B^2 mod t
    ulong base;  // chunk 组：B mod t
    ulong mi;    // chunk 组：-dinv mod B
    ushort p0;   // 打包组：块首素数在 prime_short_table 的下标
    ushort pn;   // 打包组：块内素数个数
    uchar k;     // preinv 组：lmmp_leading_zeros_(t)
} trialdiv_blk;

typedef struct {
    trialdiv_blk* blks;
    ushort blkc;      // blks 容量
    ushort bn;        // 打包组水位：已闭块数
    ushort bn_inv;    // preinv 组已初始化容量
    ushort bn_chunk;  // chunk 组已初始化容量
    ushort pi;        // 打包游标：下一素数下标
    ulong t;          // 开块部分积（扩容后恒 1：开块总在扩容内闭掉）
} trialdiv_table;

static LMMP_THREAD_LOCAL trialdiv_table G = {
    .blks = NULL, .blkc = 0,  .bn = 0, .bn_inv = 0,
    .bn_chunk = 0, .pi = 1,   .t = 1,
};

/**
 * @brief 把开块落盘成 blks[bn] 里的一个新块，随后开块清空
 * @note 新块的块首下标接在上一块末尾之后，首块从下标 1 开始（素数 2 不打包）。
 *       blks 放不下时容量翻倍，但封顶 TRIALDIV_BLK_MAX；realloc 会原样保留
 *       旧块内容，不需要重算任何字段
 */
static void trialdiv_close(void) {
    if (G.bn == G.blkc) {
        G.blkc = G.blkc ? G.blkc * 2 : 32;
        if (G.blkc > TRIALDIV_BLK_MAX) G.blkc = TRIALDIV_BLK_MAX;
        G.blks = REALLOC_TYPE(G.blks, G.blkc, trialdiv_blk);
    }
    trialdiv_blk* b = &G.blks[G.bn];
    ushort p0 = G.bn ? (ushort)(G.blks[G.bn - 1].p0 + G.blks[G.bn - 1].pn) : 1;
    b->t = G.t;
    b->p0 = p0;
    b->pn = (ushort)(G.pi - p0);
    ++G.bn;
    G.t = 1;
}

/**
 * @brief 打包素数，把下标 [1,cnt) 内的奇素数全部收进闭块
 * @param cnt <=N 的素数个数，即 lmmp_prime_cnt16_(N) 的返回值（含素数 2）
 * @note 素数逐个乘进开块，乘积一超过 MAX_T 就闭一块。盖住 cnt 之后，如果
 *       开块乘积还没满，会继续乘到满块才停。故表中存储的均是完整块，块的
 *       划分与 N 无关，扩容永远是纯追加，开块也从不跨调用留存。素数表用尽时
 *       最后一块无法乘满，自动闭合。
 */
static void trialdiv_extend_(ushort cnt) {
    while (G.pi < cnt || G.t > 1) {
        if (G.pi == PRIME_SHORT_TABLE_SIZE) {
            trialdiv_close();
            break;
        }
        G.t *= prime_short_table[G.pi++];
        if (G.t > MAX_T) trialdiv_close();
    }
    lmmp_debug_assert(G.t == 1);
}

/**
 * @brief 给 [bn_inv,n) 的块补算 k 和 inv
 * @param n 要补至的块数，不超过 bn
 */
static void trialdiv_init_inv_(ushort n) {
    for (ushort i = G.bn_inv; i < n; ++i) {
        trialdiv_blk* b = &G.blks[i];
        b->k = (uchar)lmmp_leading_zeros_(b->t);
        b->inv = lmmp_inv_1_(b->t << b->k);
    }
    G.bn_inv = n;
}

/**
 * @brief 给 [bn_chunk,n) 的块补算折叠腿要用的 dinv、mi、r2、base
 * @param n 要补到的块数，不超过 bn
 * @note dinv 是 t 模 2^64 的逆元，mi 就是它的相反数；r2 与 base 共享一次
 *       inv_1_ 未填的块顺手存进 preinv 组字段并推其水位（已填的块直接复用），
 *       后续 preinv 腿免再算。已经算过的块不会再算
 */
static void trialdiv_init_chunk_(ushort n) {
    for (ushort i = G.bn_chunk; i < n; ++i) {
        trialdiv_blk* b = &G.blks[i];
        b->dinv = lmmp_binvert_ulong_(b->t);
        b->mi = -b->dinv;
        ulong m = b->t;
        mp_bitcnt_t k = 0;
        clz_shl_u64(m, m, k);
        ulong inv;
        if (i < G.bn_inv)
            inv = b->inv;
        else {
            b->k = (uchar)k;
            b->inv = inv = lmmp_inv_1_(m);
        }
        b->r2 = _bpow_preinv_(m, k, 2, inv);
        b->base = _bpow_preinv_(m, k, 1, inv);
    }
    G.bn_chunk = n;
    if (G.bn_inv < n) G.bn_inv = n;
}

typedef mp_limb_t (*trialdiv_leg_fn)(mp_srcptr np, mp_size_t nn, const trialdiv_blk* b);

static mp_limb_t trialdiv_leg_preinv_(mp_srcptr np, mp_size_t nn, const trialdiv_blk* b) {
    return lmmp_mod_preinv_1_(np, nn, b->t, b->inv, b->k);
}

static mp_limb_t trialdiv_leg_4_(mp_srcptr np, mp_size_t nn, const trialdiv_blk* b) {
    return lmmp_mod_4chunk_1_(np, nn, b->t, b->dinv, b->r2, b->base, b->mi);
}

static mp_limb_t trialdiv_leg_8_(mp_srcptr np, mp_size_t nn, const trialdiv_blk* b) {
    return lmmp_mod_8chunk_1_(np, nn, b->t, b->dinv, b->r2, b->base, b->mi);
}

bool lmmp_trialdiv_(mp_srcptr restrict np, mp_size_t nn, ushort N) {
    lmmp_param_assert(np != NULL);
    lmmp_param_assert(nn > 0);

    ushort cnt = lmmp_prime_cnt16_(N);
    if (cnt <= 1) return false; /* N<3：无奇素数可试除 */

    trialdiv_extend_(cnt);

    /* 用满块前缀：块含 <=N 素数 ⟺ p0 < cnt（p0 升序，首块越界即停） */
    ushort un = 0;
    while (un < G.bn && G.blks[un].p0 < cnt) ++un;

    trialdiv_leg_fn leg;
    if (nn < TRIALDIV_PREINV_THRESHOLD) {
        if (G.bn_inv < un) trialdiv_init_inv_(un);
        leg = trialdiv_leg_preinv_;
    } else if (nn < TRIALDIV_K8_THRESHOLD) {
        if (G.bn_chunk < un) trialdiv_init_chunk_(un);
        leg = trialdiv_leg_4_;
    } else {
        if (G.bn_chunk < un) trialdiv_init_chunk_(un);
        leg = trialdiv_leg_8_;
    }

    for (ushort i = 0; i < un; ++i) {
        const trialdiv_blk* b = &G.blks[i];
        mp_limb_t r = leg(np, nn, b);
        if (r == 0) return true;
        for (ushort j = b->p0, e = (ushort)(b->p0 + b->pn); j < e; ++j)
            if (r % prime_short_table[j] == 0) return true;
    }
    return false;
}

void lmmp_trialdiv_table_free_(void) {
    lmmp_free(G.blks);
    G.blks = NULL;
    G.blkc = 0;
    G.bn = 0;
    G.bn_inv = 0;
    G.bn_chunk = 0;
    G.pi = 1;
    G.t = 1;
}
