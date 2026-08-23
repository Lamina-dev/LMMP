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

#include "../../../include/lmmp/lmmp.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(LMMP_WINDOWS)
#include <windows.h>

static CRITICAL_SECTION abort_func_cs;
static INIT_ONCE abort_cs_once = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK init_abort_cs(PINIT_ONCE once, PVOID param, PVOID* ctx) {
    (void)once;
    (void)param;
    (void)ctx;
    InitializeCriticalSection(&abort_func_cs);
    return TRUE;
}

static void abort_func_lock(void) {
    InitOnceExecuteOnce(&abort_cs_once, init_abort_cs, NULL, NULL);
    EnterCriticalSection(&abort_func_cs);
}

static void abort_func_unlock(void) {
    LeaveCriticalSection(&abort_func_cs);
}

#elif defined(LMMP_MACOS) || defined(LMMP_LINUX)
#include <pthread.h>

static pthread_mutex_t abort_func_mtx = PTHREAD_MUTEX_INITIALIZER;

static void abort_func_lock(void) {
    pthread_mutex_lock(&abort_func_mtx);
}

static void abort_func_unlock(void) {
    pthread_mutex_unlock(&abort_func_mtx);
}

#else
#ifdef __STDC_NO_THREADS__
#error "Threads support is required for setting abort function"
#else
#include <threads.h>
#endif

static mtx_t abort_func_mtx;
static once_flag abort_func_init_flag = ONCE_FLAG_INIT;

static void init_abort_lock(void) {
    mtx_init(&abort_func_mtx, mtx_plain);
}

static void abort_func_lock(void) {
    call_once(&abort_func_init_flag, init_abort_lock);
    mtx_lock(&abort_func_mtx);
}

static void abort_func_unlock(void) {
    mtx_unlock(&abort_func_mtx);
}

#endif

static lmmp_abort_fn lmmp_abort_func = NULL;

lmmp_abort_fn lmmp_set_abort_fn(lmmp_abort_fn func) {
    abort_func_lock();
    lmmp_abort_fn old = lmmp_abort_func;
    lmmp_abort_func = func;
    abort_func_unlock();

    return old;
}

static const char* type_to_str(lmmp_error_t type) {
    switch (type) {
        case LMMP_ERROR_ASSERT_FAILURE:
            return "ASSERT_FAILURE";
        case LMMP_ERROR_DEBUG_ASSERT_FAILURE:
            return "DEBUG_ASSERT_FAILURE";
        case LMMP_ERROR_PARAM_ASSERT_FAILURE:
            return "PARAM_ASSERT_FAILURE";
        case LMMP_ERROR_MEMORY_ALLOC_FAILURE:
            return "MEMORY_ALLOC_FAILURE";
        case LMMP_ERROR_MEMORY_FREE_FAILURE:
            return "MEMORY_FREE_FAILURE";
        case LMMP_ERROR_OUT_OF_BOUNDS:
            return "OUT_OF_BOUNDS";
        case LMMP_ERROR_MEMORY_LEAK:
            return "MEMORY_LEAK";
        case LMMP_ERROR_UNEXPECTED_ERROR:
            return "UNEXPECTED_ERROR";
        default:
            return "UNKNOWN_TYPE";
    }
}

void lmmp_abort(lmmp_error_t type, const char* msg, const char* func, int line) {
    abort_func_lock();
    lmmp_abort_fn fn = lmmp_abort_func;
    abort_func_unlock();

    if (fn != NULL) {
        fn(type, msg, func, line);
    } else {
        fprintf(stderr, "LMMP abort at [%s]:%d\n", func, line);
        fprintf(stderr, "Abort type: %s, abort msg: \n%s\n", type_to_str(type), msg);
        fflush(stderr);
    }
    abort();
}
