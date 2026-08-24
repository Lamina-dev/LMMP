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
# mp_limb_t lmmp_shr_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr);
#
# mp_limb_t lmmp_shr_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c);

#include "lmmp_asm.h"

.text

.p2align 4

# ---------- 带进位右移 ----------
.globl ASM_GSYM(lmmp_shr_c_)
ASM_GSYM(lmmp_shr_c_):
#ifdef LMMP_WINDOWS
    mov     rax, [rsp+40]           # 取参数 c
    xchg    rcx, r9                 # dst <-> shr
#else
    mov     rax, r8                 # SysV: c 在 r8
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif
    jmp     .Lentry

# ---------- 无进位右移 ----------
.p2align 4
.globl ASM_GSYM(lmmp_shr_)
ASM_GSYM(lmmp_shr_):
    xor     rax, rax                # carry = 0
#ifdef LMMP_WINDOWS
    xchg    rcx, r9
#else
    mov     r8, rdx
    mov     r9, rdi
    mov     rdx, rsi
#endif

.Lentry:
    and     cl, 63
    push    r12
    push    r13
    push    rax
    mov     r10, [rdx]
    xor     rax, rax
    lea     r9, [r9 + r8*8]
    lea     rdx, [rdx + r8*8]
    shrd    rax, r10, cl
    neg     r8

    test    r8b, 1
    jnz     .Lodd

.Leven:
    test    r8b, 2
    jnz     .Leven2

.Leven0:
    mov     r12, r10
    mov     r13, [rdx + r8*8 + 8]
    jmp     .Lent0

.Leven2:
    mov     r11, [rdx + r8*8 + 8]
    add     r8, 2
    jmp     .Lent2

.Lodd:
    test    r8b, 2
    jz      .Lodd3

.Lodd1:
    inc     r8
    jz      .Ltail1
    mov     r12, [rdx + r8*8]
    mov     r13, [rdx + r8*8 + 8]
    shrd    r10, r12, cl
    mov     [r9 + r8*8 - 8], r10
    jmp     .Lent1

.Ltail1:
    pop     r12
    shr     r10, cl
    or      r10, r12
    mov     [r9 - 8], r10
    pop     r13
    pop     r12
    ret

.Lodd3:
    mov     r13, r10
    mov     r10, [rdx + r8*8 + 8]
    mov     r11, [rdx + r8*8 + 16]
    shrd    r13, r10, cl
    dec     r8
    jmp     .Lent3

.p2align 4
.Lloop:
    mov     r12, [rdx + r8*8]
    mov     r13, [rdx + r8*8 + 8]
    shrd    r10, r11, cl
    shrd    r11, r12, cl
    mov     [r9 + r8*8 - 16], r10
    mov     [r9 + r8*8 - 8], r11

.Lent1:
.Lent0:
    mov     r10, [rdx + r8*8 + 16]
    mov     r11, [rdx + r8*8 + 24]
    shrd    r12, r13, cl
    shrd    r13, r10, cl
    mov     [r9 + r8*8], r12

.Lent3:
    mov     [r9 + r8*8 + 8], r13
    add     r8, 4

.Lent2:
    jnz     .Lloop

.Lend:
    pop     r12
    shrd    r10, r11, cl
    shr     r11, cl
    or      r11, r12
    mov     [r9 - 16], r10
    mov     [r9 - 8], r11
    pop     r13
    pop     r12
    ret
