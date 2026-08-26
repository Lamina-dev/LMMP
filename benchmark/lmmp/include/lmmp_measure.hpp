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

#ifndef LMMP_MEASURE_HPP
#define LMMP_MEASURE_HPP

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdio.h>
#include <functional>
#include <string>
#include <vector>

namespace lmmp_measure {

using Clock = std::chrono::steady_clock;

struct MeasureCase {
    const char* category;
    const char* name;
    void (*fn)();
};

inline std::vector<MeasureCase>& registry() {
    static std::vector<MeasureCase> r;
    return r;
}

struct AutoRegister {
    AutoRegister(const char* category, const char* name, void (*fn)()) {
        registry().push_back({category, name, fn});
    }
};

inline double now_sec() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

// 自动校准并测量 op 的单次耗时（ns/op）。重复执行 op，直到样本时间足够。
struct Measurement {
    uint64_t n; // 操作数长度
    double ns_per_op;
    double ops_per_sec;
    uint64_t iters;
};

struct File {
    std::string filename;
    FILE* fp;
    File(const char* name) {
        filename = std::string(name) + ".csv";
        fp = fopen(filename.c_str(), "w");
        if (!fp) {
            std::fprintf(stderr, "Failed to open file");
            exit(1);
        }
        fprintf(fp, "======= %s =======\n", name);
        fprintf(fp, "length/op,ns/op,ops/sec,iter\n");
    }
    ~File() {
        fclose(fp);
    }
};

inline Measurement measure(const std::function<void()>& op, uint64_t n, uint64_t k = 2) {
    // 预热
    for (uint64_t i = 0; i < k; ++i) op();

    double elapsed = 0.0;
    auto t0 = Clock::now();
    for (uint64_t i = 0; i < k; ++i) op();
    auto t1 = Clock::now();
    elapsed = std::chrono::duration<double>(t1 - t0).count();

    // 再测若干次，取最小值以降低调度抖动。
    int samples = 5;
    double best = elapsed * 1e9 / (double)k;  // 统一换算为 ns/op
    for (int s = 0; s < samples; ++s) {
        auto t0 = Clock::now();
        for (uint64_t i = 0; i < k; ++i) op();
        auto t1 = Clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / (double)k;
        best = std::min(best, ns);
    }

    Measurement m;
    m.n = n;
    m.ns_per_op = best;
    m.ops_per_sec = (best > 0.0) ? (1e9 / best) : 0.0;
    m.iters = k;
    return m;
}

inline void write(File& f, const Measurement& m) {
    fprintf(f.fp, "%llu,%f,%f,%llu\n", m.n, m.ns_per_op, m.ops_per_sec, m.iters);
}

inline int run_all(const std::string& filter) {
    int failed = 0;
    for (const auto& tc : registry()) {
        std::string full = std::string(tc.category) + "/" + tc.name;
        if (!filter.empty() && full.find(filter) == std::string::npos) continue;
        std::printf("[ MEASUREING  ] %s\n", full.c_str());
        tc.fn();
        std::printf("[ MEASURED    ] %s\n", full.c_str());
    }
    return failed;
}

inline void list_all() {
    for (const auto& tc : registry()) {
        std::printf("%s/%s\n", tc.category, tc.name);
    }
}

}  // namespace lmmp_measure

/**
 * @brief 打印或更新进度条
 * @param current 当前已完成数量
 * @param total 总数量
 * @param width 进度条显示宽度（字符个数）
 * @param label 进度条前面的描述文字（例如 "Downloading"）
 */
static inline void progress_bar(int current, int total, int width, const char *label) {
    if (current > total) current = total;

    int percent = (current * 100) / total;
    int filled = (current * width) / total;

    printf("\r%s: [", label);
    for (int i = 0; i < filled; i++) {
        printf("=");
    }

    // 如果未完成，显示一个 ">" 表示当前进度位置，剩余部分用空格补齐
    if (filled < width) {
        printf(">");
        for (int i = 0; i < width - filled - 1; i++) {
            printf(" ");
        }
    }
    printf("] %3d%%", percent);

    if (current == total) {
        printf("\n");  // 完成后换行
    }
    fflush(stdout);    // 强制立即输出
}

#define MEASURE_CASE(category, name)                                                           \
    static void measure_fn_##name();                                                           \
    static ::lmmp_measure::AutoRegister measure_ar_##name(category, #name, measure_fn_##name); \
    static void measure_fn_##name()


#endif  // LMMP_MEASURE_HPP
