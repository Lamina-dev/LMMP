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
//  License along with this library.  If not, see
//  <https://www.gnu.org/licenses/>.
//
//  SPDX-License-Identifier: LGPL-3.0-or-later

//
// void lmmp_mul_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);
//
//   Windows: dst=rcx, numa=rdx, na=r8, numb=r9, nb=[rsp+0x48]
//   SysV:    dst=rdi, numa=rsi, na=rdx, numb=rcx, nb=r8

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

.text

// ======================================
// lmmp_mul_basecase_
// ======================================
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

    lea     rsi, [rsi + rdx*8]      // rsi = numa + na
    lea     rdi, [rdi + rdx*8]      // rdi = dst + na
    PUSH_EXTRA

    test    r8b, 1                  // nb parity
    jnz     .Ldo_mul_1

// nb even: dst = numa * [numb,2] (mul_2)
.Ldo_mul_2:
    push    r15
    mov     r9, [rcx]               // v0 = numb[0]
    mov     r14, [rcx + 8]          // v1 = numb[1]
    lea     rsi, [rsi + rbx*8]      // rsi = numa
    lea     rdi, [rdi + rbx*8]      // rdi = dst
    mov     r11, rbx
    neg     r11
    shr     r11, 2                  // groups = na/4
    mov     rax, rbx
    and     eax, 3                  // (-na) mod 4
    jz      .Lm2e0
    cmp     eax, 2
    je      .Lm2e2
    jb      .Lm2e3                  // na mod 4 == 3

.Lm2e1:                             // na mod 4 == 1: limb 0 -> (rbp,r13)
    mov     rdx, r9
    mulx    rbp, rax, [rsi]
    mov     [rdi], rax              // dst[0] = lo0
    mov     rdx, r14
    mulx    r13, r15, [rsi]
    add     rbp, r15                // cy0 = hi0 + lo1 -> c
    adc     r13, 0                  // cy1 = hi1 + c
    mov     r10, rbp
    mov     r12, r13
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Lm2fin
    mov     rdx, r9
    jmp     .Lm2top

.Lm2e0:                             // na mod 4 == 0: (cy0,cy1) = (0,0)
    xor     r10d, r10d
    xor     r12d, r12d
    mov     rdx, r9
    jmp     .Lm2top

.Lm2e2:                             // na mod 4 == 2: limbs 0,1 -> (r10,r12)
    mov     rdx, r9
    mulx    rbp, rax, [rsi]
    mov     [rdi], rax
    mov     rdx, r14
    mulx    r13, r15, [rsi]
    add     rbp, r15
    adc     r13, 0                  // limb 0 -> (rbp,r13)
    mulx    r12, r15, [rsi + 8]     // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 8]
    add     r10, r15                // h -> c2
    adc     r12, 0                  // h2 -> c3 (0)
    add     rax, rbp                // s -> c4
    adc     r10, r13                // cy0'
    adc     r12, 0                  // cy1'
    mov     [rdi + 8], rax          // limb 1 -> (r10,r12)
    lea     rsi, [rsi + 16]
    lea     rdi, [rdi + 16]
    test    r11, r11
    jz      .Lm2fin
    mov     rdx, r9
    jmp     .Lm2top

.Lm2e3:                             // na mod 4 == 3: limbs 0,1,2 -> (rbp,r13)
    mov     rdx, r9
    mulx    rbp, rax, [rsi]
    mov     [rdi], rax
    mov     rdx, r14
    mulx    r13, r15, [rsi]
    add     rbp, r15
    adc     r13, 0                  // limb 0 -> (rbp,r13)
    mulx    r12, r15, [rsi + 8]     // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 8]
    add     r10, r15                // h
    adc     r12, 0                  // h2
    add     rax, rbp                // s
    adc     r10, r13                // cy0'
    adc     r12, 0                  // cy1'
    mov     [rdi + 8], rax          // limb 1 -> (r10,r12)
    mulx    rbp, rax, [rsi + 16]    // rdx = v0
    mov     rdx, r14
    mulx    r13, r15, [rsi + 16]
    add     rbp, r15                // h
    adc     r13, 0                  // h2
    add     rax, r10                // s
    adc     rbp, r12                // cy0'
    adc     r13, 0                  // cy1'  -> limb 2 -> (rbp,r13)
    mov     [rdi + 16], rax
    mov     r10, rbp
    mov     r12, r13
    lea     rsi, [rsi + 24]
    lea     rdi, [rdi + 24]
    test    r11, r11
    jz      .Lm2fin
    mov     rdx, r9

