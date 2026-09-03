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
// void lmmp_sqr_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
//
//   Windows: dst=rcx, numa=rdx, na=r8
//   SysV:    dst=rdi, numa=rsi, na=rdx
//   cal: dst = sum a_i^2*B^2i + 2*sum_{i<j} a_i*a_j*B^(i+j)

#include "lmmp_asm.h"

.text

#ifdef LMMP_WINDOWS
    #define PUSH_RSI push rsi
    #define PUSH_RDI push rdi
    #define POP_RSI  pop rsi
    #define POP_RDI  pop rdi
#else
    #define PUSH_RSI
    #define PUSH_RDI
    #define POP_RSI
    #define POP_RDI
#endif

// ====================== .code ======================
.text

.globl ASM_GSYM(lmmp_sqr_basecase_)
.p2align 4

ASM_GSYM(lmmp_sqr_basecase_):
    cmp     rx2, 2
    jae     .Lgt1

    // na == 1: dst = a0^2
    mov     rax, [rx1]
    mul     rax
    mov     [rx0], rax
    mov     [rx0+8], rdx
    ret

.Lgt1:
    // na == 2: dst = a0^2 | 2*a0*a1 | a1^2
    jne     .Lgt2
    mov     rax, [rx1]
    mov     r11, [rx1+8]
    mov     r8, rax
    mul     rax
    mov     [rx0], rax

    mov     rax, r11
    mov     r9, rdx
    mul     rax
    mov     r10, rax

    mov     rax, r11
    mov     r11, rdx
    mul     r8

    xor     r8, r8
    add     r9, rax
    adc     r10, rdx
    adc     r11, r8

    add     r9, rax
    mov     [rx0+8], r9
    adc     r10, rdx
    mov     [rx0+16], r10
    adc     r11, r8
    mov     [rx0+24], r11
    ret

.Lgt2:
    PUSH_RSI
    PUSH_RDI

    cmp     rx2, 4
    mov     rsi, rx1               // rsi = numa
    mov     rdi, rx0               // rdi = dst
    mov     rdx, rx2               // rdx = na

    jae     .Lgt3

    // na == 3: diagonals then cross terms * 2
    mov     rax, [rsi]
    mov     r10, rax
    mul     rax
    mov     r11, [rsi+8]
    mov     [rdi], rax

    mov     rax, r11
    mov     [rdi+8], rdx
    mul     rax
    mov     rcx, [rsi+16]
    mov     [rdi+16], rax

    mov     rax, rcx
    mov     [rdi+24], rdx
    mul     rax
    mov     [rdi+32], rax
    mov     [rdi+40], rdx

    mov     rax, r11
    mul     r10
    mov     r8, rax
    mov     rax, rcx
    mov     r9, rdx
    mul     r10

    xor     r10, r10
    add     r9, rax
    mov     rax, r11
    mov     r11, r10
    adc     r10, rdx
    mul     rcx

    add     r10, rax
    adc     rdx, r11
    add     r8, r8
    adc     r9, r9
    adc     r10, r10
    adc     rdx, rdx
    adc     r11, r11

    add     [rdi+8], r8
    adc     [rdi+16], r9
    adc     [rdi+24], r10
    adc     [rdi+32], rdx
    adc     [rdi+40], r11

    POP_RDI
    POP_RSI
    ret

.Lgt3:
.Ldo_mul_2:                        // seed mul_2: dst = numa * (a0, a1)
    mov     r8, [rsi]              // r8 = a0
    push    rbx
    lea     rdi, [rdi+rdx*8]       // rdi = dst + na
    mov     rax, [rsi+8]
    push    rbp
    lea     rsi, [rsi+rdx*8]       // rsi = numa + na
    mov     r9, rax                // r9 = a1
    push    r12
    mov     r12, 1
    push    r13
    sub     r12, rdx               // r12 = 1-na
    push    r14
    push    r12                    // [rsp] = 1-na

    mul     r8                     // seed a1*a0
    mov     [rdi+r12*8], rax
    mov     rax, [rsi+r12*8+8]
    test    r12b, 1                // (na-1) mod 2
    jnz     .Lm2b1

