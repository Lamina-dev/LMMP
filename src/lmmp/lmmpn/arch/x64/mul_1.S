//  Copyright (C) 2026 HJimmyK (Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it
//  under the terms of the GNU Lesser General Public License as
//  published by the Free Software Foundation; either version 3 of
//  the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, see
//  <https://www.gnu.org/licenses/>.
//
//  SPDX-License-Identifier: LGPL-3.0-or-later

//
// mp_limb_t lmmp_mul_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x);
//
//   Windows: dst=rcx, numa=rdx, na=r8, x=r9
//   SysV:    dst=rdi, numa=rsi, na=rdx, x=rcx
//
// Loop structure transcribed from GMP 6.3 mpn/x86_64/coreihwl/mul_1.asm
// (loop authored by David Harvey and Torbjorn Granlund).

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_mul_1_
// ======================================
.globl ASM_GSYM(lmmp_mul_1_)
.p2align 4
ASM_GSYM(lmmp_mul_1_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    push    r12
    mov     rdi, rcx                // SysV:
    mov     rsi, rdx                //   rdi=dst, rsi=numa,
    mov     rdx, r9                 //   rdx=x (mulx implied)
#else
    push    rbx
    push    rbp
    push    r12
    mov     r8, rdx                 // r8 = na
    mov     rdx, rcx                // x -> rdx
#endif
    test    r8, r8
    jz      .Lret0

    mov     rbp, r8
    shr     rbp, 2                  // groups = na/4

    test    r8d, 1
    jnz     .Lbx1

.Lbx0:
    test    r8d, 2                  // CF = 0
    jnz     .Lb10

.Lb00:                              // na%4 == 0
    mulx    r8,  r9,  [rsi]         // p0: r9=lo0,  r8=hi0
    mulx    r10, r11, [rsi+8]       // p1: r11=lo1, r10=hi1
    mulx    r12, rcx, [rsi+16]      // p2: rcx=lo2, r12=hi2
    lea     rdi, [rdi-32]
    jmp     .Llo0

.Lb10:                              // na%4 == 2
    mulx    r12, rcx, [rsi]         // p0: rcx=lo0, r12=hi0
    mulx    rax, rbx, [rsi+8]       // p1: rbx=lo1,  rax=hi1
    lea     rdi, [rdi-16]
    test    rbp, rbp                // CF = 0
    jz      .Lcj2
    mulx    r8,  r9,  [rsi+16]      // group p0
    lea     rsi, [rsi+16]
    jmp     .Llo2

.Lbx1:
    test    r8d, 2
    jnz     .Lb11

.Lb01:                              // na%4 == 1
    mulx    rax, rbx, [rsi]         // p0: rbx=lo0, rax=hi0
    lea     rdi, [rdi-24]
    test    rbp, rbp                // CF = 0
    jz      .Lcj1
    mulx    r8,  r9,  [rsi+8]       // group p0
    lea     rsi, [rsi+8]
    jmp     .Llo1

.Lb11:                              // na%4 == 3
    mulx    r10, r11, [rsi]         // p0: r11=lo0, r10=hi0
    mulx    r12, rcx, [rsi+8]       // p1: rcx=lo1, r12=hi1
    mulx    rax, rbx, [rsi+16]      // p2: rbx=lo2, rax=hi2
    lea     rdi, [rdi-8]
    test    rbp, rbp                // CF = 0
    jz      .Lcj3
    lea     rsi, [rsi+24]
    jmp     .Llo3

.p2align 5
.Ltop:
    lea     rdi, [rdi+32]
    mov     [rdi], r9               // s0 (link at .Llo1)
    adc     r11, r8                 // s1 = lo1 + hi0 + CF
.Llo3:
    mulx    r8,  r9,  [rsi]         // next p0
    mov     [rdi+8], r11            // s1
    adc     rcx, r10                // s2 = lo2 + hi1 + CF
.Llo2:
    mov     [rdi+16], rcx           // s2
    adc     rbx, r12                // s3 = lo3 + hi2 + CF
.Llo1:
    mulx    r10, r11, [rsi+8]       // next p1
    adc     r9, rax                 // s0' = lo0' + hi3 + CF
    mulx    r12, rcx, [rsi+16]      // next p2
    mov     [rdi+24], rbx           // s3
.Llo0:
    mulx    rax, rbx, [rsi+24]      // next p3
    lea     rsi, [rsi+32]
    dec     rbp                     // CF preserved
    jnz     .Ltop

.Lend:
    lea     rdi, [rdi+32]
    mov     [rdi], r9               // s0
    adc     r11, r8                 // s1
.Lcj3:
    mov     [rdi+8], r11            // s1
    adc     rcx, r10                // s2
.Lcj2:
    mov     [rdi+16], rcx           // s2
    adc     rbx, r12                // s3
.Lcj1:
    mov     [rdi+24], rbx           // s3
    adc     rax, 0                  // carry = hi3 + CF

    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

.Lret0:
    xor     eax, eax
    pop     r12
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret
