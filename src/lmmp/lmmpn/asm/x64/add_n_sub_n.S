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
// mp_limb_t lmmp_add_n_sub_n_(mp_ptr dsta, mp_ptr dstb, mp_srcptr numa, mp_srcptr numb,
//                             mp_size_t n);
//
//   Windows: dsta=rcx, dstb=rdx, numa=r8, numb=r9, n=[rsp+40]
//   SysV:    dsta=rdi, dstb=rsi, numa=rdx, numb=rcx, n=r8

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_add_n_sub_n_
// ======================================
.globl ASM_GSYM(lmmp_add_n_sub_n_)
.p2align 4
ASM_GSYM(lmmp_add_n_sub_n_):
#ifdef LMMP_WINDOWS
    mov     rax, [rsp+40]         // rax = n
    push    rbx
    push    rsi
    push    rdi
    mov     rdi, rcx              // rdi = dsta
    mov     rsi, rdx              // rsi = dstb
    mov     rdx, r8               // rdx = numa
    mov     r8,  r9               // r8  = numb
    mov     rcx, rax              // rcx = n
#else
    push    rbx
    mov     rax, r8               // rax = n
    mov     r8,  rcx              // r8  = numb
    mov     rcx, rax              // rcx = n
#endif
    shr     rcx, 2                // rcx = n/4
    and     eax, 3                // rax = n mod 4
    mov     rbx, rax              // rbx = n mod 4
    test    rcx, rcx
    jz      .Lsmall               // n < 4

    mov     r10, 0x4000000000000000
    add     r10, r10              // OF = 1, CF = 0

.p2align 4
.Lloop:                           // main loop, 4 limbs
    mov     r9,  [rdx]            // r9  = a0
    mov     r10, [r8]             // r10 = b0
    mov     r11, r9               // r11 = a0
    adcx    r9,  r10              // s0 = a0 + b0 + CF
    not     r10                   // r10 = ~b0
    adox    r11, r10              // d0 = a0 + ~b0 + OF
    mov     [rdi], r9
    mov     [rsi], r11

    mov     r9,  [rdx+8]
    mov     r10, [r8+8]
    mov     r11, r9
    adcx    r9,  r10
    not     r10
    adox    r11, r10
    mov     [rdi+8], r9
    mov     [rsi+8], r11

    mov     r9,  [rdx+16]
    mov     r10, [r8+16]
    mov     r11, r9
    adcx    r9,  r10
    not     r10
    adox    r11, r10
    mov     [rdi+16], r9
    mov     [rsi+16], r11

    mov     r9,  [rdx+24]
    mov     r10, [r8+24]
    mov     r11, r9
    adcx    r9,  r10
    not     r10
    adox    r11, r10
    mov     [rdi+24], r9
    mov     [rsi+24], r11

    lea     rdx, [rdx+32]
    lea     r8,  [r8+32]
    lea     rdi, [rdi+32]
    lea     rsi, [rsi+32]
    lea     rcx, [rcx-1]
    jrcxz   .Ltail
    jmp     .Lloop

.Lsmall:                          // n < 4
    mov     r10, 0x4000000000000000
    add     r10, r10              // OF = 1, CF = 0

.Ltail:                           // tail: n mod 4 limbs
    mov     rcx, rbx
.Ltail_chk:
    jrcxz   .Lend
    mov     r9,  [rdx]
    mov     r10, [r8]
    mov     r11, r9
    adcx    r9,  r10
    not     r10
    adox    r11, r10
    mov     [rdi], r9
    mov     [rsi], r11
    lea     rdx, [rdx+8]
    lea     r8,  [r8+8]
    lea     rdi, [rdi+8]
    lea     rsi, [rsi+8]
    lea     rcx, [rcx-1]
    jmp     .Ltail_chk

.Lend:
    setc    al                    // al = carry
    setno   cl                    // cl = !OF = borrow
    movzx   eax, al
    movzx   ecx, cl
    lea     eax, [rax*2+rcx]      // rax = 2*carry + borrow

#ifndef LMMP_WINDOWS
    pop     rbx
#else
    pop     rdi
    pop     rsi
    pop     rbx
#endif
    ret
