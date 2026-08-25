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
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#ifndef LMMP_TUNE_INTERNAL_H
#define LMMP_TUNE_INTERNAL_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lmmp/lmmp.h"

/*
 * 调优框架内部接口。
 *
 * 每个阈值调优文件只依赖本头文件提供的计时、配对测量、样本点生成和
 * 一维 badness 搜索；文件之间不共享任何其他业务逻辑。
 */

typedef double (*tune_bench_fn)(void* ctx);

typedef struct {
    double median_ns;   /* 单次调用耗时中位数 */
    double min_ns;
    double max_ns;
    double mad_ns;      /* 绝对中位差，越小越稳定 */
    unsigned loops;     /* 每次采样重复调用次数 */
    unsigned samples;   /* 有效采样数 */
    unsigned retries;   /* 因离群而触发的重测次数 */
} tune_measure_t;

typedef enum {
    TUNE_HIGH_WHEN_GE,  /* size >= T 时走 high 路径 */
    TUNE_HIGH_WHEN_GT   /* size >  T 时走 high 路径 */
} tune_pred_t;

typedef struct {
    uint64_t size;
    double low_ns;      /* 强制 low 路径的中位数耗时 */
    double high_ns;     /* 强制 high 路径的中位数耗时 */
    double low_mad;
    double high_mad;
} tune_point_t;

typedef struct {
    uint64_t threshold;       /* 推荐阈值 */
    double badness;           /* 最小总 badness（相对损失之和） */
    double total_ns;          /* 该阈值下的绝对耗时之和 */
    double margin;            /* 与次优候选的总 badness 差值 */
    uint64_t plateau_lo;      /* 与最优 badness 相差 <= 1e-9 的候选区间 */
    uint64_t plateau_hi;
} tune_choice_t;

typedef struct {
    unsigned samples;    /* 每点每路径的重复采样数，应为奇数 */
    double target_ms;    /* 每个采样希望达到的累计计时毫秒数 */
    double mad_limit;    /* MAD/median 超过该值时重测 */
    unsigned max_retry;  /* 单点最大重测轮数 */
    int write;           /* 是否写回 mparam.h */
    const char* only;    /* --only 过滤串，NULL 表示全部 */
    const char* out_path;/* 结果文件路径 */
} tune_options_t;

extern tune_options_t g_tune;

void tune_timer_init(void);
double tune_now_ns(void);
unsigned tune_calibrate(tune_bench_fn fn, void* ctx, double target_ms);

/*
 * 预热后重复采样并返回中位数统计。若 MAD/median 超过阈值，会整轮重测；
 * 重测仍不稳定时保留中位数（中位数本身已能抵抗少量离群样本）。
 */
tune_measure_t tune_measure(tune_bench_fn fn, void* ctx,
                            unsigned samples, double target_ms);

/*
 * 先分别校准 A/B，再交替采样（A,B,A,B,...），降低后台负载漂移和 CPU
 * 温漂对同一尺寸两条曲线相对关系的影响。
 */
void tune_measure_pair(tune_bench_fn fn_low, void* ctx_low,
                       tune_bench_fn fn_high, void* ctx_high,
                       unsigned samples, double target_ms,
                       tune_measure_t* low, tune_measure_t* high);

/*
 * 在 [lo,hi] 上生成密集样本点：
 *   - 小范围逐整数；
 *   - 大范围按对数均匀采样，并保证 lo、hi、hint 以及 hint 附近整数均被覆盖。
 * 返回实际点数；out 由调用者提供，容量为 cap。
 */
size_t tune_build_points(uint64_t lo, uint64_t hi, uint64_t hint,
                         uint64_t* out, size_t cap);

/*
 *   bad = max(0, chosen_ns / faster_ns - 1)
 * 在候选阈值整数域上扫描，选择总 badness 最小的阈值。
 * 若存在平坦区间，优先取更接近 hint 的候选，避免测量噪声造成平台漂移。
 */
tune_choice_t tune_choose_1d(const tune_point_t* points, size_t npoints,
                             uint64_t lo, uint64_t hi, uint64_t hint,
                             tune_pred_t pred);

void tune_print_measure(const char* tag, const tune_measure_t* m);

/*
 * 标准一维阈值任务。每个阈值文件填写一个 spec 并调用 tune_run_1d；
 * 公共实现负责生成密集样本点、成对交替测量、badness 搜索、记录结果。
 */
