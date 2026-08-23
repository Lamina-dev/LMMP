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

#ifndef LMMP_LMMP_H
#define LMMP_LMMP_H

/*
    本库的实现部分灵感来源于或改编自

                GNU-MP (https://gmplib.org/),
                FLINT (https://www.flintlib.org/),

    符号说明:

        B           基数，已被硬编码为 2^64

        [p,n,b]     表示指针p指向的、以b为基数的n位数
                    p[i-1] 代表其第i位最低有效位 (0<i<=n)
                    如果省略b，则默认基数为B。通常情况下，
                    用此符号表示函数参数时，一般暗指高位不存在0，
                    在不太严格场景中，比如乘法和加减法中，即使存在高位0
                    通常也不会导致错误，在严格的除法或者其他运算中，会强调
                    高位不能是0。
                    用此符号表示函数返回值时，也表示将会写入的区间，
                    即使可能写入为0。

        sep         指针指向的内存区域完全分离

        eqsep       完全相同的内存区域或者完全分离

                    备注：我们都假定地址是向上增长的，dst <= num+1
                    的内存布局可以这样表示
                            dst ──┐
                   num ──┐        |00000000|00000000|
                         |********|********|

        MSB(x)      x的最高有效位，比如最高有效位为1，MSB(x)=1
                    代表 x >= B / 2

        [x|y]       x或y，用于表示参数或返回值的取值范围

*/

#include <stddef.h> 
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * LMMP 调试宏由 CMake 构建选项注入（参见根 CMakeLists.txt）：
 *   LMMP_DEBUG_STACK_OVERFLOW_CHECK
 *   LMMP_DEBUG_ASSERT_CHECK
 *   LMMP_DEBUG_PARAM_ASSERT_CHECK
 *   LMMP_DEBUG_MEMORY_CHECK
 *   LMMP_MEMORY_MORE_ALLOC_TIMES
 *   LMMP_DEBUG_MEMORY_LEAK
 * 未通过构建系统定义时，下列 #if 均按 0 处理，库为默认无调试开销模式。
 */

#if defined(LMMP_WINDOWS)
    // Windows: MinGW GCC / GNU-driver clang
    #ifdef LMMP_CORE_EXPORTS
    #define LMMP_API __declspec(dllexport)
    #else
    #define LMMP_API __declspec(dllimport)
    #endif
#else
// Linux/macOS: GCC / Clang
    #define LMMP_API __attribute__((visibility("default")))
#endif

#if defined(LMMP_ASM)
#if defined(LMMP_ASM_X64) || defined(LMMP_ASM_ARM64)
#else
#error "LMMP_ASM is only supported on x86_64 or arm64 platforms"
#endif
#endif

typedef uint8_t mp_byte_t;           // 字节类型 (8位无符号整数)
typedef uint64_t mp_limb_t;          // 基本运算单元(limb)类型 (64位无符号整数)
typedef uint64_t mp_size_t;          // 表示limb数量的无符号整数类型
typedef int64_t mp_slimb_t;          // 有符号limb类型 (64位有符号整数)
typedef int64_t mp_ssize_t;          // 表示limb数量的有符号整数类型
typedef mp_limb_t* mp_ptr;           // 指向limb类型的指针
typedef const mp_limb_t* mp_srcptr;  // 指向const limb类型的指针（源操作数指针）
typedef size_t mp_bitcnt_t;          // 表示bit数量的无符号整数类型

#define LMMP_MAX_ALIGN 16  // 最大对齐单位（字节）

#define LIMB_BITS 64
#define LIMB_BYTES 8
#define LOG2_LIMB_BITS 6
#define LIMB_MAX (~(mp_limb_t)0)
#define LLIMB_BITS 32
#define LLIMB_BYTES 4
#define LLIMB_MASK ((mp_limb_t)0xffffffff)

#if defined(LMMP_WINDOWS) && (defined(__GNUC__) || defined(__clang__))
    // MinGW on Windows: use native TLS section
    #define LMMP_THREAD_LOCAL __thread
