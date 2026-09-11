/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in NO WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/*
 * 阈值调优：lmmp_fft_table_（FFT 分裂层数 k 的规模查找表）。
 *
 * 与标量阈值不同，本模块调优对象是一张 {阈值, k} 二维表：
 * best_k_(n) 沿表扫描，n 落入第 i 个桶 [t_i, t_{i+1}) 时使用 k_i 层分裂。
 *
 * 安全规则（硬约束，违反将算错）：
 *   S1 一致性：t_{i+1}-1 必须是 2^(k_i-6) 的倍数，保证
 *      best_k_(next_size_(n)) = best_k_(n)；
 *   S2 无递归：桶内所有规模的系数长度 lenw(s,k_i) < MUL_FFT_MODF_THRESHOLD。
 *      lenw = ceil_K(ceil_64(2M+k+2))/64（与 mul_fft.c 量化规则一致），
 *      其量化台阶正是性能跳跃的直接来源。lenw >= MODF 时递归子查找会以
 *      子尺寸重新查表，默认表在扫描范围内设计为不递归（lenw<=192），
 *      本模块维持该性质（实测：强制小 k 在 lenw>=MODF 时结果错误）。
 *
 * 调优方法（表驱动型，四阶段）：
 *   A. 解析建模：按上述量化规则解析 lenw 与粗成本模型，用于剪枝候选 k；
 *   B. 强制测量：在网格点安装"单桶强制表"令 best_k 恒为候选 k，交替测量
 *      lmmp_mul_fft_；每个 (n,k) 都与默认表结果 memcmp 自校验；
 *   C. 构表：逐点 argmin（容差内偏向旧表）→ 游程合并（桶数上限 + 噪声
 *      短桶抑制）→ 边界对齐到 S1/S2 合法位置 → 以"总相对耗时 + 跳跃惩罚"
 *      做边界局部搜索，压制台阶跳跃；
 *   D. 验证：新表与默认表在全网格交替测量，输出性能、跳跃统计与
 *      可直接粘贴回 fft_ssa.c 的 C 表。
 */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/impl/fft_ssa.h"
#include "lmmp/lmmpn.h"

#include <math.h>
#include <stdlib.h>

/* ---------------- 常量 ---------------- */

#define FT_K_MIN 6 /* k 下界：next_size_ 要求 k >= LOG2_LIMB_BITS */
#define FT_K_MAX 14
#define FT_FORCE_LO 1700  /* 强制表桶阈值，须 <= 扫描下界 */
#define FT_GRID_CAP 3600  /* 网格点容量 */
#define FT_MAX_BUCKETS 44 /* 调优段桶数上限（不含 [0,t1) 前缀桶与尾段） */
#define FT_TIE_TOL 0.01   /* argmin 容差：1% 内视为等价 */
#define FT_RUN_GAIN 0.015 /* 游程最小平均收益，低于此并入邻桶 */
#define FT_PRUNE 6.0      /* 粗模型剪枝窗口（相对模型最优；仅剔除 hopeless 候选） */
#define FT_JUMP_TOL 1.08  /* 跳跃惩罚起征点：相邻网格点 +8% */
#define FT_JUMP_W 3.0     /* 跳跃惩罚权重 */
#define FT_N_HI 525000    /* 扫描上界（覆盖 500000 数据范围） */

#define FT_TAIL_K (-1) /* 尾伪桶标记：沿用默认表对应点的 k */

/* ---------------- 全局状态 ---------------- */

static mp_size_t g_rows[2 * (FT_MAX_BUCKETS + 64)]; /* 新表平铺行（安装用） */
static mp_size_t g_nrows;

/* ---------------- 上下文与被测函数 ---------------- */

typedef struct {
    mp_ptr a, b, dst, ref;
    mp_size_t n;
    int force_k; /* 强制测量的 k */
} ft_ctx;

static void ft_install_forced(int k) {
    mp_size_t rows[4] = {0, FT_K_MIN, FT_FORCE_LO, (mp_size_t)k};
    lmmp_fft_tune_install_(rows, 2);
}

static double ft_bench_forced(void* v) {
    ft_ctx* c = (ft_ctx*)v;
    ft_install_forced(c->force_k);
    lmmp_mul_fft_(c->dst, c->a, c->n, c->b, c->n);
    return 0.0;
}

static double ft_bench_default(void* v) {
    ft_ctx* c = (ft_ctx*)v;
    lmmp_fft_tune_reset_();
    lmmp_mul_fft_(c->dst, c->a, c->n, c->b, c->n);
    return 0.0;
}

static double ft_bench_new(void* v) {
    ft_ctx* c = (ft_ctx*)v;
    lmmp_fft_tune_install_(g_rows, g_nrows);
    lmmp_mul_fft_(c->dst, c->a, c->n, c->b, c->n);
    return 0.0;
}

/* ---------------- 解析模型（与 mul_fft.c 的量化规则一致） ---------------- */

static mp_size_t ft_next_size(mp_size_t n, int k) {
    mp_size_t kk = (mp_size_t)(k - 6);
    return (((n - 1) >> kk) + 1) << kk;
}

/* 系数机器字长度：n_coef = ceil_K(ceil_64(2M+k+2))，M = 64*hn/K */
static mp_size_t ft_lenw(mp_size_t hn, int k) {
    mp_size_t M = (hn * LIMB_BITS) >> k;
    mp_size_t n = 2 * M + k + 2;
    n = (n + LIMB_BITS - 1) & ~(LIMB_BITS - 1);
    n = (((n - 1) >> k) + 1) << k;
    return n / LIMB_BITS;
}

/* 安全规则 S2：k 用于操作数规模 n 时不得触发递归 */
static int ft_k_safe(mp_size_t n, int k) {
    return ft_lenw(ft_next_size(n, k), k) < (mp_size_t)lmmp_tune_MUL_FFT_MODF_THRESHOLD;
}

/* 粗成本模型（任意单位，仅剔除 hopeless 候选）：蝶形 ~ 3*(K/2)*k*lenw，
 * 逐点乘 ~ K*lenw^1.465，访存 ~ K*lenw */
static double ft_model(mp_size_t n, int k) {
    mp_size_t hn = ft_next_size(n, k);
    mp_size_t K = (mp_size_t)1 << k;
    mp_size_t lenw = ft_lenw(hn, k);
    if (lenw == 0) return 1e300;
    return 1.5 * (double)K * (double)k * (double)lenw +
           0.9 * (double)K * pow((double)(lenw + 1), 1.465) +
           1.2 * (double)K * (double)lenw;
}

/* 对任意平铺表求 best_k（与 fft_ssa.c 语义一致） */
static int ft_best_k_rows(const mp_size_t* rows, mp_size_t cnt, mp_size_t n) {
    mp_size_t i = 0;
    while (i + 1 < cnt && n >= rows[2 * (i + 1)]) ++i;
    return (int)rows[2 * i + 1];
}

/* 边界合法性：S1 整除 + S2 下方桶 k 在新顶点 cand-1 处不递归 */
static int ft_boundary_ok(mp_size_t cand, int k_below) {
    const mp_size_t g = (mp_size_t)1 << (k_below - 6);
    if (cand <= 1 || (cand - 1) % g != 0) return 0;
    return ft_lenw(cand - 1, k_below) < (mp_size_t)lmmp_tune_MUL_FFT_MODF_THRESHOLD;
}

/* ---------------- 网格生成 ---------------- */

static int ft_cmp_u64(const void* a, const void* b) {
    const uint64_t x = *(const uint64_t*)a, y = *(const uint64_t*)b;
    return (x > y) - (x < y);
}

/*
 * 网格 = 几何步进(×1.025) ∪ 各 k 的 lenw 量化台阶两侧 ∪ 默认表阈值两侧。
 * 台阶点是性能跳跃的直接来源，必须在其前后沿都有采样。
 */
static size_t ft_build_grid(mp_size_t lo, mp_size_t hi, uint64_t* grid,
                            const mp_size_t* drows, mp_size_t dcnt) {
    size_t cnt = 0;
    for (double n = (double)lo; n <= (double)hi && cnt < FT_GRID_CAP; n *= 1.025)
        grid[cnt++] = (uint64_t)n;

    for (int k = FT_K_MIN; k <= FT_K_MAX && cnt + 2 < FT_GRID_CAP; ++k) {
        mp_size_t g = (mp_size_t)1 << (k - 6);
        mp_size_t hn = ft_next_size(lo, k), prev = ft_lenw(hn, k);
        for (hn += g; hn <= hi && cnt + 2 < FT_GRID_CAP; hn += g) {
            mp_size_t cur = ft_lenw(hn, k);
            if (cur > prev && prev > 0 && (double)(cur - prev) / (double)prev >= 0.02) {
                grid[cnt++] = (uint64_t)hn;
                grid[cnt++] = (uint64_t)hn - 1;
            }
            prev = cur;
        }
    }

    for (mp_size_t i = 1; i < dcnt && cnt + 2 < FT_GRID_CAP; ++i) {
        mp_size_t t = drows[2 * i];
        if (t > lo && t <= hi) {
            grid[cnt++] = t - 1;
            grid[cnt++] = t;
        }
    }

    qsort(grid, cnt, sizeof(grid[0]), ft_cmp_u64);
    size_t out = 0;
    for (size_t i = 0; i < cnt; ++i)
        if (out == 0 || grid[i] > grid[out - 1]) grid[out++] = grid[i];
    return out;
}

