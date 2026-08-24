#
#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#  by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY.
#
#  See <https://www.gnu.org/licenses/>.

#
# mp_limb_t lmmp_sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
#
# mp_limb_t lmmp_sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

#include "lmmp_asm.h"

#define ADCSBB sbb

.text
.p2align 4

# ======================================
# lmmp_sub_nc_
# ======================================
.globl ASM_GSYM(lmmp_sub_nc_)
ASM_GSYM(lmmp_sub_nc_):
#ifdef LMMP_WINDOWS
    mov     r10, [rsp+40]
#else
    mov     r10, r8
#endif
    jmp     .Lentry

# ======================================
# lmmp_sub_n_
# ======================================
.globl ASM_GSYM(lmmp_sub_n_)
ASM_GSYM(lmmp_sub_n_):
    xor     r10, r10

.Lentry:
#ifdef LMMP_WINDOWS
    push    rsi
    push    rdi
    mov     rsi, rdx
    mov     rdi, rcx
    mov     rdx, r8
    mov     rcx, r9
#endif

    mov     rax, rcx
    shr     rcx, 2
    test    al, 1
    jnz     .Lodd

.Leven:
    test    al, 2
    jnz     .Leven2

.Leven0:
    neg     r10
    mov     r8, [rsi]
    mov     r9, [rsi+8]
    ADCSBB  r8, [rdx]
    ADCSBB  r9, [rdx+8]
    mov     r10, [rsi+16]
    mov     r11, [rsi+24]
    lea     rsi, [rsi+32]
    ADCSBB  r10, [rdx+16]
    ADCSBB  r11, [rdx+24]
    lea     rdx, [rdx+32]
    lea     rdi, [rdi-16]
    jmp     .Lent0

.Leven2:
    neg     r10
    mov     r10, [rsi]
    mov     r11, [rsi+8]
    ADCSBB  r10, [rdx]
    ADCSBB  r11, [rdx+8]
    jrcxz   .Ltail2
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
    setc    al
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

.Lodd:
    test    al, 2
    jnz     .Lodd3

.Lodd1:
    neg     r10
    mov     r11, [rsi]
    ADCSBB  r11, [rdx]
    jrcxz   .Ltail1
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
    setc    al
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

.Lodd3:
    neg     r10
    mov     r9, [rsi]
    ADCSBB  r9, [rdx]
    mov     r10, [rsi+8]
    mov     r11, [rsi+16]
    lea     rsi, [rsi+24]
    ADCSBB  r10, [rdx+8]
    ADCSBB  r11, [rdx+16]
    lea     rdx, [rdx+24]
    mov     [rdi], r9
    lea     rdi, [rdi+8]
    jrcxz   .Lend

.p2align 4
.Lloop:
    mov     r8, [rsi]
    mov     r9, [rsi+8]
    ADCSBB  r8, [rdx]
    ADCSBB  r9, [rdx+8]

.Lent2:
    mov     [rdi], r10

.Lent1:
    mov     [rdi+8], r11
    mov     r10, [rsi+16]
    mov     r11, [rsi+24]
    lea     rsi, [rsi+32]
    ADCSBB  r10, [rdx+16]
    ADCSBB  r11, [rdx+24]
    lea     rdx, [rdx+32]

.Lent0:
    mov     [rdi+16], r8

.Lent3:
    mov     [rdi+24], r9
    lea     rdi, [rdi+32]
    dec     rcx
    jnz     .Lloop

.Lend:
    mov     rax, rcx
    mov     [rdi], r10
    mov     [rdi+8], r11
    setc    al

#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret
