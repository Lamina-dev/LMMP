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
// mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);
//
//   Windows: numa=rcx, numb=rdx, n=r8, b=r9
//   SysV:    numa=rdi, numb=rsi, n=rdx, b=rcx
//
// Loop structure transcribed from GMP 6.3 mpn/x86_64/alderlake/addmul_1.asm.

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_addmul_1_
// ======================================
.globl ASM_GSYM(lmmp_addmul_1_)
.p2align 4
ASM_GSYM(lmmp_addmul_1_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    push    rbx
    push    r12
    push    r13
    mov     rdi, rcx                // SysV:
    mov     rsi, rdx                //   rdi=rp, rsi=up,
    mov     rdx, r9                 //   rdx=v0 (mulx implied)
    mov     rcx, r8                 //   rcx=n
#else
    push    rbx
    push    r12
    push    r13
    mov     rax, rdx
    mov     rdx, rcx
    mov     rcx, rax                // rcx=n, rdx=v0
#endif
    test    rcx, rcx
    jz      .Lret0

    mov     eax, ecx
    and     eax, 3                  // CF = OF = 0
    jz      .Lb0
    cmp     eax, 2
    jl      .Lb1
    je      .Lb2

.Lb3:                               // n%4 == 3
    mulx    r10, r11, [rsi]         // p0: r11=lo0, r10=hi0
    mulx    r12, r13, [rsi+8]       // p1: r13=lo1, r12=hi1
    mulx    rax, rbx, [rsi+16]      // p2: rbx=lo2,  rax=hi2
    inc     rcx
    lea     rsi, [rsi-8]
    lea     rdi, [rdi-24]
    jmp     .Llo3

.Lb0:                               // n%4 == 0
    mulx    r8,  r9,  [rsi]         // p0: r9=lo0,  r8=hi0
    mulx    r10, r11, [rsi+8]       // p1: r11=lo1, r10=hi1
    mulx    r12, r13, [rsi+16]      // p2: r13=lo2, r12=hi2
    lea     rdi, [rdi-16]
    jmp     .Llo0

.Lb2:                               // n%4 == 2
    mulx    r12, r13, [rsi]         // p0: r13=lo0, r12=hi0
    mulx    rax, rbx, [rsi+8]       // p1: rbx=lo1,  rax=hi1
    lea     rcx, [rcx-2]
    jrcxz   .Ln2
    mulx    r8,  r9,  [rsi+16]      // p2: r9=lo2, r8=hi2
    lea     rsi, [rsi+16]
    jmp     .Llo2
.Ln2:
    jmp     .Lwd2

.Lb1:                               // n%4 == 1
    mulx    rax, rbx, [rsi]         // p0: rbx=lo0, rax=hi0
    sub     rcx, 1                  // also clears CF (no borrow), OF
    jrcxz   .Ln1
    mulx    r8,  r9,  [rsi+8]       // p1: r9=lo1,  r8=hi1
    mulx    r10, r11, [rsi+16]      // p2: r11=lo2, r10=hi2
    lea     rsi, [rsi+8]
    lea     rdi, [rdi-8]
    jmp     .Llo1
.Ln1:
    add     rbx, [rdi]              // s0 = lo0 + rp[0]
    adc     rax, 0                  // carry = hi0 + CF
    mov     [rdi], rbx
    jmp     .Lfin

.p2align 5
.Ltop:
    mulx    r8,  r9,  [rsi]         // next p0
    adcx    r13, r10                // prod chain
    mov     [rdi-8], r11
.Llo2:
    adox    r13, [rdi]              // rp chain
    mulx    r10, r11, [rsi+8]
    adcx    rbx, r12
    mov     [rdi], r13
.Llo1:
    adox    rbx, [rdi+8]
    mulx    r12, r13, [rsi+16]
    adcx    r9, rax
    mov     [rdi+8], rbx
.Llo0:
    adox    r9, [rdi+16]
    mulx    rax, rbx, [rsi+24]
    adcx    r11, r8
    mov     [rdi+16], r9
.Llo3:
    adox    r11, [rdi+24]
    lea     rsi, [rsi+32]
    lea     rdi, [rdi+32]
    lea     rcx, [rcx-4]
    jrcxz   .Lend
    jmp     .Ltop

.Lend:
    adcx    r13, r10
    mov     [rdi-8], r11
.Lwd2:
    adox    r13, [rdi]
    adcx    rbx, r12
    mov     [rdi], r13
    adox    rbx, [rdi+8]
    adcx    rax, rcx                // rcx = 0 here: carry = hi + CF
    adox    rax, rcx                // carry += OF
    mov     [rdi+8], rbx

.Lfin:
    pop     r13
    pop     r12
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

.Lret0:
    xor     eax, eax
    pop     r13
    pop     r12
    pop     rbx
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret
