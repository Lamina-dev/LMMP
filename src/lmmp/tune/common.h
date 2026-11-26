/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
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

#ifndef LMMP_TUNE_COMMON_H
#define LMMP_TUNE_COMMON_H

#include <stdbool.h>
#include <stdio.h>

#include "lmmp/lmmp.h"

typedef enum { LMMP_TUNE_RUN, LMMP_TUNE_INFO, LMMP_TUNE_CURRENT, LMMP_TUNE_HELP } lmmp_tune_action;

/* 程序私有命令行选项的回调：option 是以 "--" 开头的选项名，value 是其后紧邻的一个参数
   （没有后续参数时为 NULL）；返回 false 表示选项取值非法，原因由回调打印 */
typedef bool (*lmmp_tune_option_fn)(void* context, const char* option, const char* value);

typedef struct {
    lmmp_tune_action action;
    unsigned samples;
    double min_ms;
    double mad_limit;
    unsigned max_retry;
    unsigned long long seed;
} lmmp_tune_options;

/* 破坏输入的基准每段计时前准备的输入副本上限；工作区须容纳这么多份输入 */
enum { LMMP_TUNE_BATCH_COPIES = 256 };

typedef struct {
    void (*bind)(void* context);
    /* reset 与 reset_batch 二选一：reset 每块计时前重建一次输入，供只读输入的基准；
       reset_batch 供会原地改写输入的基准，在每段计时前把 count 份原始输入依次铺进工作区
       并回到首份，之后每次 call 只消费一份并自行推进游标，铺副本的时间不计入 */
    void (*reset)(void* context);
    void (*reset_batch)(void* context, mp_size_t count);
    mp_limb_t (*call)(void* context);
    void* context;
} lmmp_tune_bench;

/** @brief 配对测量末次尝试的逐样本记录：low_first 是各样本实际的先后顺序，
 *         其余两项是两路各自的原始块计时。 */
typedef struct {
    bool low_first[31];
    double low_us[31];
    double high_us[31];
} lmmp_tune_pair_trace;

void lmmp_tune_options_init(lmmp_tune_options* options);
bool lmmp_tune_parse(int argc, char** argv, lmmp_tune_options* options);
/** @brief 与 lmmp_tune_parse 相同，另把公共层不认识的选项连同其取值交给 option_fn；option_fn 为 NULL 时
 *         等同 lmmp_tune_parse。 */
bool lmmp_tune_parse_ex(int argc,
                        char** argv,
                        lmmp_tune_options* options,
                        lmmp_tune_option_fn option_fn,
                        void* context);
void lmmp_tune_usage(const char* program_id);

bool lmmp_tune_measure(const lmmp_tune_bench* bench,
                       const lmmp_tune_options* options,
                       double* median_us,
                       double* mad_ratio,
                       unsigned* retries);

/**
 * @brief 交替测量 low/high 两条路径：每个采样批次前分别重新绑定两路参数并重建输入，
 *        样本间交替先测哪一路（偶数样本先 low、奇数样本先 high），两路承受相同的漂移与
 *        热状态；任一路 MAD 超限则整组重测。重试耗尽仍超限时返回 true 并保留测量数据
 *        （诊断用），*stable 置 0 标记不可靠。
 */
bool lmmp_tune_measure_pair(const lmmp_tune_bench* low,
                            const lmmp_tune_bench* high,
                            const lmmp_tune_options* options,
                            double* low_us,
                            double* low_mad,
                            double* high_us,
                            double* high_mad,
                            unsigned* retries,
                            bool* stable);

/** @brief 与 lmmp_tune_measure_pair 相同，另把末次尝试各样本的实际测量顺序与两路原始块计时
 *         写入 trace；trace 可为 NULL。 */
bool lmmp_tune_measure_pair_traced(const lmmp_tune_bench* low,
                                   const lmmp_tune_bench* high,
                                   const lmmp_tune_options* options,
                                   double* low_us,
                                   double* low_mad,
                                   double* high_us,
                                   double* high_mad,
                                   unsigned* retries,
                                   bool* stable,
                                   lmmp_tune_pair_trace* trace);