.Lm2b0:                            // even entry
    lea     rcx, [r12+2]
    xor     r11, r11
    xor     rbx, rbx
    mov     r10, rdx
    jmp     .Lm2l0

.Lm2b1:                            // odd entry
    lea     rcx, [r12+1]
    xor     rbp, rbp
    xor     r10, r10
    mov     rbx, rdx
    jmp     .Lm2l1

.p2align 4
.Lm2tp:                            // mul_2 loop, 2 limbs
.Lm2l0:
    mul     r8
    add     r10, rax
    mov     rbp, rdx
    adc     rbp, 0
    mov     rax, [rsi+rcx*8-8]
    mul     r9
    add     r10, r11
    adc     rbp, 0
    add     rbx, rax
    mov     [rdi+rcx*8-8], r10
    mov     r10, rdx
    adc     r10, 0
    mov     rax, [rsi+rcx*8]

.Lm2l1:
    mul     r8
    add     rbx, rax
    mov     r11, rdx
    adc     r11, 0
    add     rbx, rbp
    mov     rax, [rsi+rcx*8]
    adc     r11, 0
    mul     r9
    mov     [rdi+rcx*8], rbx
    add     r10, rax
    mov     rbx, rdx
    mov     rax, [rsi+rcx*8+8]
    adc     rbx, 0
    add     rcx, 2
    jnc     .Lm2tp

.Lm2ed:                            // write dst[na-1..na+1]
    mul     r8
    add     r10, rax
    mov     rbp, rdx
    adc     rbp, 0
    mov     rax, [rsi-8]
    mul     r9
    add     r10, r11
    adc     rbp, 0
    add     rbx, rax
    mov     [rdi-8], r10
    adc     rdx, 0
    add     rbx, rbp
    mov     [rdi], rbx
    adc     rdx, 0
    mov     [rdi+8], rdx
    add     r12, 2
    lea     rdi, [rdi+16]

.Ldo_addmul_2:                     // outer: (a_i, a_{i+1}) addmul_2
    cmp     r12, -1                // r12 >= -1 -> corner
    jge     .Lcorner

.Louter:
    mov     r8, [rsi+r12*8-8]      // m0 = a_i
    mov     rax, [rsi+r12*8]
    mov     r9, rax                // m1 = a_(i+1)
    mul     r8                     // seed product
    test    r12b, 1
    jnz     .La1x1

.La1x0:
    xor     r10, r10
    mov     r14, [rdi+r12*8+8]
    add     [rdi+r12*8], rax
    mov     r11, rdx
    adc     r11, 0
    xor     rbx, rbx
    mov     rax, [rsi+r12*8+8]
    test    r12b, 2
    jnz     .La110

.La100:
    lea     rcx, [r12+2]
    jmp     .Llo0

.La110:
    lea     rcx, [r12]
    jmp     .Llo2

.La1x1:
    mov     r14, [rdi+r12*8]
    xor     rbx, rbx
    mov     r13, [rdi+r12*8+8]
    add     r14, rax
    mov     rbp, rdx
    adc     rbp, 0
    xor     r10, r10
    mov     rax, [rsi+r12*8+8]
    test    r12b, 2
    jz      .La111

.La101:
    lea     rcx, [r12+3]
    jmp     .Llo1

.La111:
    lea     rcx, [r12+1]
    jmp     .Llo3

.p2align 4
.Ltop:                             // inner loop, 4 limbs
    mov     r13, [rdi+rcx*8-16]
    mul     r9
    mov     r10, rdx
    add     r13, rax
    adc     r10, 0
    add     r14, r11
    adc     rbp, 0
    add     r13, rbx
    adc     r10, 0
    mov     rax, [rsi+rcx*8-16]

.Llo1:
    mul     r8
    add     r13, rax
    mov     r11, rdx
    adc     r11, 0
    mov     rax, [rsi+rcx*8-16]
    mul     r9
    mov     [rdi+rcx*8-24], r14
    mov     r14, [rdi+rcx*8-8]
    add     r13, rbp
    adc     r11, 0
    mov     rbx, rdx
    mov     [rdi+rcx*8-16], r13
    add     r14, rax
    adc     rbx, 0
    mov     rax, [rsi+rcx*8-8]
    add     r14, r10
    adc     rbx, 0