/* ---------------- 阶段C/D 公共状态与评价 ---------------- */

static double* g_T;       /* T[i][k]：点 i 强制 k 的中位耗时 */
static int g_stride;      /* = FT_K_MAX+1 */
static const uint64_t* g_grid;
static size_t g_ngrid;
static const mp_size_t* g_drows;
static mp_size_t g_dcnt;
static const int* g_kcur; /* 默认表在各网格点的 k */
static size_t g_span_lo[FT_MAX_BUCKETS + 4]; /* 各桶网格覆盖起点（尾伪桶除外） */

/* 点 i 在表 (thr,kk,nb) 下的耗时；thr[nb]=-1 终界，kk[b]=FT_TAIL_K 表示默认 k */
static double ft_curve_at(const mp_size_t* thr, const int* kk, mp_size_t nb, size_t i) {
    mp_size_t n = (mp_size_t)g_grid[i];
    for (mp_size_t b = 0; b < nb; ++b)
        if (n >= thr[b] && n < thr[b + 1]) {
            const int k = kk[b];
            return g_T[i * g_stride + (k == FT_TAIL_K ? g_kcur[i] : k)];
        }
    return g_T[i * g_stride + g_kcur[i]];
}

/* 目标函数：Σ T/T_default + 相邻网格点跳跃惩罚 */
static double ft_score(const mp_size_t* thr, const int* kk, mp_size_t nb) {
    double s = 0, prev = 0;
    for (size_t i = 0; i < g_ngrid; ++i) {
        const double v = ft_curve_at(thr, kk, nb, i);
        const double tref = g_T[i * g_stride + g_kcur[i]];
        if (tref > 0 && tref < 1e290) {
            s += v / tref;
            if (i > 0 && prev > 0) s += FT_JUMP_W * fmax(0.0, v / prev - FT_JUMP_TOL);
        }
        prev = v;
    }
    return s;
}

/* ---------------- 主流程 ---------------- */

