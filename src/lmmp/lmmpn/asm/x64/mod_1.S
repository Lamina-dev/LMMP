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
// mp_limb_t lmmp_mod_1_(mp_srcptr numa, mp_size_t na, mp_limb_t x);
//
//   Windows: numa=rcx, na=rdx, x=r8
//   SysV:    numa=rdi, na=rsi, x=rdx
//   ret rax = remainder [numa,na] mod x
//   cal: normalize d = x<<k, m = inv(d), per limb q = (xh*m + a_lo + xh+1)>>64, z = a - q*d, correct

#include "lmmp_asm.h"

.text
.globl ASM_GSYM(lmmp_mod_1_)

.p2align 4
ASM_GSYM(lmmp_mod_1_):
    push    rbp
    push    rbx
    push    r12
#ifdef LMMP_WINDOWS
    push    rsi
#endif

#ifdef LMMP_WINDOWS
    mov     rsi, rcx                // rsi = numa
    mov     rbx, rdx                // rbx = na
#else
    mov     rbx, rsi                // rbx = na
    mov     rsi, rdi                // rsi = numa
    mov     r8, rdx                 // r8 = x
#endif

    xor     eax, eax
    bsr     rcx, r8
    not     rcx
    add     rcx, 64
    shl     r8, cl                  // normalize d = x << k

    mov     rbp, rcx                // rbp = k = clz(x)
    mov     r12, r8                 // r12 = d
    mov     rx0, r8                 // arg d

#ifdef LMMP_WINDOWS
    sub     rsp, 40                 // align + shadow space
#endif
    CALL(lmmp_inv_1_)
#ifdef LMMP_WINDOWS
    add     rsp, 40
#endif

    mov     r9, rax                 // r9 = m = inv(d) - 2^64
    mov     r8, r12                 // r8 = d
    mov     rcx, rbp                // cl = k

    xor     eax, eax
    mov     r10, [rsi+rbx*8-8]      // top limb
    shld    rax, r10, cl            // rax = xh = top limb << k
    jmp     .Lent

.p2align 4
.Ltop:
    mov     r12, [rsi+rbx*8-8]      // next lower limb
    shld    r10, r12, cl            // r10 = a_lo << k

.Llast:                             // rax = xh < d, rbp = xh+1, r10 = a_lo
    mul     r9                      // rdx:rax = xh * m
    add     rax, r10
    adc     rdx, rbp                // rdx = q
    mov     rbp, rax                // rbp = q_lo
    imul    rdx, r8                 // rdx = q*d
    sub     r10, rdx                // z = a_lo - q*d
    lea     rax, [r8+r10]           // candidate z+d
    cmp     r10, rbp
    cmovc   rax, r10                // borrow: pick z, else +d
    cmp     rax, r8
    jae     .Lfix                   // z >= d: z -= d

.Lok:
    mov     r10, r12

.Lent:
    sub     rbx, 1
    lea     rbp, [rax+1]            // next xh+1
    ja      .Ltop
    jb      .Lret

    shl     r10, cl                 // last limb << k
    jmp     .Llast

.Lfix:
    sub     rax, r8                 // z -= d
    jmp     .Lok

.Lret:
    shr     rax, cl                 // remainder >>= k

#ifdef LMMP_WINDOWS
    pop     rsi
#endif
    pop     r12
    pop     rbx
    pop     rbp
    ret
