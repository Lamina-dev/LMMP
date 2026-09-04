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
// mp_limb_t lmmp_subshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
//   Windows: dst=rcx, numa=rdx, numb=r8, n=r9
//   SysV:    dst=rdi, numa=rsi, numb=rdx, n=rcx
//   ret rax = numa - (numb << 1) 's borrow
//   cal: dst[i] = numa[i] - (numb[i]<<1 | numb[i-1]>>63) - borrow

#include "lmmp_asm.h"

.text

.p2align 4

.globl ASM_GSYM(lmmp_subshl1_n_)
ASM_GSYM(lmmp_subshl1_n_):

#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    mov     rdi, rcx                // SysV:
    mov     rsi, rdx                //   rdi=dst, rsi=numa,
    mov     rdx, r8                 //   rdx=numb, r8=n
    mov     r8, r9
#else
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    mov     r8, rcx                 // r8 = n
#endif

    xor     r14d, r14d              // prev = 0
    xor     r15d, r15d              // borrow = 0 or -1
    mov     rcx, r8
    and     rcx, 3                  // remainder limbs
    jz      .Lgroups

.p2align 4
.Lscalar:                           // remainder limbs first
    mov     r10, [rdx]              // t = numb[i]
    shld    r10, r14, 1             // sh = (numb[i]<<1) | (prev>>63)
    mov     r14, [rdx]              // prev = numb[i]
    add     r15, r15                // restore borrow CF
    mov     r9, [rsi]               // a = numa[i]
    sbb     r9, r10                 // a - sh - CF
    sbb     r15, r15                // capture -borrow
    mov     [rdi], r9
    lea     rdi, [rdi+8]
    lea     rsi, [rsi+8]
    lea     rdx, [rdx+8]
    dec     rcx
    jnz     .Lscalar

.Lgroups:
    mov     rcx, r8
    shr     rcx, 2                  // 4 limbs per group
    jz      .Ldone

.p2align 4
.Lloop:                             // main loop, 4 limbs
    mov     r10, [rdx]              // t0 = numb[i]
    mov     r11, [rdx+8]            // t1
    mov     r12, [rdx+16]           // t2
    mov     r13, [rdx+24]           // t3
    shld    r13, r12, 1             // sh3 = (t3<<1) | (t2>>63)
    shld    r12, r11, 1             // sh2 = (t2<<1) | (t1>>63)
    shld    r11, r10, 1             // sh1 = (t1<<1) | (t0>>63)
    shld    r10, r14, 1             // sh0 = (t0<<1) | (prev>>63)
    mov     r14, [rdx+24]           // prev = numb[i+3]
    add     r15, r15                // restore borrow CF
    mov     r9,  [rsi]              // a0
    mov     rbx, [rsi+8]            // a1
    mov     rbp, [rsi+16]           // a2
    mov     rax, [rsi+24]           // a3
    sbb     r9,  r10
    sbb     rbx, r11
    sbb     rbp, r12
    sbb     rax, r13                // CF = borrow out
    sbb     r15, r15                // capture -borrow
    mov     [rdi],    r9
    mov     [rdi+8],  rbx
    mov     [rdi+16], rbp
    mov     [rdi+24], rax
    lea     rdi, [rdi+32]
    lea     rsi, [rsi+32]
    lea     rdx, [rdx+32]
    dec     rcx
    jnz     .Lloop

.Ldone:                             // ret = (numb[n-1]>>63) - borrow
    mov     rax, r14
    shr     rax, 63                 // numb[n-1] >> 63
    sub     rax, r15                // - borrow (r15 = 0 or -1)

#ifdef LMMP_WINDOWS
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
    pop     rdi
    pop     rsi
#else
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#endif
    ret
