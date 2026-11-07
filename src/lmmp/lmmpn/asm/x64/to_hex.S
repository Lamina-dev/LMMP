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
// mp_size_t lmmp_to_hex_(char* dst, mp_srcptr numa, mp_size_t na, bool upper);
//
//   Windows: dst=rcx, numa=rdx, na=r8, upper=r9b
//   SysV:    dst=rdi, numa=rsi, na=rdx, upper=cl

#include "lmmp_asm.h"

#ifdef LMMP_WINDOWS
#define rb2 r9b
#else
#define rb2 cl
#endif

.text

// ======================================
// lmmp_to_hex_
// ======================================
.globl ASM_GSYM(lmmp_to_hex_)
.p2align 4
ASM_GSYM(lmmp_to_hex_):
    // LUT base (r10)
    lea          r10, [rip + .Lhex_lower]
    lea          r11, [rip + .Lhex_upper]
    test         rb2, rb2
    cmovne       r10, r11
    vmovdqa      xmm0, [r10]
    vinserti128  ymm0, ymm0, xmm0, 1         // ymm0 = 16B LUT x2
    mov          eax, 0x0F
    vmovd        xmm1, eax
    vpbroadcastb ymm1, xmm1                  // ymm1 = 0x0F

    // strip leading zeros: rax = na-1
    mov          rax, rx2
.Lstrip:
    test         rax, rax
    jz           .Lret0
    dec          rax
    cmp          qword ptr [rx1 + rax * 8], 0
    je           .Lstrip
    lea          rx2, [rax + 1]              // rx2 = na

    // digits = ((na-1)*64 + bitlen(top) + 3) >> 2
    mov          r11, [rx1 + rax * 8]        // top limb
    lzcnt        r11, r11
    neg          r11
    add          r11, 64                     // bitlen
    shl          rax, 6
    add          rax, r11
    add          rax, 3
    shr          rax, 2
    push         rax                         // [digits]

    // nfull = na-1, r11 = nfull>>2, tail = nfull&3
    mov          rax, rx2
    dec          rax
    mov          r11, rax
    and          rax, 3
    push         rax                         // [digits][tail]
    shr          r11, 2
    jz           .Ltail_limbs

.p2align 4
.Lmain:                                      // 4 limb -> 64 chars
    vmovdqu      ymm2, [rx1]                 // {i, i+1, i+2, i+3}
    vpand        ymm3, ymm2, ymm1            // lo nibbles
    vpsrlw       ymm2, ymm2, 4
    vpand        ymm2, ymm2, ymm1            // hi nibbles
    vpshufb      ymm3, ymm0, ymm3            // loc
    vpshufb      ymm2, ymm0, ymm2            // hic
    vpunpcklbw   ymm4, ymm3, ymm2            // {limb i,   limb i+2}
    vpunpckhbw   ymm5, ymm3, ymm2            // {limb i+1, limb i+3}
    vperm2i128   ymm2, ymm4, ymm5, 0x20      // {i,   i+1}
    vmovdqu      [rx0], ymm2
    vperm2i128   ymm3, ymm4, ymm5, 0x31      // {i+2, i+3}
    vmovdqu      [rx0 + 32], ymm3
    add          rx1, 32
    add          rx0, 64
    dec          r11
    jnz          .Lmain

.Ltail_limbs:                                // 1..3 limbs (xmm, 1 limb each)
    pop          rax                         // tail
    test         rax, rax
    jz           .Ltop
.Ltail_loop:
    vmovq        xmm2, [rx1]
    vpand        xmm3, xmm2, xmm1
    vpsrlw       xmm2, xmm2, 4
    vpand        xmm2, xmm2, xmm1
    vpshufb      xmm3, xmm0, xmm3
    vpshufb      xmm2, xmm0, xmm2
    vpunpcklbw   xmm4, xmm3, xmm2
    vmovdqu      [rx0], xmm4                 // 16 chars
    add          rx1, 8
    add          rx0, 16
    dec          rax
    jnz          .Ltail_loop

.Ltop:                                       // top limb tail
    pop          r11                         // digits
    mov          rax, [rx1]                  // top limb
.Ltop_pair:
    mov          rdx, rax
    shr          rdx, 4
    jz           .Ltop_one                   // low nibble only
    mov          r8, rax
    and          r8, 0x0F
    movzx        edx, byte ptr [r10 + r8]    // lo nibble char
    mov          r8, rax
    shr          r8, 4
    and          r8, 0x0F
    movzx        r8d, byte ptr [r10 + r8]    // hi nibble char
    shl          r8, 8
    or           rdx, r8
    mov          [rx0], dx                   // lo first
    add          rx0, 2
    shr          rax, 8
    jmp          .Ltop_pair
.Ltop_one:                                   // rax in [0,15]
    test         rax, rax
    jz           .Lret
    movzx        edx, byte ptr [r10 + rax]
    mov          [rx0], dl
.Lret:
    vzeroupper
    mov          rax, r11                    // digits
    ret

.Lret0:
    xor          eax, eax
    ret

.section .rodata
.p2align 4
.Lhex_lower:
    .ascii "0123456789abcdef"
.Lhex_upper:
    .ascii "0123456789ABCDEF"
