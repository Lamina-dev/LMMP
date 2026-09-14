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

#ifndef __LMMP_GLOBAL_H__
#define __LMMP_GLOBAL_H__

#include "../lmmp.h"

/*
 * LMMP 线程局部全局资源的生命周期注册机制
 *
 * 背景：
 *   LMMP 的生命周期资源（栈式分配器、质数筛表、十进制幂表等）均为线程局部
 *   （LMMP_THREAD_LOCAL）存储，进程内不存在可变的初始化状态。因此本注册表
 *   设计为进程级只读静态表：无共享可变状态，天然线程安全，任意线程可并发
 *   进入 lmmp_global_init / lmmp_global_deinit，二者仅操作各自线程的副本。
 *
 * 顺序约定：
 *   lmmp_global_init 按表序（前向）逐项调用 init；
 *   lmmp_global_deinit 按逆表序逐项调用 deinit。
 *   被依赖的资源必须排在依赖它的资源之前（如所有使用 lmmp_alloc / TEMP_* 系列
 *   宏的资源，必须排在栈式分配器条目之后注册）。
 *
 * 约束：
 *   1. 惰性初始化的资源（首次使用时才构建）可将 init 置 NULL，仅注册 deinit；
 *   2. 每个条目的 init / deinit 都必须可重入：重复 init 不重复分配，
 *      重复 deinit 不重复释放；
 *   3. init 阶段失败只能通过 lmmp_abort 终止，不提供部分初始化的回滚。
 *
 * 新增模块注册步骤：
 *   在模块内提供 void(void) 签名的 init / deinit 钩子（可包装现有带参函数），
 *   然后在 lmmp_global_entries[]（src/lmmp/lmmp/global.c）中按依赖顺序追加
 *   一个条目即可，无需修改 lmmp_global_init / lmmp_global_deinit 本身。
 */

typedef void (*lmmp_global_hook_fn)(void);

typedef struct {
    const char* name;            // 资源名
    lmmp_global_hook_fn init;    // 初始化钩子，NULL 表示惰性初始化资源
    lmmp_global_hook_fn deinit;  // 释放钩子，NULL 表示无需释放
} lmmp_global_entry_t;

#endif /* __LMMP_GLOBAL_H__ */
