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
# int lmmp_limb_bits_(mp_limb_t x);
#
# int lmmp_limb_popcnt_(mp_limb_t x);
#
# int lmmp_leading_zeros_(mp_limb_t x);
#
# int lmmp_tailing_zeros_(mp_limb_t x);
#
# mp_limb_t lmmp_mulh_(mp_limb_t a, mp_limb_t b);
#
# void lmmp_mullh_(mp_limb_t a, mp_limb_t b, mp_ptr dst);
#
# ulong lmmp_mulmod_ulong_(ulong a, ulong b, ulong mod, ulongp q);

#include "lmmp_asm.h"

.text

# ----------------------------------------------------------------------
# int lmmp_limb_bits_(mp_limb_t x)
# 返回 x 的二进制位数（x==0 返回 0）
# ----------------------------------------------------------------------
.p2align 4
.globl ASM_GSYM(lmmp_limb_bits_)
ASM_GSYM(lmmp_limb_bits_):
    mov     rax, -1
    bsr     rx0, rx0
    cmovnz  rax, rx0
    inc     rax
    ret

# ----------------------------------------------------------------------
# int lmmp_leading_zeros_(mp_limb_t x)
# 返回 x 的前导零个数（x==0 返回 64）
# ----------------------------------------------------------------------
.p2align 4
.globl ASM_GSYM(lmmp_leading_zeros_)
ASM_GSYM(lmmp_leading_zeros_):
    mov     rax, -1
    bsr     rx0, rx0
    cmovnz  rax, rx0
    not     rax
    add     rax, 64
    ret

# ----------------------------------------------------------------------
# int lmmp_tailing_zeros_(mp_limb_t x)
# 返回 x 的尾部零个数（x==0 返回 64）
# ----------------------------------------------------------------------
.p2align 4
.globl ASM_GSYM(lmmp_tailing_zeros_)
ASM_GSYM(lmmp_tailing_zeros_):
    mov     rax, 64
    bsf     rx0, rx0
    cmovnz  rax, rx0
    ret

# ----------------------------------------------------------------------
# int lmmp_limb_popcnt_(mp_limb_t x)
# 返回 x 中置 1 的位数
# ----------------------------------------------------------------------
.p2align 4
.globl ASM_GSYM(lmmp_limb_popcnt_)
ASM_GSYM(lmmp_limb_popcnt_):
    xor     rax, rax
    popcnt  rax, rx0
    ret

# ----------------------------------------------------------------------
# mp_limb_t lmmp_mulh_(mp_limb_t a, mp_limb_t b)
# 返回 a * b 的高 64 位
# ----------------------------------------------------------------------
.p2align 4
.globl ASM_GSYM(lmmp_mulh_)
ASM_GSYM(lmmp_mulh_):
    mov     rax, rx1
    mul     rx0
    mov     rax, rdx
    ret

# ----------------------------------------------------------------------
# void lmmp_mullh_(mp_limb_t a, mp_limb_t b, mp_ptr dst)
# 计算 a * b，低 64 位存入 dst[0]，高 64 位存入 dst[1]
# ----------------------------------------------------------------------
.p2align 4
.globl ASM_GSYM(lmmp_mullh_)
ASM_GSYM(lmmp_mullh_):
    /* rdx(rx2) 会在 mul 后被高 64 位覆盖，必须先保存目标指针。 */
    mov     r9, rx2
    mov     rax, rx0
    mul     rx1
    mov     [r9], rax
    mov     [r9+8], rdx
    ret

# ----------------------------------------------------------------------
# ulong lmmp_mulmod_ulong_(ulong a, ulong b, ulong mod, ulongp q)
# 计算 (a * b) / mod，商存入 *q，返回余数
# 注意：调用者必须保证商不超过 64 位，否则会触发除法异常
# ----------------------------------------------------------------------
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
