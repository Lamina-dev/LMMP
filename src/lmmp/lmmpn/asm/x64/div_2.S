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
// void lmmp_div_2_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_ptr numb);
//
// mp_limb_t lmmp_div_2_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_srcptr numb);

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_div_2_
// ======================================
// Windows: dstq=rcx, numa=rdx, na=r8, numb=r9
// SysV:    dstq=rdi, numa=rsi, na=rdx, numb=rcx
// ret: [numb,2] = remainder; if dstq != NULL, [dstq,na-1] = quotient
// cal: normalize b, m = (2^192-1) div b - 2^64, per limb q = (m*ah_hi + ah_lo + ah_hi)>>64, z = a - q*b, correct
.globl ASM_GSYM(lmmp_div_2_)
.p2align 4
ASM_GSYM(lmmp_div_2_):
    test    rx0, rx0
    jz      .Lmod                   // dstq == NULL, mod only

#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
#endif
    push    rbp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    rx3                     // save numb

    lea     rbx, [rx2-1]            // rbx = na-1
#ifdef LMMP_WINDOWS
    mov     rdi, rcx                // rdi = dstq
    mov     rsi, rdx                // rsi = numa
#endif
    mov     r8, [rx3]               // divisor low
    mov     r9, [rx3+8]             // divisor high

    // top compare: a vs b
    mov     r11, [rsi+rbx*8-8]      // second top limb
    mov     r12, [rsi+rbx*8]        // top limb
    xor     r13, r13
    cmp     r12, r9
    ja      .L2
    setb    dl
    cmp     r11, r8
    setb    al
    or      dl, al
    jz      .L2

    mov     [rdi+rbx*8-8], r13      // qh = 0
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
#ifndef LMMP_WINDOWS
    mov     r10, rdi                // save dstq across call
#endif
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

#ifndef LMMP_WINDOWS
    mov     rdi, r10                // restore dstq
#endif
    pop     r9
    pop     r8
    mov     rbp, rax                // rbp = inv - 2^64

    // refine m: iterate until m = (2^192-1) div b - 2^64
    mov     r10, r9                 // rcx:r10:rax = b*inv(bh)
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

.Llast:                             // r13:r12 = ah < b, r11 = a1, b = r9:r8
    mov     rax, rbp
    mul     r13                     // rdx:rax = m * ah_hi
    mov     r14, r12
    add     r14, rax
    adc     rdx, r13                // rdx:r14 = q = m*ah_hi + ah_lo + ah_hi
    mov     r13, r14                // r13 = q_lo
    mov     r14, rdx                // r14 = qh
    imul    rdx, r9                 // rdx = qh*bh
    xchg    r13, r12                // r12 = q_lo, r13 = a1
    mov     rax, r8
    sub     r13, rdx                // a1 - qh*bh
    xchg    r12, r11                // r11 = q_lo, r12 = a0
    mul     r14                     // rdx:rax = qh*bl
    sub     r12, r8
    sbb     r13, r9                 // pre-subtract one b
    sub     r12, rax
    sbb     r13, rdx                // r13:r12 = z = a - q*b
    xor     rax, rax
    xor     rdx, rdx
    cmp     r13, r11
    cmovnc  rax, r8                 // borrow: z += b
    cmovnc  rdx, r9
    adc     r14, 0                  // borrow: qh--
    add     r12, rax
    adc     r13, rdx
    cmp     r13, r9
    jae     .Lfix                   // z >= b, correct

.Lok:
    mov     [rdi+rbx*8], r14         // store qh
    mov     r11, r10

.Lent:
    sub     rbx, 1
    ja      .Ltop
    jb      .Lret
    shl     r11, cl                 // last limb << k
    jmp     .Llast

.Lfix:                              // z >= b: z -= b, qh++
    seta    dl
    cmp     r12, r8
    setae   al
    or      al, dl
    jz      .Lok                    // z < b, no fix
    inc     r14
    sub     r12, r8
    sbb     r13, r9
    jmp     .Lok

.Lret:                              // remainder >>= k
    shrd    r12, r13, cl
    shr     r13, cl

.Lretnsh:
    pop     rx3
    mov     [rx3], r12              // store remainder
    mov     [rx3+8], r13
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

// dstq == NULL: tail call mod_2
.Lmod:
#ifdef LMMP_WINDOWS
    mov     rcx, rdx                // remap mod_2 args
    mov     rdx, r8
    mov     r8, r9
    sub     rsp, 40                 // align rsp
    CALL(lmmp_mod_2_)
    add     rsp, 40