int tune_run_fft_table(void) {
    const mp_size_t n_lo = LMMP_DEFAULT_MUL_FFT_THRESHOLD;

    mp_size_t dcnt = 0;
    const mp_size_t* drows = lmmp_fft_tune_default_rows_(&dcnt);
    if (drows == NULL || dcnt == 0) {
        printf("  !! failed to read default FFT table\n");
        return -1;
    }

    static uint64_t grid[FT_GRID_CAP];
    const size_t ngrid = ft_build_grid(n_lo, FT_N_HI, grid, drows, dcnt);
    printf("  FFT table tuning: %zu grid points, range [%llu, %llu], candidate k=%d..%d\n", ngrid,
           (unsigned long long)n_lo, (unsigned long long)FT_N_HI, FT_K_MIN, FT_K_MAX);

    static double T[FT_GRID_CAP * (FT_K_MAX + 1)];
    static int kcur[FT_GRID_CAP];
    g_T = T;
    g_stride = FT_K_MAX + 1;
    g_grid = grid;
    g_ngrid = ngrid;
    g_drows = drows;
    g_dcnt = dcnt;
    for (size_t i = 0; i < ngrid * (FT_K_MAX + 1); ++i) T[i] = 1e300;
    for (size_t i = 0; i < ngrid; ++i) {
        const int kd = ft_best_k_rows(drows, dcnt, (mp_size_t)grid[i]);
        kcur[i] = ft_best_k_rows(drows, dcnt, ft_next_size((mp_size_t)grid[i], kd));
    }
    g_kcur = kcur;

    /* ---------------- 阶段B：强制测量 ---------------- */

    ft_ctx c;
    int meas_set[FT_K_MAX + 2];
    for (size_t i = 0; i < ngrid; ++i) {
        c.n = (mp_size_t)grid[i];
        c.a = (mp_ptr)lmmp_alloc((size_t)c.n * sizeof(mp_limb_t));
        c.b = (mp_ptr)lmmp_alloc((size_t)c.n * sizeof(mp_limb_t));
        c.dst = (mp_ptr)lmmp_alloc((size_t)(2 * c.n + 1) * sizeof(mp_limb_t));
        c.ref = (mp_ptr)lmmp_alloc((size_t)(2 * c.n + 1) * sizeof(mp_limb_t));
        if (c.a == NULL || c.b == NULL || c.dst == NULL || c.ref == NULL) {
            printf("  !! out of memory at n=%llu\n", (unsigned long long)c.n);
            return -1;
        }
        tune_fill_limbs(c.a, c.n, UINT64_C(0x9b05688c2b3e6c1f));
        tune_fill_limbs(c.b, c.n, UINT64_C(0x1f83d9abfb41bd6b));
        c.a[c.n - 1] |= LIMB_B_2;
        c.b[c.n - 1] |= LIMB_B_2;

        /* 默认表参考积（正确性基准） */
        lmmp_fft_tune_reset_();
        lmmp_mul_fft_(c.ref, c.a, c.n, c.b, c.n);

        /* 候选集：满足 S2 的全部 k，粗模型仅剔除 hopeless 项 */
        double best = 1e300;
        for (int k = FT_K_MIN; k <= FT_K_MAX; ++k) {
            if (!ft_k_safe(c.n, k)) continue;
            const double m = ft_model(c.n, k);
            if (m < best) best = m;
        }
        size_t cnt = 0;
        for (int k = FT_K_MIN; k <= FT_K_MAX; ++k) {
            if (!ft_k_safe(c.n, k)) continue;
            if (k != kcur[i] && best < 1e290 && ft_model(c.n, k) > best * FT_PRUNE) continue;
            meas_set[cnt++] = k;
        }

        for (size_t q = 0; q < cnt; ++q) {
            c.force_k = meas_set[q];
            const tune_measure_t m = tune_measure(ft_bench_forced, &c, g_tune.samples, g_tune.target_ms);
            if (memcmp(c.dst, c.ref, (size_t)(2 * c.n) * sizeof(mp_limb_t)) != 0) {
                printf("  !! correctness failure at n=%llu k=%d, candidate rejected\n",
                       (unsigned long long)c.n, meas_set[q]);
                continue;
            }
            if (m.median_ns > 0 && m.median_ns < 1e290) T[i * g_stride + meas_set[q]] = m.median_ns;
        }

        lmmp_free(c.a);
        lmmp_free(c.b);
        lmmp_free(c.dst);
        lmmp_free(c.ref);

        if ((i & 63) == 0) {
            printf("    phase B progress %zu/%zu (n=%llu)\n", i, ngrid, (unsigned long long)grid[i]);
            fflush(stdout);
        }
    }

    /* ---------------- 阶段C：argmin → 游程 → 桶边界 ---------------- */

    static int chose[FT_GRID_CAP];
    for (size_t i = 0; i < ngrid; ++i) {
        double best = 1e300;
        for (int k = FT_K_MIN; k <= FT_K_MAX; ++k)
            if (T[i * g_stride + k] < best) best = T[i * g_stride + k];
        if (best >= 1e290) {
            printf("  !! no valid measurement at n=%llu\n", (unsigned long long)grid[i]);
            return -1;
        }
        /* argmin；容差内保持旧表 k，避免噪声驱动的无谓切换 */
        int best_k = kcur[i];
        double best_t = 1e300;
        for (int k = FT_K_MIN; k <= FT_K_MAX; ++k) {
            const double t = T[i * g_stride + k];
            if (t < best_t - 1e-9 || (t < best_t + 1e-9 && k == kcur[i])) {
                best_t = t;
                best_k = k;
            }
        }
        int pick = kcur[i];
        if (T[i * g_stride + pick] > best_t * (1.0 + FT_TIE_TOL)) pick = best_k;
        chose[i] = pick;
    }

    /* 游程序列（等值极大段） */
    static int rk[FT_GRID_CAP];
    static size_t rlo[FT_GRID_CAP], rhi[FT_GRID_CAP];
    size_t nruns = 0;
    for (size_t i = 0; i < ngrid; ++i) {
        if (nruns > 0 && rk[nruns - 1] == chose[i]) {
            rhi[nruns - 1] = i;
        } else {
            rk[nruns] = chose[i];
            rlo[nruns] = i;
            rhi[nruns] = i;
            ++nruns;
        }
    }

    /* 桶数上限：反复把跨度最小的中间游程并入耗时更低的邻桶 */
    while (nruns > FT_MAX_BUCKETS) {
        size_t worst = 1, worst_span = (size_t)-1;
        for (size_t r = 1; r + 1 < nruns; ++r) {
            const size_t sp = rhi[r] - rlo[r];
            if (sp < worst_span) {
                worst_span = sp;
                worst = r;
            }
        }
        double cl = 0, ch = 0;
        for (size_t i = rlo[worst]; i <= rhi[worst]; ++i) {
            cl += T[i * g_stride + rk[worst - 1]];
            ch += T[i * g_stride + rk[worst + 1]];
        }
        const int kn = (cl <= ch) ? rk[worst - 1] : rk[worst + 1];
        for (size_t i = rlo[worst]; i <= rhi[worst]; ++i) chose[i] = kn;
        nruns = 0;
        for (size_t i = 0; i < ngrid; ++i) {
            if (nruns > 0 && rk[nruns - 1] == chose[i]) {
                rhi[nruns - 1] = i;
            } else {
                rk[nruns] = chose[i];
                rlo[nruns] = i;
                rhi[nruns] = i;
                ++nruns;
            }
        }
    }

    /* 弱收益游程抑制：相对最优邻桶的平均收益不足阈值（短游程要求更高）的
     * 中间游程并入邻桶，避免测量噪声产生碎片桶 */
    for (int pass = 0; pass < 32; ++pass) {
        int merged = 0;
        for (size_t r = 1; r + 1 < nruns; ++r) {
            double gain = 0;
            size_t cnt = 0;
            for (size_t i = rlo[r]; i <= rhi[r]; ++i) {
                const double tr = T[i * g_stride + rk[r]];
                const double tn = fmin(T[i * g_stride + rk[r - 1]], T[i * g_stride + rk[r + 1]]);
                if (tn < 1e290 && tr < 1e290) {
                    gain += (tn - tr) / tr;
                    ++cnt;
                }
            }
            if (cnt > 0) {
                const double avg = gain / (double)cnt;
                const size_t span = rhi[r] - rlo[r] + 1;
                if (avg >= (span < 4 ? 2.0 * FT_RUN_GAIN : FT_RUN_GAIN)) continue;
            }
            double cl = 0, ch = 0;
            for (size_t i = rlo[r]; i <= rhi[r]; ++i) {
                cl += T[i * g_stride + rk[r - 1]];
                ch += T[i * g_stride + rk[r + 1]];
            }
            const int kn = (cl <= ch) ? rk[r - 1] : rk[r + 1];
            for (size_t i = rlo[r]; i <= rhi[r]; ++i) chose[i] = kn;
            nruns = 0;
            for (size_t i = 0; i < ngrid; ++i) {
                if (nruns > 0 && rk[nruns - 1] == chose[i]) {
                    rhi[nruns - 1] = i;
                } else {
                    rk[nruns] = chose[i];
                    rlo[nruns] = i;
                    rhi[nruns] = i;
                    ++nruns;
                }
            }
            merged = 1;
            break;
        }
        if (!merged) break;
    }

    /*
     * 组桶：前缀桶 [0, t1) 固定 k=6（与默认表一致，保证递归小尺寸查找
     * 行为不变）；游程桶逐个排列；最后是尾伪桶（交还默认表行）。
     */
    static mp_size_t thr[FT_MAX_BUCKETS + 6];
    static int kk[FT_MAX_BUCKETS + 6];
    thr[0] = 0;
    kk[0] = FT_K_MIN;
    g_span_lo[0] = 0;
    mp_size_t nb = 1;

    size_t r0 = (nruns > 0 && rk[0] == FT_K_MIN) ? 1 : 0; /* 跳过并入前缀桶的 6-游程 */
    if (r0 == 1) g_span_lo[0] = rlo[0];

    for (size_t r = r0; r < nruns; ++r) {
        kk[nb] = rk[r];
        thr[nb] = (mp_size_t)grid[rlo[r]];
        g_span_lo[nb] = rlo[r];
        ++nb;
    }
    kk[nb] = FT_TAIL_K;                          /* 尾伪桶 */
    thr[nb] = (mp_size_t)grid[ngrid - 1] + 1;    /* 期望值，随后合法化 */
    thr[nb + 1] = (mp_size_t)-1;
    const mp_size_t nb_total = nb + 1;

    /*
     * 边界合法化：把每个边界吸附到最近的 S1/S2 合法位置，
     * 以受影响两桶覆盖点的实测耗时评估吸附代价。
     */
    for (mp_size_t b = 1; b < nb_total; ++b) {
        const int kb = kk[b - 1];
        const mp_size_t g = (mp_size_t)1 << (kb - 6);
        const mp_size_t desired = thr[b];
        const mp_size_t win = (kk[b] == FT_TAIL_K) ? 8 * g : 2 * g;

        mp_size_t cands[64];
        size_t ncand = 0;
        /* 窗口内合法位：base 为 g 的倍数（本桶最后一个 next_size 取值） */
        const mp_size_t lo64 = desired > win ? (desired - win) / g * g : 0;
        for (mp_size_t base = lo64; base <= desired + win && ncand < 64; base += g) {
            if (base == 0) continue;
            if (ft_boundary_ok(base + 1, kb)) cands[ncand++] = base + 1;
        }

        mp_size_t best_t = thr[b];
        double best_cost = 1e300;
        for (size_t ci = 0; ci < ncand; ++ci) {
            const mp_size_t cand = cands[ci];
            if (cand <= thr[b - 1]) continue;
            if (kk[b] != FT_TAIL_K && cand >= thr[b + 1]) continue;
            /* 受影响点：桶 b-1 与桶 b 的网格覆盖 */
            const size_t i0 = (b >= 2) ? g_span_lo[b - 1] : 0;
            size_t i1 = ngrid - 1;
            if (kk[b] != FT_TAIL_K) {
                for (size_t i = g_span_lo[b]; i < ngrid; ++i)
                    if ((mp_size_t)grid[i] >= thr[b + 1]) {
                        i1 = (i > 0) ? i - 1 : 0;
                        break;
                    }
            }
            double cost = 0;
            for (size_t i = i0; i <= i1 && i < ngrid; ++i) {
                const int k = ((mp_size_t)grid[i] < cand) ? kk[b - 1] : kk[b];
                cost += g_T[i * g_stride + (k == FT_TAIL_K ? g_kcur[i] : k)];
            }
            if (cost < best_cost - 1e-9) {
                best_cost = cost;
                best_t = cand;
            }
        }
        if (best_cost < 1e290) thr[b] = best_t;
    }

    /* 跳跃惩罚局部搜索：微调边界，最小化 ft_score（±g 保持合法性） */
    for (int pass = 0; pass < 4; ++pass) {
        int improved = 0;
        for (mp_size_t b = 1; b < nb_total; ++b) {
            const int kb = kk[b - 1];
            const mp_size_t g = (mp_size_t)1 << (kb - 6);
            const double s0 = ft_score(thr, kk, nb_total);
            for (int dir = -1; dir <= 1; dir += 2) {
                const mp_size_t cand = (dir < 0) ? thr[b] - g : thr[b] + g;
                if (!ft_boundary_ok(cand, kb)) continue;
                if (cand <= thr[b - 1]) continue;
                if (kk[b] != FT_TAIL_K && cand >= thr[b + 1]) continue;
                const mp_size_t save = thr[b];
                thr[b] = cand;
                if (ft_score(thr, kk, nb_total) < s0 - 1e-9) {
                    improved = 1;
                    break;
                }
                thr[b] = save;
            }
        }
        if (!improved) break;
    }

    /* 兜底修复：若仍有非法边界（窗口内无合法候选），吸附到最近的合法位 */
    for (mp_size_t b = 1; b < nb_total; ++b) {
        const int kb = kk[b - 1];
        const mp_size_t g = (mp_size_t)1 << (kb - 6);
        if (ft_boundary_ok(thr[b], kb)) continue;
        mp_size_t t = (thr[b] + g - 2) / g * g + 1; /* 向上吸附到合法位 */
        while (!ft_boundary_ok(t, kb) && t < thr[b] + 16 * g) t += g;
        thr[b] = t;
    }

    /* ---------------- 生成最终行（前缀桶 + 调优桶 + 尾段默认行） ---------------- */

    static mp_size_t out_rows[2 * (FT_MAX_BUCKETS + 64)];
    mp_size_t out_cnt = 0;
    out_rows[out_cnt++] = 0;
    out_rows[out_cnt++] = FT_K_MIN;
    for (mp_size_t b = 1; b < nb_total - 1; ++b) {
        out_rows[out_cnt++] = thr[b];
        out_rows[out_cnt++] = (mp_size_t)kk[b];
    }
    const mp_size_t n_tuned_rows = out_cnt / 2; /* 调优段行数（含前缀桶） */
    /*
     * 尾段拼接默认表行：尾边界 thr_tail 落在默认某桶 [D_prev, D) 内部时，
     * 先补一行 {thr_tail, k_default}（保持默认行为），再续默认行 D, D', ...
     * 这样尾边界只需满足 S1/S2，无需与默认阈值整除关系对齐。
     */
    {
        const mp_size_t thr_tail = thr[nb_total - 1];
        mp_size_t idx = 0; /* 最大的 drows 阈值 <= thr_tail 的行号 */
        for (mp_size_t i = 1; i < dcnt; ++i)
            if (drows[2 * i] <= thr_tail) idx = i;
        if (idx > 0) {
            if (drows[2 * idx] == thr_tail) {
                for (mp_size_t i = idx; i < dcnt; ++i) {
                    out_rows[out_cnt++] = drows[2 * i];
                    out_rows[out_cnt++] = drows[2 * i + 1];
                }
            } else {
                out_rows[out_cnt++] = thr_tail;
                out_rows[out_cnt++] = drows[2 * idx + 1];
                for (mp_size_t i = idx + 1; i < dcnt; ++i) {
                    out_rows[out_cnt++] = drows[2 * i];
                    out_rows[out_cnt++] = drows[2 * i + 1];
                }
            }
        }
    }

    /* 终检：全行 S1（递增+整除）；调优段行附加 S2（无递归）。
     * 默认尾段行不检 S2：默认表在更大规模按设计递归（配合宏规则行）。 */
    {
        const mp_size_t rows_cnt = out_cnt / 2;
        for (mp_size_t i = 0; i < rows_cnt; ++i) {
            const mp_size_t k = out_rows[2 * i + 1];
            if (k < FT_K_MIN) {
                printf("  !! new table row %llu has invalid k=%llu\n", (unsigned long long)i,
                       (unsigned long long)k);
                return -1;
            }
            if (i + 1 < rows_cnt) {
                const mp_size_t t = out_rows[2 * (i + 1)];
                const mp_size_t g = (mp_size_t)1 << (k - 6);
                if (t <= out_rows[2 * i] || (t - 1) % g != 0 ||
                    (i < n_tuned_rows && !ft_boundary_ok(t, (int)k))) {
                    printf("  !! invalid new table boundary: row %llu t=%llu k=%llu\n",
                           (unsigned long long)i, (unsigned long long)t, (unsigned long long)k);
                    return -1;
                }
            }
        }
    }

    for (mp_size_t i = 0; i < out_cnt; ++i) g_rows[i] = out_rows[i];
    g_nrows = out_cnt / 2;

    /* ---------------- 阶段D：新旧表交替验证 ---------------- */

    printf("\n  phase D: interleaved old/new table measurement over full grid...\n");
    static double told[FT_GRID_CAP], tnew[FT_GRID_CAP];
    double sum_old = 0, sum_new = 0;
    size_t bad_new = 0;
    for (size_t i = 0; i < ngrid; ++i) {
        c.n = (mp_size_t)grid[i];
        c.a = (mp_ptr)lmmp_alloc((size_t)c.n * sizeof(mp_limb_t));
        c.b = (mp_ptr)lmmp_alloc((size_t)c.n * sizeof(mp_limb_t));
        c.dst = (mp_ptr)lmmp_alloc((size_t)(2 * c.n + 1) * sizeof(mp_limb_t));
        c.ref = (mp_ptr)lmmp_alloc((size_t)(2 * c.n + 1) * sizeof(mp_limb_t));
        if (c.a == NULL || c.b == NULL || c.dst == NULL || c.ref == NULL) break;
        tune_fill_limbs(c.a, c.n, UINT64_C(0x9b05688c2b3e6c1f));
        tune_fill_limbs(c.b, c.n, UINT64_C(0x1f83d9abfb41bd6b));
        c.a[c.n - 1] |= LIMB_B_2;
        c.b[c.n - 1] |= LIMB_B_2;
        lmmp_fft_tune_reset_();
        lmmp_mul_fft_(c.ref, c.a, c.n, c.b, c.n);

        tune_measure_t mo, mn;
        tune_measure_pair(ft_bench_default, &c, ft_bench_new, &c, g_tune.samples,
                          g_tune.target_ms, &mo, &mn);
        told[i] = mo.median_ns;
        tnew[i] = mn.median_ns;
        sum_old += told[i];
        sum_new += tnew[i];
        if (memcmp(c.dst, c.ref, (size_t)(2 * c.n) * sizeof(mp_limb_t)) != 0) ++bad_new;

        lmmp_free(c.a);
        lmmp_free(c.b);
        lmmp_free(c.dst);
        lmmp_free(c.ref);
        if ((i & 127) == 0) {
            printf("    progress %zu/%zu\n", i, ngrid);
            fflush(stdout);
        }
    }
    if (bad_new != 0) {
        printf("  !! new table correctness failures: %zu, aborting\n", bad_new);
        lmmp_fft_tune_reset_();
        return -1;
    }

    {
        double jmax_o = 0, jmax_n = 0;
        size_t j8_o = 0, j15_o = 0, j8_n = 0, j15_n = 0;
        for (size_t i = 1; i < ngrid; ++i) {
            const double jo = told[i] / told[i - 1] - 1;
            const double jn = tnew[i] / tnew[i - 1] - 1;
            if (jo > jmax_o) jmax_o = jo;
            if (jn > jmax_n) jmax_n = jn;
            if (jo > 0.08) ++j8_o;
            if (jo > 0.15) ++j15_o;
            if (jn > 0.08) ++j8_n;
            if (jn > 0.15) ++j15_n;
        }
        printf("\n  ===== verification stats =====\n");
        printf("  total time (grid sum): old=%.3fms new=%.3fms (new/old=%.4f)\n", sum_old * 1e-6,
               sum_new * 1e-6, sum_new / sum_old);
        printf("  jumps>8%%:  old=%zu new=%zu\n", j8_o, j8_n);
        printf("  jumps>15%%: old=%zu new=%zu\n", j15_o, j15_n);
        printf("  max jump: old=%.1f%% new=%.1f%%\n", jmax_o * 100, jmax_n * 100);

        double worst_reg = 0;
        size_t worst_i = 0;
        for (size_t i = 0; i < ngrid; ++i) {
            const double r = tnew[i] / told[i];
            if (r > worst_reg) {
                worst_reg = r;
                worst_i = i;
            }
        }
        printf("  worst point regression: new/old=%.4f @ n=%llu\n", worst_reg,
               (unsigned long long)grid[worst_i]);
    }

    printf("\n  ===== new FFT table (paste into lmmp_fft_table_default_ in fft_ssa.c) =====\n");
    printf("  static const mp_size_t lmmp_fft_table_default_[][2] = {\n");
    for (mp_size_t i = 0; i < g_nrows; ++i) {
        if (g_rows[2 * i] >= 6291457) break; /* 宏区段保持原样 */
        printf("      {%llu, %llu},\n", (unsigned long long)g_rows[2 * i],
               (unsigned long long)g_rows[2 * i + 1]);
    }
    printf("      _FFT_TABLE_ENTRY4(13),\n      _FFT_TABLE_ENTRY4(17),\n");
    printf("      _FFT_TABLE_ENTRY4(21),\n      _FFT_TABLE_ENTRY4(25),\n");
    printf("      {(mp_size_t)-1, 127}};\n");

    /* 平方路径抽查：新表下 sqr 与 mul 一致 */
    {
        static const mp_size_t spots[] = {2000, 3500, 6000, 12000, 25000, 50000,
                                          100000, 200000, 350000, 520000};
        size_t bad_sqr = 0;
        for (size_t s = 0; s < sizeof(spots) / sizeof(spots[0]); ++s) {
            const mp_size_t n = spots[s];
            mp_ptr a = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
            mp_ptr d1 = (mp_ptr)lmmp_alloc((size_t)(2 * n + 1) * sizeof(mp_limb_t));
            mp_ptr d2 = (mp_ptr)lmmp_alloc((size_t)(2 * n + 1) * sizeof(mp_limb_t));
            if (a == NULL || d1 == NULL || d2 == NULL) break;
            tune_fill_limbs(a, n, UINT64_C(0x243f6a8885a308d3));
            a[n - 1] |= LIMB_B_2;
            lmmp_fft_tune_install_(g_rows, g_nrows);
            lmmp_mul_fft_(d1, a, n, a, n);
            lmmp_sqr_(d2, a, n);
            if (memcmp(d1, d2, (size_t)(2 * n) * sizeof(mp_limb_t)) != 0) ++bad_sqr;
            lmmp_free(a);
            lmmp_free(d1);
            lmmp_free(d2);
        }
        printf("\n  squaring path spot check (new table): %s\n",
               bad_sqr == 0 ? "all consistent" : "MISMATCH!!");
        lmmp_fft_tune_reset_();
    }

    mp_size_t old_buckets = 0;
    for (mp_size_t i = 1; i < dcnt; ++i)
        if (drows[2 * i] <= (mp_size_t)grid[ngrid - 1]) ++old_buckets;
    tune_record_add("FFT_TABLE", old_buckets, g_nrows - 1, 0.0, sum_new / sum_old - 1.0);
    return 0;
}
