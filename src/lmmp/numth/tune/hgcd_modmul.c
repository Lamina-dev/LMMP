/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  This library is free software: you can redistribute it and/or modify it
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

#include "mparam.h"
#include "impl/inlines.h"
#include "impl/mul_cache.h"
#include "numth/gcd.h"
#include "numth/tune/gcd_tune.h"

/* 与 hgcd.c 的 lmmp_hgcd_adjust_、lmmp_hgcd_apply_mod_ 同步。调优时只替换
   子递归矩阵的应用方式，保留乘法分派、模长取整和缓存的创建与释放。 */

static mp_size_t tune_hgcd_adjust(
    lmmp_hgcd_matrix_t*  M1,
    int                  detsign,
    mp_ptr               dap,
    mp_ptr               dbp,
    mp_size_t*           blen,
    mp_srcptr            ha,
    mp_srcptr            hb,
    mp_size_t            hn0,
    mp_srcptr            ap,
    mp_srcptr            bp,
    mp_size_t            n,
    mp_size_t            p,
    lmmp_hgcd_scratch_t* sc
) {
    mp_size_t mn = lmmp_gcd_matrix_maxlen_(M1);
    mp_size_t mark = sc->used;
    mp_size_t N = p + mn + 1;
    mp_ptr t0 = lmmp_hgcd_salloc_(sc, N);
    mp_ptr t1 = lmmp_hgcd_salloc_(sc, N);
    mp_ptr t2 = lmmp_hgcd_salloc_(sc, N);
    mp_ptr t3 = lmmp_hgcd_salloc_(sc, N);
    /* 长度前提（由子递归的下限不变量保证，防御性检查） */
    if (n - p < mn || n < p + mn) {
        *blen = 0;
        sc->used = mark;
        return 0;
    }

    /*
    det 由穿线符号跟踪（构造保证 = ±1）。零元素（长度 0）跳过乘法——
    lmmp_mul_ 契约要求 nb>0，乘积缓冲置零以维持后续加减的定长语义。
    乘积仅写 [0, p+n_ij)，而后续加减按 mn 定长读取 [p, p+mn)：
    元素长度 n_ij < mn 时须补零 [p+n_ij, p+mn)，否则读到未写残留
    */
    int det = detsign;
    mp_size_t n11 = M1->n[1][1], n10 = M1->n[1][0], n01 = M1->n[0][1], n00 = M1->n[0][0];
    if (n11 != 0) {
        lmmp_gcd_mul_(t0, ap, p, M1->m[1][1], n11); /* m11*a_lo */
        lmmp_zero(t0 + p + n11, mn - n11);
    } else {
        lmmp_zero(t0, N);
    }
    if (n10 != 0) {
        lmmp_gcd_mul_(t1, ap, p, M1->m[1][0], n10); /* m10*a_lo */
        lmmp_zero(t1 + p + n10, mn - n10);
    } else {
        lmmp_zero(t1, N);
    }
    if (n01 != 0) {
        lmmp_gcd_mul_(t2, bp, p, M1->m[0][1], n01); /* m01*b_lo */
        lmmp_zero(t2 + p + n01, mn - n01);
    } else {
        lmmp_zero(t2, N);
    }
    if (n00 != 0) {
        lmmp_gcd_mul_(t3, bp, p, M1->m[0][0], n00); /* m00*b_lo */
        lmmp_zero(t3 + p + n00, mn - n00);
    } else {
        lmmp_zero(t3, N);
    }

    /*
    det=+1：new_a = ha + t0 - t2，new_b = hb + t3 - t1
    det=-1：new_a = ha + t2 - t0，new_b = hb + t1 - t3
    */
    mp_ptr adda = det > 0 ? t0 : t2;
    mp_ptr suba = det > 0 ? t2 : t0;
    mp_ptr addb = det > 0 ? t3 : t1;
    mp_ptr subb = det > 0 ? t1 : t3;

    /* new_a：低 p 位直写被加积，高位段为 ha 再加被加积高位，最后整体减被减积 */
    lmmp_copy(dap + p, ha, hn0);
    lmmp_zero(dap + p + hn0, n - p - hn0);
    lmmp_copy(dap, adda, p);
    mp_limb_t carry = lmmp_add_(dap + p, dap + p, n - p, adda + p, mn);
    if (carry != 0) {
        sc->used = mark;
        return 0;
    }
    mp_limb_t borrow = lmmp_sub_(dap, dap, n, suba, p + mn);
    if (borrow != 0) {
        sc->used = mark;
        return 0; /* 过冲：new_a < 0 */
    }

    /* new_b：同构 */
    lmmp_copy(dbp + p, hb, hn0);
    lmmp_zero(dbp + p + hn0, n - p - hn0);
    lmmp_copy(dbp, addb, p);
    carry = lmmp_add_(dbp + p, dbp + p, n - p, addb + p, mn);
    if (carry != 0) {
        sc->used = mark;
        return 0;
    }
    borrow = lmmp_sub_(dbp, dbp, n, subb, p + mn);
    if (borrow != 0) {
        sc->used = mark;
        return 0; /* 过冲：new_b < 0 */
    }

    /*
    结果分量经加减抵消，长度无进位信息可用，须扫描获取（数对输出）；
    返回公共长度 rn = max(la,lb)，blen 出参单独给出分量 b 的长度
    （供上层校验，免去二次扫描）。数对为零填充公共长度表示：输出
    需将两分量补零至 rn（上层的规范化序比较按公共长度读取）
    */
    mp_size_t la = n;
    while (la > 0 && dap[la - 1] == 0) {
        --la;
    }
    mp_size_t lb = n;
    while (lb > 0 && dbp[lb - 1] == 0) {
        --lb;
    }
    mp_size_t rn = la > lb ? la : lb;
    lmmp_zero(dap + la, rn - la);
    lmmp_zero(dbp + lb, rn - lb);
    *blen = lb;
    sc->used = mark;
    return rn;
}

static inline mp_size_t tune_hgcd_bound(mp_size_t an, mp_size_t bn, mp_size_t m00, mp_size_t m01,
                                        mp_size_t m10, mp_size_t m11) {
    /* 结果 limb 上界：new_a < B^min(an-m00, bn-m10)+1 等 */
    mp_size_t un = (an > m00 ? an - m00 : 1);
    if (bn - m10 < un) un = bn > m10 ? bn - m10 : 1;
    mp_size_t vn = (an > m01 ? an - m01 : 1);
    if (bn > m11 && bn - m11 < vn) vn = bn - m11;
    mp_size_t nn = un > vn ? un : vn;
    ++nn;

    return nn;
}

static void tune_hgcd_fold_mod(mp_ptr dst, mp_srcptr src, mp_size_t sn, mp_size_t rn) {
    lmmp_copy(dst, src, sn <= rn ? sn : rn);
    lmmp_zero(dst + (sn <= rn ? sn : rn), rn - (sn <= rn ? sn : rn));
    while (sn > rn) {
        src += rn;
        sn -= rn;
        mp_limb_t cy = lmmp_add_(dst, dst, rn, src, sn <= rn ? sn : rn);
        if (cy != 0) {
            lmmp_inc(dst);
        }
    }
}

/**
 * @brief 模 B^rn-1 梅森乘法版矩阵应用：
 *        new = adj(M1)*(a;b)，经 4 次模乘与模减精确重构
 * @param M1 子递归累积的矩阵（只读，各元素长度为 M1->n 个limb，M1->n<rn）
 * @param detsign det(M1) 的符号，[1|-1]
 * @param dap 分量 a 输出数组（写入 rn_out 个limb，rn_out 为返回值；返回 0 时内容无效）
 * @param dbp 分量 b 输出数组（写入 rn_out 个limb）
 * @param ap 源完整较大分量（只读，长度为 n 个limb）
 * @param bp 源完整较小分量（只读，长度为 n 个limb）
 * @param n ap,bp 的公共长度
 * @param sc 临时空间（6*rn 个limb；FFT 缓存上下文另行经由库内临时池分配）
 * @return 合入后的公共长度 rn_out（两分量真实长度最大值）；0 表示过冲
 * @warning M1!=NULL, dap!=NULL, dbp!=NULL, sc!=NULL, sep([dap|dbp],[ap|bp]),
 *          n>0, M1->n>1, ap[n-1]>0
 * @note 真值满足 0 <= new < B^nn <= B^(rn-1)（nn 为下方推导的界），
 *       过冲（new < 0）时模结果回绕为接近 rn 长度，据此刻检测并返回 0。
 *       数对经梅森折叠到 rn limb 参与模乘；矩阵元素以原始长度 M1->n 直接参与
 *      （M1->n<rn 保证落在模内）。fa/fb 各被两个矩阵元素乘，经
 *       lmmp_mul_mersenne_cache_ 系列复用其前向 FFT 变换
 */
static mp_size_t tune_hgcd_apply_mod(
    lmmp_hgcd_matrix_t*  M1,
    int                  detsign,
    mp_ptr               dap,
    mp_ptr               dbp,
    mp_size_t*           blen,
    mp_srcptr            ap,
    mp_srcptr            bp,
    mp_size_t            n,
    int                  use_cache,
    lmmp_hgcd_scratch_t* sc
) {
    mp_size_t mark = sc->used;
    mp_size_t m00 = M1->n[0][0];
    mp_size_t m01 = M1->n[0][1];
    mp_size_t m10 = M1->n[1][0];
    mp_size_t m11 = M1->n[1][1];
    mp_size_t an = n;
    /* 数对为零填充公共长度表示，b 真实长度显式信息不可得，须扫描
       （下限不变量保证 bn >= fl，扫描量有界） */
    mp_size_t bn = lmmp_gcd_norm_(bp, n);

    mp_size_t nn = tune_hgcd_bound(an, bn, m00, m01, m10, m11);

    mp_size_t rn = use_cache ? lmmp_fft_next_size_(nn + 1)      /* 缓存档：仅 fft 尺寸可用 */
                             : lmmp_mersenne_next_size_(nn + 1); /* 分发档：细粒度低 padding */
    if (rn <= lmmp_gcd_matrix_maxlen_(M1) || rn > n + 1) {
        *blen = 0;
        sc->used = mark;
        return 0; /* 尺寸关系异常（防御） */
    }

    /* 梅森折叠与填充 */
    mp_ptr fa = lmmp_hgcd_salloc_(sc, rn);
    mp_ptr fb = lmmp_hgcd_salloc_(sc, rn);
    tune_hgcd_fold_mod(fa, ap, an, rn);
    tune_hgcd_fold_mod(fb, bp, bn, rn);

    mp_ptr t11 = lmmp_hgcd_salloc_(sc, rn);
    mp_ptr t01 = lmmp_hgcd_salloc_(sc, rn);
    mp_ptr t00 = lmmp_hgcd_salloc_(sc, rn);
    mp_ptr t10 = lmmp_hgcd_salloc_(sc, rn);
    /*
    逐元素长度可能相差且可为 0（矩阵元素平衡，通常 0~2 limb），显式补零
    对齐到 ml（行列式非零保证 ml1/ml2 >= 1）：缓存版须等长操作数，
    分发版亦须 nb>=1
    */
    mp_size_t ml1 = LMMP_MAX(m00, m01);
    mp_size_t ml2 = LMMP_MAX(m10, m11);
    lmmp_zero(M1->m[0][0] + m00, ml1 - m00);
    lmmp_zero(M1->m[0][1] + m01, ml1 - m01);
    lmmp_zero(M1->m[1][0] + m10, ml2 - m10);
    lmmp_zero(M1->m[1][1] + m11, ml2 - m11);
    if (use_cache) {
        /* fa/fb 前向变换缓存，第二次模乘省去一次全尺寸前向变换 */
        fft_gr_cache ca, cb;
        lmmp_mul_mersenne_cache_init_(t11, rn, M1->m[1][1], ml2, fa, rn, &ca);
        lmmp_mul_mersenne_cache_(t10, M1->m[1][0], &ca);
        lmmp_mul_mersenne_cache_init_(t01, rn, M1->m[0][1], ml1, fb, rn, &cb);
        lmmp_mul_mersenne_cache_(t00, M1->m[0][0], &cb);
        lmmp_fft_gr_cache_free_(&ca);
        lmmp_fft_gr_cache_free_(&cb);
    } else {
        /* 分发入口不缓存（长操作数 fa/fb 置于 numa 满足 nb<=na 契约） */
        lmmp_mul_mersenne_(t11, rn, fa, rn, M1->m[1][1], ml2);
        lmmp_mul_mersenne_(t10, rn, fa, rn, M1->m[1][0], ml2);
        lmmp_mul_mersenne_(t01, rn, fb, rn, M1->m[0][1], ml1);
        lmmp_mul_mersenne_(t00, rn, fb, rn, M1->m[0][0], ml1);
    }

    /* 模减 x - y (mod B^rn-1)。借位时减 1，将加 B^rn 改为加 B^rn-1。 */
    mp_ptr va, vb;
    mp_limb_t cy;
    if (detsign > 0) {
        va = t11; vb = t00;
        cy = lmmp_sub_n_(va, t11, t01, rn);
        if (cy) lmmp_dec(va);
        cy = lmmp_sub_n_(vb, t00, t10, rn);
        if (cy) lmmp_dec(vb);
    } else {
        va = t01; vb = t10;
        cy = lmmp_sub_n_(va, t01, t11, rn);
        if (cy) lmmp_dec(va);
        cy = lmmp_sub_n_(vb, t10, t00, rn);
        if (cy) lmmp_dec(vb);
    }

    /* 真值校验：结果必须落在 nn limb 内（负值回绕会产生接近 rn 的长度） */
    /* 模减回绕结果无进位信息，长度须扫描（数对中间量） */
    mp_size_t alen = lmmp_gcd_norm_(va, rn);
    mp_size_t lb = lmmp_gcd_norm_(vb, rn);
    if (alen > nn || lb > nn) {
        *blen = 0;
        sc->used = mark;
        return 0; /* 过冲 */
    }

    mp_size_t rn_out = alen > lb ? alen : lb;
    if (rn_out == 0) {
        *blen = 0;
        sc->used = mark;
        return 0;
    }
    *blen = lb;
    lmmp_copy(dap, va, alen);
    lmmp_zero(dap + alen, rn_out - alen);
    lmmp_copy(dbp, vb, lb);
    lmmp_zero(dbp + lb, rn_out - lb);
    sc->used = mark;
    return rn_out;
}

enum { ADJUST, MERSENNE, CACHE, PATH_COUNT };
enum { FIRST, SECOND, STAGE_COUNT };
enum { MIN_N = 45, MAX_N = 16384, CACHE_MIN = 256 };

static const char program[] = "tune_numth_hgcd_modmul";
static const char low_parameter[] = "HGCD_MODMUL_THRESHOLD";
static const char cache_parameter[] = "HGCD_MODMUL_CACHE_THRESHOLD";

typedef struct {
    lmmp_hgcd_matrix_t matrix;
    lmmp_hgcd_scratch_t scratch;
    mp_ptr a, b, ha, hb, matrix_buf, out_a, out_b, ref_a, ref_b;
    mp_size_t n, p, hn;
    int det;
} hgcd_input;

typedef struct {
    hgcd_input* input;
    int path;
} hgcd_bench;

typedef struct {
    int n, stage;
    double adjust_ratios[GCD_TUNE_MAX_ROUNDS], cache_ratios[GCD_TUNE_MAX_ROUNDS];
} point;

static mp_size_t apply(hgcd_input* in, int path, mp_ptr a, mp_ptr b, mp_size_t* bn) {
    if (path == ADJUST)
        return tune_hgcd_adjust(&in->matrix, in->det, a, b, bn, in->ha, in->hb, in->hn,
                                in->a, in->b, in->n, in->p, &in->scratch);
    return tune_hgcd_apply_mod(&in->matrix, in->det, a, b, bn, in->a, in->b, in->n, path == CACHE, &in->scratch);
}

static mp_limb_t call_apply(void* opaque) {
    hgcd_bench* bench = opaque;
    hgcd_input* in = bench->input;
    mp_size_t bn = 0;
    mp_size_t n = apply(in, bench->path, in->out_a, in->out_b, &bn);
    return n ? n ^ bn ^ in->out_a[0] ^ in->out_b[0] : 0;
}

