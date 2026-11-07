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
// mp_limb_t lmmp_mod_preinv_1_(mp_srcptr numa, mp_size_t na, mp_limb_t x, mp_limb_t inv, mp_limb_t k);
//
//   Windows: numa=rcx, na=rdx, x=r8, inv=r9, k=[rsp+40]
//   SysV:    numa=rdi, na=rsi, x=rdx, inv=rcx, k=r8
//

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_mod_preinv_1_
// ======================================
.globl ASM_GSYM(lmmp_mod_preinv_1_)
.p2align 4
ASM_GSYM(lmmp_mod_preinv_1_):
#ifdef LMMP_WINDOWS
    mov     r11, [rsp+40]          // r11 = k
#endif
    push    rbp
    push    rbx
#ifdef LMMP_WINDOWS
    push    rsi
#endif

#ifdef LMMP_WINDOWS
    mov     rsi, rcx                // rsi = numa
    mov     rbx, rdx                // rbx = na
    mov     rcx, r11                // rcx = k (cl)
#else
    mov     r9, rcx                 // r9 = m = inv
    mov     rbx, rsi                // rbx = na
    mov     rsi, rdi                // rsi = numa
    mov     rcx, r8                 // rcx = k (cl)
    mov     r8, rdx                 // r8 = x
#endif
    shl     r8, cl                  // d = x << k

    xor     eax, eax
    mov     r10, [rsi+rbx*8-8]      // top limb
    shld    rax, r10, cl            // rax = xh = top limb << k
    jmp     .Lent

.p2align 4
.Ltop:
    mov     r11, [rsi+rbx*8-8]      // next lower limb
    shld    r10, r11, cl            // r10 = a_lo << k

.Llast:                             // rax = xh < d, rbp = xh+1, r10 = a_lo
    mul     r9                      // rdx:rax = xh * m
    add     rax, r10
    adc     rdx, rbp                // rdx = q
    mov     rbp, rax                // rbp = q_lo
    imul    rdx, r8                 // rdx = q*d
    sub     r10, rdx                // z = a_lo - q*d
    lea     rax, [r8+r10]           // candidate z+d
    cmp     r10, rbp
    cmovc   rax, r10                // borrow: pick z, else +d
    cmp     rax, r8
    jae     .Lfix                   // z >= d: z -= d

.Lok:
    mov     r10, r11

.Lent:
    sub     rbx, 1
    lea     rbp, [rax+1]            // next xh+1
    ja      .Ltop
    jb      .Lret

    shl     r10, cl                 // last limb << k
    jmp     .Llast

.Lfix:
    sub     rax, r8                 // z -= d
    jmp     .Lok

.Lret:
    shr     rax, cl                 // remainder >>= k

#ifdef LMMP_WINDOWS
    pop     rsi
#endif
    pop     rbx
    pop     rbp
    ret
