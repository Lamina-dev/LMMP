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
//   cal: dst[i] = ~numa[i], 8 limbs per 2x vpxor

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_not_)
ASM_GSYM(lmmp_not_):
    vpcmpeqd ymm0, ymm0, ymm0       // ymm0 = ~0
    mov     r10, rx2                // r10 = na
    mov     r9, r10
    shr     r9, 3                   // r9 = na >> 3
    and     r10d, 7                 // r10 = na & 7
    test    r9, r9
    jz      .Lmid                   // n < 8, tail only

.p2align 4
.Lloop:                             // main loop, 8 limbs
    vmovdqu ymm1, [rx1]
    vmovdqu ymm2, [rx1+32]
    vpxor   ymm1, ymm1, ymm0        // dst = numa ^ ~0
    vpxor   ymm2, ymm2, ymm0
    vmovdqu [rx0], ymm1
    vmovdqu [rx0+32], ymm2
    add     rx0, 64
    add     rx1, 64
    dec     r9
    jnz     .Lloop

.Lmid:                              // odd 4-limb group
    test    r10, 4
    jz      .Ltail
    vmovdqu ymm1, [rx1]
    vpxor   ymm1, ymm1, ymm0
    vmovdqu [rx0], ymm1
    add     rx0, 32
    add     rx1, 32
    and     r10d, 3                 // r10 = na & 3

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
