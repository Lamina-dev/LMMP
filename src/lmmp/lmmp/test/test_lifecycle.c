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

/*
 * lmmp_global_init / lmmp_global_deinit 注册机制测试：
 *   1. 生命周期幂等与重入（重复 init/deinit、释放后重建）；
 *   2. 全周期堆配平（计数分配器验证注册表释放了全部线程局部堆资源）；
 *   3. 线程隔离（主线程 init 后创建新线程再次 init/deinit，互不影响）。
 */

#include "lmmp/lmmp.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "cases.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#if defined(LMMP_WINDOWS)
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

enum { BIG_N = 64, DECIMAL_CAP = 20 * BIG_N + 8, FACTORIAL_N = 65537 };

static void fill_big(mp_ptr a) {
    for (mp_size_t i = 0; i < BIG_N; ++i)
        a[i] = 0x9e3779b97f4a7c15ull * (i + 1) + 0x123456789abcdefull;
    a[BIG_N - 1] |= 1;
}

static mp_size_t big_to_decimal(mp_byte_t *out, mp_srcptr a) {
    return lmmp_to_str_(out, a, BIG_N, 10);
}

static mp_ptr factorial_buffer(mp_size_t n) {
    mp_ptr buffer = calloc(n + 1, sizeof(*buffer));
    if (!buffer)
        abort();
    return buffer;
}

TEST_CASE("global/lifecycle", lifecycle_idempotent) {
    mp_limb_t a[BIG_N], dst[2 * BIG_N], dst_ref[2 * BIG_N];
    mp_byte_t before[DECIMAL_CAP], after[DECIMAL_CAP];
    fill_big(a);

    lmmp_global_init();
    lmmp_global_init();

    mp_size_t len = big_to_decimal(before, a);
    TEST_CHECK(len > BIG_N * 15);
    lmmp_mul_(dst_ref, a, BIG_N, a, BIG_N);

    mp_bitcnt_t bits = 0;
    mp_size_t rn = lmmp_factorial_size_(FACTORIAL_N, &bits);
    mp_ptr fac = factorial_buffer(rn);
    TEST_CHECK(lmmp_factorial_(fac, bits, rn + 1, FACTORIAL_N) > 0);

    lmmp_global_deinit();
    lmmp_global_deinit();
    lmmp_global_init();

    TEST_CHECK(big_to_decimal(after, a) == len && memcmp(after, before, len) == 0);
    lmmp_mul_(dst, a, BIG_N, a, BIG_N);
    TEST_CHECK(memcmp(dst, dst_ref, sizeof(dst)) == 0);

    mp_bitcnt_t bits2 = 0;
    mp_size_t rn2 = lmmp_factorial_size_(FACTORIAL_N, &bits2);
    TEST_CHECK(rn2 == rn && bits2 == bits);
    mp_ptr fac2 = factorial_buffer(rn2);
    TEST_CHECK(lmmp_factorial_(fac2, bits2, rn2 + 1, FACTORIAL_N) > 0);
    TEST_CHECK(rn == rn2 && memcmp(fac, fac2, (rn + 1) * sizeof(*fac)) == 0);
    free(fac2);
    free(fac);
}

static atomic_long live_allocations;

static void *count_alloc(size_t n) {
    void *p = malloc(n);
    if (p)
        ++live_allocations;
    return p;
}

static void count_free(void *p) {
    if (p)
        --live_allocations;
    free(p);
}

static void *count_realloc(void *p, size_t n) {
    return p ? realloc(p, n) : count_alloc(n);
}

