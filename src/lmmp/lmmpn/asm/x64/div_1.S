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
// mp_limb_t lmmp_div_1_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_limb_t x);
//
// mp_limb_t lmmp_div_1_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_limb_t x);

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_div_1_
// ======================================
// Windows: dstq=rcx, numa=rdx, na=r8, x=r9
// SysV:    dstq=rdi, numa=rsi, na=rdx, x=rcx
// ret rax = remainder [numa,na] mod x; if dstq != NULL, [dstq,na] = quotient
// cal: normalize d = x<<k, m = inv(d), per limb q = (xh*m + a_lo + xh+1)>>64, z = a - q*d, correct
.globl ASM_GSYM(lmmp_div_1_)
.p2align 4
ASM_GSYM(lmmp_div_1_):
    test    rx0, rx0
    jz      .Lmod                   // dstq == NULL, mod only

#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
#endif
    push    rbp
    push    rbx
    push    r12

    mov     rbx, rx2                // rbx = na
#ifdef LMMP_WINDOWS
    mov     rdi, rcx                // rdi = dstq
    mov     rsi, rdx                // rsi = numa
#endif
    mov     r8, rx3                 // r8 = x

    xor     eax, eax
    bsr     rcx, r8
    not     rcx
    add     rcx, 64
    shl     r8, cl                  // normalize d = x << k

#ifndef LMMP_WINDOWS
    mov     r10, rdi                // save dstq across call
#endif
    mov     r11, rax                // r11 = qh = 0
    mov     rbp, rcx                // rbp = k = clz(x)
    mov     r12, r8                 // r12 = d
    mov     rx0, r8                 // arg d

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
    mov     r9, rax                 // r9 = m = inv(d) - 2^64
    mov     r8, r12                 // r8 = d
    mov     rcx, rbp                // cl = k
    mov     rax, r11                // rax = 0

    mov     r10, [rsi+rbx*8-8]      // top limb
    shld    rax, r10, cl            // rax = top limb << k
    jmp     .Lent

.p2align 4
.Ltop:
    mov     r12, [rsi+rbx*8-8]      // next lower limb
    shld    r10, r12, cl            // r10 = a_lo << k
.Llast:                             // rax = xh < d, rbp = xh+1, r10 = a_lo
    mul     r9                      // rdx:rax = xh * m
    add     rax, r10
    adc     rdx, rbp                // rdx = q = (xh*m + a_lo + xh+1) >> 64
    mov     rbp, rax                // rbp = q_lo
    mov     r11, rdx                // r11 = qh
    imul    rdx, r8                 // rdx = qh*d
    sub     r10, rdx                // z = a_lo - qh*d
    lea     rax, [r8+r10]           // candidate z+d
    cmp     r10, rbp
    cmovc   rax, r10                // borrow: pick z, else +d
    adc     r11, -1                 // borrow: qh--
    cmp     rax, r8
    jae     .Lfix                   // z >= d, correct

.Lok:
    mov     [rdi+rbx*8], r11        // store qh
    mov     r10, r12

.Lent:
    sub     rbx, 1
    lea     rbp, [rax+1]            // next xh+1
    ja      .Ltop
    jb      .Lret                   // done

    shl     r10, cl                 // last limb << k
    jmp     .Llast

.Lfix:                              // z -= d, qh++
    sub     rax, r8
    inc     r11
    jmp     .Lok

.Lret:
    shr     rax, cl                 // remainder >> k
    pop     r12
    pop     rbx
    pop     rbp
#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    ret

// dstq == NULL: tail call mod
.Lmod:
#ifdef LMMP_WINDOWS
    mov     rcx, rdx                // remap mod_1 args
    mov     rdx, r8
    mov     r8, r9
    sub     rsp, 40                 // align rsp
    CALL(lmmp_mod_1_)
    add     rsp, 40
#else
    mov     rdi, rsi                // remap mod_1 args
    mov     rsi, rdx
    mov     rdx, rcx
    sub     rsp, 8
    CALL(lmmp_mod_1_)
    add     rsp, 8
#endif
    ret

// ======================================
// lmmp_div_1_s_
// ======================================
// Windows: dstq=rcx, numa=rdx, na=r8, x=r9
// SysV:    dstq=rdi, numa=rsi, na=rdx, x=rcx
// ret rax = top quotient qh; [dstq,na-1] = quotient, [numa,1] = remainder
// cal: d normalized (MSB=1), m = inv(d), same q estimate and correction as div_1_
.globl ASM_GSYM(lmmp_div_1_s_)
.p2align 4
ASM_GSYM(lmmp_div_1_s_):
    push    rbp
    push    rbx
#ifdef LMMP_WINDOWS
    push    rdi
    push    rsi
#endif

    mov     rbx, rx2
#ifdef LMMP_WINDOWS
    mov     rdi, rcx
    mov     rsi, rdx
#else
    mov     r10, rdi                // save dstq
#endif
    mov     rbp, rx3
    mov     rx0, rx3                // arg x

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
    mov     r8, rbp                 // r8 = d = x
    mov     r9, rax                 // r9 = m
    xor     rcx, rcx                // rcx = top qh
    mov     r10, [rsi+rbx*8-8]      // top limb
    mov     rax, r10
    sub     rax, r8
    setnc   cl                      // qh = (a_top >= d)
    cmovc   rax, r10                // rax = xh = a_top mod d
    dec     rbx

.p2align 4
.Lstop:                             // rax = xh < d, rbp = xh+1
    lea     rbp, [rax+1]
    mov     r10, [rsi+rbx*8-8]      // a_lo = next limb
    mul     r9                      // rdx:rax = xh * m
    add     rax, r10
    adc     rdx, rbp                // rdx = q
    mov     rbp, rax                // rbp = q_lo
    mov     r11, rdx                // r11 = qh
    imul    rdx, r8                 // rdx = qh*d
    sub     r10, rdx                // z = a_lo - qh*d
    lea     rax, [r8+r10]
    cmp     r10, rbp
    cmovc   rax, r10                // borrow fix as div_1_
    adc     r11, -1
    cmp     rax, r8
    jae     .Lsfx

.Lsok:
    mov     [rdi+rbx*8-8], r11      // store qh
    dec     rbx
    jnz     .Lstop
    mov     [rsi], rax              // [numa] = remainder
    mov     rax, rcx                // ret qh

#ifdef LMMP_WINDOWS
    pop     rsi
    pop     rdi
#endif
    pop     rbx
    pop     rbp
    ret

.Lsfx:                              // z -= d, qh++
    sub     rax, r8
    inc     r11
    jmp     .Lsok