#else
    mov     rdi, rsi                // remap mod_2 args
    mov     rsi, rdx
    mov     rdx, rcx
    sub     rsp, 8
    CALL(lmmp_mod_2_)
    add     rsp, 8
#endif
    ret

// ======================================
// lmmp_div_2_s_
// ======================================
// Windows: dstq=rcx, numa=rdx, na=r8, numb=r9
// SysV:    dstq=rdi, numa=rsi, na=rdx, numb=rcx
// ret rax = top quotient qh; qh:[dstq,na-2] = quotient, [numa,2] = remainder
// cal: b normalized (MSB(bh)=1), m = (2^192-1) div b - 2^64, same q estimate and correction as div_2_
.globl ASM_GSYM(lmmp_div_2_s_)
.p2align 4
ASM_GSYM(lmmp_div_2_s_):
    push    rbp
    push    rbx
    push    r12
    push    r13
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
#endif

    lea     rbx, [rx2-1]
#ifdef LMMP_WINDOWS
    mov     rdi, rcx
    mov     rsi, rdx
#endif
    mov     r12, [rx3]              // divisor low
    mov     r13, [rx3+8]            // divisor high
#ifndef LMMP_WINDOWS
    mov     r10, rdi                // save dstq
#endif
    mov     rx0, r13

#ifdef LMMP_WINDOWS
    sub     rsp, 40                 // align + shadow space
#else
    sub     rsp, 8
#endif
    CALL(lmmp_inv_1_)
#ifdef LMMP_WINDOWS
    add     rsp, 40
#else
    add     rsp, 8
#endif

#ifndef LMMP_WINDOWS
    mov     rdi, r10
#endif
    mov     r9, r13                 // r9 = bh
    mov     r8, r12                 // r8 = bl, b = r9:r8
    mov     rbp, rax

    // refine m as div_2_
    imul    r13, rax
    xor     rcx, rcx
    mul     r8
    add     r13, r8
    adc     rcx, -1
    add     r13, rdx
    adc     rcx, 0
    js      .L2f_s

.L1b_s:
    dec     rbp
    sub     rax, r8
    sbb     r13, r9
    sbb     rcx, 0
    jns     .L1b_s

.L2f_s:
    xor     rcx, rcx                // rcx = top qh
    mov     rax, [rsi+rbx*8-8]
    mov     rdx, [rsi+rbx*8]        // top two limbs
    mov     r12, rax
    mov     r13, rdx
    sub     r12, r8
    sbb     r13, r9
    setnc   cl                      // qh = (a_top >= b)
    cmovc   r12, rax
    cmovc   r13, rdx                // r13:r12 = ah < b
    dec     rbx

.p2align 4
.Ltop_s:                            // r13:r12 = ah < b, as div_2_
    mov     r11, [rsi+rbx*8-8]      // a_lo = next limb
    mov     rax, rbp
    mul     r13                     // rdx:rax = m * ah_hi
    mov     r10, r12
    add     r10, rax
    adc     rdx, r13                // rdx:r10 = q
    mov     r13, r10                // r13 = q_lo
    mov     r10, rdx                // r10 = qh
    imul    rdx, r9
    xchg    r13, r12
    mov     rax, r8
    sub     r13, rdx
    xchg    r12, r11                // r11 = q_lo
    mul     r10
    sub     r12, r8
    sbb     r13, r9
    sub     r12, rax
    sbb     r13, rdx                // r13:r12 = z = a - q*b
    xor     rax, rax
    xor     rdx, rdx
    cmp     r13, r11
    cmovnc  rax, r8                 // borrow: z += b, qh--
    cmovnc  rdx, r9
    adc     r10, 0
    add     r12, rax
    adc     r13, rdx
    cmp     r13, r9
    jae     .Lfix_s

.Lok_s:
    mov     [rdi+rbx*8-8], r10      // store qh
    dec     rbx
    jnz     .Ltop_s
    mov     [rsi], r12              // [numa,2] = remainder
    mov     [rsi+8], r13
    mov     rax, rcx                // ret qh
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

.Lfix_s:
    seta    dl
    cmp     r12, r8
    setae   al
    or      al, dl
    jz      .Lok_s
    inc     r10
    sub     r12, r8
    sbb     r13, r9
    jmp     .Lok_s
