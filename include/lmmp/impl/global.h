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


typedef void (*lmmp_global_hook_fn)(void);

typedef struct {
    lmmp_global_hook_fn init;    // 初始化钩子，NULL 表示惰性初始化资源
    lmmp_global_hook_fn deinit;  // 释放钩子，NULL 表示无需释放
} lmmp_global_entry_t;

#endif /* __LMMP_GLOBAL_H__ */
