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

#ifndef __LMMP_TMP_ALLOC_H__
#define __LMMP_TMP_ALLOC_H__

#include "../lmmp.h"
#include <stdio.h>

typedef struct {
    void* stack_begin; // 栈内存起始地址
    void* stack_end;   // 栈内存结束地址
    void* stack_top;   // 栈内存当前指针位置
    void* pool_begin;  // 缓冲池起始地址
    void* pool_top;    // 缓冲池当前指针位置
    size_t remain;     // 缓冲池剩余内存字节数
    size_t capacity;   // 缓冲池容量（初始化后不变）
} lmmp_memory_ctx;

extern LMMP_THREAD_LOCAL lmmp_memory_ctx lmmp_tmpmem_ctx;
extern LMMP_THREAD_LOCAL lmmp_heap_allocator_t global_heap;

typedef struct {
    void* stack_marker; // 栈内存标记
    void* pool_marker;  // 缓冲池标记
    void* heap_marker;  // 堆内存标记
} lmmp_alloc_marker;

static inline void* lmmp_temp_heap_alloc_(lmmp_alloc_marker* pmarker, size_t size) {
    /*
     * pmarker->heap_marker is a head pointer to a linked list of allocated memory blocks.
     * Each allocated block has a header of size HSIZE, which is used to store the
     * next pointer of the block. The actual data starts at (mp_byte_t*)p + offset.
     */
    // 在经过缓冲池后，size已经对齐至LMMP_MAX_ALIGN
    // size = LMMP_ROUND_UP_MULTIPLE(size, LMMP_MAX_ALIGN);
#define HSIZE sizeof(void*)
    const size_t offset = LMMP_ROUND_UP_MULTIPLE(HSIZE, LMMP_MAX_ALIGN);
#undef HSIZE
    void* p = global_heap.alloc(size + offset);
#if LMMP_DEBUG_MEMORY_CHECK == 1
    if (p == NULL) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Memory allocation failure (trying to allocate: %zu bytes)", size);
        lmmp_abort(LMMP_ERROR_MEMORY_ALLOC_FAILURE, msg, __func__, __LINE__);
    }
#endif
    *(void**)p = pmarker->heap_marker;
    pmarker->heap_marker = p;
    return (mp_byte_t*)p + offset;
}

static inline void lmmp_temp_heap_free_(lmmp_alloc_marker* pmarker) {
    /*
     *  Free all allocated memory blocks in the linked list pointed to by pmarker->heap_marker.
     */
    while (pmarker->heap_marker != NULL) {
        void* next = *(void**)(pmarker->heap_marker);
        global_heap.free(pmarker->heap_marker);
        pmarker->heap_marker = next;
    }
}

static inline void* lmmp_temp_stack_alloc_(lmmp_alloc_marker* pmarker, size_t size) {
    /*
     * On the first call, pmarker->stack_marker is a null pointer.
     * We will use pmarker->stack_marker to record the stack frame at this time.
     * When allocating memory subsequently, we will not modify pmarker->stack_marker.
     * Until all stack memory is finally released, we will move to the initial stack position at once,
     * which is the position recorded by pmarker->stack_marker.
     */
    mp_byte_t* p = (mp_byte_t*)(lmmp_tmpmem_ctx.stack_top);
    size_t offset = LMMP_ROUND_UP_MULTIPLE(size, LMMP_MAX_ALIGN);
    mp_byte_t* new_top = p + offset;
#if LMMP_DEBUG_STACK_OVERFLOW_CHECK == 1
    if (new_top > (mp_byte_t*)(lmmp_tmpmem_ctx.stack_end)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Stack overflow (trying to allocate: %zu bytes, stack remaining: %zu bytes)", offset,
                 (size_t)((mp_byte_t*)lmmp_tmpmem_ctx.stack_end - (mp_byte_t*)lmmp_tmpmem_ctx.stack_top));
        lmmp_abort(LMMP_ERROR_MEMORY_ALLOC_FAILURE, msg, __func__, __LINE__);
    }
#endif
    lmmp_tmpmem_ctx.stack_top = new_top;
    if (pmarker->stack_marker == NULL)
        pmarker->stack_marker = p;
    return p;
}

static inline void lmmp_temp_stack_free_(lmmp_alloc_marker* pmarker) {
    // pmarker->stack_marker is not NULL
    lmmp_tmpmem_ctx.stack_top = pmarker->stack_marker;
}

static inline void* lmmp_temp_pool_alloc_(lmmp_alloc_marker* pmarker, size_t size) {
    mp_byte_t* p = (mp_byte_t*)(lmmp_tmpmem_ctx.pool_top);
    size_t offset = LMMP_ROUND_UP_MULTIPLE(size, LMMP_MAX_ALIGN);
    size_t remaining = lmmp_tmpmem_ctx.remain;
    if (remaining < 2 * offset) {
        // 保证pool留有一个offset的空间，使得递归后子问题也尽可能分配到pool中，避免递归时子问题分配到堆上
        return lmmp_temp_heap_alloc_(pmarker, offset);
    } else {
        mp_byte_t* new_top = p + offset;
        lmmp_tmpmem_ctx.remain -= offset;
        lmmp_tmpmem_ctx.pool_top = new_top;
        if (pmarker->pool_marker == NULL)
            pmarker->pool_marker = p;
        return p;
    }
}

static inline void lmmp_temp_pool_free_(lmmp_alloc_marker* pmarker) {
    // pmarker->pool_marker is not NULL
    size_t freed = (mp_byte_t*)(lmmp_tmpmem_ctx.pool_top) - (mp_byte_t*)(pmarker->pool_marker);
    lmmp_tmpmem_ctx.remain += freed;
    lmmp_tmpmem_ctx.pool_top = pmarker->pool_marker;
}

// 临时内存标记声明：用于跟踪临时内存分配
#define TEMP_DECL                                          \
    lmmp_alloc_marker __lmmp_marker_ = {NULL, NULL, NULL}; \
    int __lmmp_temp_decl_guard_ = 0 // if TEMP_DECL without TEMP_FREE, compiler will warn this

#define TEMP_B_DECL                                        \
    lmmp_alloc_marker __lmmp_marker_ = {NULL, NULL, NULL}; \
    int __lmmp_temp_b_decl_guard_ = 0 // if TEMP_B_DECL without TEMP_B_FREE, compiler will warn this

#define TEMP_S_DECL                                        \
    lmmp_alloc_marker __lmmp_marker_ = {NULL, NULL, NULL}; \
    int __lmmp_temp_s_decl_guard_ = 0 // if TEMP_S_DECL without TEMP_S_FREE, compiler will warn this

#define TEMP_SALLOC_THRESHOLD 0x7f00  // 小内存分配阈值（小于等于该值的内存分配在栈上）

// 栈内存分配：使用lmmp_temp_stack_alloc_在栈上分配n字节内存（小内存）
#define TEMP_SALLOC(n) lmmp_temp_stack_alloc_(&__lmmp_marker_, (n))
// 缓冲池分配：使用lmmp_temp_pool_alloc_优先在pool上分配n字节内存（大内存）
#define TEMP_BALLOC(n) lmmp_temp_pool_alloc_(&__lmmp_marker_, (n))
// 临时内存分配：小内存用栈，大内存用堆
#define TEMP_TALLOC(n) ((n) <= TEMP_SALLOC_THRESHOLD ? TEMP_SALLOC(n) : TEMP_BALLOC(n))
// 类型化栈内存分配：分配n个type类型的栈内存
#define SALLOC_TYPE(n, type) ((type*)TEMP_SALLOC((n) * sizeof(type)))
// 类型化堆内存分配：分配n个type类型的堆内存
#define BALLOC_TYPE(n, type) ((type*)TEMP_BALLOC((n) * sizeof(type)))
// 类型化临时内存分配：智能选择栈/堆分配n个type类型内存
#define TALLOC_TYPE(n, type) ((type*)TEMP_TALLOC((n) * sizeof(type)))
// 临时内存释放：释放所有通过TEMP_XALLOC系列函数分配的临时内存
#define TEMP_FREE                                   \
    do {                                            \
        (void)__lmmp_temp_decl_guard_;              \
        lmmp_temp_heap_free_(&__lmmp_marker_);      \
        if (__lmmp_marker_.stack_marker != NULL)    \
            lmmp_temp_stack_free_(&__lmmp_marker_); \
        if (__lmmp_marker_.pool_marker != NULL)     \
            lmmp_temp_pool_free_(&__lmmp_marker_);  \
    } while (0)
#define TEMP_B_FREE                                \
    do {                                           \
        (void)__lmmp_temp_b_decl_guard_;           \
        lmmp_temp_heap_free_(&__lmmp_marker_);     \
        if (__lmmp_marker_.pool_marker != NULL)    \
            lmmp_temp_pool_free_(&__lmmp_marker_); \
    } while (0)
#define TEMP_S_FREE                                 \
    do {                                            \
        (void)__lmmp_temp_s_decl_guard_;            \
        if (__lmmp_marker_.stack_marker != NULL)    \
            lmmp_temp_stack_free_(&__lmmp_marker_); \
    } while (0)

// 类型化内存分配：分配n个type类型的内存（堆）
#define ALLOC_TYPE(n, type) ((type*)lmmp_alloc((size_t)(n) * sizeof(type)))
// 类型化内存重分配：将p指向的内存重分配为new_size个type类型
#define REALLOC_TYPE(p, new_size, type) ((type*)lmmp_realloc((p), (new_size) * sizeof(type)))

#endif /* __LMMP_TMP_ALLOC_H__ */