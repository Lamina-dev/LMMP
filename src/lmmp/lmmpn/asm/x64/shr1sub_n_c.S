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
//  This program is distributed in WITHOUT ANY WARRANTY.
//
//  See <https://www.gnu.org/licenses/>.

//
// mp_limb_t lmmp_shr1sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
// mp_limb_t lmmp_shr1sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
//
//   Windows: dst=rcx, numa=rdx, numb=r8, n=r9, c=[rsp+40] (only nc_)
//   SysV:    dst=rdi, numa=rsi, numb=rdx, n=rcx, c=r8 (only nc_)
//   ret rax = t0 & 1, shifted out bit
//   cal: t_i = numa[i] - numb[i] - CF; dst[i-1] = (t_{i-1}>>1)|(t_i<<63)

#include "lmmp_asm.h"

.text

.p2align 4

// ======================================
// lmmp_shr1sub_nc_
// ======================================
.globl ASM_GSYM(lmmp_shr1sub_nc_)
ASM_GSYM(lmmp_shr1sub_nc_):
#ifdef LMMP_WINDOWS
    mov     rax, [rsp+40]       // rax = c
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    push    r12
    push    r13
    mov     rdi, rcx            // SysV:
    mov     rsi, rdx            //   rdi=dst, rsi=numa,
    mov     rdx, r8             //   rdx=numb, rcx=n
    mov     rcx, r9
#else
    push    rbx
    push    rbp
    push    r12
    push    r13
    mov     rax, r8             // rax = c
#endif
    neg     rax                 // CF = (c != 0)
    jmp     .Lentry

// ======================================
// lmmp_shr1sub_n_
// ======================================
.globl ASM_GSYM(lmmp_shr1sub_n_)
ASM_GSYM(lmmp_shr1sub_n_):
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    push    r12
    push    r13
    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    mov     rcx, r9
#else
    push    rbx
    push    rbp
    push    r12
    push    r13
#endif
    clc                         // CF = 0

.Lentry:
    sbb     rbp, rbp            // rbp = -CF, save borrow in
    test    rcx, rcx
    jz      .Lret0              // n == 0

    dec     rcx                 // remaining = n - 1
    mov     r9, rcx
    and     r9d, 3              // tail = remaining & 3
    shr     rcx, 2              // groups = remaining >> 2

    add     rbp, rbp            // restore CF

    mov     r10, [rsi]          // numa[0]
    sbb     r10, [rdx]          // t0 = numa[0] - numb[0] - CF
    sbb     rbp, rbp            // capture -borrow out
    mov     ebx, r10d
    and     ebx, 1              // ret_lsb = t0 & 1
    mov     r8, r10             // prev = t0
    lea     rsi, [rsi+8]
    lea     rdx, [rdx+8]

    test    rcx, rcx
    jz      .Ltail              // groups == 0

.p2align 4
.Lgroup_loop:                   // main loop : 4 limb
    add     rbp, rbp            // restore CF
    mov     r10, [rsi]
    mov     r11, [rsi+8]
    mov     r12, [rsi+16]
    mov     r13, [rsi+24]
    sbb     r10, [rdx]          // t_i
    sbb     r11, [rdx+8]        // t_i+1
    sbb     r12, [rdx+16]       // t_i+2
    sbb     r13, [rdx+24]       // t_i+3
    sbb     rbp, rbp            // capture -borrow out
    shrd    r8,  r10, 1         // dst[i-1] = (t_{i-1}>>1)|(t_i<<63)
    shrd    r10, r11, 1         // dst[i]
    shrd    r11, r12, 1         // dst[i+1]
    shrd    r12, r13, 1         // dst[i+2]
    mov     [rdi],    r8
    mov     [rdi+8],  r10
    mov     [rdi+16], r11
    mov     [rdi+24], r12
    mov     r8, r13             // prev = t_i+3
    lea     rdi, [rdi+32]
    lea     rsi, [rsi+32]
    lea     rdx, [rdx+32]
    dec     rcx
    jnz     .Lgroup_loop

.Ltail:
    test    r9, r9
    jz      .Lepilogue

.Ltail_loop:                    // tail limbs
    add     rbp, rbp            // restore CF
    mov     r10, [rsi]
    sbb     r10, [rdx]          // t_i
    sbb     rbp, rbp            // capture -borrow out
    shrd    r8, r10, 1          // dst[i-1] = (prev>>1)|(t_i<<63)
    mov     [rdi], r8
    mov     r8, r10             // prev = t_i
    lea     rdi, [rdi+8]
    lea     rsi, [rsi+8]
    lea     rdx, [rdx+8]
    dec     r9
    jnz     .Ltail_loop

.Lepilogue:                     // dst[n-1] = (prev>>1)|(borrow<<63)
    add     rbp, rbp            // restore CF
    sbb     rax, rax            // rax = -CF
    shrd    r8, rax, 1
    mov     [rdi], r8
    mov     rax, rbx            // ret ret_lsb

#ifdef LMMP_WINDOWS
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
    pop     rdi
    pop     rsi
#else
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#endif
    ret

.Lret0:
    xor     eax, eax
#ifdef LMMP_WINDOWS
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
    pop     rdi
    pop     rsi
#else
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx
#endif
    ret
