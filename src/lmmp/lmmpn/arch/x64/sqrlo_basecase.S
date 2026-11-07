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
// void lmmp_sqrlo_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t n);
//
//   Windows: dst=rcx, numa=rdx, n=r8
//   SysV:    dst=rdi, numa=rsi, n=rdx

#include "lmmp_asm.h"

.text

#ifdef LMMP_WINDOWS
    #define P_DST rcx
    #define P_NUMA rdx
    #define P_N r8
#else
    #define P_DST rdi
    #define P_NUMA rsi
    #define P_N rdx
#endif

.macro PUSH_ALL
#ifdef LMMP_WINDOWS
    push rsi
    push rdi
#endif
    push rbx
    push rbp
.endm

.macro POP_ALL
    pop rbp
    pop rbx
#ifdef LMMP_WINDOWS
    pop rdi
    pop rsi
#endif
.endm

.macro PUSH_EXTRA
    push r12
    push r13
    push r14
.endm

.macro POP_EXTRA
    pop r14
    pop r13
    pop r12
.endm


.text

// ======================================
// lmmp_sqrlo_basecase_
// ======================================
.globl ASM_GSYM(lmmp_sqrlo_basecase_)
.p2align 4
ASM_GSYM(lmmp_sqrlo_basecase_):
    test    P_N, P_N
    jz      .Lret                 // n == 0
    cmp     P_N, 1
    jne     .Lbig
    mov     rax, [P_NUMA]         // n == 1: dst[0] = lo(a0^2)
    mul     rax
    mov     [P_DST], rax
.Lret:
    ret

.Lbig:
    PUSH_ALL
    PUSH_EXTRA

#ifdef LMMP_WINDOWS
    mov     rdi, P_DST            // dst -> rdi
    mov     rsi, P_NUMA           // numa -> rsi
    mov     rdx, P_N              // n -> rdx
#endif

    mov     r12, rsi              // r12 = numa
    mov     r13, rdi              // r13 = dst
    mov     r14, rdx              // r14 = n
    lea     rsi, [rsi + rdx*8]    // rsi = numa + n
    lea     rdi, [rdi + rdx*8]    // rdi = dst + n
    lea     rbx, [rdx - 1]
    neg     rbx                   // rbx = 1-n

    mov     r9, [rsi + rbx*8 - 8] // r9 = a_0
    mov     rax, [rsi + rbx*8]    // rax = a_1
    mul     r9                    // rdx:rax = a_0 * a_1

    test    bl, 1
    jnz     .Lmul1_odd

.Lmul1_even:                      // base even
    mov     r10, rax
    mov     r11, rdx
    mov     rax, [rsi + rbx*8 + 8]
    test    bl, 2
    jnz     .Lmul1_even_2mod4

    lea     rbp, [rbx + 2]        // rbx = 0 (mod 4)
    jmp     .Lmul1_l0

.Lmul1_even_2mod4:                // rbx = 2 (mod 4)
    lea     rbp, [rbx]
    jmp     .Lmul1_l2

.Lmul1_odd:                       // base odd
    mov     r11, rax
    mov     r10, rdx
    test    bl, 2
    jz      .Lmul1_odd_3mod4

    lea     rbp, [rbx + 3]        // rbx = 1 (mod 4)
    test    rbp, rbp
    js      .Lmul1_l1
    mov     [rdi - 8], rax        // w=1 (n == 2): dst[1] = lo(a0*a1)
    jmp     .Lrow

.Lmul1_odd_3mod4:                 // rbx = 3 (mod 4)
    lea     rbp, [rbx + 1]
    mov     rax, [rsi + rbx*8 + 8]
    jmp     .Lmul1_l3

.p2align 4
.Lmul1_tp:                        // mul_1 main loop, 4 limbs
    mov     r10, rdx
    add     r11, rax
.Lmul1_l1:
    mov     rax, [rsi + rbp*8 - 16]
    adc     r10, 0
    mul     r9
    add     r10, rax
    mov     [rdi + rbp*8 - 24], r11
    mov     rax, [rsi + rbp*8 - 8]
    mov     r11, rdx
    adc     r11, 0
.Lmul1_l0:
    mul     r9
    mov     [rdi + rbp*8 - 16], r10
    add     r11, rax
    mov     r10, rdx
    mov     rax, [rsi + rbp*8]
    adc     r10, 0
.Lmul1_l3:
    mul     r9
    mov     [rdi + rbp*8 - 8], r11
    mov     r11, rdx
    add     r10, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r11, 0
.Lmul1_l2:
    mul     r9
    mov     [rdi + rbp*8], r10
    add     rbp, 4
    jnc     .Lmul1_tp

.Lmul1_ed:
    add     r11, rax
    adc     rdx, 0
    mov     [rdi - 8], r11        // dst[n-1]
    // carry discarded

// row loop: window [2j+1-n, -1], rdi = dst+n, rsi = numa+n-8j
.p2align 4
.Lrow:
    add     rbx, 2                // next base = 2j+1-n
    jns     .Ldiag                // base >= 0: rows done
    sub     rsi, 8
    mov     r9, [rsi + rbx*8 - 8] // r9 = a_j
    mov     rax, [rsi + rbx*8]    // rax = a_{j+1}
    mul     r9

    test    bl, 1
    jnz     .Lrow_odd

.Lrow_even:
    mov     r10, rax
    mov     r11, rdx
    mov     rax, [rsi + rbx*8 + 8]
    test    bl, 2
    jnz     .Lrow_even_2mod4

    lea     rbp, [rbx + 2]        // rbx = 0 (mod 4)
    jmp     .Lam_l0

.Lrow_even_2mod4:                 // rbx = 2 (mod 4)
    lea     rbp, [rbx]
    jmp     .Lam_l2

.Lrow_odd:
    mov     r11, rax
    mov     r10, rdx
    test    bl, 2
    jz      .Lrow_odd_3mod4

.Lrow_odd_1mod4:                  // rbx = 1 (mod 4)
    lea     rbp, [rbx + 3]
    test    rbp, rbp
    js      .Lam_l1
    add     r11, [rdi - 8]        // w=1: dst[n-1] += lo
    adc     r10, 0
    mov     [rdi - 8], r11
    // hi discarded
    jmp     .Lrow

.Lrow_odd_3mod4:                  // rbx = 3 (mod 4)
    lea     rbp, [rbx + 1]
    mov     rax, [rsi + rbx*8 + 8]
    jmp     .Lam_l3

.p2align 4
.Lam_tp:                          // addmul_1 main loop, 4 limbs
    mov     r10, rdx
    add     r11, rax
.Lam_l1:
    mov     rax, [rsi + rbp*8 - 16]
    adc     r10, 0
    mul     r9
    add     r11, [rdi + rbp*8 - 24]
    adc     r10, 0
    mov     [rdi + rbp*8 - 24], r11
    add     r10, rax
    mov     rax, [rsi + rbp*8 - 8]
    mov     r11, rdx
    adc     r11, 0
.Lam_l0:
    mul     r9
    add     r10, [rdi + rbp*8 - 16]
    adc     r11, 0
    mov     [rdi + rbp*8 - 16], r10
    add     r11, rax
    mov     r10, rdx
    mov     rax, [rsi + rbp*8]
    adc     r10, 0
.Lam_l3:
    mul     r9
    add     r11, [rdi + rbp*8 - 8]
    adc     r10, 0
    mov     [rdi + rbp*8 - 8], r11
    mov     r11, rdx
    add     r10, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r11, 0
.Lam_l2:
    mul     r9
    add     r10, [rdi + rbp*8]
    adc     r11, 0
    mov     [rdi + rbp*8], r10
    add     rbp, 4
    jnc     .Lam_tp

.Lam_ed:
    add     r11, rax
    adc     rdx, 0
    add     r11, [rdi - 8]        // dst[n-1] += s
    adc     rdx, 0
    mov     [rdi - 8], r11
    // carry discarded
    jmp     .Lrow

// diag: dst = 2*dst + sum a_i^2*B^2i, CF chain live, r10 = pending
.Ldiag:
    mov     rax, [r12]
    mul     rax                   // rdx:rax = a_0^2
    mov     [r13], rax            // dst[0] = lo(a_0^2)
    mov     r10, rdx              // pending = hi(a_0^2)
    lea     rdi, [r13 + 8]        // &dst[1]
    lea     rsi, [r12 + 8]        // &a[1]
    lea     rcx, [r14 - 1]
    shr     rcx, 1                // pairs = (n-1)/2
    mov     r11, r14
    and     r11, 1                // 0 = n even
    xor     ebx, ebx              // c = 0
    clc
    jecxz   .Ldiag_tail

.p2align 4
.Ldiag_pair:
    mov     r8, [rdi]             // odd col
    mov     r9, [rdi + 8]         // even col
    mov     rax, [rsi]
    adc     r8, r8
    adc     r9, r9
    setc    bl                    // save CF
    mul     rax                   // rdx:rax = a_i^2
    add     r8, r10               // odd += pending
    adc     r9, rax               // even += lo(a_i^2)
    mov     [rdi], r8
    mov     [rdi + 8], r9
    lea     r10, [rdx + rbx]      // pending = hi(a_i^2) + c
    lea     rdi, [rdi + 16]
    lea     rsi, [rsi + 8]
    dec     ecx
    jnz     .Ldiag_pair

.Ldiag_tail:
    dec     r11                   // dec keeps CF; 0 (n even) wraps to tail
    jz      .Ldone                // 1 (n odd): no lone tail col
    mov     r8, [rdi]
    adc     r8, r8                // 2d + CF
    add     r8, r10               // + pending, drop out-of-range carry
    mov     [rdi], r8

.Ldone:
    POP_EXTRA
    POP_ALL
    ret