#elif defined(__GNUC__) || defined(__clang__)
    #define LMMP_THREAD_LOCAL __thread
#else
    #define LMMP_THREAD_LOCAL _Thread_local
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define STATIC_ASSERT _Static_assert
#elif defined(__cplusplus) && __cplusplus >= 201103L
    #define STATIC_ASSERT static_assert
#else
    // C99/C89 fallback (no message)
    #define STATIC_ASSERT(cond, msg) typedef char static_assert_##__LINE__[(cond) ? 1 : -1]
#endif

STATIC_ASSERT(sizeof(void*) == 8, "64-bit architecture required");

#undef STATIC_ASSERT

#ifdef __cplusplus
    /* C++ */
    #if __cplusplus >= 201103L
        /* C++11 and higher */
        #define LMMP_NORETURN [[noreturn]]
    #else
        #if defined(__GNUC__) || defined(__clang__)
            #define LMMP_NORETURN __attribute__((noreturn))
        #else
            #define LMMP_NORETURN /* no definition */
        #endif
    #endif
#else
    /* C  */
    #if defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= 202311L
            /* C23 and higher */
            #define LMMP_NORETURN [[noreturn]]
        #elif __STDC_VERSION__ >= 201112L
            /* C11 / C17 */
            #define LMMP_NORETURN _Noreturn
        #endif
    #endif

    /* fallback */
    #ifndef LMMP_NORETURN
        #if defined(__GNUC__) || defined(__clang__)
            #define LMMP_NORETURN __attribute__((noreturn))
        #else
            #define LMMP_NORETURN /* no definition */
        #endif
    #endif
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    /* C23 std */
    #define LMMP_ASSUME(expr) do { [[assume(expr)]]; } while(0)
#elif defined(__clang__)
    /* Clang */
    #define LMMP_ASSUME(expr) __builtin_assume(expr)
#elif defined(__GNUC__)
    /* GCC */
    #if __GNUC__ >= 13
        /* GCC 13+ */
        #define LMMP_ASSUME(expr) do { __attribute__((assume(expr))); } while(0)
    #else
        /* GCC 13 lower */
        #define LMMP_ASSUME(expr) do { if (!(expr)) __builtin_unreachable(); } while(0)
    #endif
#else
    /* fallback noassume */
    #define LMMP_ASSUME(expr) ((void)(expr))
#endif

/*
 LMMP 内存分配函数指针类型：
 1. heap_alloc : 堆内存分配器
 2. heap_free : 堆内存释放器
 3. realloc : 堆内存重新分配器

 默认使用 malloc、free、realloc 实现。
 如果你需要传入自定义的分配器，为了避免重复处理，且由于LMMP内部几乎不直接
 通过指定的指针来进行内存分配，所以为了避免不必要的错误处理，建议遵循一下的行为：
 1. 传给free的指针应可以为NULL
 2. malloc(size)可以假定size>0，若size为0，可以是未定义行为，可以直接中断。
 3. realloc(p, size)可以假定size>0，若size为0，可以是未定义行为，可以直接中断。
 4. realloc/malloc(p, size)若内存耗尽，或者无空闲块，可以返回NULL，代表分配失败。
 */

typedef void* (*lmmp_heap_alloc_fn)(size_t size);
typedef void  (*lmmp_heap_free_fn )(void*  ptr);
typedef void* (*lmmp_realloc_fn   )(void*  ptr, size_t size);

typedef struct {
    lmmp_heap_alloc_fn   alloc;
    lmmp_heap_free_fn     free;
    lmmp_realloc_fn    realloc;
} lmmp_heap_allocator_t;

/**
 * @brief 设置 LMMP 全局堆内存分配函数
 * @param heap 新的堆内存分配器，可以为NULL，此时会直接返回，而不进行任何操作。
 * @warning 由于所有共享内存都采用堆内存分配。因此，传入新的堆分配器将会首先调用
 *          lmmp_global_deinit 函数，释放所有共享内存，以保证老旧的堆资源被释放。
 *          然后将会自动调用 lmmp_global_init 函数，重新分配所有线程局部内存。
 *          同时，为保证内存不泄露，将会对堆计数器和栈顶以及缓冲池顶进行检查。
 *          若此时堆栈分配计数器不为 0，或当前栈帧不在栈底，都会触发lmmp_abort函数，
 *          并输出相应的错误信息。
 * @note 堆分配器是线程局部的，因此，创建新线程时，如有需要，请自行设置新的堆分配器。
 *       同时也请自行保证分配器和释放器相匹配。
 */
LMMP_API void lmmp_set_heap_allocator(const lmmp_heap_allocator_t* heap);

/**
 * @brief LMMP 全局栈释放函数（通常不需要手动调用）
 * @warning 请注意调用此函数后，访问之前的分配的栈空间将会导致未定义行为。在定义了
 *          LMMP_DEBUG_MEMORY_LEAK 宏时，会检查默认栈是否为空，
 *          则会触发lmmp_abort函数。
 * @note 如果此后再使用栈内存，请调用 lmmp_stack_init 函数，以重置栈空间。
 * @return 返回0表示成功释放，返回-1表示已经释放（并非错误）。
 */
LMMP_API int lmmp_stack_deinit(void);

/**
 * @brief LMMP 全局栈初始化函数（通常不需要手动调用）
 * @note 初始化默认栈，并分配大小为size的额外内存池。如果已经初始化，
 *       则会直接返回-1，表示已经初始化（并非错误），且不进行任何其他操作。
 *       lmmp_global_init 函数会自动调用此函数，分配的额外缓冲池大小为 LMMP_POOL_SIZE 的大小
 *       （默认为512kb），无需手动调用。
 *       如果你想手动设置缓冲池的大小，可以先调用 lmmp_stack_deinit 函数，再调用此函数，将 POOL_SIZE 
 *       设置为你想要的值。同时你也可以禁用缓冲池（即size为0），此时不会分配额外缓冲池。
 * @return 返回0表示成功初始化，返回-1表示已经初始化（并非错误）。
 */
LMMP_API int lmmp_stack_init(size_t size);

typedef enum {
    LMMP_ERROR_ASSERT_FAILURE = 1,
    LMMP_ERROR_DEBUG_ASSERT_FAILURE = 2,
    LMMP_ERROR_PARAM_ASSERT_FAILURE = 3,
    LMMP_ERROR_MEMORY_ALLOC_FAILURE = 4,
    LMMP_ERROR_MEMORY_FREE_FAILURE = 5,
    LMMP_ERROR_OUT_OF_BOUNDS = 6,
    LMMP_ERROR_MEMORY_LEAK = 7,
    LMMP_ERROR_UNEXPECTED_ERROR = 8
} lmmp_error_t;

/**
 * @brief LMMP 全局退出函数指针类型
 * @param type 退出类型（可以查看lmmp_abort函数对此参数的说明，这里不再重复）
 * @param msg 退出信息，取决于type
 * @param func 退出处的函数名
 * @param line 退出处的行号
 */
typedef void (*lmmp_abort_fn)(lmmp_error_t type, const char* msg, const char* func, int line);

/**
 * @brief 设置 LMMP 全局退出函数（所有线程均生效）
 * @param func 退出函数指针，可以为NULL
 * @warning 请注意，我们将不会对 func 的调用做任何保护，因此请不要在 func 里做任何危险的操作，
 *          本库的开发者不对 func 函数的调用产生的影响做任何保证。
 * @note 若 func 为 NULL，则代表使用默认的退出机制。lmmp_abort 函数被标记为无返回值，因此
 *       设置的函数也应当无返回值，即使自定义的func函数真的会返回了，那么也将会调用 abort 函数中断程序。
 * @return 返回之前的退出函数指针，若原指针为NULL，则返回NULL。
 */
LMMP_API lmmp_abort_fn lmmp_set_abort_fn(lmmp_abort_fn func);

/**
 * @brief LMMP 全局退出函数，内部错误或断言失败时调用，若设置了全局退出函数，则会调用该函数，否则会调用默认的退出函数。
 * @param msg 退出信息，assert类型的错误信息通常仅包含断言内容，其他类型的错误则因类型不同而不同。
 * @param func 退出处的函数名
 * @param line 退出处的行号
 * @param type 退出类型。有以下几个类型：
 *
 *        1. ASSERT_FAILURE （枚举值为1）为lmmp_assert触发的退出，lmmp_assert触发的普通退出几乎不可能发生，
 *             其通常代表不可能发生的计算错误，可能表明程序执行此处时必须正确的输入错误。比如预期无进位的加法产生了进位。
 *             此类错误不可接受，会导致计算无法继续进行，导致程序崩溃。
 *
 *        2. DEBUG_ASSERT_FAILURE （枚举值为2）为lmmp_debug_assert触发的退出。其通常表明预期之外的错误，
 *             这通常是调用者的UB，如无UB的情况下触发此错误；也可能是LMMP内部的逻辑错误，开发者期待的输入错误，
 *             在该逻辑处仅简单考虑了某些情况。如有此类错误，可以报告给开发者。此类型只会在定义了 
 *             LMMP_DEBUG_ASSERT_CHECK 宏为 1 的情况下才会触发。
 *
 *        3. PARAM_ASSERT_FAILURE （枚举值为3）为参数检查失败导致的退出。其通常表明调用者传入了无效的参数，
 *             导致函数的行为不符合预期。此类错误不可接受，会导致计算无法继续进行，导致程序崩溃。此类型的错误只有在
 *             定义了 LMMP_DEBUG_PARAM_ASSERT_CHECK 宏为 1 的情况下才会触发。
 *
 *        4. MEMORY_ALLOC_FAILURE （枚举值为4）为内存分配失败退出。在使用堆分配器时，申请内存失败，导致触发此错误。
 *             此类型错误不受明确的宏调控，Realse模式下，会保留必要的触发场景，部分场景下，在一些检查宏定义时，可能会额外触发此错误。
 *
 *        5. MEMORY_FREE_FAILURE （枚举值为5）为内存释放错误。当释放堆内存时可能触发，原因通常为头部信息被损坏，极有可能
 *             源于传入错误的指针，又或者缓冲区溢出导致此头部损坏。LMMP_DEBUG_MEMORY_CHECK 宏为 1 时，才可能触发。
 *
 *        6. OUT_OF_BOUNDS （枚举值为6）为数组越界访问导致的退出。通常表明未按规定使用空间。此类型的错误在堆分配器中，当释放
 *             堆内存时，检测到数组尾部魔数不匹配，导致此错误。LMMP_DEBUG_MEMORY_CHECK 宏为 1 时，才可能触发。
 *
 *        7. MEMORY_LEAK （枚举值为7）为内存泄漏导致的退出。在检查点处，堆内存计数器不为0，或者当前栈帧不在栈底，又或者缓冲池
 *             定不在缓冲池底，都会触发此错误。触发原因可能为，在调用堆分配器重置函数时，显式检查到此时的堆计数器不为0或者栈帧
 *             以及缓冲池不为底部。又或者手动调用 lmmp_leak_tracker 宏来进行检查，检查到了相同的情况。需注意，手动调用此宏时，
 *             请先调用 lmmp_global_deinit 进行全局共享的堆资源的安全释放（因为部分全局共享资源也会被计数）。
 *
 *        8. UNEXPECTED_ERROR （枚举值为8）为其他未知错误导致的退出。
 *
 * @warning LMMP内部中断都将会调用此函数，如果全局退出函数为NULL，则使用默认的退出函数，会打印出全部错误信息，并调用
 *          abort 函数中断程序。自定义全局退出函数请通过 lmmp_set_abort_fn 函数进行设置。请不要在全局退出函数里做任
 *          何危险的操作，本库的开发者不对其调用产生的影响做任何保证。
 */
LMMP_NORETURN LMMP_API void lmmp_abort(lmmp_error_t type, const char *msg, const char *func, int line);

#if LMMP_DEBUG_MEMORY_CHECK == 1
LMMP_API void* lmmp_alloc(size_t size, const char* func, int line);
/**
 * @brief 内存分配函数（调用lmmp_heap_alloc_fn）
 * @param size 要分配的内存字节数
 * @warning size>0
 * @note 调用堆内存分配器，分配失败将触发 lmmp_abort
 * @return 返回指向分配内存的指针（分配失败不会 return NULL，而是直接触发 lmmp_abort）
 */
#define lmmp_alloc(size) lmmp_alloc(size, __func__, __LINE__)
#else
/**
 * @brief 内存分配函数（调用lmmp_heap_alloc_fn）
 * @param size 要分配的内存字节数
 * @warning size>0
 * @note 调用堆内存分配器，分配失败将触发 lmmp_abort
 * @return 返回指向分配内存的指针（分配失败不会 return NULL，而是直接触发 lmmp_abort）
 */
LMMP_API void* lmmp_alloc(size_t size);
#endif

#if LMMP_DEBUG_MEMORY_CHECK == 1
LMMP_API void* lmmp_realloc(void* ptr, size_t size, const char* func, int line);
#define lmmp_realloc(ptr, size) lmmp_realloc(ptr, size, __func__, __LINE__)
#else
/**
 * @brief 内存重分配函数（调用lmmp_realloc_fn）
 * @param ptr 已分配的内存指针
 * @param size 新的内存大小（字节）
 * @warning size>0
 * @note 调用堆内存重新分配器，分配失败将触发 lmmp_abort
 *       在LMMP_DEBUG_PARAM_ASSERT_CHECK宏为1时，重分配 0 字节将触发 lmmp_abort
 * @return 成功返回指向新内存区域的指针（分配失败不会 return NULL，而是直接触发 lmmp_abort）
 */
LMMP_API void* lmmp_realloc(void* ptr, size_t size);
#endif 

#if LMMP_DEBUG_MEMORY_CHECK == 1
LMMP_API void lmmp_free(void* ptr, const char* func, int line);
/**
 * @brief 内存释放函数（调用lmmp_heap_free_fn）
 * @param ptr 要释放的内存指针
 * @note ptr 可以为空指针
 */
#define lmmp_free(ptr) lmmp_free(ptr, __func__, __LINE__)
#else
/**
 * @brief 内存释放函数（调用lmmp_heap_free_fn）
 * @param ptr 要释放的内存指针
 * @note ptr 可以为空指针
 */
LMMP_API void lmmp_free(void* ptr);
#endif

/**
 * @brief 堆内存分配计数器（线程局部）
 * @param cnt 若不为0，则将堆内存计数器置为cnt
 * @return 返回当前的heap分配计数（如果被设置，即返回旧的计数值），即目前未被释放的堆内存数量
 */
LMMP_API int lmmp_alloc_count(int cnt);

/**
 * @brief 内存泄漏检测器
 * @param func 泄漏发生的函数名
 * @param line 泄漏发生的行号
 * @warning 内存计数器均是线程局部的，仅会检测单线程的内存泄漏。
 * @note 将会同时检验堆内存和栈内存，若堆内存计数器不为0，或栈内存的栈顶不在栈底，都会触发lmmp_abort
 *       两者同时发生则将输出两者的信息。
 */
LMMP_API void lmmp_leak_tracker(const char* func, int line);

#if LMMP_DEBUG_MEMORY_LEAK == 1
// 同时检验堆内存和栈内存，若堆内存计数器不为0，或栈内存的栈顶不在栈底，都会触发lmmp_abort
// 两者同时发生则将输出两者的信息。
// 请注意，内存计数器均是线程局部的，仅会检测单线程的内存泄漏。
#define lmmp_leak_tracker lmmp_leak_tracker(__func__, __LINE__)
#else
#define lmmp_leak_tracker ((void)0)
#endif

/**
 * @brief 计算绝对值
 */
#define LMMP_ABS(x) ((x) >= 0 ? (x) : -(x))
/**
 * @brief 返回两个数中的较小值
 */
