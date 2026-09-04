//
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
//

//
// void lmmp_not_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
//
//   Windows: dst=rcx, numa=rdx, na=r8
//   SysV:    dst=rdi, numa=rsi, na=rdx
//   cal: dst[i] = ~numa[i], 4 limbs per vpxor

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_not_)
ASM_GSYM(lmmp_not_):
    vpcmpeqd ymm0, ymm0, ymm0        // ymm0 = ~0
    mov     r10, rx2                 // r10 = na
    mov     r9, r10
    shr     r9, 2                    // r9 = na >> 2
    and     r10d, 3                  // r10 = na & 3
    test    r9, r9
    jz      .Ltail                    // n < 4, tail only

.p2align 4
.Lloop:                             // main loop, 4 limbs
    vmovdqu ymm1, [rx1]
    vpxor   ymm1, ymm1, ymm0        // dst = numa ^ ~0
    vmovdqu [rx0], ymm1
    add     rx0, 32
    add     rx1, 32
    dec     r9
    jnz     .Lloop

.Ltail:
    test    r10, r10
    jz      .Lret

.Ltail_loop:                        // remainder limbs
    mov     rax, [rx1]
    not     rax
    mov     [rx0], rax
    add     rx0, 8
    add     rx1, 8
    dec     r10
    jnz     .Ltail_loop

.Lret:
    vzeroupper
    ret