.Llo0:
    mul     r8
    add     r14, rax
    mov     rbp, rdx
    adc     rbp, 0
    mov     rax, [rsi+rcx*8-8]
    mul     r9
    add     r14, r11
    mov     r13, [rdi+rcx*8]
    adc     rbp, 0
    mov     r10, rdx
    add     r13, rax
    adc     r10, 0
    mov     rax, [rsi+rcx*8]

.Llo3:
    mul     r8
    add     r13, rbx
    mov     [rdi+rcx*8-8], r14
    mov     r11, rdx
    adc     r10, 0
    add     r13, rax
    adc     r11, 0
    mov     rax, [rsi+rcx*8]
    add     r13, rbp
    adc     r11, 0
    mul     r9
    mov     r14, [rdi+rcx*8+8]
    add     r14, rax
    mov     rbx, rdx
    adc     rbx, 0
    mov     rax, [rsi+rcx*8+8]
    mov     [rdi+rcx*8], r13

.Llo2:
    mul     r8
    add     r14, r10
    mov     rbp, rdx
    adc     rbx, 0
    add     r14, rax
    mov     rax, [rsi+rcx*8+8]
    adc     rbp, 0
    add     rcx, 4
    jnc     .Ltop

.Lend:                             // write last 3 limbs
    mul     r9
    add     r14, r11
    adc     rbp, 0
    add     rax, rbx
    adc     rdx, 0
    mov     [rdi-8], r14
    add     rax, rbp
    adc     rdx, 0
    mov     [rdi], rax
    mov     [rdi+8], rdx
    add     r12, 2
    lea     rdi, [rdi+16]
    cmp     r12, -1
    jl      .Louter

.Lcorner:
    jg      .Lsqr_diag             // r12 == 0 -> diag

.Lsmall_corner:                    // dst += a_(na-2)*a_(na-1)
    mov     r8, [rsi-16]
    mov     rax, [rsi-8]
    lea     rdi, [rdi+8]
    mul     r8
    add     [rdi-16], rax
    adc     rdx, 0
    mov     [rdi-8], rdx

.Lsqr_diag:                        // dst *= 2, then add a_i^2
    pop     rcx                    // rcx = 1-na
    mov     rax, [rsi+rcx*8-8]     // a_0
    shl     rcx, 1                 // rcx = 2(1-na)
    mul     rax                    // a_0^2
    mov     [rdi+rcx*8-8], rax
    xor     rbx, rbx               // carry collector
    test    cl, 2
    jnz     .Ldm1
    lea     rcx, [rcx+2]
    jmp     .Ldm0

.p2align 4
.Ldtop:                            // 2 diag limbs per iter
    add     r8, r10
    adc     r9, rax
    mov     [rdi+rcx*8-32], r8
    mov     [rdi+rcx*8-24], r9

.Ldm0:                             // even offset: dst[2i] = 2*dst[2i] + a_i^2
    mov     r8, [rdi+rcx*8-16]
    mov     r9, [rdi+rcx*8-8]
    adc     r8, r8
    adc     r9, r9
    mov     rax, [rsi+rcx*4-8]
    lea     r10, [rdx+rbx]         // hi(a_(i-1)^2) + carry
    setc    bl
    mul     rax
    add     r8, r10
    adc     r9, rax
    mov     [rdi+rcx*8-16], r8
    mov     [rdi+rcx*8-8], r9

.Ldm1:                             // next diag limb
    mov     r8, [rdi+rcx*8]
    mov     r9, [rdi+rcx*8+8]
    adc     r8, r8
    adc     r9, r9
    mov     rax, [rsi+rcx*4]
    lea     r10, [rdx+rbx]
    setc    bl
    mul     rax
    add     rcx, 4
    jnc     .Ldtop

.Ldend:                            // write last pair
    add     r8, r10
    adc     r9, rax
    mov     [rdi-16], r8
    mov     [rdi-8], r9
    adc     rdx, rbx
    mov     [rdi], rdx

    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx

    POP_RDI
    POP_RSI
    ret
