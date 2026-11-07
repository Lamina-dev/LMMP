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
//   Windows: x/a=rcx, b=rdx, dst/mod=r8, q=r9
//   SysV:    x/a=rdi, b=rsi, dst/mod=rdx, q=rcx

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_limb_bits_
// ======================================
.globl ASM_GSYM(lmmp_limb_bits_)
.p2align 4
ASM_GSYM(lmmp_limb_bits_):
    lzcnt   rax, rx0
    neg     rax
    add     rax, 64
    ret

// ======================================
// lmmp_leading_zeros_
// ======================================
.globl ASM_GSYM(lmmp_leading_zeros_)
.p2align 4
ASM_GSYM(lmmp_leading_zeros_):
    lzcnt   rax, rx0
    ret

// ======================================
// lmmp_tailing_zeros_
// ======================================
.globl ASM_GSYM(lmmp_tailing_zeros_)
.p2align 4
ASM_GSYM(lmmp_tailing_zeros_):
    tzcnt   rax, rx0
    ret

// ======================================
// lmmp_limb_popcnt_
// ======================================
.globl ASM_GSYM(lmmp_limb_popcnt_)
.p2align 4
ASM_GSYM(lmmp_limb_popcnt_):
    xor     rax, rax
    popcnt  rax, rx0
    ret

// ======================================
// lmmp_mulh_
// ======================================
.globl ASM_GSYM(lmmp_mulh_)
.p2align 4
ASM_GSYM(lmmp_mulh_):
#ifdef LMMP_WINDOWS
    mulx    rax, r11, rcx         // b in rdx
#else
    mov     rdx, rsi              // b -> rdx
    mulx    rax, r11, rdi
#endif
    ret

// ======================================
// lmmp_mullh_
// ======================================
.globl ASM_GSYM(lmmp_mullh_)
.p2align 4
ASM_GSYM(lmmp_mullh_):
#ifdef LMMP_WINDOWS
    mulx    r9, rax, rcx          // b in rdx, dst=r8
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
.globl ASM_GSYM(lmmp_mulmod_ulong_)
.p2align 4
ASM_GSYM(lmmp_mulmod_ulong_):
    mov     rax, rx0
    mov     r10, rx2
    mul     rx1
    div     r10
    mov     [rx3], rax
    mov     rax, rdx
    ret