.p2align 4
.Lm2top:                            // 4 limbs/group, consumes (r10,r12), rdx = v0
    mulx    rbp, rax, [rsi]         // lo0=rax hi0=rbp
    mov     rdx, r14
    mulx    r13, r15, [rsi]         // lo1=r15 hi1=r13
    add     rbp, r15                // h = hi0 + lo1 -> c2
    adc     r13, 0                  // h2 = hi1 + c2 -> c3 (0)
    add     rax, r10                // s = lo0 + cy0 -> c4
    adc     rbp, r12                // cy0' = h + cy1 + c4 -> c5
    adc     r13, 0                  // cy1' = h2 + c5 -> c6 (0)
    mov     [rdi], rax

    mulx    r12, r15, [rsi + 8]     // rdx = v1: lo1=r15 hi1=r12
    mov     rdx, r9
    mulx    r10, rax, [rsi + 8]     // lo0=rax hi0=r10
    add     r10, r15                // h
    adc     r12, 0                  // h2
    add     rax, rbp                // s
    adc     r10, r13                // cy0'
    adc     r12, 0                  // cy1'
    mov     [rdi + 8], rax

    mulx    rbp, rax, [rsi + 16]    // rdx = v0
    mov     rdx, r14
    mulx    r13, r15, [rsi + 16]
    add     rbp, r15                // h
    adc     r13, 0                  // h2
    add     rax, r10                // s
    adc     rbp, r12                // cy0'
    adc     r13, 0                  // cy1'
    mov     [rdi + 16], rax

    mulx    r12, r15, [rsi + 24]    // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 24]
    add     r10, r15                // h
    adc     r12, 0                  // h2
    add     rax, rbp                // s
    adc     r10, r13                // cy0'
    adc     r12, 0                  // cy1'
    mov     [rdi + 24], rax

    lea     rsi, [rsi + 32]
    lea     rdi, [rdi + 32]
    dec     r11
    jnz     .Lm2top

.Lm2fin:                            // write [dst+na,2]
    mov     [rdi], r10
    mov     [rdi + 8], r12
    pop     r15

    sub     r8, 2                   // nb -= 2
    jz      .Lret5
    lea     rcx, [rcx + 16]         // numb += 2
    lea     rdi, [rdi + 16]         // dst += 2

// addmul_2 outer loop, p7 carry-save rows
// r13 = -8*na, r14 = numb, r15 = limbs left (2 per row)
.Ldo_addmul:
    push    r15
    imul    r13, rbx, 8             // r13 = -8*na
    mov     r15, r8                 // limbs left
    lea     rsi, [rsi + rbx*8]      // rsi = numa
    lea     rdi, [rdi + rbx*8]      // rdi = dst + j
    mov     r14, rcx                // r14 = numb
    jmp     .Lrow_body

.Lrow:                              // [dst+j,na+2] += [numa,na] * (numb[j], numb[j+1])
    lea     rsi, [rsi + r13]        // up = numa
    lea     rdi, [rdi + r13 + 16]   // rp = dst + j + 2

.Lrow_body:
    mov     r9, [r14]               // v0 = numb[j]
    mov     r8, [r14 + 8]           // v1 = numb[j+1]
    lea     r14, [r14 + 16]
    mov     rax, r13
    neg     rax                     // 8*na
    shr     rax, 3                  // na
    mov     r11, rax
    shr     r11, 2                  // 4-limb groups
    test    al, 3
    jz      .Lh0
    test    al, 1
    jz      .Lh2
    test    al, 2
    jz      .Lh1

.Lh3:                               // na%4 == 3: limb 0 -> (r10,r12); limbs 1,2 rotate
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
    jz      .Lkt
    jmp     .Lk4

.Lh0:                               // na%4 == 0: (cy0,cy1) = (0,0), loop covers [numa,na]
    mov     rdx, r9
    xor     r10d, r10d
    xor     r12d, r12d
    jmp     .Lk4

.Lh1:                               // na%4 == 1: limb 0 emits (r10,r12)
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
    jz      .Lkt
    jmp     .Lk4

.Lh2:                               // na%4 == 2: limb 0 -> (rcx,rbp); limb 1 -> (r10,r12)
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
    jz      .Lkt

.p2align 4
.Lk4:                               // inner loop, 4 limbs, pair rotates (kernel = addmul_2.S)
    mulx    rcx, rax, [rsi]         // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi]
    add     rax, [rdi]              // t = lo0 + rp
    adc     rcx, rbx                // h = hi0 + l1 + c1
    adc     rbp, 0                  // h2 = hi1 + c2
    add     rax, r10                // s = t + cy0
    adc     rcx, r12                // cy0 = h + cy1 + c4
    adc     rbp, 0                  // cy1 = h2 + c5
    mov     [rdi], rax

    mulx    r12, rbx, [rsi + 8]     // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 8]
    add     rax, [rdi + 8]
    adc     r10, rbx
    adc     r12, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r12, 0
    mov     [rdi + 8], rax

    mulx    rcx, rax, [rsi + 16]    // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi + 16]
    add     rax, [rdi + 16]
    adc     rcx, rbx
    adc     rbp, 0
    add     rax, r10
    adc     rcx, r12
    adc     rbp, 0
    mov     [rdi + 16], rax

    mulx    r12, rbx, [rsi + 24]    // rdx = v1
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

.Lkt:                               // write [dst+j+na,2]
    mov     [rdi], r10
    mov     [rdi + 8], r12
    sub     r15, 2
    jnz     .Lrow
    pop     r15

.Lret5:
    POP_EXTRA
    POP_ALL
    ret

// nb odd: dst = numa * numb[0] (mul_1, carry kept as (hi, CF) across the loop)
.Ldo_mul_1:
    mov     rdx, [rcx]              // rdx = b0 (mulx implied)
    mov     r10, rbx
    neg     r10
    shr     r10, 2
    inc     r10                     // r10 = na/4 + 1 (dec hits 0 after na/4 passes)

    mov     rbp, rbx                // rbp = -na (seed index)
    mov     rax, rbx
    and     eax, 3                  // (-na) mod 4, CF = 0
    jz      .Lm1m0
    cmp     eax, 2
    je      .Lm1rem2
    jb      .Lm1rem3                // na mod 4 == 3

.Lm1rem1:                           // na mod 4 == 1 (CF = 0 from cmp)
    mulx    r14, rax, [rsi + rbp*8] // p0: hi0 -> r14 (pending home)
    mov     [rdi + rbp*8], rax      // s0 = lo0
    lea     rbp, [rbp + 1]
    dec     r10                     // CF preserved
    jz      .Lm1ed
    jmp     .Lm1loop

.Lm1m0:                             // na mod 4 == 0
    xor     r14d, r14d              // pending hi = 0
    dec     r10                     // consume the +1 (CF preserved)
    jmp     .Lm1loop

.Lm1rem2:                                  // na mod 4 == 2 (CF = 0 from je)
    mulx    r11, rax, [rsi + rbp*8]        // p0: hi0 -> r11
    mulx    r14, r12, [rsi + rbp*8 + 8]    // p1: hi1 -> r14 (pending)
    mov     [rdi + rbp*8], rax             // s0 = lo0
    adc     r12, r11                       // s1 = lo1 + hi0 + CF
    mov     [rdi + rbp*8 + 8], r12
    lea     rbp, [rbp + 2]
    dec     r10
    jz      .Lm1ed
    jmp     .Lm1loop

.Lm1rem3:                                  // na mod 4 == 3 (CF = 1 from jb)
    clc                                    // normalize CF
    mulx    r11, rax, [rsi + rbp*8]        // p0: hi0 -> r11
    mulx    r9,  r12, [rsi + rbp*8 + 8]    // p1: hi1 -> r9
    mov     [rdi + rbp*8], rax             // s0 = lo0
    adc     r12, r11                       // s1
    mov     [rdi + rbp*8 + 8], r12
    mulx    r14, rax, [rsi + rbp*8 + 16]   // p2: hi2 -> r14 (pending)
    adc     rax, r9                        // s2 = lo2 + hi1 + CF
    mov     [rdi + rbp*8 + 16], rax
    lea     rbp, [rbp + 3]
    dec     r10
    jz      .Lm1ed
    // fall into .Lm1loop

.p2align 4
.Lm1loop:                                  // mul_1 seed, 4 limbs/iter
    mulx    r11, rax, [rsi + rbp*8]        // p0: hi0 -> r11
    mulx    r9,  r12, [rsi + rbp*8 + 8]    // p1: hi1 -> r9
    adc     rax, r14                       // s0 = lo0 + hi3' + CF
    mov     [rdi + rbp*8], rax
    adc     r12, r11                       // s1
    mov     [rdi + rbp*8 + 8], r12
    mulx    r11, rax, [rsi + rbp*8 + 16]   // p2: hi2 -> r11
    adc     rax, r9                        // s2
    mov     [rdi + rbp*8 + 16], rax
    mulx    r14, r12, [rsi + rbp*8 + 24]   // p3: hi3 -> r14 (new pending)
    adc     r12, r11                       // s3
    mov     [rdi + rbp*8 + 24], r12
    lea     rbp, [rbp + 4]
    dec     r10                            // CF preserved
    jnz     .Lm1loop

.Lm1ed:
    mov     rax, r14
    adc     rax, 0                  // carry = hi3 + CF
    mov     [rdi], rax              // dst[na] = carry

    dec     r8                      // nb--
    jz      .Lret5

    lea     rcx, [rcx + 8]          // numb++
    lea     rdi, [rdi + 8]          // dst++
    jmp     .Ldo_addmul