static bool reduce_high(hgcd_input* in, mp_size_t n, mp_size_t p) {
    mp_size_t high_n = n - p;
    if (lmmp_zero_q_(in->b + p, high_n) || lmmp_cmp_(in->a + p, in->b + p, high_n) <= 0)
        return false;

    in->n = n;
    in->p = p;
    lmmp_copy(in->ha, in->a + p, high_n);
    lmmp_copy(in->hb, in->b + p, high_n);
    lmmp_gcd_matrix_init_buf_(&in->matrix, in->matrix_buf, high_n + 2);
    in->hn = lmmp_hgcd_core_(&in->matrix, in->ha, in->hb, high_n, &in->scratch, &in->det, 0);
    return in->hn > 0;
}

static bool accepted(const hgcd_input* in, mp_size_t n, mp_size_t bn) {
    return n > 0 && n < in->n && bn > lmmp_gcd_matrix_maxlen_(&in->matrix);
}

/* 近似模式在阶段 B 复合矩阵后直接返回，数对停在阶段 B 开始前。
   用此数对复现精确模式的阶段 B，包含生产递归核的阶段 A 和桥接步进。 */
static bool prepare(hgcd_input* in, int target, int stage, mp_limb_t seed) {
    int parent = stage == FIRST ? target : LMMP_MAX(HGCD_RECURSE_THRESHOLD + 1, (4 * (target - 1) + 2) / 3);
    for (unsigned attempt = 0; attempt < 12; ++attempt) {
        mp_size_t root = parent + (stage == SECOND ? attempt / 4 : 0);
        fill_random(in->a, root, seed + 2 * attempt);
        fill_random(in->b, root, seed + 2 * attempt + 1);
        if (lmmp_cmp_(in->a, in->b, root) < 0)
            LMMP_SWAP(in->a, in->b, mp_ptr);

        mp_size_t p = root / 2;
        if (stage == SECOND) {
            int det = 1;
            lmmp_gcd_matrix_init_buf_(&in->matrix, in->matrix_buf, root + 2);
            mp_size_t n = lmmp_hgcd_core_(&in->matrix, in->a, in->b, root, &in->scratch, &det, 1);
            mp_size_t floor = root / 2 + 2;
            if (n != (mp_size_t)target || n > 3 * root / 4 + 1 || n <= floor + 2)
                continue;
            p = 2 * floor - n + 1;
        }

        /* 修补路径需要精确高位数对。三条路径在同一矩阵与原数对上对拍和计时。 */
        if (reduce_high(in, target, p)) {
            mp_size_t bn = 0;
            mp_size_t n = apply(in, ADJUST, in->ref_a, in->ref_b, &bn);
            if (accepted(in, n, bn))
                return true;
        }
    }
    return false;
}

static bool check_paths(hgcd_input* in, mp_size_t* mersenne_n, mp_size_t* cache_n) {
    const lmmp_hgcd_matrix_t* m = &in->matrix;
    mp_size_t bound = tune_hgcd_bound(in->n, lmmp_gcd_norm_(in->b, in->n), m->n[0][0], m->n[0][1],
                                      m->n[1][0], m->n[1][1]);
    *mersenne_n = lmmp_mersenne_next_size_(bound + 1);
    *cache_n = lmmp_fft_next_size_(bound + 1);

    mp_size_t ref_bn = 0;
    mp_size_t ref_n = lmmp_hgcd_adjust_(&in->matrix, in->det, in->ref_a, in->ref_b, &ref_bn, in->ha, in->hb,
                                       in->hn, in->a, in->b, in->n, in->p, &in->scratch);
    if (!accepted(in, ref_n, ref_bn))
        return false;
    for (int path = ADJUST; path < PATH_COUNT; ++path) {
        if (path == CACHE && in->n < CACHE_MIN)
            continue;
        mp_size_t bn = 0;
        mp_size_t n = apply(in, path, in->out_a, in->out_b, &bn);
        if (n != ref_n || bn != ref_bn || lmmp_cmp_(in->out_a, in->ref_a, n) ||
            lmmp_cmp_(in->out_b, in->ref_b, n)) {
            fprintf(stderr, "matrix application mismatch: path=%d n=%llu p=%llu mod_lengths=%llu,%llu\n",
                    path, (unsigned long long)in->n, (unsigned long long)in->p,
                    (unsigned long long)*mersenne_n, (unsigned long long)*cache_n);
            return false;
        }
    }
    return true;
}

