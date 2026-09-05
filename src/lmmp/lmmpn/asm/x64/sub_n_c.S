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
// mp_limb_t lmmp_sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
//
// mp_limb_t lmmp_sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
//   Windows: dst=rcx, numa=rdx, numb=r8, n=r9, c=[rsp+40] (only nc_)
//   SysV:    dst=rdi, numa=rsi, numb=rdx, n=rcx, c=r8 (only nc_)
//   ret rax = highest limb borrow
//   cal: dst[i] = numa[i] - numb[i] - CF, sbb chain

#include "lmmp_asm.h"

#define ADCSBB sbb

.text
.p2align 4

// ======================================
// lmmp_sub_nc_
// ======================================
.globl ASM_GSYM(lmmp_sub_nc_)
ASM_GSYM(lmmp_sub_nc_):
#ifdef LMMP_WINDOWS
    mov     r10, [rsp+40]          // r10 = c
#else
    mov     r10, r8                // r10 = c
#endif
    jmp     .Lentry

// ======================================
// lmmp_sub_n_
// ======================================
.globl ASM_GSYM(lmmp_sub_n_)
ASM_GSYM(lmmp_sub_n_):
    xor     r10, r10               // borrow = 0

.Lentry:
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    mov     rsi, rdx               // SysV:
    mov     rdi, rcx               //   rdi=dst, rsi=numa,
    mov     rdx, r8                //   rdx=numb, rcx=n
    mov     rcx, r9
#endif

    mov     rax, rcx
    shr     rcx, 2                 // rcx = n >> 2 (4-limbs size)
    test    al, 1
    jnz     .Lodd                  // n mod 4 in {1,3}

.Leven:
    test    al, 2
    jnz     .Leven2                // n mod 4 == 2

.Leven0:                           // n mod 4 == 0
    neg     r10                    // CF = (c != 0)
    mov     r8, [rsi]
    mov     r9, [rsi+8]
    ADCSBB  r8, [rdx]              // t0 = numa[0] - numb[0] - CF
    ADCSBB  r9, [rdx+8]            // t1
    mov     r10, [rsi+16]
    mov     r11, [rsi+24]
    lea     rsi, [rsi+32]
    ADCSBB  r10, [rdx+16]          // t2
    ADCSBB  r11, [rdx+24]          // t3
    lea     rdx, [rdx+32]
    lea     rdi, [rdi-16]
    jmp     .Lent0

.Leven2:                           // n mod 4 == 2
    neg     r10
    mov     r10, [rsi]
    mov     r11, [rsi+8]
    ADCSBB  r10, [rdx]             // t0 = numa[0] - numb[0] - CF
    ADCSBB  r11, [rdx+8]
    jrcxz   .Ltail2                // n == 2
    mov     r8, [rsi+16]
    mov     r9, [rsi+24]
    lea     rsi, [rsi+16]
    ADCSBB  r8, [rdx+16]
    ADCSBB  r9, [rdx+24]
    lea     rdx, [rdx+16]
    jmp     .Lent2

.Ltail2:
    mov     [rdi], r10
    mov     [rdi+8], r11
    setc    al                     // return highest limb borrow
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

.Lodd:
    test    al, 2
    jnz     .Lodd3                 // n mod 4 == 3

.Lodd1:                            // n mod 4 == 1
    neg     r10
    mov     r11, [rsi]
    ADCSBB  r11, [rdx]             // t0 = numa[0] - numb[0] - CF
    jrcxz   .Ltail1                // n == 1, ret
    mov     r8, [rsi+8]
    mov     r9, [rsi+16]
    lea     rsi, [rsi+8]
    lea     rdi, [rdi-8]
    ADCSBB  r8, [rdx+8]
    ADCSBB  r9, [rdx+16]
    lea     rdx, [rdx+8]
    jmp     .Lent1

.Ltail1:
    mov     [rdi], r11
    setc    al                     // return highest limb borrow
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

.Lodd3:                            // n mod 4 == 3
    neg     r10
    mov     r9, [rsi]
    ADCSBB  r9, [rdx]              // t0
    mov     r10, [rsi+8]
    mov     r11, [rsi+16]
    lea     rsi, [rsi+24]
    ADCSBB  r10, [rdx+8]
    ADCSBB  r11, [rdx+16]
    lea     rdx, [rdx+24]
    mov     [rdi], r9
    lea     rdi, [rdi+8]
    jrcxz   .Lend                  // n == 3, ret

.p2align 4
.Lloop:                            // main loop : 4 limb
    mov     r8, [rsi]
    mov     r9, [rsi+8]
    ADCSBB  r8, [rdx]              // t_i   = a - b - CF
    ADCSBB  r9, [rdx+8]            // t_i+1

.Lent2:
    mov     [rdi], r10

.Lent1:
    mov     [rdi+8], r11
    mov     r10, [rsi+16]
    mov     r11, [rsi+24]
    lea     rsi, [rsi+32]
    ADCSBB  r10, [rdx+16]          // t_i+2
    ADCSBB  r11, [rdx+24]          // t_i+3
    lea     rdx, [rdx+32]

.Lent0:
    mov     [rdi+16], r8

.Lent3:
    mov     [rdi+24], r9
    lea     rdi, [rdi+32]
    dec     rcx
    jnz     .Lloop

.Lend:                             // now rcx == 0
    mov     rax, rcx               // rax = 0
    mov     [rdi], r10
    mov     [rdi+8], r11
    setc    al                     // rax = highest limb borrow

#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret
