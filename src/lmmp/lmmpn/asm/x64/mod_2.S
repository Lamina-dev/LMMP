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
// void lmmp_mod_2_(mp_srcptr numa, mp_size_t na, mp_ptr numb);
//
//   Windows: numa=rcx, na=rdx, numb=r8
//   SysV:    numa=rdi, na=rsi, numb=rdx
//   ret: [numb,2] = [numa,na] mod [numb,2]
//   cal: normalize b, m = (2^192-1) div b - 2^64, per limb q = (m*ah_hi + ah_lo + ah_hi)>>64, z = a - q*b, correct

#include "lmmp_asm.h"

.text
.globl ASM_GSYM(lmmp_mod_2_)

.p2align 4
ASM_GSYM(lmmp_mod_2_):
    push    rbp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15                     // r15 = numb

#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
#endif

#ifdef LMMP_WINDOWS
    mov     r15, r8                 // numb
    mov     rsi, rcx                // numa
    mov     rbx, rdx                // na
#else
    mov     r15, rdx                // numb
    mov     rbx, rsi                // na
    mov     rsi, rdi                // numa
#endif
    lea     rbx, [rbx-1]            // rbx = na - 1

    mov     r8, [r15]               // divisor low
    mov     r9, [r15+8]             // divisor high

    // top compare: a vs b
    mov     r11, [rsi+rbx*8-8]
    mov     r12, [rsi+rbx*8]
    xor     r13, r13
    cmp     r12, r9
    ja      .L2
    setb    dl
    cmp     r11, r8
    setb    al
    or      dl, al
    jz      .L2
    dec     rbx
    mov     r13, r12
    mov     r12, r11
    jz      .Lretnsh
    mov     r11, [rsi+rbx*8-8]

.L2:                                // normalize divisor
    bsr     rcx, r9
    not     rcx
    add     rcx, 64
    shld    r9, r8, cl
    shl     r8, cl

    push    rcx                     // save shift k
    push    r8
    push    r9
    mov     rx0, r9                 // arg b_hi

#ifdef LMMP_WINDOWS
    sub     rsp, 32                 // shadow space
#endif
    CALL(lmmp_inv_1_)
#ifdef LMMP_WINDOWS
    add     rsp, 32
#endif

    pop     r9
    pop     r8
    mov     rbp, rax                // rbp = inv - 2^64

    // refine m: iterate until m = (2^192-1) div b - 2^64
    mov     r10, r9
    imul    r10, rax
    xor     rcx, rcx
    mul     r8
    add     r10, r8
    adc     rcx, -1
    add     r10, rdx
    adc     rcx, 0
    js      .L2f

.L1b:                               // m -= 1, r -= b
    dec     rbp
    sub     rax, r8
    sbb     r10, r9
    sbb     rcx, 0
    jns     .L1b

.L2f:
    pop     rcx                     // restore k
    shld    r13, r12, cl            // top two limbs << k
    shld    r12, r11, cl
    jmp     .Lent

.p2align 4
.Ltop:
    mov     r10, [rsi+rbx*8-8]      // next lower limb
    shld    r11, r10, cl            // r11 = a1 << k

.Llast:                             // r13:r12 = ah < b, b = r9:r8
    mov     rax, rbp
    mul     r13                     // rdx:rax = m * ah_hi
    mov     r14, r12
    add     r14, rax
    adc     rdx, r13                // rdx = q
    mov     r13, r14                // r13 = q_lo
    mov     r14, rdx
    imul    rdx, r9                 // rdx = q*bh
    xchg    r13, r12                // r12 = q_lo, r13 = a1
    mov     rax, r8
    sub     r13, rdx                // a1 - q*bh
    xchg    r12, r11                // r11 = q_lo, r12 = a0
    mul     r14                     // rdx:rax = q*bl
    sub     r12, r8
    sbb     r13, r9                 // pre-subtract one b
    sub     r12, rax
    sbb     r13, rdx                // r13:r12 = z = a - q*b
    xor     rax, rax
    xor     rdx, rdx
    cmp     r13, r11
    cmovnc  rax, r8                 // no borrow: z += b
    cmovnc  rdx, r9

    add     r12, rax
    adc     r13, rdx
    cmp     r13, r9
    jae     .Lfix                   // z >= b: subtract again

.Lok:
    mov     r11, r10

.Lent:
    sub     rbx, 1
    ja      .Ltop
    jb      .Lret
    shl     r11, cl                 // last limb << k
    jmp     .Llast

.Lfix:                              // 128-bit compare z >= b: z -= b
    seta    dl
    cmp     r12, r8
    setae   al
    or      al, dl
    jz      .Lok                    // z < b, no fix

    sub     r12, r8
    sbb     r13, r9
    jmp     .Lok

.Lret:                              // remainder >>= k
    shrd    r12, r13, cl
    shr     r13, cl

.Lretnsh:
    mov     [r15], r12              // store remainder
    mov     [r15+8], r13

#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret
