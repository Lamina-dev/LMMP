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
// int lmmp_limb_bits_(mp_limb_t x);
//
// int lmmp_limb_popcnt_(mp_limb_t x);
//
// int lmmp_leading_zeros_(mp_limb_t x);
//
// int lmmp_tailing_zeros_(mp_limb_t x);
//
// mp_limb_t lmmp_mulh_(mp_limb_t a, mp_limb_t b);
//
// void lmmp_mullh_(mp_limb_t a, mp_limb_t b, mp_ptr dst);
//
// ulong lmmp_mulmod_ulong_(ulong a, ulong b, ulong mod, ulongp q);
//
//   Windows: a(mod)=rcx, b=rdx, numb/dst=r8, n/q=r9; ret rax
//   SysV:    a(mod)=rdi, b=rsi, numb/dst=rdx, n/q=rcx; ret rax
//   cal: bit/popcount helpers, mul high, mul store, div-based mulmod

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_limb_bits_
// ======================================
// ret rax = 64 - lzcnt(x)
.p2align 4
.globl ASM_GSYM(lmmp_limb_bits_)
ASM_GSYM(lmmp_limb_bits_):
    lzcnt   rax, rx0
    neg     rax
    add     rax, 64
    ret

// ======================================
// lmmp_leading_zeros_
// ======================================
// ret rax = lzcnt(x), 64 if x == 0
.p2align 4
.globl ASM_GSYM(lmmp_leading_zeros_)
ASM_GSYM(lmmp_leading_zeros_):
    lzcnt   rax, rx0
    ret

// ======================================
// lmmp_tailing_zeros_
// ======================================
// ret rax = tzcnt(x), 64 if x == 0
.p2align 4
.globl ASM_GSYM(lmmp_tailing_zeros_)
ASM_GSYM(lmmp_tailing_zeros_):
    tzcnt   rax, rx0
    ret

// ======================================
// lmmp_limb_popcnt_
// ======================================
// ret rax = popcount(x)
.p2align 4
.globl ASM_GSYM(lmmp_limb_popcnt_)
ASM_GSYM(lmmp_limb_popcnt_):
    xor     rax, rax
    popcnt  rax, rx0
    ret

// ======================================
// lmmp_mulh_
// ======================================
// ret rax = (a*b) >> 64
.p2align 4
.globl ASM_GSYM(lmmp_mulh_)
ASM_GSYM(lmmp_mulh_):
#ifdef LMMP_WINDOWS
    mulx    rax, r11, rcx         // b 已在 rdx (mulx 隐式乘数)
#else
    mov     rdx, rsi              // b -> rdx
    mulx    rax, r11, rdi
#endif
    ret

// ======================================
// lmmp_mullh_
// ======================================
// dst[0] = a*b low, dst[1] = high
.p2align 4
.globl ASM_GSYM(lmmp_mullh_)
ASM_GSYM(lmmp_mullh_):
#ifdef LMMP_WINDOWS
    mulx    r9, rax, rcx          // b 已在 rdx, dst=r8 不被 mulx 碰
    mov     [r8], rax
    mov     [r8+8], r9
#else
    mov     r10, rdx              // save dst
    mov     rdx, rsi              // b -> rdx
    mulx    r9, rax, rdi
    mov     [r10], rax
    mov     [r10+8], r9
#endif
    ret

// ======================================
// lmmp_mulmod_ulong_
// ======================================
// *q = (a*b)/mod, ret rax = remainder
// caller must ensure quotient fits 64 bits
.p2align 4
.globl ASM_GSYM(lmmp_mulmod_ulong_)
ASM_GSYM(lmmp_mulmod_ulong_):
    mov     rax, rx0
    mov     r10, rx2
    mul     rx1
    div     r10
    mov     [rx3], rax
    mov     rax, rdx
    ret