static bool measure_paths(hgcd_input* in, int path, point* row, unsigned round, mp_size_t mod_n,
                          const lmmp_tune_options* options) {
    hgcd_bench mersenne = {in, MERSENNE}, other = {in, path};
    double* ratio = path == ADJUST ? &row->adjust_ratios[round] : &row->cache_ratios[round];
    gcd_tune_pair pair;
    if (!gcd_tune_measure(call_apply, &mersenne, &other, options, &pair, ratio))
        return false;

    printf("%u %c %d %llu %llu %llu %s %llu %.6f %.6f %.6f %.4f %.4f %u %d\n", round + 1,
           row->stage == FIRST ? 'A' : 'B', row->n, (unsigned long long)in->p,
           (unsigned long long)lmmp_gcd_matrix_maxlen_(&in->matrix), (unsigned long long)lmmp_gcd_norm_(in->b, in->n),
           path == ADJUST ? "adjust" : "cache", (unsigned long long)mod_n, pair.low_us, pair.high_us, *ratio,
           pair.low_mad, pair.high_mad, pair.retries, pair.stable);
    return true;
}

static bool run_point(point* row, unsigned round, bool measure, const lmmp_tune_options* options,
                      unsigned* checked, unsigned* skipped) {
    mp_size_t cap = (4 * (mp_size_t)row->n + 2) / 3 + 8;
    mp_size_t scratch_n = lmmp_hgcd_scratch_size_(cap);
    mp_ptr storage = alloc_limbs(12 * cap + scratch_n);
    hgcd_input in = {.a = storage, .b = storage + cap, .ha = storage + 2 * cap, .hb = storage + 3 * cap,
                     .out_a = storage + 4 * cap, .out_b = storage + 5 * cap,
                     .ref_a = storage + 6 * cap, .ref_b = storage + 7 * cap, .matrix_buf = storage + 8 * cap,
                     .scratch = {storage + 12 * cap, scratch_n, 0}};
    mp_limb_t seed = options->seed + ((mp_limb_t)round << 32) + 64 * (mp_limb_t)row->n + 32 * row->stage;
    bool ok = true;
    if (prepare(&in, row->n, row->stage, seed)) {
        mp_size_t mersenne_n, cache_n;
        ok = check_paths(&in, &mersenne_n, &cache_n);
        if (ok && measure)
            ok = measure_paths(&in, ADJUST, row, round, mersenne_n, options) &&
                 (row->n < CACHE_MIN || measure_paths(&in, CACHE, row, round, cache_n, options));
        *checked += ok;
    } else {
        ++*skipped;
        if (row->stage == FIRST)
            ok = false;
    }
    if (!ok)
        fprintf(stderr, "hgcd check or measurement failed at stage %c n=%d\n",
                row->stage == FIRST ? 'A' : 'B', row->n);
    lmmp_free(storage);
    return ok;
}

/* 得到分派在两个阈值处的损失差。每对候选的评分为两项之和。 */
static double low_cost(double loss[][MAX_N + 2], unsigned n) {
    return loss[ADJUST][n] - loss[MERSENNE][n];
}

static double cache_cost(double loss[][MAX_N + 2], unsigned n) {
    return loss[MERSENNE][n] + loss[CACHE][MAX_N + 1] - loss[CACHE][n];
}

static void recommend(const point* rows, unsigned count, unsigned rounds, int stage) {
    double (*loss)[MAX_N + 2] = lmmp_alloc(PATH_COUNT * sizeof(*loss));
    memset(loss, 0, PATH_COUNT * sizeof(*loss));
    unsigned points = 0, cache_points = 0;
    for (unsigned i = 0; i < count; ++i) {
        const point* row = &rows[i];
        if (stage != STAGE_COUNT && row->stage != stage)
            continue;
        double adjust = gcd_tune_median(row->adjust_ratios, rounds);
        double cache = gcd_tune_median(row->cache_ratios, rounds);
        if (adjust <= 0 || (row->n >= CACHE_MIN && cache <= 0))
            continue;
        double fastest = LMMP_MIN(1., adjust);
        if (cache > 0)
            fastest = LMMP_MIN(fastest, cache);
        unsigned k = row->n + 1;
        loss[ADJUST][k] += adjust / fastest - 1.;
        loss[MERSENNE][k] += 1. / fastest - 1.;
        if (cache > 0) {
            loss[CACHE][k] += cache / fastest - 1.;
            ++cache_points;
        }
        ++points;
    }

    const char* label = stage == FIRST ? "A" : stage == SECOND ? "B" : "joint";
    if (!points || !cache_points) {
        printf("# stage=%s insufficient stable points; recommendation withheld\n", label);
        lmmp_free(loss);
        return;
    }
    for (int path = ADJUST; path < PATH_COUNT; ++path)
        for (int n = 1; n <= MAX_N + 1; ++n)
            loss[path][n] += loss[path][n - 1];
    double scale = 100. / points;
    unsigned best_low = MIN_N, best_cache = CACHE_MIN, prefix_low = MIN_N;
    double best = INFINITY;
    for (unsigned cache = MIN_N; cache <= MAX_N + 1; ++cache) {
        if (low_cost(loss, cache) < low_cost(loss, prefix_low))
            prefix_low = cache;
        if (cache < CACHE_MIN)
            continue;
        double candidate = (low_cost(loss, prefix_low) + cache_cost(loss, cache)) * scale;
        if (candidate < best) {
            best = candidate;
            best_low = prefix_low;
            best_cache = cache;
        }
    }

    unsigned low_min = best_low, low_max = best_low, cache_min = best_cache, cache_max = best_cache;
    double suffix_cache = INFINITY;
    for (unsigned low = MAX_N + 1; low >= MIN_N; --low) {
        if (low >= CACHE_MIN)
            suffix_cache = LMMP_MIN(suffix_cache, cache_cost(loss, low));
        if ((low_cost(loss, low) + suffix_cache) * scale <= best + 0.1) {
            low_min = LMMP_MIN(low_min, low);
            low_max = LMMP_MAX(low_max, low);
        }
    }
    double prefix_cost = INFINITY;
    for (unsigned cache = MIN_N; cache <= MAX_N + 1; ++cache) {
        prefix_cost = LMMP_MIN(prefix_cost, low_cost(loss, cache));
        if (cache >= CACHE_MIN && (prefix_cost + cache_cost(loss, cache)) * scale <= best + 0.1) {
            cache_min = LMMP_MIN(cache_min, cache);
            cache_max = LMMP_MAX(cache_max, cache);
        }
    }

    double current = (low_cost(loss, HGCD_MODMUL_THRESHOLD) + cache_cost(loss, HGCD_MODMUL_CACHE_THRESHOLD)) * scale;
    double adjust_only = loss[ADJUST][MAX_N + 1] * scale;
    double current_low = (low_cost(loss, HGCD_MODMUL_THRESHOLD) + cache_cost(loss, MAX_N + 1)) * scale;
    unsigned best_fixed_cache = LMMP_MAX(HGCD_MODMUL_THRESHOLD, CACHE_MIN);
    for (unsigned cache = best_fixed_cache + 1; cache <= MAX_N + 1; ++cache)
        if (cache_cost(loss, cache) < cache_cost(loss, best_fixed_cache))
            best_fixed_cache = cache;
    double fixed_pair = (low_cost(loss, HGCD_MODMUL_THRESHOLD) + cache_cost(loss, best_fixed_cache)) * scale;
    printf("# stage=%s points=%u cache_points=%u best=%u,%u loss_pct=%.5f within_0.1pp_low=%u..%u "
           "within_0.1pp_cache=%u..%u current=%u,%u current_loss_pct=%.5f\n",
           label, points, cache_points, best_low, best_cache, best, low_min, low_max, cache_min, cache_max,
           HGCD_MODMUL_THRESHOLD, HGCD_MODMUL_CACHE_THRESHOLD, current);
    printf("# stage=%s adjust_only_loss_pct=%.5f current_low_no_cache_loss_pct=%.5f "
           "current_low_best_cache=%u current_low_best_cache_loss_pct=%.5f\n",
           label, adjust_only, current_low, best_fixed_cache, fixed_pair);
    if (stage == STAGE_COUNT)
        printf("recommended joint (%s,%s) = (%u,%u)\n", low_parameter, cache_parameter, best_low, best_cache);
    lmmp_free(loss);
}

