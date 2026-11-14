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

    mov     r9, [rsi + rbx*8 - 8] // r9 = a0
    mov     rax, [rsi + rbx*8]    // rax = a1
    mul     r9                    // rdx:rax = a0 * a1

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
    jmp     .Lrow_entry

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

// two-column rows: r12 = j+2-n, r13 = &a[j+1], r14 = &dst[2j+1]
// row = [numa+j+1,L] x (a[j], a[j+1]) with up[0]*v1 omitted,
// plus tail limb a[j] * a[n-1-j] into dst[n-1]; carry at col n discarded
.Lrow_entry:
    cmp     rbx, -6               // rbx = 1-n: n <= 7 keeps classic rows
    jge     .Lrow_old
    add     rbx, 2                // rbx = 3-n (row j=1)
    jns     .Ldiag

    push    r15
    push    r12                     // numa (diag wants it in r12)
    push    r13                     // dst
    push    r14                     // n
    mov     r12, rbx                // r12 = j+2-n
    lea     r13, [rsi + rbx*8 - 8]  // r13 = &a[2]
    lea     r14, [rdi + rbx*8]      // r14 = &dst[3]

.Lrow:
    mov     r9, [r13 - 8]         // v0 = a[j]
    mov     r8, [r13]             // v1 = a[j+1]
    lea     rax, [r12 + 1]
    neg     rax                   // rax = L = n-2j-2
    jz      .Lrow0
    lea     rsi, [r13]            // up = &a[j+1]
    lea     rdi, [r14]            // rp = &dst[2j+1]
    mov     r11, rax
    dec     r11                   // L-1 kernel limbs
    shr     r11, 2                // 4-limb groups
    test    al, 3                 // dispatch on L mod 4
    jz      .Lh0
    test    al, 1
    jz      .Lh2
    test    al, 2
    jz      .Lh1
    jmp     .Lh3

// limb 0 is v0-only: up[0]*v1 = a[j+1]*a[j+1] belongs to the diag pass, not rows
.Lh0:                             // L%4 == 0: special, B, A, B
    mov     rdx, r9
    mulx    rcx, rax, [rsi]
    add     rax, [rdi]
    adc     rcx, 0                // cy0 = hi0 + c1
    mov     [rdi], rax            // cy1 = 0
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mov     rdx, r8
    mulx    r15, rbx, [rsi]       // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, rbx
    adc     r15, 0
    add     rax, rcx
    adc     r10, 0                // cy1 = 0
    adc     r15, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mulx    rcx, rax, [rsi]       // rdx = v0
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
    mulx    r15, rbx, [rsi]       // rdx = v1
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

.Lh1:                             // L%4 == 1: special only
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, 0                // cy0 = hi0 + c1
    xor     r15d, r15d            // cy1 = 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Lkt
    jmp     .Lk4

.Lh2:                             // L%4 == 2: special, B
    mov     rdx, r9
    mulx    rcx, rax, [rsi]
    add     rax, [rdi]
    adc     rcx, 0                // cy0 = hi0 + c1
    mov     [rdi], rax            // cy1 = 0
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mov     rdx, r8
    mulx    r15, rbx, [rsi]       // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, rbx
    adc     r15, 0
    add     rax, rcx
    adc     r10, 0                // cy1 = 0
    adc     r15, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Lkt
    jmp     .Lk4

.Lh3:                             // L%4 == 3: special, A, B
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]
    adc     r10, 0                // cy0 = hi0 + c1
    mov     [rdi], rax            // cy1 = 0
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mulx    rcx, rax, [rsi]       // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi]
    add     rax, [rdi]
    adc     rcx, rbx
    adc     rbp, 0
    add     rax, r10
    adc     rcx, 0                // cy1 = 0
    adc     rbp, 0
    mov     [rdi], rax
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mulx    r15, rbx, [rsi]       // rdx = v1
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
.Lk4:                             // inner loop, 4 limbs, pair rotates (kernel = addmul_2.S)
    mulx    rcx, rax, [rsi]       // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi]
    add     rax, [rdi]            // t = lo0 + rp
    adc     rcx, rbx              // h = hi0 + l1 + c1
    adc     rbp, 0                // h2 = hi1 + c2
    add     rax, r10              // s = t + cy0
    adc     rcx, r15              // cy0 = h + cy1 + c4
    adc     rbp, 0                // cy1 = h2 + c5
    mov     [rdi], rax

    mulx    r15, rbx, [rsi + 8]   // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 8]
    add     rax, [rdi + 8]
    adc     r10, rbx
    adc     r15, 0
    add     rax, rcx
    adc     r10, rbp
    adc     r15, 0
    mov     [rdi + 8], rax

    mulx    rcx, rax, [rsi + 16]  // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi + 16]
    add     rax, [rdi + 16]
    adc     rcx, rbx
    adc     rbp, 0
    add     rax, r10
    adc     rcx, r15
    adc     rbp, 0
    mov     [rdi + 16], rax

    mulx    r15, rbx, [rsi + 24]  // rdx = v1
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

.Lkt:                             // rsi = &a[n-1-j], rdi = &dst[n-1]
    mulx    rbx, rax, [rsi]       // rdx = v0: a[j] * a[n-1-j]
    add     rax, [rdi]            // lo + dst[n-1]
    adc     rbx, 0                // h = hi0 + c1
    add     rax, r10              // + cy0; col n carry discarded
    mov     [rdi], rax
    lea     r13, [r13 + 16]
    lea     r14, [r14 + 32]
    add     r12, 4
    js      .Lrow
    pop     r14                   // n
    pop     r13                   // dst
    pop     r12                   // numa
    pop     r15
    jmp     .Ldiag

.Lrow0:                           // L == 0 (last row): a[j]*a[j+1]
    mov     rax, [r13]            // a[j+1]
    mul     r9                    // a[j]
    add     [r14], rax            // dst[n-1] += lo; carry discarded
    pop     r14                   // n
    pop     r13                   // dst
    pop     r12                   // numa
    pop     r15
    jmp     .Ldiag

// classic single-multiplier rows, kept for n <= 7 (two-column form loses there)
.Lrow_old:
    add     rbx, 2                // next base = 2j+1-n
    jns     .Ldiag
    sub     rsi, 8
    mov     r9, [rsi + rbx*8 - 8] // r9 = a[j]
    mov     rax, [rsi + rbx*8]    // rax = a[j+1]
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
    jmp     .Lrow_old

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
    jmp     .Lrow_old

// diag: dst = 2*dst + sum a[i]^2*B^2i, CF chain live, r10 = pending
.Ldiag:
    mov     rax, [r12]
    mul     rax                   // rdx:rax = a0^2
    mov     [r13], rax            // dst[0] = lo(a0^2)
    mov     r10, rdx              // pending = hi(a0^2)
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
    mul     rax                   // rdx:rax = a[i]^2
    add     r8, r10               // odd += pending
    adc     r9, rax               // even += lo(a[i]^2)
    mov     [rdi], r8
    mov     [rdi + 8], r9
    lea     r10, [rdx + rbx]      // pending = hi(a[i]^2) + c
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