typedef struct tune_1d_spec {
    const char* macro_name;
    const char* low_name;
    const char* high_name;
    uint64_t lo;        /* 寻优阈值下限 */
    uint64_t hi;        /* 寻优阈值上限 */
    uint64_t sample_lo; /* 0 表示沿用 lo */
    uint64_t sample_hi; /* 0 表示沿用 hi */
    tune_pred_t pred;
    uint64_t (*get)(void);
    void (*set)(uint64_t);
    /* 为指定 size/path 设置阈值；NULL 时按 pred 自动设置。 */
    void (*apply_path)(uint64_t size, int use_high);
    /* 创建/销毁被测上下文；上下文需包含 size 并绑定好阈值。 */
    void* (*make_ctx)(uint64_t size, int use_high);
    /* 可选：把工作负载 size 映射为实际参与阈值比较的 size。 */
    uint64_t (*map_size)(uint64_t raw_size);
    void (*free_ctx)(void* ctx);
    tune_bench_fn bench;
} tune_1d_spec_t;

int tune_run_1d(const tune_1d_spec_t* spec);

/* nPr 的 product/factor 分界线为 n + B > r*K。 */
typedef struct {
    uint64_t n;
    uint64_t r;
    double product_ns;
    double factor_ns;
} tune_line_point_t;

typedef struct {
    uint64_t k;
    uint64_t b;
    double badness;
    double margin;
} tune_line_choice_t;

tune_line_choice_t tune_choose_2d_line(const tune_line_point_t* points, size_t npoints,
                                       uint64_t k_lo, uint64_t k_hi,
                                       uint64_t b_lo, uint64_t b_hi,
                                       uint64_t k_hint, uint64_t b_hint,
                                       size_t b_steps);
double tune_badness_at(const tune_point_t* points, size_t npoints,
                       uint64_t threshold, tune_pred_t pred);

/*
 * 确定性测试数据/常用小工具。所有调优文件使用同一套数据源，
 * 避免每个文件各自实现随机数生成器。
 */
void tune_fill_limbs(mp_ptr p, mp_size_t n, uint64_t seed);
void tune_fill_bytes(mp_byte_t* p, mp_size_t n, uint64_t seed);
void tune_print_points_1d(const char* low_name, const char* high_name,
                          const tune_point_t* points, size_t npoints);

/*
 * 简单结果记录。调优文件完成后调用；主程序最后写文本和头文件。
 */
void tune_record_add(const char* macro_name, uint64_t old_value, uint64_t new_value,
                     double old_badness, double new_badness);
int tune_record_write_files(const char* txt_path, const char* h_path);
void tune_record_print_summary(void);
int tune_write_mparam(const char* path, const char* backup_path);

/* 每个调优文件的统一入口；实现位于 src/tune_<name>.c。 */
int tune_run_mul_toom22(void);
int tune_run_mul_toom33(void);
int tune_run_mul_toom44(void);
int tune_run_mul_fft(void);
int tune_run_mullo_basecase(void);
int tune_run_mullo_dc(void);
int tune_run_div_divide(void);
int tune_run_bninv_newton(void);
int tune_run_mul_fft_modf(void);
int tune_run_mulhi_mersenne(void);
int tune_run_to_str_basepow(void);
int tune_run_to_str_divide(void);
int tune_run_from_str_basepow(void);
int tune_run_from_str_divide(void);
int tune_run_pow_1_exp(void);
int tune_run_pow_win2_exp(void);
int tune_run_pow_win2_n(void);
int tune_run_factors_mul_n(void);
int tune_run_permutation_ushort(void);
int tune_run_permutation_uint(void);
int tune_run_binomial_rn(void);
int tune_run_elem_mul(void);
int tune_run_mat22_mul(void);
int tune_run_mat22_sqr(void);
int tune_run_sqrt_invnewton(void);
int tune_run_divexact_basecase(void);
int tune_run_divexact_nn(void);

/* 每个调优文件的统一入口；实现位于 src/tune_<name>.c。 */
typedef struct {
    const char* id;
    const char* aliases; /* 逗号分隔的兼容名，可为 NULL */
    const char* title;
    int (*run)(void);
} tune_module_t;

int tune_module_run(const tune_module_t* modules, size_t count,
                    const char* only);

#endif /* LMMP_TUNE_INTERNAL_H */
