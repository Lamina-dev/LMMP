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

#ifdef LMMP_WINDOWS
    mov     rdi, rcx                // numa -> rdi
    mov     rsi, rdx                // numb -> rsi
    mov     rdx, r8                 // n -> rdx
    mov     rcx, r9                 // bp -> rcx
#endif

    test    rdx, rdx                // n == 0: return 0
    jz      .Lret0

    mov     r9, [rcx]               // v0 = bp[0]
    mov     r8, [rcx + 8]           // v1 = bp[1]

    cmp     rdx, 1
    jne     .Lgen

// n == 1: [numa,2] += [numb,1] * (v0 + v1*B), return limb 2
.Lone:
    mov     r11, [rsi]              // u = numb[0] (cached: eqsep-safe)
    mov     rax, r11
    mul     r9
    add     rax, [rdi]
    adc     rdx, 0
    mov     [rdi], rax
    mov     r10, rdx                // r10 = limb 1 partial
    mov     rax, r11
    mul     r8
    add     rax, r10
    adc     rdx, 0
    mov     [rdi + 8], rax          // numa[1] = limb 1
    jmp     .Lretdx                 // return rdx = limb 2

// per limb (u = up[i]; lo0/hi0 = u*v0, l1/hi1 = u*v1):
//   t  = lo0 + rp            out c1
//   h  = hi0 + l1 + c1       out c2
//   h2 = hi1 + c2            out c3 = 0 always
//   s  = t + cy0             out c4
//   cy0 = h + cy1 + c4       out c5
//   cy1 = h2 + c5            out c6 = 0 always
// hi(u*v) <= B-2 bounds h2, cy1 < B and excludes c2 = 1 with hi1 = B-2,
// so plain adds may break the flag chain at s and in the next limb.
// The (h,h2) pair of limb j becomes (cy0,cy1) of limb j+1.

.Lgen:                              // n >= 2: numa += numb * (v0, v1)
    mov     r11, rdx
    shr     r11, 2                  // 4-limb groups after head
    test    dl, 3
    jz      .Le0                    // n%4 == 0
    test    dl, 1
    jz      .Le2                    // n%4 == 2
    test    dl, 2
    jz      .Le1                    // n%4 == 1
    jmp     .Le3                    // n%4 == 3

// n%4 == 0: (cy0,cy1) = (0,0), loop covers [numa, n]
.Le0:
    mov     rdx, r9
    xor     r10, r10
    xor     r12, r12
    jmp     .Ltop

// n%4 == 1: limb 0 has (cy0,cy1) = (0,0), emits (r10,r12)
.Le1:
    mov     rdx, r8
    mulx    r12, rbx, [rsi]
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]              // t = lo0 + rp
    adc     r10, rbx                // h = hi0 + l1 + c1
    adc     r12, 0                  // h2 = hi1 + c2
    mov     [rdi], rax              // rp = t  (cy0 = 0)
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    jmp     .Ltop

// n%4 == 2: limb 0 has (cy0,cy1) = (0,0) -> (rcx,rbp); limb 1 -> (r10,r12)
.Le2:
    mov     rdx, r9
    mulx    rcx, rax, [rsi]
    mov     rdx, r8
    mulx    rbp, rbx, [rsi]
    add     rax, [rdi]              // t = lo0 + rp
    adc     rcx, rbx                // h = hi0 + l1 + c1
    adc     rbp, 0                  // h2 = hi1 + c2
    mov     [rdi], rax              // rp = t  (cy0 = 0)
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    mulx    r12, rbx, [rsi]
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]              // t = lo0 + rp
    adc     r10, rbx                // h = hi0 + l1 + c1
    adc     r12, 0                  // h2 = hi1 + c2
    add     rax, rcx                // s = t + cy0
    adc     r10, rbp                // cy0 = h + cy1 + c4
    adc     r12, 0                  // cy1 = h2 + c5
    mov     [rdi], rax              // rp = s
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    test    r11, r11
    jz      .Ltail
    jmp     .Ltop

