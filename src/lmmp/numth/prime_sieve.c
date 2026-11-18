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
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "impl/prime_table.h"
#include "mparam.h"
#include "impl/inlines.h"
#include "impl/tmp_alloc.h"


typedef struct prime_int {
    lmmp_bitset_p restrict map;  // mod-30 位图（1 为素数，字节 b 位 s 代表数 30b+res8[s]）
    uint map_size;               // 位图字容量
    uint max;                    // 位图记录的最大值
} prime_int;

static LMMP_THREAD_LOCAL prime_int global_prime_int_table = {
    .map      = NULL,
    .map_size = 0,
    .max      = 0
};

#define G global_prime_int_table

/*
 * mod-30 轮存储：与 30 互素的余数共 8 个（slot 0..7 升序），位图每字节存 30 个
 * 数中的 8 个候选，密度 4/15 bit/数，为奇数编码（1/2 bit/数）的 8/15，同上限内存
 * ÷1.875。数与位下标互转（x 为 wheel 数，i 为其位下标）：
 *   I(x) = 8*(x/30) + slot30[x%30]
 *   N(i) = 30*(i/8) + res8[i%8]
 * 3、5 的倍数被结构性排除，不入位图（3、5 本身由遍历端补入）。
 */
static const uint res8[8] = {1, 7, 11, 13, 17, 19, 23, 29};

// 余数 -> slot（与 30 非互素为 0xFF）
static const uchar slot30[30] = {0xFF, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 1, 0xFF, 0xFF, 0xFF, 2,    0xFF, 3, 0xFF, 0xFF,
                                 0xFF, 4, 0xFF, 5,    0xFF, 0xFF, 0xFF, 6, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 7};

// 与 30 非互素的余数 -> 到下一个 wheel 数的增量（互素余数为 0）
static const uchar bump30[30] = {1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3,
                                 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0};

// 互素余数 -> 到下一个 wheel 数的差（wheel 数序列的循环间隙 6,4,2,4,2,4,6,2）
static const uchar gap30[30] = {0, 6, 0, 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 4, 0,
                                0, 0, 2, 0, 4, 0, 0, 0, 6, 0, 0, 0, 0, 0, 2};

// slot -> res8[slot] 的 mod 30 逆元（{7,13}、{17,23} 互换，1/11/19/29 自逆）
static const uchar inv30_by_slot[8] = {1, 13, 11, 7, 23, 19, 17, 29};

#define WHEEL_IDX(x) (8 * ((x) / 30) + slot30[(x) % 30])
#define WHEEL_NUM(i) (30 * ((i) >> 3) + res8[(i) & 7])
#define set_not_prime(p, i) ((p)[(i) >> 3] &= (uchar)~(1u << ((i) & 7)))

// 最大 wheel 数 <= n 的位下标（要求 n >= 3，保证 r==0 时 b >= 1）
static inline uint wheel_leq_idx(uint n) {
    uint r = n % 30;
    uint b = n / 30;
    if (r == 0)
        return 8 * b - 1; // 29 属于前一字节
    while (slot30[r] == 0xFF)
        --r;
    return 8 * b + slot30[r];
}

// 位图字容量：字节数 n/30+1 上取整到字再加一字松弛（容纳尾段整字模式填充）
static inline uint prime_table_words(uint n) {
    return (n / 30 + 1) / 8 + 1;
}

// 分段大小（字）。512KB 恰为目标 L2 容量级，且段容量 2^22 位 >> 最大标记增量
// （8*(65535*6/30)+8），保证段内每个已启动的质数至少标记数十次，无近空转循环
#define PRIME_SEG_WORDS (1u << 16)

/*
 * 7*11 预筛基表：presieve77_table[j] 为绝对字节 j（mod 77）的字节值，清除位恰对应
 * 7 或 11 的 wheel 倍数（含 7、11 自身，由首次初始化的修正恢复）。7 与 11 在位图
 * 空间的标记周期为 7/11 字节，故基表为绝对相位周期 77 = 7*11 字节。
 * 推导规则（可再生成）：全 0xFF 起始，对 j in [0,77)、s in [0,8)、x = 30j+res8[s]，
 * x%7==0 或 x%11==0 则清位 s；无任何豁免（b >= 1 的 wheel 倍数全为 >= 49 的真合数，
 * x=7/11 仅出现在字节 0）。pat[0] = 0xF9：slot1(7)、slot2(11) 清除。
 */
static const uchar presieve77_table[77] = {
    249, 223, 239, 126, 182, 251, 61,  253, 215, 239, 94,  243, 251, 175,
    253, 158, 239, 124, 247, 243, 191, 221, 219, 239, 110, 247, 186, 191,
    125, 223, 231, 126, 215, 251, 191, 237, 223, 174, 126, 117, 251, 183,
    253, 223, 235, 126, 231, 251, 190, 253, 93,  239, 118, 247, 219, 187,
    253, 207, 239, 62,  247, 121, 191, 245, 223, 207, 122, 247, 235, 191,
    188, 223, 109, 126, 247, 251, 159};

// 以 pat 周期填充字节 [b0, bend)：先铺至多 77 字节种子对齐绝对相位，再倍增 memcpy
static void fill_pattern(uchar* restrict dst, uint b0, uint bend, const uchar* restrict pat) {
    uint n = bend - b0;
    uint seed = n < 77 ? n : 77;
    for (uint t = 0; t < seed; ++t)
        dst[b0 + t] = pat[(b0 + t) % 77];
    for (uint i = b0 + seed; i < bend;) {
        uint chunk = i - b0;
        if (chunk > bend - i)
            chunk = bend - i;
        memcpy(dst + i, dst + b0, chunk);
        i += chunk;
    }
}

void lmmp_prime_int_table_init_(uint n) {
    if (n < PRIME_SHORT_TABLE_N || G.max >= n)
        return;

    uint old_size = G.map_size; // 旧表字数（0 表示首次初始化）
    uint old_max = G.max;
    uint new_size = prime_table_words(n);

    if (G.map == NULL) {
        G.map = ALLOC_TYPE(new_size, lmmp_bitset_t);
    } else {
        G.map = REALLOC_TYPE(G.map, new_size, lmmp_bitset_t);
    }
    G.map_size = new_size;
    G.max = n;

    uchar* restrict p = (uchar*)G.map;
    uint limit_idx = wheel_leq_idx(n); // 有效位上界（含），对应 <= n 的最大 wheel 数
    uint skip_num = 0, skip_to = 0;    // 扩容时旧区域之后的第一个 wheel 数及其位下标
    if (old_max != 0) {
        // 宽类型算后继 wheel 数：old_max 贴近 2^32 时 uint 中间量回绕，得到
        // 失配的 (skip_num,skip_to) 会使重定位标到素数自身（13..29 被清位）
        ulong sk = (ulong)old_max + 1 + bump30[((ulong)old_max + 1) % 30];
        if (sk <= MP_UINT_MAX) {
            skip_num = (uint)sk;
            skip_to = WHEEL_IDX(skip_num);
        } else {
            // uint 域内已无后继 wheel 数，新区域无数可标：MP_UINT_MAX 作域外
            // 哨兵，重定位 m = p*ceil(UINT_MAX/p) >= UINT_MAX > p^2 恒超 end，
            // 各素数经既有 idx>=end 路径直接跳过标记（每素数一条除法链）
            skip_num = MP_UINT_MAX;
            skip_to = (uint)-1;
        }
    }

    ushort sqrt_n = n > 4294836225 ? 0xffff : (ushort)sqrt(n);
    uint max_idx = lmmp_prime_cnt16_(sqrt_n); // 筛质数个数上界（不含），质数从下标 5（=13）起

    /*
     * 筛质数状态同块分配（生命周期接力）：
     *   next[i]：第 i 个筛质数下一个待清除的全局位下标。保持单调的 p*p 起始
     *            （而非预先跳过旧区域），保证"已启动"质数构成前缀，act 前缀扫描
     *            才成立；旧区域的跳过在首个段内 O(1) 完成
     *   dslot[s]：标记位落在 slot s 时到下一标记位的位下标增量。标记为
     *            m = p*k（k 跑遍 >= p 的 wheel 数），k 到下一 wheel 数差
     *            g = gap30[k%30]，而 k ≡ m*inv(p) (mod 30) 只依赖 m%30 = res8[s]
     *            与 p，故增量可写成 s 的函数：
     *              dslot[s] = 8*((res8[s]+p*g)/30) + slot30[(res8[s]+p*g)%30] - s
     *            （slot 差为负时由字节进位吸收；含完整 p*g 而非 p%30，故每质数
     *             现算 8 项而非静态表）。转移 sigma(s) = (s + dslot[s]) & 7 必为
     *             单一 8 环（k 的余数历经全部 8 个，m = p*k 的余数亦然）。
     *   ds[8i+k]：第 i 个质数的增量按 slot 访问环次序存放；
     *   dpos[8i+s]：slot s 在环中的位置。按环序存放后标记循环展开 8 步取增量
     *            为常量索引，加载地址可提前计算，打破 idx→load→idx 依赖链
     *            （否则每次标记吃满 load 延迟；旧奇数编码常量步长无此税）。
     *            跨段/跳过旧区域后由 idx&7 经 dpos 重建环内相位
     */
    uchar* restrict blk = ALLOC_TYPE((size_t)max_idx * 44, uchar);
    uint* restrict next = (uint*)blk;
    uint* restrict ds = (uint*)(blk + 4 * (size_t)max_idx);
    uchar* restrict dpos = blk + 36 * (size_t)max_idx;
    for (uint i = 5; i < max_idx; ++i) {
        uint prime = prime_short_table[i];
        next[i] = WHEEL_IDX(prime * prime);
        uint inv = inv30_by_slot[slot30[prime % 30]];
        uint dslot[8];
        for (uint s = 0; s < 8; ++s) {
            uint d = prime * gap30[(res8[s] * inv) % 30];
            dslot[s] = 8 * ((res8[s] + d) / 30) + slot30[(res8[s] + d) % 30] - s;
        }
        // 自 slot 0 走 sigma 环：ds 按访问次序存放，dpos 记各 slot 的环内位置
        uint s = 0;
        for (uint k = 0; k < 8; ++k) {
            ds[8 * i + k] = dslot[s];
            dpos[8 * i + s] = (uchar)k;
            s = (s + dslot[s]) & 7;
        }
    }

    uint act = 5; // [5, act) 为已启动的筛质数前缀
    uint w = old_size;
    for (;;) {
        uint wend = w + PRIME_SEG_WORDS;
        uint last = 0;
        if (wend >= new_size) {
            wend = new_size;
            last = 1;
        }
        uint seg_hi = wend * LMMP_BITSET_BITS; // 段结束全局位（不含）
        uint end = (seg_hi <= limit_idx + 1) ? seg_hi : limit_idx + 1; // 本段标记上界（不含）

        // 段内 7*11 预筛填充（3、5 的倍数由 mod-30 存储结构性排除）。
        // 扩容增量不足一字时（new_size == old_size）无新字节可填，仅执行标记：
        // 有效增量位于旧表末字的陈旧 wheel 位上（皆为真合数，标记幂等）
        fill_pattern(p, w * 8, wend * 8, presieve77_table);

        // p*p 落入界内的质数开始参与（next 单调，构成前缀）
        while (act < max_idx && next[act] < end)
            ++act;

        for (uint i = 5; i < act; ++i) {
            uint prime = prime_short_table[i];
            uint idx = next[i];
            if (idx < skip_to) {
                // 仅首段发生：跳到 >= skip_num 的首个标记 m' = p*k'，k' 为
                // >= ceil(skip_num/p) 的最小 wheel 数（被跳过的 k 满足 p*k < skip_num，
                // 全在旧区域）。ulong 计算：p*k' 与 skip_num 均可能贴近 2^32
                ulong k = skip_num / prime + (skip_num % prime != 0);
                k += bump30[k % 30];
                ulong m = prime * k;
                idx = (uint)(8 * (m / 30) + slot30[m % 30]);
            }
            if (idx >= end) {
                // 本段无需标记（仅可能发生在最后一段），保留取整后的 idx
                next[i] = idx;
                continue;
            }
            const uint* restrict c8 = ds + 8 * i;
            uint k = dpos[8 * i + (idx & 7)];
            while (idx < end) {
                // 环序展开 8 步：增量索引为常量，加载地址提前可算，idx 链只含加法
                for (uint u = 0; u < 8 && idx < end; ++u, k = (k + 1) & 7) {
                    set_not_prime(p, idx);
                    idx += c8[k];
                }
            }
            next[i] = idx;
        }
        if (last)
            break;
        w = wend;
    }
    lmmp_free(blk);

    if (old_max == 0) {
        // 首次修正：1 不是素数；7、11 是素数（预筛基表将其自身当倍数抹除）。
        // 13..29 未被任何 >= 13*13 的标记触碰，自然保留
        p[0] = 0xFE;
    }
}

