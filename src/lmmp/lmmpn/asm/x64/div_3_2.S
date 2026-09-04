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
// mp_limb_t lmmp_div_3_2_(mp_ptr numa, mp_srcptr numb, mp_limb_t inv21);
//
//   Windows: numa=rcx, numb=rdx, inv21=r8
//   SysV:    numa=rdi, numb=rsi, inv21=rdx
//   ret rax = quotient q; [numa,2] = [numa,3] mod [numb,2]
//   cal: q = (a2*inv21 + a1) >> 64, z = a - q*b, correct

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_div_3_2_)
ASM_GSYM(lmmp_div_3_2_):
    push    rbx
    push    rsi
    push    rdi
    push    r12
    push    r13
    push    r15

#ifdef LMMP_WINDOWS
    mov     r15, rcx                // r15 = numa
    mov     rsi, rdx                // rsi = numb
    mov     rax, r8                 // rax = inv21
#else
    mov     r15, rdi                // r15 = numa
    mov     rax, rdx                // rax = inv21
#endif

    mov     r8,  [rsi]              // b0
    mov     r9,  [rsi+8]            // b1
    mov     r11, [r15]              // a0
    mov     r12, [r15+8]            // a1
    mov     r13, [r15+16]           // a2

    mul     r13                     // rdx:rax = a2 * inv21
    mov     r10, r12
    add     r10, rax
    adc     rdx, r13                // rdx:r10 = q = a2*inv21 + a1
    mov     r13, r10                // r13 = q_lo
    mov     r10, rdx                // r10 = qh
    imul    rdx, r9                 // rdx = qh*bh
    xchg    r13, r12                // r12 = q_lo, r13 = a1
    mov     rax, r8
    sub     r13, rdx                // r13 = a1 - qh*bh
    xchg    r12, r11                // r11 = q_lo, r12 = a0
    mul     r10                     // rdx:rax = qh*bl
    sub     r12, r8
    sbb     r13, r9                 // pre-subtract one b
    sub     r12, rax
    sbb     r13, rdx                // r13:r12 = z = a - q*b

    xor     rax, rax
    xor     rdx, rdx

    cmp     r13, r11
    cmovnc  rax, r8                 // no borrow: z += b
    cmovnc  rdx, r9
    adc     r10, 0                  // borrow: qh--

    add     r12, rax
    adc     r13, rdx                // z += b if needed

    cmp     r13, r9                 // z >= b?
    jae     .Lfix                   // yes: z -= b, qh++

.Lok:
    mov     [r15],   r12            // remainder low
    mov     [r15+8], r13            // remainder high
    mov     rax, r10                // ret qh

    pop     r15
    pop     r13
    pop     r12
    pop     rdi
    pop     rsi
    pop     rbx
    ret

.Lfix:                              // 128-bit compare z >= b
    seta    dl
    cmp     r12, r8
    setae   al
    or      al, dl
    jz      .Lok                    // z < b, no fix

    inc     r10
    sub     r12, r8
    sbb     r13, r9
    jmp     .Lok
