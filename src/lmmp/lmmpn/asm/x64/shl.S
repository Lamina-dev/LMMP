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
// mp_limb_t lmmp_shl_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl);
//
// mp_limb_t lmmp_shl_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl, mp_limb_t c);
//
//   Windows: dst=rcx, numa=rdx, na=r8, shl=r9, c=[rsp+40] (only c_)
//   SysV:    dst=rdi, numa=rsi, na=rdx, shl=rcx, c=r8 (only c_)
//   ret rax = numa[na-1] >> (64 - shl)
//   cal: dst[i] = (numa[i] << shl) | (numa[i-1] >> (64-shl)), shld chain

#include "lmmp_asm.h"

.text

.p2align 4

// lmmp_shl_c_
.globl ASM_GSYM(lmmp_shl_c_)
ASM_GSYM(lmmp_shl_c_):
#ifdef LMMP_WINDOWS
    mov     rax, [rsp+40]           // rax = c
    xchg    rcx, r9                 // dst <-> shl
#else
    mov     rax, r8                 // rax = c
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif
    jmp     .Lentry

// lmmp_shl_
.p2align 4
.globl ASM_GSYM(lmmp_shl_)
ASM_GSYM(lmmp_shl_):
    xor     rax, rax                // c = 0
#ifdef LMMP_WINDOWS
    xchg    rcx, r9                 // dst <-> shl
#else
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif

.Lentry:
    and     cl, 63                  // shl &= 63
    push    r12
    push    r13
    push    rax                     // save c
    xor     rax, rax

    mov     r10, [rdx + r8*8 - 8]
    shld    rax, r10, cl            // ret = numa[na-1] >> (64-shl)

    test    r8b, 1
    jnz     .Lodd                   // n mod 4 choose path

.Leven:
    test    r8b, 2
    jnz     .Leven2

.Leven0:                            // n mod 4 == 0
    lea     rdx, [rdx + r8*8 - 32]
    lea     r9, [r9 + r8*8]
    mov     r12, r10
    mov     r13, [rdx + 16]
    jmp     .Lent0

.Leven2:                            // n mod 4 == 2
    lea     rdx, [rdx + r8*8 - 16]
    lea     r9, [r9 + r8*8 - 16]
    mov     r11, [rdx]
    jmp     .Lent2

.Lodd:
    test    r8b, 2
    jnz     .Lodd3

.Lodd1:                             // n mod 4 == 1
    dec     r8
    jz      .Ltail1
    lea     rdx, [rdx + r8*8 - 32]
    lea     r9, [r9 + r8*8]
    mov     r12, [rdx + 24]
    mov     r13, [rdx + 16]
    shld    r10, r12, cl            // t = (t << cl) | (t' >> 64-cl)
    mov     [r9], r10
    jmp     .Lent1

.Ltail1:                            // n == 1
    pop     r12
    shl     r10, cl
    or      r10, r12                // (numa[0]<<shl) | c
    mov     [r9], r10
    pop     r13
    pop     r12
    ret

.Lodd3:                             // n mod 4 == 3
    mov     r13, r10
    lea     rdx, [rdx + r8*8 - 24]
    lea     r9, [r9 + r8*8 + 8]
    mov     r10, [rdx + 8]
    mov     r11, [rdx]
    shld    r13, r10, cl
    jmp     .Lent3

.p2align 4
.Lloop:                             // main loop, high to low
    mov     r12, [rdx - 8]
    mov     r13, [rdx - 16]
    lea     rdx, [rdx - 32]
    shld    r10, r11, cl            // t = (t << cl) | (t' >> 64-cl)
    shld    r11, r12, cl
    mov     [r9 + 8], r10
    mov     [r9], r11

.Lent1:
.Lent0:
    mov     r10, [rdx + 8]
    mov     r11, [rdx]
    shld    r12, r13, cl
    shld    r13, r10, cl
    mov     [r9 - 8], r12

.Lent3:
    mov     [r9 - 16], r13
    lea     r9, [r9 - 32]

.Lent2:
    sub     r8, 4
    ja      .Lloop

.Lend:                              // last 2 limbs, merge c
    pop     r12
    shld    r10, r11, cl
    shl     r11, cl
    or      r11, r12                // c into dst[0]
    mov     [r9 + 8], r10
    mov     [r9], r11
    pop     r13
    pop     r12
    ret