int lmmp_is_prime_table_(uint p) {
    if (p < PRIME_SHORT_TABLE_N) {
        ushort i = lmmp_prime_cnt16_(p);
        if (prime_short_table[i - 1] == p)
            return 1;
        return 0;
    } else if (p > G.max) {
        return -1;
    } else {
        // p>=3
        lmmp_param_assert(p > 1);
        lmmp_param_assert((p & 1) == 1);
        uint s = slot30[p % 30];
        if (s == 0xFF)
            return 0; // p > 65536 且被 3 或 5 整除，必为合数
        uint idx = 8 * (p / 30) + s;
        return (int)((((const uchar*)G.map)[idx >> 3] >> (idx & 7)) & 1);
    }
}

void lmmp_prime_cache_init_(prime_cache_t* cache, uint n) {
    lmmp_param_assert(n > 2);
    lmmp_param_assert(n <= G.max);
    cache->pp = ALLOC_TYPE(PRIME_CACHE_SIZE, uint);
    cache->size = 0;
    cache->start_idx = 0;
    cache->end_num = n;
    cache->end_idx = wheel_leq_idx(n) / LMMP_BITSET_BITS;
    cache->is_end = 0;
}

#define DEC_WORD(m, wbase)                                             \
    while (m) {                                                        \
        uint t = lmmp_tailing_zeros_(m);                               \
        begin[size++] = (uint)((wbase) + 30 * (t >> 3) + res8[t & 7]); \
        m &= (m - 1);                                                  \
    }

void lmmp_prime_cache_next_(prime_cache_t* cache) {
    uint size = 0;
    ulong idx = cache->start_idx;

    // 3、5 与 30 不互素，不在位图内；首次解码前补入（end_num >= 5 才含 5）
    if (cache->start_idx == 0) {
        cache->pp[size++] = 3;
        if (cache->end_num >= 5)
            cache->pp[size++] = 5;
    }

    if (idx + PRIME_CACHE_BLOCK_NUM <= cache->end_idx) {
        lmmp_bitset_t m0, m1, m2, m3;
        uintp begin = cache->pp;
        for (uint i = 0; i < PRIME_CACHE_BLOCK_NUM / 4; ++i) {
            m0 = G.map[idx + 0];
            m1 = G.map[idx + 1];
            m2 = G.map[idx + 2];
            m3 = G.map[idx + 3];
            ulong base = idx * 240; // 每字 8 字节 = 240 个数
            DEC_WORD(m0, base)
            base += 240;
            DEC_WORD(m1, base)
            base += 240;
            DEC_WORD(m2, base)
            base += 240;
            DEC_WORD(m3, base)
            idx += 4;
        }
        lmmp_debug_assert(idx - cache->start_idx == PRIME_CACHE_BLOCK_NUM);
        lmmp_debug_assert(size <= PRIME_CACHE_SIZE);
        cache->start_idx = idx;
        cache->size = size;
        return;
    } else {
        uint last_off = wheel_leq_idx(cache->end_num) & (LMMP_BITSET_BITS - 1);
        lmmp_bitset_t last_mask =
            (last_off == LMMP_BITSET_BITS - 1) ? (lmmp_bitset_t)-1 : (((lmmp_bitset_t)1 << (last_off + 1)) - 1);
        lmmp_bitset_t m;
        uintp begin = cache->pp;
        for (uint i = (uint)idx; i < cache->end_idx; ++i) {
            m = G.map[i];
            ulong base = (ulong)i * 240;
            DEC_WORD(m, base)
        }
        m = G.map[cache->end_idx] & last_mask;
        ulong base = (ulong)cache->end_idx * 240;
        DEC_WORD(m, base)
        lmmp_debug_assert(size <= PRIME_CACHE_SIZE);
        cache->start_idx = G.map_size;
        cache->size = size;
        cache->is_end = 1;
        return;
    }
}

#undef DEC_WORD
#undef WHEEL_IDX
#undef WHEEL_NUM
#undef set_not_prime

void lmmp_prime_cache_free_(prime_cache_t* cache) {
    lmmp_free(cache->pp);
    cache->pp = NULL;
}

ushort lmmp_prime_cnt16_(ushort n) {
    if (n < 2)
        return 0;
    if (n >= prime_short_table[PRIME_SHORT_TABLE_SIZE - 1])
        return PRIME_SHORT_TABLE_SIZE;

    ushort lo = 0;
    ushort hi = PRIME_SHORT_TABLE_SIZE;  // [lo, hi)

    while (lo < hi) {
        ushort mid = lo + (hi - lo) / 2;
        if (prime_short_table[mid] <= n) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

void lmmp_prime_int_table_free_(void) {
    lmmp_free(G.map);
    G.map = NULL;
    G.max = 0;
    G.map_size = 0;
}