int main(int argc, char** argv) {
    lmmp_tune_options options;
    lmmp_tune_options_init(&options);
    options.min_ms = 0.20;
    options.max_retry = 1;
    unsigned rounds = 5;
    if (!lmmp_tune_parse_ex(argc, argv, &options, gcd_tune_parse_rounds, &rounds))
        return EXIT_FAILURE;
    if (options.action == LMMP_TUNE_HELP) {
        lmmp_tune_usage(program);
        puts("  --rounds N        odd scan count, default 5; 0 checks inputs and results only");
        puts("  Program defaults: --min-ms 0.20 --max-retry 1");
        return EXIT_SUCCESS;
    }
    if (options.action == LMMP_TUNE_INFO) {
        printf("program=%s\npaths=adjust,mersenne,cache\n", program);
        puts("dispatch_size=pair size immediately before application; stage B sampled after the library's bridge");
        puts("input_limb_grid=45..144/1,148..320/4,328..768/8,800..1536/32,1600..6144/64,6400..16384/256"
             " plus current boundaries +/- 1");
        puts("candidate_limb_grids=45..16385/1,256..16385/1; low <= cache; 16385 disables a path on this grid");
        puts("cache setup and free timed; high-half recursion and matrix composition excluded");
        lmmp_tune_print_env();
    }
    if (options.action == LMMP_TUNE_CURRENT || options.action == LMMP_TUNE_INFO) {
        lmmp_tune_print_current(low_parameter, HGCD_MODMUL_THRESHOLD, HGCD_MODMUL_THRESHOLD);
        lmmp_tune_print_current(cache_parameter, HGCD_MODMUL_CACHE_THRESHOLD, HGCD_MODMUL_CACHE_THRESHOLD);
        return EXIT_SUCCESS;
    }
    if (HGCD_MODMUL_THRESHOLD < MIN_N || HGCD_MODMUL_THRESHOLD > MAX_N + 1 ||
        HGCD_MODMUL_CACHE_THRESHOLD < CACHE_MIN || HGCD_MODMUL_CACHE_THRESHOLD > MAX_N + 1 ||
        HGCD_MODMUL_THRESHOLD > HGCD_MODMUL_CACHE_THRESHOLD) {
        fputs("current HGCD thresholds are outside the ordered candidate grid\n", stderr);
        return EXIT_FAILURE;
    }

    lmmp_set_abort_fn(lmmp_tune_abort);
    lmmp_global_init();
    point* rows = lmmp_alloc((MAX_N - MIN_N + 1) * STAGE_COUNT * sizeof(*rows));
    unsigned count = 0;
    for (int n = MIN_N; n <= MAX_N; ++n) {
        int step = n <= 144 ? 1 : n <= 320 ? 4 : n <= 768 ? 8 : n <= 1536 ? 32 : n <= 6144 ? 64 : 256;
        if (n % step != 0 && abs(n - HGCD_MODMUL_THRESHOLD) > 1 && abs(n - HGCD_MODMUL_CACHE_THRESHOLD) > 1)
            continue;
        if (n > HGCD_RECURSE_THRESHOLD)
            rows[count++] = (point){.n = n, .stage = FIRST};
        rows[count++] = (point){.n = n, .stage = SECOND};
    }
    printf("# program=%s points=%u rounds=%u samples=%u min_ms=%.3f seed=%llu\n",
           program, count, rounds, options.samples, options.min_ms, options.seed);
    lmmp_tune_print_env_comment();
    puts("# round stage n p matrix_n bn path modulus_n mersenne_us path_us path_over_mersenne "
         "mersenne_mad path_mad retries stable");
    bool ok = true;
    for (unsigned round = 0; round < LMMP_MAX(1U, rounds) && ok; ++round) {
        unsigned checked = 0, skipped = 0;
        for (unsigned i = 0; i < count && ok; ++i)
            ok = run_point(&rows[i], round, rounds != 0, &options, &checked, &skipped);
        printf("# round=%u checks_passed=%u stage_B_skipped=%u\n", round + 1, checked, skipped);
    }
    if (ok && rounds)
        for (int stage = FIRST; stage <= STAGE_COUNT; ++stage)
            recommend(rows, count, rounds, stage);
    lmmp_free(rows);
    lmmp_global_deinit();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
