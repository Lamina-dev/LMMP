//  Copyright (C) 2026 HJimmyK(Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it under
//  the terms of the GNU Lesser General Public License (LGPL) as published
//  by the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed WITHOUT ANY WARRANTY.
//
//  See <https://www.gnu.org/licenses/>.

// void lmmp_sqrlo_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t n);
//
//   x0=dst, x1=numa, x2=n
//   ret: none
//   cal: dst[0..n) = (numa^2) mod B^n
//
//   临时实现：尾调用路由至 lmmp_mullo_basecase_(dst, numa, numa, n)，
//   专用三角实现在 arm64 机器上另行补齐。

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_sqrlo_basecase_)
.p2align 2

ASM_GSYM(lmmp_sqrlo_basecase_):
    mov     x3, x2                 // n
    mov     x2, x1                 // numb = numa
    b       ASM_GSYM(lmmp_mullo_basecase_)
