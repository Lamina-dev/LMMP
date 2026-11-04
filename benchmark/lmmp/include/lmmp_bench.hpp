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

#ifndef LMMP_BENCH_FRAMEWORK_HPP
#define LMMP_BENCH_FRAMEWORK_HPP

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace lmmp_bench {

using Clock = std::chrono::steady_clock;

/* 多样本聚合策略 */
enum class Agg {
    MIN,     // 最小值：对调度抖动最稳健，默认
    MEDIAN,  // 中位数：对偶发离群样本稳健
    MEAN,    // 平均值：反映平均负载
};

/*
    测量配置。编写基准时按上下文指定采样次数与聚合策略：
    BenchOpts o; o.max_samples = 9; o.agg = Agg::MEDIAN;
    double us = bench_mul(1000, 1000, o);
*/
struct BenchOpts {
    Agg agg = Agg::MIN;              // 聚合策略
    unsigned max_samples = 5;        // 最大采样次数
    double min_block_sec = 0.02;     // 采样块自适应下限：单块不足此时长则倍增重复次数
    double slow_block_sec = 0.05;    // 单块超过此时长的慢操作，采样数自动降为 2 省墙钟
    uint64_t max_iters = 1ull << 26; // 单块重复次数上限
};

struct BenchCase {
    const char* category;
    const char* name;
    void (*fn)();
};

inline std::vector<BenchCase>& registry() {
    static std::vector<BenchCase> r;
    return r;
}

struct AutoRegister {
    AutoRegister(const char* category, const char* name, void (*fn)()) {
        registry().push_back({category, name, fn});
    }
};

/**
 * @brief 测量 op 的单次耗时
 * @param op 被测操作（模板参数，直接内联调用，无 std::function 间接开销）
 * @param o 采样配置（采样次数与聚合策略等）
 * @return 单次耗时（微秒/op）
 * @note 预热 1 次后自适应倍增重复次数至采样块达到 min_block_sec，
 *       再取至多 max_samples 个样本按 agg 聚合；慢操作自动减采样。
 */
template <class Op>
inline double measure(Op&& op, const BenchOpts& o = {}) {
    op();

    // 校准：倍增 k 直至单块时长足够（越过计时器分辨率与单次噪声）
    uint64_t k = 1;
    double block_sec = 0.0;
    for (;;) {
        auto t0 = Clock::now();
        for (uint64_t i = 0; i < k; ++i) op();
        auto t1 = Clock::now();
        block_sec = std::chrono::duration<double>(t1 - t0).count();
        if (block_sec >= o.min_block_sec || k >= o.max_iters) break;
        k <<= 1;
    }

    /* max_samples==0 亦至少采 1 个样本，避免聚合面对空 us */
    unsigned samples = std::max(1u, block_sec >= o.slow_block_sec ? std::min(2u, o.max_samples)
                                                                  : o.max_samples);
    std::vector<double> us;
    us.reserve(samples);
    for (unsigned s = 0; s < samples; ++s) {
        auto t0 = Clock::now();
        for (uint64_t i = 0; i < k; ++i) op();
        auto t1 = Clock::now();
        us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count() / (double)k);
    }

    switch (o.agg) {
        case Agg::MEDIAN:
            std::sort(us.begin(), us.end());
            return us.size() & 1 ? us[us.size() / 2]
                                 : 0.5 * (us[us.size() / 2 - 1] + us[us.size() / 2]);
        case Agg::MEAN: {
            double sum = 0.0;
            for (double x : us) sum += x;
            return sum / (double)us.size();
        }
        default:
            return *std::min_element(us.begin(), us.end());
    }
}

/**
 * @brief 打印一行测量结果
 * @param name 结果标签（通常为 函数名(尺寸) 形式）
 * @param us_per_op 单次耗时（微秒/op），measure/bench_* 的返回值
 * @param bytes_per_op 每 op 流量（字节），>0 时追加 MiB/s 列
 */
inline void report(const char* name, double us_per_op, double bytes_per_op = 0.0) {
    double ops = us_per_op > 0.0 ? 1e6 / us_per_op : 0.0;
    std::printf("  %-40s %14.4f us/op  %14.2f ops/s", name, us_per_op, ops);
    if (bytes_per_op > 0.0) {
        std::printf("  %10.2f MiB/s", bytes_per_op * ops / (1024.0 * 1024.0));
    }
    std::printf("\n");
}

inline int run_all(const std::string& filter) {
    for (const auto& tc : registry()) {
        std::string full = std::string(tc.category) + "/" + tc.name;
        if (!filter.empty() && full.find(filter) == std::string::npos) continue;
        std::printf("[ BENCH  ] %s\n", full.c_str());
        tc.fn();
    }
    return 0;
}

inline void list_all() {
    for (const auto& tc : registry()) {
        std::printf("%s/%s\n", tc.category, tc.name);
    }
}

}  // namespace lmmp_bench

#define BENCH_CASE(category, name)                                                       \
    static void bench_fn_##name();                                                       \
    static ::lmmp_bench::AutoRegister bench_ar_##name(category, #name, bench_fn_##name); \
    static void bench_fn_##name()

#endif  // LMMP_BENCH_FRAMEWORK_HPP