TEST_CASE("global/lifecycle", heap_balance_full_cycle) {
    const lmmp_heap_allocator_t counting = {count_alloc, count_free, count_realloc};
    const lmmp_heap_allocator_t libc = {malloc, free, realloc};
    lmmp_set_heap_allocator(&counting);
    long baseline = atomic_load(&live_allocations);
    TEST_CHECK(baseline > 0);

    mp_limb_t a[BIG_N], dst[2 * BIG_N];
    mp_byte_t decimal[DECIMAL_CAP];
    fill_big(a);
    lmmp_mul_(dst, a, BIG_N, a, BIG_N);
    big_to_decimal(decimal, a);

    mp_bitcnt_t bits = 0;
    mp_size_t rn = lmmp_factorial_size_(FACTORIAL_N, &bits);
    mp_ptr fac = factorial_buffer(rn);
    lmmp_factorial_(fac, bits, rn + 1, FACTORIAL_N);
    free(fac);
    TEST_CHECK(atomic_load(&live_allocations) > baseline);

    lmmp_global_deinit();
    TEST_CHECK(atomic_load(&live_allocations) == 0);
    lmmp_global_init();
    TEST_CHECK(atomic_load(&live_allocations) == baseline);
    lmmp_set_heap_allocator(&libc);
    TEST_CHECK(atomic_load(&live_allocations) == 0);
}

typedef struct {
    mp_srcptr input;
    const mp_byte_t *decimal;
    mp_size_t len, rn;
    mp_bitcnt_t bits;
    bool pass;
} thread_case;

#if defined(LMMP_WINDOWS)
static unsigned __stdcall thread_worker(void *arg) {
#else
static void *thread_worker(void *arg) {
#endif
    thread_case *test = arg;
    lmmp_global_init();

    mp_byte_t decimal[DECIMAL_CAP];
    bool match = big_to_decimal(decimal, test->input) == test->len &&
                 memcmp(decimal, test->decimal, test->len) == 0;
    mp_bitcnt_t bits = 0;
    mp_size_t rn = lmmp_factorial_size_(FACTORIAL_N, &bits);
    mp_ptr fac = factorial_buffer(rn);
    test->pass = match && rn == test->rn && bits == test->bits &&
                 lmmp_factorial_(fac, bits, rn + 1, FACTORIAL_N) > 0;
    free(fac);

    lmmp_global_deinit();
    return 0;
}

TEST_CASE("global/thread", thread_isolation) {
    enum { WORKERS = 4 };
    mp_limb_t a[BIG_N];
    mp_byte_t expected[DECIMAL_CAP], decimal[DECIMAL_CAP];
    fill_big(a);
    mp_size_t len = big_to_decimal(expected, a);
    mp_bitcnt_t bits = 0;
    mp_size_t rn = lmmp_factorial_size_(FACTORIAL_N, &bits);
    thread_case cases[WORKERS];
#if defined(LMMP_WINDOWS)
    HANDLE workers[WORKERS];
#else
    pthread_t workers[WORKERS];
#endif
    size_t started = 0;
    for (; started < WORKERS; ++started) {
        cases[started] = (thread_case){a, expected, len, rn, bits, false};
#if defined(LMMP_WINDOWS)
        workers[started] = (HANDLE)_beginthreadex(NULL, 0, thread_worker, &cases[started], 0, NULL);
        if (!workers[started])
            break;
#else
        if (pthread_create(&workers[started], NULL, thread_worker, &cases[started]) != 0)
            break;
#endif
    }
    TEST_CHECK_EQ(started, WORKERS);

    for (int repeat = 0; repeat < 8; ++repeat)
        TEST_CHECK(big_to_decimal(decimal, a) == len && memcmp(decimal, expected, len) == 0);

    for (size_t i = 0; i < started; ++i) {
#if defined(LMMP_WINDOWS)
        if (WaitForSingleObject(workers[i], INFINITE) != WAIT_OBJECT_0)
            abort();
        CloseHandle(workers[i]);
#else
        if (pthread_join(workers[i], NULL) != 0)
            abort();
#endif
        TEST_CHECK_MSG(cases[i].pass, "worker thread result mismatch");
    }
    TEST_CHECK(big_to_decimal(decimal, a) == len && memcmp(decimal, expected, len) == 0);
}

void test_lifecycle(void) {
    TEST_RUN("global/lifecycle", lifecycle_idempotent);
    TEST_RUN("global/lifecycle", heap_balance_full_cycle);
    TEST_RUN("global/thread", thread_isolation);
}
