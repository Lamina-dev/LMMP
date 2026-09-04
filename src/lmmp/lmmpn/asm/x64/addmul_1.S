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
// mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);
//
//   Windows: rp=rcx, up=rdx, n=r8, vl=r9
//   SysV:    rp=rdi, up=rsi, n=rdx, vl=rcx
//   ret rax = highest limb carry
//   cal: rp[i] += up[i]*vl

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_addmul_1_)

ASM_GSYM(lmmp_addmul_1_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    mov     rdi, rcx                // SysV:
    mov     rsi, rdx                //   rdi=rp, rsi=up,
    mov     rdx, r9                 //   rdx=vl (mulx implied)
#else
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15
    mov     r8, rdx                 // r8 = n
    mov     rdx, rcx                // vl -> rdx
#endif

    xor     r9d, r9d                // carry = 0
    xor     r14d, r14d              // zero
    test    r8, r8
    jz      .Lret

    mov     rcx, r8
    and     r8d, 3                  // leftover = n & 3
    shr     rcx, 2                  // groups = n >> 2

    test    r8, r8                  // leftover limbs
    jz      .Lmain
    xor     eax, eax                // CF = 0, OF = 0

.Lscalar:                           // one limb: s = rp + lo + carry
    mulx    r12, rax, [rsi]         // p = up[i]*vl
    adcx    rax, [rdi]              // s = rp[i] + lo
    adox    rax, r9                 // s += carry
    mov     [rdi], rax
    adcx    r12, r14                // carry = hi + o1
    adox    r12, r14                // carry += o2
    mov     r9, r12
    lea     rdi, [rdi+8]
    lea     rsi, [rsi+8]
    dec     r8
    jnz     .Lscalar

.Lmain:
    test    rcx, rcx
    jz      .Lret
    xor     eax, eax                // CF = 0, OF = 0

.p2align 4
.Lloop4:                            // main loop, 4 limbs
    mulx    r12, rax, [rsi]         // lo_0, hi_0
    mulx    r13, rbx, [rsi+8]       // lo_1, hi_1
    mulx    r15, rbp, [rsi+16]      // lo_2, hi_2
    mulx    r11, r8,  [rsi+24]      // lo_3, hi_3

    adcx    rax, r9                 // s_0 = lo_0 + carry
    adox    r12, r14
    adcx    rbx, r12                // s_1 = lo_1 + hi_0 + C
    adox    r13, r14
    adcx    rbp, r13                // s_2 = lo_2 + hi_1 + C
    adox    r15, r14
    adcx    r8, r15                 // s_3 = lo_3 + hi_2 + C
    adox    r11, r14

    adcx    r11, r14                // hi_3 += C, CF = 0

    add     rax, [rdi]              // s_0 += rp[0]
    mov     [rdi], rax
    adcx    rbx, [rdi+8]            // s_1 += rp[1]
    mov     [rdi+8], rbx
    adcx    rbp, [rdi+16]           // s_2 += rp[2]
    mov     [rdi+16], rbp
    adcx    r8, [rdi+24]            // s_3 += rp[3]
    mov     [rdi+24], r8

    adcx    r11, r14                // carry = hi_3 + C
    mov     r9, r11

    lea     rdi, [rdi+32]
    lea     rsi, [rsi+32]
    dec     rcx
    jnz     .Lloop4

.Lret:
    mov     rax, r9                 // ret carry
#ifdef LMMP_WINDOWS
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
    pop     rdi
    pop     rsi
#else
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#endif
    ret
