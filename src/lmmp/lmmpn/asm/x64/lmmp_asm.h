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


#ifndef LMMP_X64_ASM_H
#define LMMP_X64_ASM_H

.intel_syntax noprefix

#ifdef LMMP_MACOS
#define ASM_GSYM(s) _##s
#else
#define ASM_GSYM(s) s
#endif

#ifdef LMMP_WINDOWS
#define rx0 rcx
#define rx1 rdx
#define rx2 r8
#define rx3 r9
#define CALL(name) call name
#else
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
