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
// mp_limb_t lmmp_shr_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr);
//
// mp_limb_t lmmp_shr_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c);
//
//   Windows: dst=rcx, numa=rdx, na=r8, shr=r9, c=[rsp+40] (only c_)
//   SysV:    dst=rdi, numa=rsi, na=rdx, shr=rcx, c=r8 (only c_)

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_shr_c_
// ======================================
.globl ASM_GSYM(lmmp_shr_c_)
.p2align 4
ASM_GSYM(lmmp_shr_c_):
#ifdef LMMP_WINDOWS
    mov     rax, [rsp+40]           // rax = c
    xchg    rcx, r9                 // dst <-> shr
#else
    mov     rax, r8                 // rax = c
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif
    jmp     .Lentry

// ======================================
// lmmp_shr_
// ======================================
.globl ASM_GSYM(lmmp_shr_)
.p2align 4
ASM_GSYM(lmmp_shr_):
    xor     rax, rax                // c = 0
#ifdef LMMP_WINDOWS
    xchg    rcx, r9
#else
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif

.Lentry:
    and     cl, 63                  // shr &= 63
    push    r12
    push    r13
    push    r14
    jz      .Lcopy                  // shr == 0: copy

    mov     r10d, 64
    sub     r10d, ecx               // r10 = s = 64 - shr
    vmovq   xmm4, rcx               // k = shr
    vmovq   xmm5, r10               // s = 64 - shr

    mov     r11, [rdx]              // a[0]
    xor     r12d, r12d
    shrd    r12, r11, cl            // r12 = a[0] << s

    mov     r10, [rdx + r8*8 - 8]   // a[n-1]
    shr     r10, cl
    or      r10, rax                // | c
    mov     rax, r10                // top limb, stored late

    cmp     r8, 512
    jae     .Lpeel
    lea     r11, [r8 - 1]           // n-1
    xor     r14d, r14d              // P = 0
    jmp     .Lcommon

.Lpeel:                             // n >= 512: peel P limbs for 32B stores
    mov     r10, r9
    shr     r10, 3
    neg     r10
    and     r10d, 3                 // P = (-(dst>>3)) & 3
    mov     r14, r10                // P
    lea     r11, [r8 - 1]
    sub     r11, r10                // M = n-1-P

.Lcommon:
    mov     r8, r11
    shr     r8, 2                   // F = groups
    mov     r13, r11
    and     r13d, 3                 // T = top limbs

    test    r14, r14
    jz      .Lvec

.Lhead:                             // peeled P limbs
    mov     r10, [rdx]              // a[i]
    mov     r11, [rdx + 8]          // a[i+1]
    shrd    r10, r11, cl
    mov     [r9], r10
    add     rdx, 8
    add     r9, 8
    dec     r14
    jnz     .Lhead

.Lvec:
    test    r8, 1
    jz      .Lpairs
    vmovdqu ymm0, [rdx]             // A = a[i..i+3]
    vmovdqu ymm1, [rdx + 8]         // A' = a[i+1..i+4]
    vpsrlq  ymm2, ymm0, xmm4        // A >> k
    vpsllq  ymm3, ymm1, xmm5        // A' << s
    vpor    ymm2, ymm2, ymm3
    vmovdqu [r9], ymm2
    add     rdx, 32
    add     r9, 32
    dec     r8

.Lpairs:
    shr     r8, 1                   // pairs
    jz      .Ltail

.p2align 4
.Lloop:                             // main loop, 8 limbs/pair
    vmovdqu ymm0, [rdx]             // A lo
    vmovdqu ymm1, [rdx + 8]         // A' lo
    vpsrlq  ymm2, ymm0, xmm4
    vpsllq  ymm3, ymm1, xmm5
    vmovdqu ymm0, [rdx + 32]        // A hi
    vmovdqu ymm1, [rdx + 40]        // A' hi
    vpor    ymm2, ymm2, ymm3
    vmovdqu [r9], ymm2
    vpsrlq  ymm2, ymm0, xmm4
    vpsllq  ymm3, ymm1, xmm5
    vpor    ymm2, ymm2, ymm3
    vmovdqu [r9 + 32], ymm2
    add     rdx, 64
    add     r9, 64
    dec     r8
    jnz     .Lloop

.Ltail:                             // top T limbs
    test    r13, r13
    jz      .Ltop
    mov     r10, [rdx]              // a[i]
    mov     r11, [rdx + 8]          // a[i+1]
    shrd    r10, r11, cl
    mov     [r9], r10
    add     rdx, 8
    add     r9, 8
    dec     r13
    jnz     .Ltail

.Ltop:                              // r9 = dst + 8(n-1)
    mov     [r9], rax               // store top limb
    mov     rax, r12                // ret
    vzeroupper
    pop     r14
    pop     r13
    pop     r12
    ret

.Lcopy:                             // shr == 0: copy
    xor     eax, eax
    cmp     r9, rdx
    je      .Lcopy_out
    mov     r13, r8
    and     r13d, 3                 // tail limbs
    shr     r8, 2                   // groups
    jz      .Lcopy_tail

.p2align 4
.Lcopy_loop:
    vmovdqu ymm0, [rdx]
    vmovdqu [r9], ymm0
    add     rdx, 32
    add     r9, 32
    dec     r8
    jnz     .Lcopy_loop

.Lcopy_tail:
    test    r13, r13
    jz      .Lcopy_out

.Lcopy_tail_loop:
    mov     r10, [rdx]
    mov     [r9], r10
    add     rdx, 8
    add     r9, 8
    dec     r13
    jnz     .Lcopy_tail_loop

.Lcopy_out:
    vzeroupper
    pop     r14
    pop     r13
    pop     r12
    ret
