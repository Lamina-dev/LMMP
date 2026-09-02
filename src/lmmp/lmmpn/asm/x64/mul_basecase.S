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
// void lmmp_mul_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);
//
//   Windows: dst=rcx, numa=rdx, na=r8, numb=r9, nb=[rsp+0x48]
//   SysV:    dst=rdi, numa=rsi, na=rdx, numb=rcx, nb=r8
//   cal: dst[0..na+nb) = numa*numb, mul_1/mul_2 seed then addmul_2

#include "lmmp_asm.h"

.text

#ifdef LMMP_WINDOWS
    #define P_DST rcx
    #define P_NUMA rdx
    #define P_NA r8
    #define P_NUMB r9
    #define P_NB QWORD PTR [rsp+0x48]
#else
    #define P_DST rdi
    #define P_NUMA rsi
    #define P_NA rdx
    #define P_NUMB rcx
    #define P_NB r8
#endif

.macro PUSH_ALL
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
#endif
    push    rbx
    push    rbp
.endm

.macro POP_ALL
    pop     rbp
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
.endm

.macro PUSH_EXTRA
    push    r12
    push    r13
    push    r14
.endm

.macro POP_EXTRA
    pop     r14
    pop     r13
    pop     r12
.endm

.macro PUSH_FULL
    push    r15
    push    r8
.endm

.macro POP_FULL
    pop     rax
    pop     r15
.endm

// ====================== .code ======================

.text
.globl ASM_GSYM(lmmp_mul_basecase_)
.p2align 4

ASM_GSYM(lmmp_mul_basecase_):
    PUSH_ALL

#ifdef LMMP_WINDOWS
    mov     rdi, P_DST              // dst -> rdi
    mov     rsi, P_NUMA             // numa -> rsi
    mov     rdx, P_NA               // na -> rdx
    mov     rcx, P_NUMB             // numb -> rcx
    mov     r8, P_NB                // nb -> r8
#endif

    mov     rbx, rdx                // rbx = na
    neg     rbx                     // rbx = -na

    mov     rax, [rsi]              // rax = numa[0]
    lea     rsi, [rsi + rdx*8]      // rsi = numa + na
    lea     rdi, [rdi + rdx*8]      // rdi = dst + na

    mov     r9, [rcx]               // r9 = numb[0]
    mul     r9                      // rdx:rax = numa[0] * numb[0]

    test    r8b, 1                  // nb parity
    jz      .Ldo_mul_2

// ======================================
// nb odd: dst = numa * numb[0] (mul_1)
// ======================================
.Ldo_mul_1:
    test    bl, 1                   // na mod 4
    jnz     .Lm1x1

.Lm1x0:                             // na mod 4 in {0,2}
    mov     r10, rax
    mov     r11, rdx
    mov     rax, [rsi + rbx*8 + 8]
    test    bl, 2
    jnz     .Lm110

.Lm100:                             // na mod 4 == 0
    lea     rbp, [rbx + 2]
    jmp     .Lm1l0

.Lm110:                             // na mod 4 == 2
    lea     rbp, [rbx]
    jmp     .Lm1l2

.Lm1x1:                             // na mod 4 in {1,3}
    mov     r11, rax
    mov     r10, rdx
    test    bl, 2
    jz      .Lm111

.Lm101:                             // na mod 4 == 3
    lea     rbp, [rbx + 3]
    test    rbp, rbp
    js      .Lm1l1
    mov     [rdi - 8], rax          // na == 1, ret
    mov     [rdi], rdx
    POP_ALL
    ret

.Lm111:                             // na mod 4 == 1
    lea     rbp, [rbx + 1]
    mov     rax, [rsi + rbx*8 + 8]
    jmp     .Lm1l3

.p2align 4
.Lm1tp:                             // mul_1 loop, 4 limbs
    mov     r10, rdx
    add     r11, rax

.Lm1l1:
    mov     rax, [rsi + rbp*8 - 16]
    adc     r10, 0
    mul     r9
    add     r10, rax
    mov     [rdi + rbp*8 - 24], r11
    mov     rax, [rsi + rbp*8 - 8]
    mov     r11, rdx
    adc     r11, 0

.Lm1l0:
    mul     r9
    mov     [rdi + rbp*8 - 16], r10
    add     r11, rax
    mov     r10, rdx
    mov     rax, [rsi + rbp*8]
    adc     r10, 0

.Lm1l3:
    mul     r9
    mov     [rdi + rbp*8 - 8], r11
    mov     r11, rdx
    add     r10, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r11, 0

.Lm1l2:
    mul     r9
    mov     [rdi + rbp*8], r10
    add     rbp, 4
    jnc     .Lm1tp

.Lm1ed:                             // write dst[na-1], dst[na]
    add     r11, rax
    adc     rdx, 0
    mov     [rdi - 8], r11
    mov     [rdi], rdx

    dec     r8                      // nb--
    jz      .Lret2

    lea     rcx, [rcx + 8]          // numb++
    lea     rdi, [rdi + 8]          // dst++
    PUSH_EXTRA
    jmp     .Ldo_addmul

// ======================================
// nb even: dst = numa * numb[0..2) (mul_2)
// ======================================
.Ldo_mul_2:
    PUSH_EXTRA
    mov     r14, [rcx + 8]          // r14 = numb[1]

    test    bl, 1                   // na parity
    jnz     .Lm2b1

.Lm2b0:                             // na even
    lea     rbp, [rbx]
    xor     r10, r10
    mov     r12, rax
    mov     r11, rdx
    jmp     .Lm2l0

.Lm2b1:                             // na odd
    lea     rbp, [rbx + 1]
    xor     r11, r11
    xor     r12, r12
    mov     r10, rax
    mov     r13, rdx
    jmp     .Lm2l1

.p2align 4
.Lm2tp:                             // mul_2 loop, 2 limbs
    mul     r9
    add     r10, rax
    mov     r13, rdx
    adc     r13, 0

.Lm2l1:
    mov     rax, [rsi + rbp*8 - 8]
    mul     r14
    add     r10, r11
    adc     r13, 0
    add     r12, rax
    mov     [rdi + rbp*8 - 8], r10
    mov     r10, rdx
    adc     r10, 0
    mov     rax, [rsi + rbp*8]
    mul     r9
    add     r12, rax
    mov     r11, rdx
    adc     r11, 0
    add     r12, r13

.Lm2l0:
    mov     rax, [rsi + rbp*8]
    adc     r11, 0
    mul     r14
    mov     [rdi + rbp*8], r12
    add     r10, rax
    mov     r12, rdx
    mov     rax, [rsi + rbp*8 + 8]
    adc     r12, 0
    add     rbp, 2
    jnc     .Lm2tp

.Lm2ed:                             // write dst[na-1..na+1]
    mul     r9
    add     r10, rax
    mov     r13, rdx
    adc     r13, 0
    mov     rax, [rsi - 8]
    mul     r14
    add     r10, r11
    adc     r13, 0
    add     r12, rax
    mov     [rdi - 8], r10
    adc     rdx, 0
    add     r12, r13
    mov     [rdi], r12
    adc     rdx, 0
    mov     [rdi + 8], rdx

    sub     r8, 2                   // nb -= 2
    jz      .Lret5
    lea     rcx, [rcx + 16]         // numb += 2
    lea     rdi, [rdi + 16]         // dst += 2

// ======================================
// addmul_2 outer loop
// ======================================
.Ldo_addmul:
    PUSH_FULL

.Louter:                            // dst += numa * (numb[j], numb[j+1])
    mov     r9, [rcx]               // b0 = numb[j]
    mov     r8, [rcx + 8]           // b1 = numb[j+1]
    mov     rax, [rsi + rbx*8]
    mul     r9                      // seed numa[0]*b0
    test    bl, 1                   // na mod 4
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

.Lend:                              // write dst[na-1..na+1]
    mul     r8
    add     r15, r11
    adc     r13, 0
    add     rax, r12
    adc     rdx, 0
    mov     [rdi - 8], r15
    add     rax, r13
    adc     rdx, 0
    mov     [rdi], rax
    mov     [rdi + 8], rdx

    sub     QWORD PTR [rsp], 2      // pairs -= 2
    lea     rcx, [rcx + 16]         // numb += 2
    lea     rdi, [rdi + 16]         // dst += 2
    jnz     .Louter

    POP_FULL

.Lret5:
    POP_EXTRA

.Lret2:
    POP_ALL
    ret
