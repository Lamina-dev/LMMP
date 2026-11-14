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
// void lmmp_mullo_basecase_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
//   Windows: dst=rcx, numa=rdx, numb=r8, n=r9
//   SysV:    dst=rdi, numa=rsi, numb=rdx, n=rcx

#include "lmmp_asm.h"

.text

#ifdef LMMP_WINDOWS
    #define P_DST rcx
    #define P_NUMA rdx
    #define P_NUMB r8
    #define P_N r9
#else
    #define P_DST rdi
    #define P_NUMA rsi
    #define P_NUMB rdx
    #define P_N rcx
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
// lmmp_mullo_basecase_
// ======================================
.globl ASM_GSYM(lmmp_mullo_basecase_)
.p2align 4
ASM_GSYM(lmmp_mullo_basecase_):
    PUSH_ALL

#ifdef LMMP_WINDOWS
    mov     rdi, P_DST             // dst -> rdi
    mov     rsi, P_NUMA            // numa -> rsi
    mov     r8,  P_NUMB            // numb -> r8
    mov     r9,  P_N               // n -> r9
    mov     rcx, r8                // numb -> rcx
    mov     rdx, r9                // n -> rdx
    mov     r8,  r9                // n -> r8
#else
    mov     r8,  rcx               // n -> r8
    mov     rcx, rdx               // numb -> rcx
    mov     rdx, r8                // n -> rdx
#endif

    test    rdx, rdx
    jz      .Lret                  // n == 0

    mov     rbx, rdx
    neg     rbx                    // rbx = -n
    lea     rsi, [rsi + rdx*8]     // rsi = numa + n
    lea     rdi, [rdi + rdx*8]     // rdi = dst + n

    mov     r9, [rcx]              // r9 = numb[0]
    mov     rax, [rsi + rbx*8]     // rax = numa[0]
    mul     r9                     // rdx:rax = numa[0] * numb[0]

    test    bl, 1                  // n parity
    jnz    .Lmul1_odd

.Lmul1_even:                       // n even
    mov     r10, rax
    mov     r11, rdx
    mov     rax, [rsi + rbx*8 + 8]
    test    bl, 2
    jnz     .Lmul1_even_2mod4

    lea     rbp, [rbx + 2]         // n mod 4 == 0
    jmp     .Lmul1_l0

.Lmul1_even_2mod4:                 // n mod 4 == 2
    lea     rbp, [rbx]
    jmp     .Lmul1_l2

.Lmul1_odd:                        // n odd
    mov     r11, rax
    mov     r10, rdx
    test    bl, 2
    jz      .Lmul1_odd_3mod4

    lea     rbp, [rbx + 3]         // n mod 4 == 1
    test    rbp, rbp
    js      .Lmul1_l1
    mov     [rdi - 8], rax         // n == 1, ret
    POP_ALL
    ret

.Lmul1_odd_3mod4:                  // n mod 4 == 3
    lea     rbp, [rbx + 1]
    mov     rax, [rsi + rbx*8 + 8]
    jmp     .Lmul1_l3

.p2align 4
.Lmul1_tp:                         // mul_1 loop, 4 limbs
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
    mov     [rdi - 8], r11         // dst[n-1]
    // carry discarded

    dec     r8                     // r8 = n - 1
    jz      .Lret

    lea     rcx, [rcx + 8]         // numb++
    inc     rbx                    // rbx = -(n-1)
    add     rsi, 8                 // rsi = numa + n + 1 (row anchor)

    PUSH_EXTRA
    push    r15

    mov     r15, rbx               // r15 = -(n-1)
    mov     r14, rcx               // r14 = numb

// two-column outer loop, p7 carry-save rows
// r15 = -L, r14 = numb, rsi = numa+L+2 anchor, rdi = dst+n
.p2align 4
.Louter2:
    cmp     r15, -4
    jg      .Ltailconv

    mov     r9, [r14]                // v0 = numb[j]
    mov     r8, [r14 + 8]            // v1 = numb[j+1]
    lea     r14, [r14 + 16]
    lea     rsi, [rsi + r15*8 - 16]  // up = [numa,L]
    lea     rdi, [rdi + r15*8]       // rp = [dst+j,n-j]
    mov     rax, r15
    neg     rax                      // rax = L
    mov     r11, rax
    shr     r11, 2                   // 4-limb groups
    test    al, 3
    jz      .Lh0
    test    al, 1
    jz      .Lh2
    test    al, 2
    jz      .Lh1
    jmp     .Lh3

.Lh0:                              // L%4 == 0
    mov     rdx, r9
    xor     r10d, r10d
    xor     r12d, r12d
    jmp     .Lk4

.Lh1:                              // L%4 == 1
    mov     rdx, r8
    mulx    r12, rbx, [rsi]
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, rbx
    adc     r12, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Larow
    jmp     .Lk4

.Lh2:                              // L%4 == 2
    mov     rdx, r9
    mulx    rcx, rax, [rsi]
    mov     rdx, r8
    mulx    rbp, rbx, [rsi]
    add     rax, [rdi]
    adc     rcx, rbx
    adc     rbp, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mulx    r12, rbx, [rsi]
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, rbx
    adc     r12, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r12, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Larow
    jmp     .Lk4

.Lh3:                              // L%4 == 3
    mov     rdx, r8
    mulx    r12, rbx, [rsi]
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, rbx
    adc     r12, 0
    mov     [rdi], rax
    mulx    rcx, rax, [rsi + 8]
    mov     rdx, r8
    mulx    rbp, rbx, [rsi + 8]
    add     rax, [rdi + 8]
    adc     rcx, rbx
    adc     rbp, 0
    add     rax, r10
    adc     rcx, r12
    adc     rbp, 0
    mov     [rdi + 8], rax
    mulx    r12, rbx, [rsi + 16]
    mov     rdx, r9
    mulx    r10, rax, [rsi + 16]
    add     rax, [rdi + 16]
    adc     r10, rbx
    adc     r12, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r12, 0
    mov     [rdi + 16], rax
    lea     rsi, [rsi + 24]
    lea     rdi, [rdi + 24]
    test    r11, r11
    jz      .Larow
    jmp     .Lk4

.p2align 4
.Lk4:                              // inner loop, 4 limbs, pair rotates (kernel = addmul_2.S)
    mulx    rcx, rax, [rsi]        // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi]
    add     rax, [rdi]             // t = lo0 + rp
    adc     rcx, rbx               // h = hi0 + l1 + c1
    adc     rbp, 0                 // h2 = hi1 + c2
    add     rax, r10               // s = t + cy0
    adc     rcx, r12               // cy0 = h + cy1 + c4
    adc     rbp, 0                 // cy1 = h2 + c5
    mov     [rdi], rax

    mulx    r12, rbx, [rsi + 8]    // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 8]
    add     rax, [rdi + 8]
    adc     r10, rbx
    adc     r12, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r12, 0
    mov     [rdi + 8], rax

    mulx    rcx, rax, [rsi + 16]   // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi + 16]
    add     rax, [rdi + 16]
    adc     rcx, rbx
    adc     rbp, 0
    add     rax, r10
    adc     rcx, r12
    adc     rbp, 0
    mov     [rdi + 16], rax

    mulx    r12, rbx, [rsi + 24]   // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 24]
    add     rax, [rdi + 24]
    adc     r10, rbx
    adc     r12, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r12, 0
    mov     [rdi + 24], rax

    lea     rsi, [rsi + 32]
    lea     rdi, [rdi + 32]
    dec     r11
    jnz     .Lk4

.Larow:                            // carry discarded (truncated product)
    add     r15, 2                 // L -= 2
    jmp     .Louter2

// tail: single-multiplier columns
.Ltailconv:
    sub     rsi, 16                // rsi = numa + L
    mov     rbx, r15               // rbx = -L
    mov     rcx, r14               // rcx = numb
    neg     r15
    mov     r8, r15                // r8 = L remaining cols
    pop     r15

.Louter:                           // dst += numa * numb[j] (low part)
    mov     r9, [rcx]              // r9 = numb[j]
    mov     rax, [rsi + rbx*8]     // rax = numa[0]
    mul     r9

    test    bl, 1
    jnz     .La1_odd

.La1_even:
    mov     r10, rax
    mov     r11, rdx
    mov     rax, [rsi + rbx*8 + 8]
    test    bl, 2
    jnz     .La1_even_2mod4

    lea     rbp, [rbx + 2]         // len mod 4 == 0
    jmp     .La1_l0

.La1_even_2mod4:                   // len mod 4 == 2
    lea     rbp, [rbx]
    jmp     .La1_l2

.La1_odd:
    mov     r11, rax
    mov     r10, rdx
    test    bl, 2
    jz      .La1_odd_3mod4

.La1_odd_1mod4:                    // len mod 4 == 1
    lea     rbp, [rbx + 3]
    test    rbp, rbp
    js      .La1_l1
    add     r11, [rdi - 8]         // dst[n-1] += old
    adc     r10, 0
    mov     [rdi - 8], r11
    jmp     .Lnext

.La1_odd_3mod4:                    // len mod 4 == 3
    lea     rbp, [rbx + 1]
    mov     rax, [rsi + rbx*8 + 8]
    jmp     .La1_l3

.p2align 4
.La1_tp:                           // inner loop, 4 limbs
    mov     r10, rdx
    add     r11, rax
.La1_l1:
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
.La1_l0:
    mul     r9
    add     r10, [rdi + rbp*8 - 16]
    adc     r11, 0
    mov     [rdi + rbp*8 - 16], r10
    add     r11, rax
    mov     r10, rdx
    mov     rax, [rsi + rbp*8]
    adc     r10, 0
.La1_l3:
    mul     r9
    add     r11, [rdi + rbp*8 - 8]
    adc     r10, 0
    mov     [rdi + rbp*8 - 8], r11
    mov     r11, rdx
    add     r10, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r11, 0
.La1_l2:
    mul     r9
    add     r10, [rdi + rbp*8]
    adc     r11, 0
    mov     [rdi + rbp*8], r10
    add     rbp, 4
    jnc     .La1_tp

.La1_ed:
    add     r11, rax
    adc     rdx, 0
    add     r11, [rdi - 8]         // dst[n-1] += old
    adc     rdx, 0
    mov     [rdi - 8], r11
    // carry discarded

.Lnext:
    dec     r8
    jz      .Ldone
    lea     rcx, [rcx + 8]         // numb++
    inc     rbx                    // len--
    sub     rsi, 8
    jmp     .Louter

.Ldone:
    POP_EXTRA

.Lret:
    POP_ALL
    ret
