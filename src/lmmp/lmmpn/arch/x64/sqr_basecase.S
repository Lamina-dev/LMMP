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
// void lmmp_sqr_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
//
//   Windows: dst=rcx, numa=rdx, na=r8
//   SysV:    dst=rdi, numa=rsi, na=rdx

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

.text

// ======================================
// lmmp_sqr_basecase_
// ======================================
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
    sub     r12, rdx               // r12 = 1-na
    push    r15
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

.Lm2ed:                            // write [dst+na-1, 3]
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

.Ldo_addmul_2:                     // outer: (a[j], a[j+1]) addmul_2 rows
    cmp     r12, -1                // r12 >= -1 -> corner
    jge     .Lcorner

    lea     rsi, [rsi + r12*8]     // up = &a[3] (row j=2)
    lea     rdi, [rdi + r12*8]     // rp = &dst[5]
    jmp     .Lrow_body

.Lrow:                             // rp += 4, up += 2 (next row)
    lea     rsi, [rsi + r12*8]
    lea     rdi, [rdi + r12*8 + 16]
.Lrow_body:
    mov     r9, [rsi - 8]          // v0 = a[j]
    mov     r8, [rsi]              // v1 = a[j+1]
    mov     rax, r12
    not     rax                    // rax = L-1 = na-j-2
    mov     r11, rax
    shr     r11, 2                 // 4-limb groups
    test    al, 3                  // dispatch on (L-1) mod 4
    jz      .Lh1
    test    al, 1
    jz      .Lh3
    test    al, 2
    jz      .Lh2
    jmp     .Lh0

// limb 0 is v0-only: up[0]*v1 = a[j+1]*a[j+1] belongs to the diag pass, not rows
.Lh0:                              // (L-1)%4 == 3: special, B, A, B
    mov     rdx, r9
    mulx    rcx, rax, [rsi]
    add     rax, [rdi]
    adc     rcx, 0                 // cy0 = hi0 + c1
    mov     [rdi], rax             // cy1 = 0
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mov     rdx, r8
    mulx    r15, rbx, [rsi]        // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, rbx
    adc     r15, 0
    add     rax, rcx
    adc     r10, 0                 // cy1 = 0
    adc     r15, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mulx    rcx, rax, [rsi]        // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi]
    add     rax, [rdi]
    adc     rcx, rbx
    adc     rbp, 0
    add     rax, r10
    adc     rcx, r15
    adc     rbp, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mulx    r15, rbx, [rsi]        // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, rbx
    adc     r15, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r15, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Lkt
    jmp     .Lk4

.Lh1:                              // (L-1)%4 == 0: special only
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, 0                 // cy0 = hi0 + c1
    xor     r15d, r15d             // cy1 = 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Lkt
    jmp     .Lk4

.Lh2:                              // (L-1)%4 == 1: special, B
    mov     rdx, r9
    mulx    rcx, rax, [rsi]
    add     rax, [rdi]
    adc     rcx, 0                 // cy0 = hi0 + c1
    mov     [rdi], rax             // cy1 = 0
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mov     rdx, r8
    mulx    r15, rbx, [rsi]        // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, rbx
    adc     r15, 0
    add     rax, rcx
    adc     r10, 0                 // cy1 = 0
    adc     r15, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Lkt
    jmp     .Lk4

.Lh3:                              // (L-1)%4 == 2: special, A, B
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, 0                 // cy0 = hi0 + c1
    mov     [rdi], rax             // cy1 = 0
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mulx    rcx, rax, [rsi]        // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi]
    add     rax, [rdi]
    adc     rcx, rbx
    adc     rbp, 0
    add     rax, r10
    adc     rcx, 0                 // cy1 = 0
    adc     rbp, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mulx    r15, rbx, [rsi]        // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, rbx
    adc     r15, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r15, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Lkt
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
    adc     rcx, r15               // cy0 = h + cy1 + c4
    adc     rbp, 0                 // cy1 = h2 + c5
    mov     [rdi], rax

    mulx    r15, rbx, [rsi + 8]    // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 8]
    add     rax, [rdi + 8]
    adc     r10, rbx
    adc     r15, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r15, 0
    mov     [rdi + 8], rax

    mulx    rcx, rax, [rsi + 16]   // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi + 16]
    add     rax, [rdi + 16]
    adc     rcx, rbx
    adc     rbp, 0
    add     rax, r10
    adc     rcx, r15
    adc     rbp, 0
    mov     [rdi + 16], rax

    mulx    r15, rbx, [rsi + 24]   // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 24]
    add     rax, [rdi + 24]
    adc     r10, rbx
    adc     r15, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r15, 0
    mov     [rdi + 24], rax

    lea     rsi, [rsi + 32]
    lea     rdi, [rdi + 32]
    dec     r11
    jnz     .Lk4

.Lkt:                              // write [dst+na+j,2]
    mov     [rdi], r10
    mov     [rdi + 8], r15
    add     r12, 2
    cmp     r12, -1
    jl      .Lrow
    lea     rdi, [rdi + 16]        // rp += 2 limbs (rows done)

.Lcorner:
    jg      .Lsqr_diag             // r12 == 0 -> diag

.Lsmall_corner:                    // dst += a[na-2]*a[na-1]
    mov     r8, [rsi-16]
    mov     rax, [rsi-8]
    lea     rdi, [rdi+8]
    mul     r8
    add     [rdi-16], rax
    adc     rdx, 0
    mov     [rdi-8], rdx

.Lsqr_diag:                        // dst *= 2, then add a[i]^2
    pop     rcx                    // rcx = 1-na
    mov     rax, [rsi+rcx*8-8]     // a0
    shl     rcx, 1                 // rcx = 2(1-na)
    mul     rax                    // a0^2
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

.Ldm0:                             // even offset: dst[2i] = 2*dst[2i] + a[i]^2
    mov     r8, [rdi+rcx*8-16]
    mov     r9, [rdi+rcx*8-8]
    adc     r8, r8
    adc     r9, r9
    mov     rax, [rsi+rcx*4-8]
    lea     r10, [rdx+rbx]         // hi(a[i-1]^2) + carry
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

    pop     r15
    pop     r12
    pop     rbp
    pop     rbx

    POP_RDI
    POP_RSI
    ret
