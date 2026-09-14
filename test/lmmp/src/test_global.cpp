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

/*
 * lmmp_global_init / lmmp_global_deinit 注册机制测试：
 *   1. 生命周期幂等与重入（重复 init/deinit、释放后重建）；
 *   2. 全周期堆配平（计数分配器验证注册表释放了全部线程局部堆资源）；
 *   3. 线程隔离（主线程 init 后创建新线程再次 init/deinit，互不影响）。
 */

#include "lmmp/lmmp.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"

#include <atomic>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

// 64 limb（高位非零），na >= TO_STR_BASEPOW_THRESHOLD(30)，触发十进制幂表分治路径
constexpr mp_size_t kBigN = 64;
// n > MP_USHORT_MAX 时 lmmp_factorial_ 内部调用 lmmp_prime_int_table_init_，触发质数筛表
constexpr unsigned kFacN = 65537;

void fill_big(mp_limb_t* a, mp_size_t n) {
    for (mp_size_t i = 0; i < n; ++i)
        a[i] = 0x9E3779B97F4A7C15ULL * (mp_limb_t)(i + 1) + 0x123456789ABCDEFULL;
    a[n - 1] |= 1; // 保证高位非零
}

std::string big_to_decimal(const mp_limb_t* a, mp_size_t n) {
    mp_size_t len = lmmp_to_str_len_(a, n, 10);
    std::string s((size_t)len + 8, '\0');
    mp_size_t actual = lmmp_to_str_((mp_byte_t*)&s[0], a, n, 10);
    s.resize((size_t)actual);
    return s;
}

}  // namespace

TEST_CASE("global/lifecycle", lifecycle_idempotent) {
    mp_limb_t a[kBigN];
    fill_big(a, kBigN);

    // 重复 init 幂等
    lmmp_global_init();
    lmmp_global_init();

    std::string s1 = big_to_decimal(a, kBigN);
    TEST_CHECK(s1.size() > (size_t)kBigN * 15);  // 64 limb 十进制位数应在 1220 左右

    mp_limb_t dst[2 * kBigN];
    lmmp_mul_(dst, a, kBigN, a, kBigN);
    mp_limb_t dst_ref[2 * kBigN];
    std::memcpy(dst_ref, dst, sizeof(dst));

    // 触发两张惰性表后整体释放，重复 deinit 幂等
    mp_bitcnt_t bits = 0;
    mp_size_t rn = lmmp_factorial_size_(kFacN, &bits);
    std::vector<mp_limb_t> fac((size_t)rn + 1, 0);
    TEST_CHECK(lmmp_factorial_(fac.data(), bits, rn + 1, kFacN) > 0);

    lmmp_global_deinit();
    lmmp_global_deinit();

    // 释放后重新 init，惰性资源应可重建且结果一致
    lmmp_global_init();
    TEST_CHECK(big_to_decimal(a, kBigN) == s1);

    lmmp_mul_(dst, a, kBigN, a, kBigN);
    TEST_CHECK(std::memcmp(dst, dst_ref, sizeof(dst)) == 0);

    mp_bitcnt_t bits2 = 0;
    mp_size_t rn2 = lmmp_factorial_size_(kFacN, &bits2);
    TEST_CHECK(rn2 == rn && bits2 == bits);
    std::vector<mp_limb_t> fac2((size_t)rn2 + 1, 0);
    TEST_CHECK(lmmp_factorial_(fac2.data(), bits2, rn2 + 1, kFacN) > 0);
    TEST_CHECK(fac2 == fac);
}

namespace {

std::atomic<long> cnt_live{0};

void* cnt_alloc(size_t n) {
    void* p = std::malloc(n);
    if (p != NULL)
        ++cnt_live;
    return p;
}

void cnt_free(void* p) {
    if (p != NULL)
        --cnt_live;
    std::free(p);
}

void* cnt_realloc(void* p, size_t n) {
    void* q = std::realloc(p, n);
    if (q != NULL && p == NULL)
        ++cnt_live;  // realloc(NULL, n) 等价于分配
    return q;
}

}  // namespace

TEST_CASE("global/lifecycle", heap_balance_full_cycle) {
    lmmp_heap_allocator_t counting = {cnt_alloc, cnt_free, cnt_realloc};
    lmmp_heap_allocator_t libc = {(lmmp_heap_alloc_fn)std::malloc, (lmmp_heap_free_fn)std::free,
                                  (lmmp_realloc_fn)std::realloc};

    // 切换即完成一次 deinit+init：此后栈与缓冲池经由计数分配器分配
    lmmp_set_heap_allocator(&counting);
    long live_base = cnt_live.load();
    TEST_CHECK(live_base > 0);  // 栈式分配器+缓冲池经新分配器分配

    mp_limb_t a[kBigN];
    fill_big(a, kBigN);
    mp_limb_t dst[2 * kBigN];
    lmmp_mul_(dst, a, kBigN, a, kBigN);
    (void)big_to_decimal(a, kBigN);  // 构建十进制幂表

    mp_bitcnt_t bits = 0;
    mp_size_t rn = lmmp_factorial_size_(kFacN, &bits);
    std::vector<mp_limb_t> fac((size_t)rn + 1);
    lmmp_factorial_(fac.data(), bits, rn + 1, kFacN);  // 构建质数筛表

    long live_with_tables = cnt_live.load();
    TEST_CHECK(live_with_tables > live_base);  // 两张惰性表持有堆资源

    // deinit 应释放全部注册的线程局部堆资源（栈、缓冲池、两张表）
    lmmp_global_deinit();
    TEST_CHECK(cnt_live.load() == 0);

    // 重新 init 后资源集合应复原（栈+缓冲池）
    lmmp_global_init();
    TEST_CHECK(cnt_live.load() == live_base);

    // 恢复默认分配器（内部再次 deinit+init，计数应先归零后脱离计数）
    lmmp_set_heap_allocator(&libc);
    TEST_CHECK(cnt_live.load() == 0);
}

TEST_CASE("global/thread", thread_isolation) {
    mp_limb_t a[kBigN];
    fill_big(a, kBigN);

    // 主线程已 init（main.cpp），先在主线程构建全部资源并记录期望值
    std::string expect_str = big_to_decimal(a, kBigN);
    mp_bitcnt_t bits = 0;
    mp_size_t expect_rn = lmmp_factorial_size_(kFacN, &bits);

    // 场景：进程内主线程 init 后创建新线程，新线程内再次 init/deinit
    constexpr int kThreads = 4;
    std::vector<std::thread> workers;
    std::vector<int> ok(kThreads, 0);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([t, &a, &ok, &expect_str, expect_rn, bits]() {
            lmmp_global_init();  // 仅初始化本线程的线程局部副本

            bool pass = big_to_decimal(a, kBigN) == expect_str;

            mp_bitcnt_t fb = 0;
            mp_size_t rn = lmmp_factorial_size_(kFacN, &fb);
            pass = pass && (rn == expect_rn) && (fb == bits);
            std::vector<mp_limb_t> fac((size_t)rn + 1, 0);
            pass = pass && (lmmp_factorial_(fac.data(), fb, rn + 1, kFacN) > 0);

            ok[t] = pass;
            lmmp_global_deinit();  // 仅释放本线程副本，不影响主线程
        });
    }

    // 工作线程并发运行期间，主线程持续使用本线程资源
    for (int i = 0; i < 8; ++i)
        TEST_CHECK(big_to_decimal(a, kBigN) == expect_str);

    for (auto& w : workers)
        w.join();
    for (int t = 0; t < kThreads; ++t)
        TEST_CHECK_MSG(ok[t] != 0, "worker thread result mismatch");

    // 工作线程 deinit 后，主线程资源应完好
    TEST_CHECK(big_to_decimal(a, kBigN) == expect_str);
}
