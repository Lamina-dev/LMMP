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
// mp_limb_t lmmp_submul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);
//
//   Windows: numa=rcx, numb=rdx, n=r8, b=r9
//   SysV:    numa=rdi, numb=rsi, n=rdx, b=rcx
//
// Loop structure transcribed from GMP 6.3 mpn/x86_64/alderlake/submul_1.asm.

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_submul_1_
// ======================================
.globl ASM_GSYM(lmmp_submul_1_)
.p2align 4
ASM_GSYM(lmmp_submul_1_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    mov     rdi, rcx                // SysV:
    mov     rsi, rdx                //   rdi=rp, rsi=up,
    mov     rdx, r9                 //   rdx=v0 (mulx implied)
    mov     rcx, r8                 //   rcx=n
#else
    mov     rax, rdx
    mov     rdx, rcx
    mov     rcx, rax                // rcx=n, rdx=v0
#endif
    test    rcx, rcx
    jz      .Lret0

    test    ecx, 1
    jz      .Lbx0

.Lbx1:
    mulx    rax, r9, [rsi]          // p0: r9=lo0, rax=hi0
    test    ecx, 2
    stc                             // CF=1 (complement chain +1), OF=0
    jz      .Lb01

.Lb11:                              // n%4 == 3
    lea     rcx, [rcx+1]
    lea     rsi, [rsi+16]
    lea     rdi, [rdi+16]
    jmp     .Llo3

.Lb01:                              // n%4 == 1
    lea     rcx, [rcx+3]
    jmp     .Llo1

.Lbx0:
    mulx    r8,  r9, [rsi]          // p0: r9=lo0, r8=hi0
    test    ecx, 2
    stc                             // CF=1, OF=0
    jz      .Lb00

.Lb10:                              // n%4 == 2
    lea     rsi, [rsi+8]
    lea     rdi, [rdi+8]
    lea     rcx, [rcx+2]
    jmp     .Llo2

.Lb00:                              // n%4 == 0
    lea     rsi, [rsi+24]
    lea     rdi, [rdi+24]
    jmp     .Llo0

.p2align 5
.Ltop:
    lea     rsi, [rsi+32]
    lea     rdi, [rdi+32]
    mulx    r8,  r9, [rsi-24]       // p(k)
    adox    r9,  rax                // s = lo + hi(k-1) + OF
.Llo0:
    not     r9                      // complement of s
    adcx    r9,  [rdi-24]           // rp + ~s + CF  (CF = 1-borrow)
    mov     [rdi-24], r9
    mulx    rax, r9, [rsi-16]       // p(k+1)
    adox    r9,  r8
.Llo3:
    not     r9
    adcx    r9,  [rdi-16]
    mov     [rdi-16], r9
    mulx    r8,  r9, [rsi-8]        // p(k+2)
    adox    r9,  rax
.Llo2:
    not     r9
    adcx    r9,  [rdi-8]
    mov     [rdi-8], r9
    mulx    rax, r9, [rsi]          // p(k+3)
    adox    r9,  r8
.Llo1:
    not     r9
    adcx    r9,  [rdi]
    mov     [rdi], r9
    lea     rcx, [rcx-4]
    jrcxz   .Lend
    jmp     .Ltop

.Lend:
    adox    rax, rcx                // T = hi(last) + OF   (rcx = 0)
    sbb     rax, -1                 // ret = T + 1 - CF = T + borrow

#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

.Lret0:
    xor     eax, eax
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret
