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

#include "../../../include/lmmp/impl/fft_ssa.h"
#include "../../../include/lmmp/impl/mparam.h"


// ((mp_size_t)3 << (2 * (n) - 5)) + 1 是预计算的阈值，n是对应的k值
#define _FFT_TABLE_ENTRY(n) {((mp_size_t)3 << (2 * (n) - 5)) + 1, (n)}
#define _FFT_TABLE_ENTRY4(n) \
    _FFT_TABLE_ENTRY(n), _FFT_TABLE_ENTRY((n) + 1), _FFT_TABLE_ENTRY((n) + 2), _FFT_TABLE_ENTRY((n) + 3)

// best_k_(next_size_(n)) = best_k_(n)
// table[i+1][0]-1 必须是 2^(table[i][1]-LOG2_LIMB_BITS) 的整数倍
// LOG2_LIMB_BITS：每个 limb 的比特数的2对数，为 log2(64) = 6
static const mp_size_t lmmp_fft_table_[][2] = {
    {0, 6},
    {1597, 7},
    {1655, 6},
    {1917, 7},
    {3447, 8},
    {3565, 7},
    {3831, 8},
    {7661, 9},
    {8145, 8},
    {8685, 9},
    {14289, 10},
    {16289, 9},
    {20433, 10},
    {24481, 9},
    {26577, 10},
    {28593, 11},
    {32545, 10},
    {57249, 11},
    {65313, 10},
    {73633, 11},
    {98081, 12},
    {130625, 11},
    {196385, 12},
    {261697, 11},
    {294689, 12},
    {392769, 13},
    {523265, 12},
    {654913, 11},
    {917281, 13},
    {1047553, 11},
    {1600001, 12},
    {1834561, 14},
    {2095105, 12},
    _FFT_TABLE_ENTRY4(13),
    _FFT_TABLE_ENTRY4(17),
    _FFT_TABLE_ENTRY4(21),
    _FFT_TABLE_ENTRY4(25),
    {(mp_size_t)-1, 127}};

mp_size_t lmmp_fft_best_k_(mp_size_t n) {
    mp_size_t k = 0;
    while (n >= lmmp_fft_table_[k + 1][0]) ++k;
    return lmmp_fft_table_[k][1];
}

mp_size_t lmmp_fft_next_size_(mp_size_t n) {
    mp_size_t k = lmmp_fft_best_k_(n);
    lmmp_debug_assert(k >= LOG2_LIMB_BITS);
    k -= LOG2_LIMB_BITS;
    n = (((n - 1) >> k) + 1) << k;
    return n;
}

void* lmmp_fft_memstack_(fft_memstack* ms, mp_size_t size) {
    if (size) {
        if (++ms->tempdepth > ms->maxdepth) {
            lmmp_debug_assert(ms->maxdepth + 1 < FFT_MEMSTACK_DEPTH);
            ms->mem[++ms->maxdepth] = lmmp_alloc(size);
            ms->memsize[ms->maxdepth] = size;
        }
        lmmp_debug_assert(ms->memsize[ms->tempdepth] == size);
        return ms->mem[ms->tempdepth];
    } else {
        if (--ms->tempdepth < 0) {
            for (mp_size_t i = 0; i <= (mp_size_t)(ms->maxdepth); ++i) lmmp_free(ms->mem[i]);
            ms->maxdepth = -1;
        }
        return 0;
    }
}

void lmmp_fft_extract_coef_(mp_ptr dst, mp_srcptr numa, mp_size_t bitoffset, mp_size_t bits, mp_size_t lenw) {
    mp_size_t shr = bitoffset & (LIMB_BITS - 1), offset = bitoffset / LIMB_BITS;

    mp_size_t lena = (bitoffset + bits - 1) / LIMB_BITS - offset + 1, endp = (bits - 1) / LIMB_BITS;

    if (shr)
        lmmp_shr_(dst, numa + offset, lena, shr);
    else
        lmmp_copy(dst, numa + offset, lena);

    dst[endp] &= LIMB_MAX >> (-bits & (LIMB_BITS - 1));

    lmmp_zero(dst + endp + 1, lenw - endp);
}

void lmmp_fft_shl_coef_(fft_memstack* ms, mp_ptr* coef, mp_size_t shl) {
    mp_size_t l = ms->lenw;         // 系数的机器字长度
    mp_size_t w = shl / LIMB_BITS;  // 左移的机器字数量
    shl &= LIMB_BITS - 1;           // 剩余的比特偏移（0~LIMB_BITS-1）
    mp_ptr src = *coef;             // 源系数数组
    mp_ptr dst = ms->temp_coef;     // 目标临时数组
    mp_limb_t cc, rd;               // 进位变量（cc=carry, rd=read）

    if (w >= l) {
        w -= l;
        if (shl) {
            lmmp_shl_(dst, src + l - w, w + 1, shl);
            rd = dst[w];
            cc = lmmp_shlnot_(dst + w, src, l - w, shl);
        } else {
            if (w)
                lmmp_copy(dst, src + l - w, w);
            rd = src[l];
            lmmp_not_(dst + w, src, l - w);
            cc = 0;
        }
        dst[l] = 0;
        ++cc;
        lmmp_inc_1(dst, cc);

        if (++rd == 0)
            lmmp_inc(dst + w + 1);
        else
            lmmp_inc_1(dst + w, rd);
    } else {
        if (shl) {
            lmmp_shlnot_(dst, src + l - w, w + 1, shl);
            rd = ~dst[w];
            cc = lmmp_shl_(dst + w, src, l - w, shl);
        } else {
            if (w)
                lmmp_not_(dst, src + l - w, w);
            rd = src[l];

            lmmp_copy(dst + w, src, l - w);
            cc = 0;
        }
        dst[l] = 2;
        lmmp_inc_1(dst, 3);
        lmmp_dec_1(dst, cc);

        if (++rd == 0)
            lmmp_dec(dst + w + 1);
        else
            lmmp_dec_1(dst + w, rd);

        cc = dst[l];
        dst[l] = dst[0] < cc;
        lmmp_dec_1(dst, cc - dst[l]);
    }

    ms->temp_coef = src;
    *coef = dst;
}

void lmmp_fft_shr_coef_(fft_memstack* ms, mp_ptr* coef, mp_size_t shr) {
    lmmp_fft_shl_coef_(ms, coef, 2 * ms->lenw * LIMB_BITS - shr);
}

void lmmp_fft_bfy_(fft_memstack* ms, mp_ptr* coef, mp_size_t wing, mp_size_t w) {
    mp_ptr numa = coef[0];                // 系数a
    mp_ptr numb = coef[wing];             // 系数b
    mp_ptr numc = ms->temp_coef;          // 临时数组（存储a-b<<w）
    mp_size_t shl = w & (LIMB_BITS - 1);  // 比特级左移量
    w /= LIMB_BITS;                       // 机器字级左移量
    mp_size_t l = ms->lenw;               // 系数长度（机器字）

    mp_slimb_t acyo = 0, scyo = 0;
    mp_limb_t shlcyo = 0, chp = 0, chn = 0;

    for (mp_size_t off = 0; off < l - w; off += PART_SIZE) {
        mp_size_t cursize = LMMP_MIN(l - w - off, PART_SIZE);
        scyo = lmmp_sub_nc_(numc + w + off, numa + off, numb + off, cursize, scyo);
        acyo = lmmp_add_nc_(numa + off, numa + off, numb + off, cursize, acyo);
        if (shl)
            shlcyo = lmmp_shl_c_(numc + w + off, numc + w + off, cursize, shl, shlcyo);
    }

    {
        /* 等价于 ch = shlcyo - (scyo << shl)，使用无符号运算避免有符号左移 UB */
        mp_limb_t adj = (mp_limb_t)scyo << shl;
        if (shlcyo >= adj) {
            chp = shlcyo - adj;
        } else {
            chn = adj - shlcyo;
        }
    }

    scyo = 0;
    shlcyo = 0;

    for (mp_size_t off = l - w; off < l; off += PART_SIZE) {
        mp_size_t cursize = LMMP_MIN(l - off, PART_SIZE);
        scyo = lmmp_sub_nc_(numc + off - (l - w), numb + off, numa + off, cursize, scyo);
        acyo = lmmp_add_nc_(numa + off, numa + off, numb + off, cursize, acyo);
        if (shl)
            shlcyo = lmmp_shl_c_(numc + off - (l - w), numc + off - (l - w), cursize, shl, shlcyo);
    }

    numc[w] += shlcyo;                 // 左移进位加到numc[w]
    scyo = -scyo + numb[l] - numa[l];  // 调整借位（包含最高位）
    acyo += numa[l] + numb[l];         // 调整进位（包含最高位）

    numa[l] = numa[0] < (mp_limb_t)(acyo);
    lmmp_dec_1(numa, acyo - numa[l]);

    numc[l] = 1;
    ++chn;
    if (scyo > 0)
        lmmp_inc_1(numc + w, (mp_limb_t)scyo << shl);
    else if (scyo < 0) {
        if (scyo == -2 && shl == LIMB_BITS - 1)
            lmmp_dec(numc + w + 1);
        else
            lmmp_dec_1(numc + w, (mp_limb_t)(-scyo) << shl);
    }
    chp += numc[l];

    if (chn >= chp) {
        numc[l] = 0;
        lmmp_inc_1(numc, chn - chp);
    } else {
        chp -= chn;
        numc[l] = numc[0] < chp;
        lmmp_dec_1(numc, chp - numc[l]);
    }

    coef[wing] = numc;
    ms->temp_coef = numb;
}

void lmmp_ifft_bfy_(fft_memstack* ms, mp_ptr* coef, mp_size_t wing, mp_size_t w) {
    mp_ptr numa = coef[0];
    mp_ptr numb = coef[wing];
    mp_ptr numc = ms->temp_coef;
    mp_size_t shr = w & (LIMB_BITS - 1);
    w /= LIMB_BITS;
    mp_size_t l = ms->lenw;

    mp_slimb_t bcyo = 0, acyo = 0, ah;
    mp_limb_t shrcyo = shr ? numb[0] << (LIMB_BITS - shr) : 0;

    for (mp_size_t off = l - w; off < l; off += PART_SIZE) {
        mp_size_t cursize = LMMP_MIN(l - off, PART_SIZE);
        if (shr)
            lmmp_shr_c_(numb + off - (l - w), numb + off - (l - w), cursize, shr,
                        numb[off - (l - w) + cursize] << (LIMB_BITS - shr));
        bcyo = lmmp_add_nc_(numc + off, numa + off, numb + off - (l - w), cursize, bcyo);
        acyo = lmmp_sub_nc_(numa + off, numa + off, numb + off - (l - w), cursize, acyo);
    }

    for (mp_size_t off = 0; off < l - w; off += PART_SIZE) {
        mp_size_t cursize = LMMP_MIN(l - w - off, PART_SIZE);
        if (shr)
            lmmp_shr_c_(numb + w + off, numb + w + off, cursize, shr, numb[off + w + cursize] << (LIMB_BITS - shr));
        bcyo = lmmp_sub_nc_(numc + off, numa + off, numb + w + off, cursize, bcyo);
        acyo = lmmp_add_nc_(numa + off, numa + off, numb + w + off, cursize, acyo);
    }

    acyo += numb[l] >> shr;
    bcyo = -bcyo - (numb[l] >> shr);

    acyo -= numa[l - w - 1] < shrcyo;
    numa[l - w - 1] -= shrcyo;
    numc[l - w - 1] += shrcyo;
    bcyo += numc[l - w - 1] < shrcyo;

    ah = numa[l];

    numa[l] += 1;
    if (w == 0)
        numa[l] += acyo;
    else {
        if (acyo < 0)
            lmmp_dec(numa + l - w);
        else
            lmmp_inc_1(numa + l - w, acyo);
    }
    acyo = numa[l] - 1;
    if (acyo < 0) {
        numa[l] = 0;
        lmmp_inc(numa);
    } else {
        numa[l] = numa[0] < (mp_limb_t)acyo;
        lmmp_dec_1(numa, acyo - numa[l]);
    }

    numc[l] = ah + 2;
    if (w == 0)
        numc[l] += bcyo;
    else {
        if (bcyo > 0)
            lmmp_inc(numc + l - w);
        else
            lmmp_dec_1(numc + l - w, -bcyo);
    }
    bcyo = numc[l] - 2;
    if (bcyo <= 0) {
        numc[l] = 0;
        lmmp_inc_1(numc, -bcyo);
    } else {
        numc[l] = numc[0] < (mp_limb_t)bcyo;
        lmmp_dec_1(numc, bcyo - numc[l]);
    }

    coef[wing] = numc;
    ms->temp_coef = numb;
}

static void lmmp_fft_b1_(fft_memstack* ms, mp_ptr* coef, mp_size_t dis, mp_size_t k, mp_size_t w, mp_size_t w0) {
    if (k == 1)
        lmmp_fft_bfy_(ms, coef, dis, w0);
    else {
        k -= 2;
        mp_size_t Kq = dis << k;
        for (mp_size_t i = 0; i < Kq; i += dis) {
            lmmp_fft_bfy_(ms, coef + i, 2 * Kq, i * w + w0);
            lmmp_fft_bfy_(ms, coef + i + Kq, 2 * Kq, (i + Kq) * w + w0);
            lmmp_fft_bfy_(ms, coef + i, Kq, 2 * (i * w + w0));
            lmmp_fft_bfy_(ms, coef + i + Kq * 2, Kq, 2 * (i * w + w0));
        }
        if (k > 0) {
            lmmp_fft_b1_(ms, coef, dis, k, 4 * w, 4 * w0);
            lmmp_fft_b1_(ms, coef + Kq, dis, k, 4 * w, 4 * w0);
            lmmp_fft_b1_(ms, coef + Kq * 2, dis, k, 4 * w, 4 * w0);
            lmmp_fft_b1_(ms, coef + Kq * 3, dis, k, 4 * w, 4 * w0);
        }
    }
}

static void lmmp_fft_4_(fft_memstack* ms, mp_ptr* coef, mp_size_t k, mp_size_t w) {
    if (k == 1)
        lmmp_fft_bfy_(ms, coef, 1, 0);
    else {
        k -= 2;
        mp_size_t Kq = ((mp_size_t)1) << k;
        for (mp_size_t i = 0; i < Kq; ++i) {
            lmmp_fft_bfy_(ms, coef + i, Kq * 2, i * w);
            lmmp_fft_bfy_(ms, coef + i + Kq, Kq * 2, (i + Kq) * w);
            lmmp_fft_bfy_(ms, coef + i, Kq, 2 * i * w);
            lmmp_fft_bfy_(ms, coef + i + 2 * Kq, Kq, 2 * i * w);
        }
        if (k > 0) {
            lmmp_fft_4_(ms, coef, k, w * 4);
            lmmp_fft_4_(ms, coef + Kq, k, w * 4);
            lmmp_fft_4_(ms, coef + 2 * Kq, k, w * 4);
            lmmp_fft_4_(ms, coef + 3 * Kq, k, w * 4);
        }
    }
}

void lmmp_fft_(fft_memstack* ms, mp_ptr* coef, mp_size_t k, mp_size_t w) {
    mp_size_t k1 = k >> 1;
    k -= k1;
    mp_size_t Kp = ((mp_size_t)1) << k;
    mp_size_t Kq = ((mp_size_t)1) << k1;

    for (mp_size_t i = 0; i < Kp; ++i) lmmp_fft_b1_(ms, coef + i, Kp, k1, w, i * w);

    for (mp_size_t i = 0; i < Kq; ++i) lmmp_fft_4_(ms, coef + Kp * i, k, w * Kq);
}

static void lmmp_ifft_b1_(fft_memstack* ms, mp_ptr* coef, mp_size_t dis, mp_size_t k, mp_size_t w, mp_size_t w0) {
    if (k == 1)
        lmmp_ifft_bfy_(ms, coef, dis, w0);
    else {
        k -= 2;
        mp_size_t Kq = dis << k;
        if (k > 0) {
            lmmp_ifft_b1_(ms, coef, dis, k, 4 * w, 4 * w0);
            lmmp_ifft_b1_(ms, coef + Kq, dis, k, 4 * w, 4 * w0);
            lmmp_ifft_b1_(ms, coef + Kq * 2, dis, k, 4 * w, 4 * w0);
            lmmp_ifft_b1_(ms, coef + Kq * 3, dis, k, 4 * w, 4 * w0);
        }
        for (mp_size_t i = 0; i < Kq; i += dis) {
            lmmp_ifft_bfy_(ms, coef + i, Kq, 2 * (i * w + w0));
            lmmp_ifft_bfy_(ms, coef + i + Kq * 2, Kq, 2 * (i * w + w0));
            lmmp_ifft_bfy_(ms, coef + i, 2 * Kq, i * w + w0);
            lmmp_ifft_bfy_(ms, coef + i + Kq, 2 * Kq, (i + Kq) * w + w0);
        }
    }
}

static void lmmp_ifft_4_(fft_memstack* ms, mp_ptr* coef, mp_size_t k, mp_size_t w) {
    if (k == 1)
        lmmp_ifft_bfy_(ms, coef, 1, 0);
    else {
        k -= 2;
        mp_size_t Kq = ((mp_size_t)1) << k;
        if (k > 0) {
            lmmp_ifft_4_(ms, coef, k, w * 4);
            lmmp_ifft_4_(ms, coef + Kq, k, w * 4);
            lmmp_ifft_4_(ms, coef + 2 * Kq, k, w * 4);
            lmmp_ifft_4_(ms, coef + 3 * Kq, k, w * 4);
        }
        for (mp_size_t i = 0; i < Kq; ++i) {
            lmmp_ifft_bfy_(ms, coef + i, Kq, 2 * i * w);
            lmmp_ifft_bfy_(ms, coef + i + 2 * Kq, Kq, 2 * i * w);
            lmmp_ifft_bfy_(ms, coef + i, Kq * 2, i * w);
            lmmp_ifft_bfy_(ms, coef + i + Kq, Kq * 2, (i + Kq) * w);
        }
    }
}

void lmmp_ifft_(fft_memstack* ms, mp_ptr* coef, mp_size_t k, mp_size_t w) {
    mp_size_t k1 = k >> 1;
    k -= k1;
    mp_size_t Kp = ((mp_size_t)1) << k;
    mp_size_t Kq = ((mp_size_t)1) << k1;

    for (mp_size_t i = 0; i < Kq; ++i) lmmp_ifft_4_(ms, coef + Kp * i, k, w * Kq);

    for (mp_size_t i = 0; i < Kp; ++i) lmmp_ifft_b1_(ms, coef + i, Kp, k1, w, i * w);
}

void lmmp_mul_fermat_recombine_(
    fft_memstack* ms,
    mp_ptr       dst,
    mp_ptr*     pfca,
    mp_size_t      K,
    mp_size_t      k,
    mp_size_t      n,
    mp_size_t      M,
    mp_size_t     rn
) {
    mp_size_t rhead = 0, nlen = ms->lenw + 1;
    mp_slimb_t borrow = 0, maxc = 0;

    for (mp_size_t i = 0; i < K; ++i) {
        lmmp_fft_shr_coef_(ms, pfca + i, (i * n >> k) + k);
        mp_ptr nums = pfca[i];

        if (nums[nlen - 1]) {
            lmmp_dec(nums);
            --nums[nlen - 1];
        }
        if (nums[nlen - 1] == 0 && nums[nlen - 2] >> (LIMB_BITS - 1)) {
            lmmp_dec(nums);
            --nums[nlen - 1];
        }

        if (borrow) {
            mp_size_t brshift = borrow - 1 + n - M;
            mp_size_t bshl = brshift & (LIMB_BITS - 1);
            brshift /= LIMB_BITS;
            --nums[nlen - 1];
            lmmp_dec_1(nums + brshift, (mp_limb_t)1 << bshl);
            ++nums[nlen - 1];
        }
        borrow = -nums[nlen - 1];
        nums[nlen - 1] = 0;

        mp_size_t roffset = i * M;
        mp_size_t shl = roffset & (LIMB_BITS - 1);
        roffset /= LIMB_BITS;

        if (shl)
            lmmp_shl_(nums, nums, nlen, shl);

        if (i == 0) {
            lmmp_copy(dst, nums, nlen);
            rhead = nlen;
        } else if (roffset + nlen <= rn) {
            lmmp_add_(dst + roffset, nums, nlen, dst + roffset, rhead - roffset);
            rhead = roffset + nlen;
        } else {
            maxc += lmmp_add_(dst + roffset, nums, rn - roffset, dst + roffset, rhead - roffset);
            maxc -= lmmp_sub_(dst, dst, rn, nums + rn - roffset, nlen + roffset - rn);
            rhead = rn;
        }
    }

    if (borrow) {
        mp_size_t cyshift = borrow - 1 + n - M;
        mp_size_t cshl = cyshift & (LIMB_BITS - 1);
        cyshift /= LIMB_BITS;
        maxc += lmmp_add_1_(dst + cyshift, dst + cyshift, rn - cyshift, (mp_limb_t)1 << cshl);
    }

    if (maxc > 0) {
        dst[rn] = dst[0] < (mp_limb_t)maxc;
        lmmp_dec_1(dst, maxc - dst[rn]);
    } else {
        dst[rn] = 0;
        lmmp_inc_1(dst, -maxc);
    }
}
