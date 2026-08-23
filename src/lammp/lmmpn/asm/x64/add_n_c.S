#
#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#   by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY. 
#
#  See <https://www.gnu.org/licenses/>.


# ==========================================================
# mp_limb_t lmmp_add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);
#
# mp_limb_t lmmp_add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
# ==========================================================

#include "lammp_asm.h"

.text

#define ADCSBB adc


.text
.p2align 4

# ======================================
# lmmp_add_nc_
# ======================================
.globl ASM_GSYM(lmmp_add_nc_)

ASM_GSYM(lmmp_add_nc_):
#ifdef LMMP_WINDOWS
    mov     r10, [rsp+40]
#else
    mov     r10, r8
#endif
    jmp     lab_ent

# ======================================
# lmmp_add_n_
# ======================================
.globl ASM_GSYM(lmmp_add_n_)
ASM_GSYM(lmmp_add_n_):
    xor     r10, r10

ASM_GSYM(lab_ent):
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
    jnz     .lab_bx1

.lab_bx0:
    test    al, 2
    jnz     .lab_b10

.lab_b00:
    neg     r10
    mov     r8,      [rsi]
    mov     r9,      [rsi+8]
    ADCSBB  r8,      [rdx]
    ADCSBB  r9,      [rdx+8]
    mov     r10,     [rsi+16]
    mov     r11,     [rsi+24]
    lea     rsi,     [rsi+32]
    ADCSBB  r10,     [rdx+16]
    ADCSBB  r11,     [rdx+24]
    lea     rdx,     [rdx+32]
    lea     rdi,     [rdi-16]
    jmp     .lab_lo0

.lab_b10:
    neg     r10
    mov     r10,     [rsi]
    mov     r11,     [rsi+8]
    ADCSBB  r10,     [rdx]
    ADCSBB  r11,     [rdx+8]
    jrcxz   .lab_e2
    mov     r8,      [rsi+16]
    mov     r9,      [rsi+24]
    lea     rsi,     [rsi+16]
    ADCSBB  r8,      [rdx+16]
    ADCSBB  r9,      [rdx+24]
    lea     rdx,     [rdx+16]
    jmp     .lab_lo2

.lab_e2:
    mov     [rdi],   r10
    mov     [rdi+8], r11
    setc    al
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

.lab_bx1:
    test    al,      2
    jnz     .lab_b11

.lab_b01:
    neg     r10
    mov     r11,     [rsi]
    ADCSBB  r11,     [rdx]
    jrcxz   .lab_e1
    mov     r8,      [rsi+8]
    mov     r9,      [rsi+16]
    lea     rsi,     [rsi+8]
    lea     rdi,     [rdi-8]
    ADCSBB  r8,      [rdx+8]
    ADCSBB  r9,      [rdx+16]
    lea     rdx,     [rdx+8]
    jmp     .lab_lo1

.lab_e1:
    mov     [rdi],   r11
    setc    al
#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret

.lab_b11:
    neg     r10
    mov     r9,      [rsi]
    ADCSBB  r9,      [rdx]
    mov     r10,     [rsi+8]
    mov     r11,     [rsi+16]
    lea     rsi,     [rsi+24]
    ADCSBB  r10,     [rdx+8]
    ADCSBB  r11,     [rdx+16]
    lea     rdx,     [rdx+24]
    mov     [rdi],   r9
    lea     rdi,     [rdi+8]
    jrcxz   .lab_end

.p2align 4
.lab_top:
    mov     r8,      [rsi]
    mov     r9,      [rsi+8]
    ADCSBB  r8,      [rdx]
    ADCSBB  r9,      [rdx+8]

.lab_lo2:
    mov     [rdi],   r10

.lab_lo1:
    mov     [rdi+8], r11
    mov     r10,     [rsi+16]
    mov     r11,     [rsi+24]
    lea     rsi,     [rsi+32]
    ADCSBB  r10,     [rdx+16]
    ADCSBB  r11,     [rdx+24]
    lea     rdx,     [rdx+32]

.lab_lo0:
    mov     [rdi+16], r8

.lab_lo3:
    mov     [rdi+24], r9
    lea     rdi,     [rdi+32]
    dec     rcx
    jnz     .lab_top

.lab_end:
    mov     rax,     rcx
    mov     [rdi],   r10
    mov     [rdi+8], r11
    setc    al

#ifdef LMMP_WINDOWS
    pop     rdi
    pop     rsi
#endif
    ret