/** @brief 调优程序统一安装的静默退出回调：把库内断言失败打印到标准错误后立即退出，避免弹窗。 */
void lmmp_tune_abort(lmmp_error_t type, const char* message, const char* function, int line);

/** @brief 把汇编后端、目标系统与构建类型逐行打印到标准输出，供 --info 使用。 */
void lmmp_tune_print_env(void);

/** @brief 把编译器、构建类型、目标系统与汇编后端以 # 注释行打印到标准输出，作为结果头。 */
void lmmp_tune_print_env_comment(void);

/** @brief 打印单个阈值的架构默认值与当前运行时值，供 --current 使用。 */
void lmmp_tune_print_current(const char* name, unsigned long long default_value, unsigned long long current_value);

/* ---------- double 阈值 ---------- */

/** @brief 解析逗号分隔的正实数候选表（如 "96,138,192"）：至少一个、至多 max_count 个，取值须有限且大于 0；
 *         结果按升序排列并去重。 */
bool lmmp_tune_parse_doubles(const char* text, double* values, unsigned max_count, unsigned* count);

/** @brief 把 value 并入升序候选表（已有相同取值时不重复）；表满时返回 false。 */
bool lmmp_tune_add_double(double* values, unsigned* count, unsigned max_count, double value);

/** @brief 打印单个 double 阈值的架构默认值与当前运行时值，供 --current 使用。 */
void lmmp_tune_print_current_double(const char* name, double default_value, double current_value);

/** @brief 打印 double 候选表的规模与步长：等差时给 range 与 step，否则如实标 irregular 并列出全部取值。 */
void lmmp_tune_print_double_candidates(const char* name, const double* values, unsigned count);

typedef struct {
    unsigned x, y;  /* 候选表下标 */
    double excess;  /* 平均相对超出 */
} lmmp_tune_pair_pick;

/** @brief 二维候选表的联合 argmin（行主序，并列时在先者胜）；无测量点时 excess 为 NaN。 */
lmmp_tune_pair_pick lmmp_tune_pair_argmin(const double* scores,
                                          const unsigned long* counts,
                                          unsigned x_count,
                                          unsigned y_count);

/** @brief 二维 double 候选 (x,y) 的联合推荐输出：逐候选 badness 矩阵（行 x 列 y，百分数）、最优对、
 *         平坦带（平均相对超出不超过最优值加 1 个百分点的候选对：个数、两轴外延、过最优点的两条轴切片）、
 *         每行每列的最优搭档、现值对的 badness，末行打印推荐对。 */
lmmp_tune_pair_pick lmmp_tune_recommend_pair(const char* name_x,
                                             const char* name_y,
                                             const double* scores,
                                             const unsigned long* counts,
                                             const double* candidates_x,
                                             unsigned x_count,
                                             const double* candidates_y,
                                             unsigned y_count,
                                             double current_x,
                                             double current_y);

/** @brief 把单个测量点按候选阈值应选路径的耗时累加进评分：候选高于该点规模时计低路，
 *         不高于该点规模且高于 sub_n（该行高路同阈值递归子问题的规模，无此递归时传 0）
 *         时计高路——仅当部署路径与测量路径一致时才复用；候选不高于 sub_n 时不累加，
 *         由调用方按该候选实际启用的递归路径另行实测。
 */
void lmmp_tune_score_point(double* scores,
                           unsigned long* counts,
                           int candidate_min,
                           int candidate_max,
                           mp_size_t n,
                           mp_size_t sub_n,
                           double low_us,
                           double high_us);

/** @brief 汇总候选评分，向标准输出打印逐候选 badness、推荐值与 ±1% 平坦区间；无测量点时不推荐。 */
void lmmp_tune_recommend(const char* name,
                         const double* scores,
                         const unsigned long* counts,
                         int candidate_min,
                         int candidate_max,
                         unsigned long long current_value);

#endif
