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

#ifndef LMMP_BENCH_FRAMEWORK_HPP
#define LMMP_BENCH_FRAMEWORK_HPP

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace lmmp_bench {

using Clock = std::chrono::steady_clock;

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

inline double now_sec() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

// 自动校准并测量 op 的单次耗时（ns/op）。重复执行 op，直到样本时间足够。
struct Measurement {
    double ns_per_op;
    double ops_per_sec;
    uint64_t iters;
};

inline Measurement measure(const std::function<void()>& op, double target_sec = 0.15) {
    // 预热
    for (int i = 0; i < 3; ++i) op();

    uint64_t k = 1;
    double elapsed = 0.0;
    for (;;) {
        auto t0 = Clock::now();
        for (uint64_t i = 0; i < k; ++i) op();
        auto t1 = Clock::now();
        elapsed = std::chrono::duration<double>(t1 - t0).count();
        if (elapsed >= target_sec || k >= (1ull << 30)) break;
        k <<= 1;
    }

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
    m.ns_per_op = best;
    m.ops_per_sec = (best > 0.0) ? (1e9 / best) : 0.0;
    m.iters = k;
    return m;
}

inline void report(const char* name, const Measurement& m, double bytes_per_op = 0.0) {
    std::printf("  %-32s %12.2f ns/op  %14.2f ops/s", name, m.ns_per_op, m.ops_per_sec);
    if (bytes_per_op > 0.0) {
        double mib_per_sec = bytes_per_op * m.ops_per_sec / (1024.0 * 1024.0);
        std::printf("  %10.2f MiB/s", mib_per_sec);
    }
    std::printf("\n");
}

inline int run_all(const std::string& filter) {
    int failed = 0;
    for (const auto& tc : registry()) {
        std::string full = std::string(tc.category) + "/" + tc.name;
        if (!filter.empty() && full.find(filter) == std::string::npos) continue;
        std::printf("[ BENCH  ] %s\n", full.c_str());
        tc.fn();
    }
    return failed;
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
