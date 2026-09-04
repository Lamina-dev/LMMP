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
// mp_limb_t lmmp_shlnot_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl);
//
//   Windows: dst=rcx, numa=rdx, na=r8, shl=r9
//   SysV:    dst=rdi, numa=rsi, na=rdx, shl=rcx
//   ret rax = numa[na-1] >> (64 - shl)
//   cal: dst[i] = ~((numa[i] << shl) | (numa[i-1] >> (64-shl)))

#include "lmmp_asm.h"

.text

.macro win64_args
#ifdef LMMP_WINDOWS
    xchg    rcx, r9
#endif
.endm

.macro lin64_args
#ifndef LMMP_WINDOWS
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif
.endm

.macro prolog
    push    r12
    push    r13
.endm

.macro epilog
    pop     r13
    pop     r12
    ret
.endm

.text
.p2align 4

.globl ASM_GSYM(lmmp_shlnot_)
ASM_GSYM(lmmp_shlnot_):
    xor     rax, rax
    lin64_args
    win64_args
    and     cl, 63                  // shl &= 63
    prolog

    mov     r10, [rdx+r8*8-8]
    shld    rax, r10, cl            // ret = numa[na-1] >> (64-shl)

    test    r8b, 1
    jnz     .Lodd                   // n mod 4 choose path

.Leven:
    test    r8b, 2
    jnz     .Leven2

.Leven0:                            // n mod 4 == 0
    lea     rdx, [rdx+r8*8-32]
    lea     r9, [r9+r8*8]
    mov     r12, r10
    mov     r13, [rdx+16]
    jmp     .Lent0

.Leven2:                            // n mod 4 == 2
    lea     rdx, [rdx+r8*8-16]
    lea     r9, [r9+r8*8-16]
    mov     r11, [rdx]
    jmp     .Lent2

.Lodd:
    test    r8b, 2
    jnz     .Lodd3

.Lodd1:                             // n mod 4 == 1
    dec     r8
    jz      .Ltail1
    lea     rdx, [rdx+r8*8-32]
    lea     r9, [r9+r8*8]
    mov     r12, [rdx+24]
    mov     r13, [rdx+16]
    shld    r10, r12, cl            // t = (t << cl) | (t' >> 64-cl)
    not     r10                     // t = ~t
    mov     [r9], r10
    jmp     .Lent1

.Ltail1:                            // n == 1
    shl     r10, cl
    not     r10
    mov     [r9], r10
    epilog

.Lodd3:                             // n mod 4 == 3
    mov     r13, r10
    lea     rdx, [rdx+r8*8-24]
    lea     r9, [r9+r8*8+8]
    mov     r10, [rdx+8]
    mov     r11, [rdx]
    shld    r13, r10, cl
    not     r13
    jmp     .Lent3

.p2align 4
.Lloop:                             // main loop, high to low
    mov     r12, [rdx-8]
    mov     r13, [rdx-16]
    lea     rdx, [rdx-32]
    shld    r10, r11, cl            // t = (t << cl) | (t' >> 64-cl)
    shld    r11, r12, cl
    not     r10                     // t = ~t
    not     r11
    mov     [r9+8], r10
    mov     [r9], r11

.Lent1:
.Lent0:
    mov     r10, [rdx+8]
    mov     r11, [rdx]
    shld    r12, r13, cl
    shld    r13, r10, cl
    not     r12
    not     r13
    mov     [r9-8], r12

.Lent3:
    mov     [r9-16], r13
    lea     r9, [r9-32]

.Lent2:
    sub     r8, 4
    ja      .Lloop

.Lend:                              // last 2 limbs
    shld    r10, r11, cl
    shl     r11, cl
    not     r10
    not     r11
    mov     [r9+8], r10
    mov     [r9], r11
    epilog
