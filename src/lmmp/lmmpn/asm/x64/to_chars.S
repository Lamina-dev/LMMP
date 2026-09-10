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

//
// void lmmp_dec_to_chars_(mp_byte_t* dst, mp_size_t n);
// void lmmp_b36_to_chars_(mp_byte_t* dst, mp_size_t n, bool upper);
//
//   Windows: dst=rcx, n=rdx, upper=r8b
//   SysV:    dst=rdi, n=rsi, upper=dl
//   cal: dst[i] = '0' + dst[i] (+ adj*(dst[i]>9)),
//           adj = 39(lowercase)/7(capitals),
//

#include "lmmp_asm.h"

#ifdef LMMP_WINDOWS
#define rb2 r8b
#else
#define rb2 dl
#endif

.text

// ---------- lmmp_dec_to_chars_: [0,9] -> ['0','9'] ----------

.p2align 4

.globl ASM_GSYM(lmmp_dec_to_chars_)
ASM_GSYM(lmmp_dec_to_chars_):
    mov          eax, 48
    vmovd        xmm0, eax
    vpbroadcastb ymm0, xmm0         // ymm0 = '0'
    mov          rax, rx1
    shr          rax, 6             // rax = n >> 6
    jz           .Ldec_tail

.p2align 4
.Ldec_loop:                         // main loop, 64 bytes
    vmovdqu      ymm1, [rx0]
    vmovdqu      ymm2, [rx0 + 32]
    vpaddb       ymm1, ymm1, ymm0   // x + '0'
    vpaddb       ymm2, ymm2, ymm0
    vmovdqu      [rx0], ymm1
    vmovdqu      [rx0 + 32], ymm2
    add          rx0, 64
    dec          rax
    jnz          .Ldec_loop

.Ldec_tail:
    and          rx1, 63            // remainder bytes
    jz           .Ldec_ret

.Ldec_tail_loop:                    // remainder bytes
    add          byte ptr [rx0], 48
    inc          rx0
    dec          rx1
    jnz          .Ldec_tail_loop

.Ldec_ret:
    vzeroupper
    ret

// ---------- lmmp_b36_to_chars_: [0,35] -> ['0'-'9',a-z/A-Z] ----------

.p2align 4

.globl ASM_GSYM(lmmp_b36_to_chars_)
ASM_GSYM(lmmp_b36_to_chars_):
    xor          eax, eax
    test         rb2, rb2
    setne        al
    shl          eax, 5
    xor          eax, 39
    mov          r11d, eax          // r11 = adj,

    vmovd        xmm3, eax
    vpbroadcastb ymm3, xmm3         // ymm3 = adj
    mov          eax, 48
    vmovd        xmm0, eax
    vpbroadcastb ymm0, xmm0         // ymm0 = '0'
    mov          eax, 9
    vmovd        xmm4, eax
    vpbroadcastb ymm4, xmm4         // ymm4 = 9

    mov          rax, rx1
    shr          rax, 6             // rax = n >> 6
    jz           .Lb36_tail

.p2align 4
.Lb36_loop:                         // main loop, 64 bytes
    vmovdqu      ymm1, [rx0]
    vmovdqu      ymm2, [rx0 + 32]
    vpcmpgtb     ymm5, ymm1, ymm4   // mask = x > 9 (signed cmp, x>=0)
    vpand        ymm5, ymm5, ymm3   // mask & adj
    vpaddb       ymm1, ymm1, ymm0   // x + '0'
    vpaddb       ymm1, ymm1, ymm5   // + adj*(x>9)
    vpcmpgtb     ymm5, ymm2, ymm4   // ymm5 (in Win64, ymm6-15 is callee-saved)
    vpand        ymm5, ymm5, ymm3
    vpaddb       ymm2, ymm2, ymm0
    vpaddb       ymm2, ymm2, ymm5
    vmovdqu      [rx0], ymm1
    vmovdqu      [rx0 + 32], ymm2
    add          rx0, 64
    dec          rax
    jnz          .Lb36_loop

.Lb36_tail:
    and          rx1, 63            // remainder bytes
    jz           .Lb36_ret

.Lb36_tail_loop:                    // remainder bytes
    movzx        eax, byte ptr [rx0]
    lea          r10d, [rax + 48]
    cmp          eax, 9
    jbe          .Lb36_st
    add          r10d, r11d         // + adj
.Lb36_st:
    mov          [rx0], r10b
    inc          rx0
    dec          rx1
    jnz          .Lb36_tail_loop

.Lb36_ret:
    vzeroupper
    ret
