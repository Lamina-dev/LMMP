//  Copyright (C) 2026 HJimmyK (Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it
//  under the terms of the GNU Lesser General Public License as
//  published by the Free Software Foundation, either version 3 of
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
// mp_limb_t lmmp_addmul_2_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_srcptr bp);
//
//   Windows: numa=rcx, numb=rdx, n=r8, bp=r9
//   SysV:    numa=rdi, numb=rsi, n=rdx, bp=rcx

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_addmul_2_
// ======================================
.globl ASM_GSYM(lmmp_addmul_2_)
.p2align 4
ASM_GSYM(lmmp_addmul_2_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
#endif
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15

#ifdef LMMP_WINDOWS
    mov     rdi, rcx                // numa -> rdi
    mov     rsi, rdx                // numb -> rsi
    mov     rdx, r8                 // n -> rdx
    mov     rcx, r9                 // bp -> rcx
#endif

    test    rdx, rdx                // n == 0: return 0 (rdx = 0)
    jz      .Lret

    mov     r9, [rcx]               // b0 = bp[0]
    mov     r8, [rcx + 8]           // b1 = bp[1]

    mov     rbx, rdx                // rbx = -n
    neg     rbx
    lea     rsi, [rsi + rdx*8]      // rsi = numb + n
    lea     rdi, [rdi + rdx*8]      // rdi = numa + n

    cmp     rdx, 1
    jne     .Lgen

// n == 1: [numa,2] += [numb,1] * (b0 + b1*B), return limb 2
    mov     r11, [rsi - 8]          // s = numb[0] (cached: eqsep-safe)
    mov     rax, r11
    mul     r9
    add     rax, [rdi - 8]
    adc     rdx, 0
    mov     [rdi - 8], rax
    mov     r10, rdx                // r10 = limb 1 partial
    mov     rax, r11
    mul     r8
    add     rax, r10
    adc     rdx, 0
    mov     [rdi], rax              // numa[1] = limb 1
    jmp     .Lret                   // return rdx = limb 2

.Lgen:                              // n >= 2: numa += numb * (b0, b1)
    mov     rax, [rsi + rbx*8]
    mul     r9                      // seed numb[0]*b0
    test    bl, 1                   // n mod 4
    jnz     .La1x1

.La1x0:
    mov     r14, [rdi + rbx*8]
    xor     r10, r10
    mov     r11, rdx
    test    bl, 2
    jnz     .La110

.La100:
    lea     rbp, [rbx + 2]
    add     r14, rax
    adc     r11, 0
    mov     rax, [rsi + rbx*8]
    mul     r8
    mov     r15, [rdi + rbx*8 + 8]
    jmp     .Llo0

.La110:
    lea     rbp, [rbx]
    xor     r13, r13
    jmp     .Llo2

.La1x1:
    mov     r15, [rdi + rbx*8]
    xor     r12, r12
    xor     r11, r11
    test    bl, 2
    jz      .La111

.La101:
    lea     rbp, [rbx + 3]
    mov     r13, rdx
    add     r15, rax
    mov     rax, [rsi + rbx*8]
    adc     r13, 0
    jmp     .Ltop

.La111:
    lea     rbp, [rbx + 1]
    jmp     .Llo3

.p2align 4
.Ltop:                              // inner loop, 4 limbs
    mov     r14, [rdi + rbp*8 - 16]
    mul     r8
    mov     r10, rdx
    add     r14, rax
    adc     r10, 0
    add     r15, r11
    adc     r13, 0
    add     r14, r12
    adc     r10, 0
    mov     rax, [rsi + rbp*8 - 16]
    mul     r9
    add     r14, rax
    mov     r11, rdx
    adc     r11, 0
    mov     rax, [rsi + rbp*8 - 16]
    mul     r8
    mov     [rdi + rbp*8 - 24], r15
    mov     r15, [rdi + rbp*8 - 8]
    add     r14, r13
    adc     r11, 0

.Llo0:
    mov     r12, rdx
    mov     [rdi + rbp*8 - 16], r14
    add     r15, rax
    adc     r12, 0
    mov     rax, [rsi + rbp*8 - 8]
    add     r15, r10
    adc     r12, 0
    mul     r9

.Llo3:
    add     r15, rax
    mov     r13, rdx
    adc     r13, 0
    mov     rax, [rsi + rbp*8 - 8]
    mul     r8
    add     r15, r11
    mov     r14, [rdi + rbp*8]
    adc     r13, 0
    mov     r10, rdx
    add     r14, rax
    adc     r10, 0
    mov     rax, [rsi + rbp*8]
    mul     r9
    add     r14, r12
    mov     [rdi + rbp*8 - 8], r15
    mov     r11, rdx
    adc     r10, 0

.Llo2:
    add     r14, rax
    adc     r11, 0
    mov     rax, [rsi + rbp*8]
    add     r14, r13
    adc     r11, 0
    mul     r8
    mov     r15, [rdi + rbp*8 + 8]
    add     r15, rax
    mov     r12, rdx
    adc     r12, 0
    mov     rax, [rsi + rbp*8 + 8]
    mov     [rdi + rbp*8], r14
    mul     r9
    add     r15, r10
    mov     r13, rdx
    adc     r12, 0
    add     r15, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r13, 0
    add     rbp, 4
    jnc     .Ltop

.Lend:                              // write [numa+n-1, 2]
    mul     r8
    add     r15, r11
    adc     r13, 0
    add     rax, r12
    adc     rdx, 0
    mov     [rdi - 8], r15
    add     rax, r13
    adc     rdx, 0
    mov     [rdi], rax              // numa[n] = limb n
                                    // rdx = limb n+1

.Lret:
    mov     rax, rdx
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
