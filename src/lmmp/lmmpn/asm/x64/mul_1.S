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
// mp_limb_t lmmp_mul_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x);
//
//   Windows: dst=rcx, numa=rdx, na=r8, x=r9
//   SysV:    dst=rdi, numa=rsi, na=rdx, x=rcx
//   ret rax = highest limb carry
//   cal: dst[i] = numa[i]*x, s_i = lo_i + hi_(i-1) + CF

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_mul_1_)

ASM_GSYM(lmmp_mul_1_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    push    rbx
    mov     rdi, rcx                // SysV:
    mov     rsi, rdx                //   rdi=dst, rsi=numa,
    mov     rdx, r9                 //   rdx=x (mulx implied)
#else
    push    rbx
    mov     r8, rdx                 // r8 = na
    mov     rdx, rcx                // x -> rdx
#endif

    lea     rsi, [rsi + r8*8]       // src end
    lea     rdi, [rdi + r8*8]       // dst end
    mov     r9, r8
    neg     r9                      // r9 = -na
    xor     r10d, r10d              // carry = 0

    mov     eax, r8d
    and     eax, 3                  // na mod 4
    jz      .Lcheck_post            // n mod 4 == 0
    cmp     eax, 2
    je      .Ldo_rem2
    jb      .Ldo_rem1               // na mod 4 == 1
    // na mod 4 == 3

.Ldo_rem3:
    mulx    r11, rax, [rsi + r9*8]       // p0 = a0*x
    mulx    r8,  rbx, [rsi + r9*8 + 8]   // p1 = a1*x
    add     rax, r10                     // s_0 = lo0 + carry
    mov     [rdi + r9*8], rax
    adc     rbx, r11                     // s_1 = lo1 + hi0 + CF
    mov     [rdi + r9*8 + 8], rbx
    adc     r8, 0                        // carry = hi1 + CF
    mov     r10, r8
    mulx    r11, rax, [rsi + r9*8 + 16]  // p2 = a2*x
    add     rax, r10                     // s_2 = lo2 + carry
    mov     [rdi + r9*8 + 16], rax
    adc     r11, 0
    mov     r10, r11
    add     r9, 3
    jmp     .Lcheck_post

.Ldo_rem2:                               // na mod 4 == 2
    mulx    r11, rax, [rsi + r9*8]       // p0 = a0*x
    mulx    r8,  rbx, [rsi + r9*8 + 8]   // p1 = a1*x
    add     rax, r10                     // s_0 = lo0 + carry
    mov     [rdi + r9*8], rax
    adc     rbx, r11                     // s_1 = lo1 + hi0 + CF
    mov     [rdi + r9*8 + 8], rbx
    adc     r8, 0
    mov     r10, r8
    add     r9, 2
    jmp     .Lcheck_post

.Ldo_rem1:                               // na mod 4 == 1
    mulx    r11, rax, [rsi + r9*8]       // p0 = a0*x
    add     rax, r10                     // s_0 = lo0 + carry
    mov     [rdi + r9*8], rax
    adc     r11, 0
    mov     r10, r11
    add     r9, 1
    // fall through

.Lcheck_post:
    test    r9, r9
    jz      .Ldone

.p2align 4
.Lloop4:                                    // main loop, 4 limbs
    mulx    r11, rax, [rsi + r9*8]          // p0 = a0*x
    mulx    r8,  rbx, [rsi + r9*8 + 8]      // p1 = a1*x
    add     rax, r10                        // s_0 = lo0 + carry
    mov     [rdi + r9*8], rax
    adc     rbx, r11                        // s_1 = lo1 + hi0 + CF
    mov     [rdi + r9*8 + 8], rbx
    mulx    r11, rax, [rsi + r9*8 + 16]     // p2 = a2*x
    adc     rax, r8                         // s_2 = lo2 + hi1 + CF
    mov     [rdi + r9*8 + 16], rax
    mulx    r8,  rbx, [rsi + r9*8 + 24]     // p3 = a3*x
    adc     rbx, r11                        // s_3 = lo3 + hi2 + CF
    mov     [rdi + r9*8 + 24], rbx
    adc     r8, 0
    mov     r10, r8                         // carry = hi3 + CF
    add     r9, 4
    jnz     .Lloop4

.Ldone:
    mov     rax, r10                        // ret carry
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret
