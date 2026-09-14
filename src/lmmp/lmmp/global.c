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

#include "../../../include/lmmp/impl/global.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/prime_table.h"
#include "../../../include/lmmp/impl/str_conv.h"


static void stack_init_default_(void) {
    lmmp_stack_init(LMMP_POOL_SIZE);
}

static void stack_deinit_default_(void) {
    // 栈式分配器释放（返回-1仅表示此前未初始化，并非错误，故忽略）
    (void)lmmp_stack_deinit();
}

static const lmmp_global_entry_t lmmp_global_entries[] = {
    // 栈式分配器+缓冲池：其余资源的分配基础，须最先初始化、最后释放
    {stack_init_default_, stack_deinit_default_},
    // 质数筛表：首次调用 lmmp_prime_int_table_init_ 时惰性构建，仅注册释放
    {NULL, lmmp_prime_int_table_free_},
    // 十进制幂表：首次调用 lmmp_dec_pow_table_ 时惰性构建，仅注册释放
    {NULL, lmmp_dec_pow_table_free_},
};

#define GLOBAL_ENTRIES_NUM (sizeof(lmmp_global_entries) / sizeof(lmmp_global_entries[0]))

void lmmp_global_init(void) {
    for (size_t i = 0; i < GLOBAL_ENTRIES_NUM; ++i) {
        if (lmmp_global_entries[i].init != NULL)
            lmmp_global_entries[i].init();
    }
}

void lmmp_global_deinit(void) {
    for (size_t i = GLOBAL_ENTRIES_NUM; i-- > 0;) {
        if (lmmp_global_entries[i].deinit != NULL)
            lmmp_global_entries[i].deinit();
    }
}