#define LMMP_MIN(l, o) ((l) < (o) ? (l) : (o))
/**
 * @brief 返回两个数中的较大值
 */
#define LMMP_MAX(h, i) ((h) > (i) ? (h) : (i))
/**
 * @brief 交换两个变量的值
 */
#define LMMP_SWAP(x, y, type) \
    do {                      \
        type _swap_ = (x);    \
        (x) = (y);            \
        (y) = _swap_;         \
    } while (0)
/**
 * @brief 检查n是否为2的幂次方
 */
#define LMMP_POW2_Q(n) (((n) & ((n) - 1)) == 0)
/**
 * @brief 将a向上取整到m的倍数
 */
#define LMMP_ROUND_UP_MULTIPLE(a, m) ((a) + (m) - 1 - ((a) + (m) - 1) % (m))

// 内存拷贝宏：拷贝n个limb（每个8字节），使用memmove保证重叠安全
#define lmmp_copy(dst, src, n) memmove((dst), (src), ((size_t)(n) << 3))
// 内存置零宏：将n个limb置零（每个8字节）
#define lmmp_zero(dst, n) memset((dst), 0, ((size_t)(n) << 3))

// 断言宏：检查条件x是否成立，不成立则触发段错误（严格的错误检查）
// RELEASE 版本也会检查
// 不可使用有副作用的表达式
#define lmmp_assert(x)                                                      \
    do {                                                                    \
        if (!(x)) {                                                         \
            lmmp_abort(LMMP_ERROR_ASSERT_FAILURE, #x, __func__, __LINE__); \
        }                                                                   \
    } while (0)

#if LMMP_DEBUG_ASSERT_CHECK == 1
// 调试断言宏：检查条件x是否成立，不成立则触发段错误（调试版本）
#define lmmp_debug_assert(x)                                                      \
    do {                                                                          \
        if (!(x)) {                                                               \
            lmmp_abort(LMMP_ERROR_DEBUG_ASSERT_FAILURE, #x, __func__, __LINE__); \
        }                                                                         \
    } while (0)
#else
// 调试断言宏：检查条件x是否成立，不成立则触发段错误（调试版本）
#define lmmp_debug_assert(x) LMMP_ASSUME(x)
#endif

#if LMMP_DEBUG_PARAM_ASSERT_CHECK == 1
#define lmmp_param_assert(x)                                                      \
    do {                                                                          \
        if (!(x)) {                                                               \
            lmmp_abort(LMMP_ERROR_PARAM_ASSERT_FAILURE, #x, __func__, __LINE__); \
        }                                                                         \
    } while (0)
#else
#define lmmp_param_assert(x) LMMP_ASSUME(x)
#endif

LMMP_API void lmmp_fill(mp_ptr dst, mp_size_t begin, mp_size_t end, mp_limb_t val);

/**
 * @brief 全局初始化函数（线程局部的）
 * @note 调用此函数将初始化全局范围内的所有线程局部资源，如栈式分配器等。
 *       部分惰性初始化的资源将在首次使用时初始化。调用此函数不会进行初始化。
 *       此函数多次调用不会导致重复初始化。
 * @warning 我们建议在进程或线程启动时调用此函数，以保证线程安全。
 */
LMMP_API void lmmp_global_init(void);

/**
 * @brief （线程局部的）全局共享的动态分配的堆内存资源释放函数
 * @note 调用此函数将释放全局范围内的所有动态分配的堆内存资源。
 *       此函数多次调用不会导致重复释放。如需要再次使用，请重新调用 lmmp_global_init 初始化。
 *       部分惰性初始化的资源将在再次首次使用时再次初始化，调用此函数可能导致部分缓存失效，导致性能下降。
 * @warning 我们建议在线程结束时或程序进程结束时调用此函数。多线程下，每个线程都会拥有独立的副本，
 *          未调用此函数结束线程可能导致内存泄漏。
 */
LMMP_API void lmmp_global_deinit(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LMMP_LMMP_H