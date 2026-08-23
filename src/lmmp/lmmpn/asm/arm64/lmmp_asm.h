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


#ifndef LMMP_ARM64_ASM_H
#define LMMP_ARM64_ASM_H

#ifdef LMMP_MACOS
#define ASM_GSYM(s) _##s
#else
#define ASM_GSYM(s) s
#endif

#endif /* LMMP_ARM64_ASM_H */
