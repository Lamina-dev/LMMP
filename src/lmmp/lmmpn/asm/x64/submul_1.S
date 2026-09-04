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
// mp_limb_t lmmp_submul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);
//
//   Windows: rp=rcx, up=rdx, n=r8, vl=r9
//   SysV:    rp=rdi, up=rsi, n=rdx, vl=rcx
//   ret rax = highest limb borrow
//   cal: rp[i] -= up[i]*vl

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_submul_1_)

ASM_GSYM(lmmp_submul_1_):
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

    xor     r9d, r9d                // borrow = 0
    xor     r14d, r14d              // zero
    test    r8, r8
    jz      .Lret

    mov     rcx, r8
    and     r8d, 3                  // leftover = n & 3
    shr     rcx, 2                  // groups = n >> 2

    test    r8, r8                  // leftover limbs
    jz      .Lmain
    xor     eax, eax                // CF = 0, OF = 0

.Lscalar:                           // one limb: rp[i] -= lo + borrow
    mulx    r12, rax, [rsi]         // p = up[i]*vl
    adcx    rax, r9                 // t = lo + borrow
    adcx    r12, r14                // borrow = hi + o1
    mov     r13, [rdi]              // rp[i]
    sub     r13, rax                // rp[i] -= t
    mov     [rdi], r13
    adcx    r12, r14                // borrow += o2
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

    adcx    rax, r9                 // s_0 = lo_0 + borrow
    adox    r12, r14
    adcx    rbx, r12                // s_1 = lo_1 + hi_0 + C
    adox    r13, r14
    adcx    rbp, r13                // s_2 = lo_2 + hi_1 + C
    adox    r15, r14
    adcx    r8, r15                 // s_3 = lo_3 + hi_2 + C
    adox    r11, r14

    adcx    r11, r14                // hi_3 += C, CF = 0

    mov     r12, [rdi]              // rp[0]
    sub     r12, rax                // rp[0] -= s_0
    mov     [rdi], r12
    mov     r13, [rdi+8]            // rp[1]
    sbb     r13, rbx                // rp[1] -= s_1 + b0
    mov     [rdi+8], r13
    mov     r15, [rdi+16]           // rp[2]
    sbb     r15, rbp                // rp[2] -= s_2 + b1
    mov     [rdi+16], r15
    mov     r10, [rdi+24]           // rp[3]
    sbb     r10, r8                 // rp[3] -= s_3 + b2
    mov     [rdi+24], r10

    adcx    r11, r14                // borrow = hi_3 + b3
    mov     r9, r11

    lea     rdi, [rdi+32]
    lea     rsi, [rsi+32]
    dec     rcx
    jnz     .Lloop4

.Lret:
    mov     rax, r9                 // ret borrow
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
