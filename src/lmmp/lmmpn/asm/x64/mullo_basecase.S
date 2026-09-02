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
// void lmmp_mullo_basecase_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
//   Windows: dst=rcx, numa=rdx, numb=r8, n=r9
//   SysV:    dst=rdi, numa=rsi, numb=rdx, n=rcx
//   cal: dst[0..n) = lo n limbs of numa*numb

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

// ====================== .text ======================

.text
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
    sub     rsi, 8                 // rsi = numa + (n-1)

    PUSH_EXTRA
    push    r15
    push    r8                     // remaining cols

// ======================================
// two-column outer loop
// ======================================
.p2align 4
.Louter2:
    cmp     rbx, -4
    jg      .Ltail                 // L < 4

    mov     r9, [rcx]              // m0 = numb[j]
    mov     r8, [rcx + 8]          // m1 = numb[j+1]
    mov     rax, [rsi + rbx*8]     // rax = numa[0]
    mul     r9
    test    bl, 1
    jnz     .a2x1

.a2x0:
    mov     r14, [rdi + rbx*8]     // dst[j]
    xor     r10, r10
    mov     r11, rdx
    test    bl, 2
    jnz     .a210

.a200:
    lea     rbp, [rbx + 2]
    add     r14, rax
    adc     r11, 0
    mov     rax, [rsi + rbx*8]
    mul     r8
    mov     r15, [rdi + rbx*8 + 8]
    jmp     .a2lo0

.a210:
    lea     rbp, [rbx]
    xor     r13, r13
    jmp     .a2lo2

.a2x1:
    mov     r15, [rdi + rbx*8]
    xor     r12, r12
    xor     r11, r11
    test    bl, 2
    jz      .a211

.a201:
    lea     rbp, [rbx + 3]
    mov     r13, rdx
    add     r15, rax
    mov     rax, [rsi + rbx*8]
    adc     r13, 0
    jmp     .a2top

.a211:
    lea     rbp, [rbx + 1]
    jmp     .a2lo3

.p2align 4
.a2top:                            // inner loop, 4 limbs
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
.a2lo0:
    mov     r12, rdx
    mov     [rdi + rbp*8 - 16], r14
    add     r15, rax
    adc     r12, 0
    mov     rax, [rsi + rbp*8 - 8]
    add     r15, r10
    adc     r12, 0
    mul     r9
.a2lo3:
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
.a2lo2:
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
    jnc     .a2top

.a2end:
    mul     r8                     // dummy mul
    add     r15, r11
    adc     r13, 0
    add     rax, r12
    adc     rdx, 0
    mov     [rdi - 8], r15         // dst[n-1]
    add     rax, r13
    adc     rdx, 0
    // carry discarded

    sub     QWORD PTR [rsp], 2     // cols -= 2
    lea     rcx, [rcx + 16]        // numb += 2
    lea     rsi, [rsi - 16]        // numa -= 2, L -= 2
    add     rbx, 2                 // rbx = -(L-2)
    jmp     .Louter2

// ======================================
// tail: single-multiplier columns
// ======================================
.Ltail:
    pop     r8                     // remaining cols
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