// n%4 == 3: limb 0 has (cy0,cy1) = (0,0) -> (r10,r12); limbs 1,2 rotate
.Le3:
    mov     rdx, r8
    mulx    r12, rbx, [rsi]
    mov     rdx, r9
    mulx    r10, rax, [rsi]
    add     rax, [rdi]              // t = lo0 + rp
    adc     r10, rbx                // h = hi0 + l1 + c1
    adc     r12, 0                  // h2 = hi1 + c2
    mov     [rdi], rax              // rp = t  (cy0 = 0)
    mulx    rcx, rax, [rsi + 8]
    mov     rdx, r8
    mulx    rbp, rbx, [rsi + 8]
    add     rax, [rdi + 8]          // t = lo0 + rp
    adc     rcx, rbx                // h = hi0 + l1 + c1
    adc     rbp, 0                  // h2 = hi1 + c2
    add     rax, r10                // s = t + cy0
    adc     rcx, r12                // cy0 = h + cy1 + c4
    adc     rbp, 0                  // cy1 = h2 + c5
    mov     [rdi + 8], rax          // rp = s
    mulx    r12, rbx, [rsi + 16]
    mov     rdx, r9
    mulx    r10, rax, [rsi + 16]
    add     rax, [rdi + 16]         // t = lo0 + rp
    adc     r10, rbx                // h = hi0 + l1 + c1
    adc     r12, 0                  // h2 = hi1 + c2
    add     rax, rcx                // s = t + cy0
    adc     r10, rbp                // cy0 = h + cy1 + c4
    adc     r12, 0                  // cy1 = h2 + c5
    mov     [rdi + 16], rax         // rp = s
    lea     rsi, [rsi + 24]
    lea     rdi, [rdi + 24]
    test    r11, r11
    jz      .Ltail
    jmp     .Ltop

.p2align 4
.Ltop:                              // inner loop, 4 limbs, pair rotates
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
    add     rax, [rdi + 8]          // t = lo0 + rp
    adc     r10, rbx                // h = hi0 + l1 + c1
    adc     r12, 0                  // h2 = hi1 + c2
    add     rax, rcx                // s = t + cy0
    adc     r10, rbp                // cy0 = h + cy1 + c4
    adc     r12, 0                  // cy1 = h2 + c5
    mov     [rdi + 8], rax

    mulx    rcx, rax, [rsi + 16]    // rdx = v0
    mov     rdx, r8
    mulx    rbp, rbx, [rsi + 16]
    add     rax, [rdi + 16]         // t = lo0 + rp
    adc     rcx, rbx                // h = hi0 + l1 + c1
    adc     rbp, 0                  // h2 = hi1 + c2
    add     rax, r10                // s = t + cy0
    adc     rcx, r12                // cy0 = h + cy1 + c4
    adc     rbp, 0                  // cy1 = h2 + c5
    mov     [rdi + 16], rax

    mulx    r12, rbx, [rsi + 24]    // rdx = v1
    mov     rdx, r9
    mulx    r10, rax, [rsi + 24]
    add     rax, [rdi + 24]         // t = lo0 + rp
    adc     r10, rbx                // h = hi0 + l1 + c1
    adc     r12, 0                  // h2 = hi1 + c2
    add     rax, rcx                // s = t + cy0
    adc     r10, rbp                // cy0 = h + cy1 + c4
    adc     r12, 0                  // cy1 = h2 + c5
    mov     [rdi + 24], rax

    lea     rsi, [rsi + 32]
    lea     rdi, [rdi + 32]
    dec     r11
    jnz     .Ltop

.Ltail:                             // last pair is always (r10,r12)
    mov     [rdi], r10              // numa[n] = limb n
    mov     rax, r12                // return limb n+1

.Lret:
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
    jmp     .Lret
.Lretdx:
    mov     rax, rdx
    jmp     .Lret
