/*
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
 * x64 GAS/LLVM 汇编公共头。
 *
 * 本目录 .S 文件统一使用 Intel 语法（GNU as 与 LLVM 集成汇编器均支持），
 * 通过 LMMP_WINDOWS 选择 Windows x64 ABI 或 System V ABI。
 *
 * 说明：
 *   win / lin 是行级开关。win 开头的指令仅在 Windows ABI 下生效；
 *   lin 开头的指令仅在 System V ABI 下生效。其原理是利用 C 预处理器：
 *   win 展开为空，lin 展开为 '#'，从而注释掉不需要的行（反之亦然）。
 */

#ifndef LMMP_X64_ASM_H
#define LMMP_X64_ASM_H

.intel_syntax noprefix

#ifdef LMMP_MACOS
#define ASM_GSYM(s) _##s
#else
#define ASM_GSYM(s) s
#endif

#ifdef LMMP_WINDOWS
#define win
#define lin #
#define rx0 rcx
#define rx1 rdx
#define rx2 r8
#define rx3 r9
#define CALL(name) call name
#else
#define win #
#define lin
#define rx0 rdi
#define rx1 rsi
#define rx2 rdx
#define rx3 rcx
#ifdef LMMP_MACOS
#define CALL(name) call name
#else
#define CALL(name) call name@PLT
#endif
#endif

#endif /* LMMP_X64_ASM_H */
