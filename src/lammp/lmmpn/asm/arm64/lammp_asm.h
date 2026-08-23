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
 * arm64 GAS/LLVM 汇编公共头。
 *
 * Apple 平台 C 符号带有下划线前缀；Linux ELF 不带。本头文件通过
 * ASM_GSYM(name) 统一两者差异，所有 .S 仅需使用 ASM_GSYM 即可同时
 * 兼容 macOS 与 Linux 的 GAS/LLVM 汇编器。
 */

#ifndef LMMP_ARM64_ASM_H
#define LMMP_ARM64_ASM_H

#ifdef LMMP_MACOS
#define ASM_GSYM(s) _##s
#else
#define ASM_GSYM(s) s
#endif

#endif /* LMMP_ARM64_ASM_H */
