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

# [dst, n] = [numa, n] * [numb, n] mod B^n
# void lmmp_mullo_basecase_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

#include "lmmp_asm.h"

.text

#ifdef LMMP_WINDOWS
    # Windows: dst=rcx, numa=rdx, numb=r8, n=r9
    #define P_DST rcx
    #define P_NUMA rdx
    #define P_NUMB r8
    #define P_N r9
#else
    # Linux: dst=rdi, numa=rsi, numb=rdx, n=rcx
    #define P_DST rdi
    #define P_NUMA rsi
    #define P_NUMB rdx
    #define P_N rcx
#endif

.macro PUSH_ALL
#ifdef LMMP_WINDOWS
    push rsi
    push rdi
#endif
    push rbx
    push rbp
.endm

.macro POP_ALL
    pop rbp
    pop rbx
#ifdef LMMP_WINDOWS
    pop rdi
    pop rsi
#endif
.endm

.macro PUSH_EXTRA
    push r12
    push r13
    push r14
.endm

.macro POP_EXTRA
    pop r14
    pop r13
    pop r12
.endm

# ====================== .text ======================

.text
.globl ASM_GSYM(lmmp_mullo_basecase_)
.p2align 4

ASM_GSYM(lmmp_mullo_basecase_):
    PUSH_ALL

#ifdef LMMP_WINDOWS
    mov rdi, P_DST             # dst  -> rdi
    mov rsi, P_NUMA            # numa -> rsi
    mov r8,  P_NUMB            # numb -> r8
    mov r9,  P_N               # n    -> r9
    mov rcx, r8                # numb -> rcx
    mov rdx, r9                # n    -> rdx
    mov r8,  r9                # n    -> r8  (外循环计数)
#else
    mov r8,  rcx               # n    -> r8
    mov rcx, rdx               # numb -> rcx
    mov rdx, r8                # n    -> rdx
#endif

    test rdx, rdx
    jz   .Lret                 # n == 0

    mov rbx, rdx
    neg rbx                    # rbx = -n
    lea rsi, [rsi + rdx*8]     # rsi = numa + n
    lea rdi, [rdi + rdx*8]     # rdi = dst  + n

    mov r9, [rcx]              # r9 = numb[0]
    mov rax, [rsi + rbx*8]     # rax = numa[0]
    mul r9                     # rdx:rax = numa[0] * numb[0]

    test bl, 1                 # n 为奇数？
    jnz  .mul1_odd

    # ~~~~~~~~ n 为偶数 ~~~~~~~~
.mul1_even:
    mov r10, rax
    mov r11, rdx
    mov rax, [rsi + rbx*8 + 8] # 预取 numa[1]
    test bl, 2                 # n ≡ 2 mod 4 ?
    jnz  .mul1_even_2mod4

    # n ≡ 0 mod 4
    lea rbp, [rbx + 2]         # rbp = -n + 2
    jmp  .mul1_l0

.mul1_even_2mod4:
    lea rbp, [rbx]             # rbp = -n
    jmp  .mul1_l2

    # ~~~~~~~~ n 为奇数 ~~~~~~~~
.mul1_odd:
    mov r11, rax
    mov r10, rdx
    test bl, 2                 # n ≡ 3 mod 4 ?
    jz   .mul1_odd_3mod4

    # n ≡ 1 mod 4
    lea rbp, [rbx + 3]         # rbp = -n + 3
    test rbp, rbp
    js   .mul1_l1              # rbp < 0 → 进入循环
    # n == 1 直接返回
    mov [rdi - 8], rax         # dst[0] = 低位
    # 进位丢弃
    POP_ALL
    ret

.mul1_odd_3mod4:
    lea rbp, [rbx + 1]         # rbp = -n + 1
    mov rax, [rsi + rbx*8 + 8] # 预取 numa[1]
    jmp  .mul1_l3

.p2align 4
.mul1_tp:
    mov r10, rdx
    add r11, rax
.mul1_l1:
    mov rax, [rsi + rbp*8 - 16]
    adc r10, 0
    mul r9
    add r10, rax
    mov [rdi + rbp*8 - 24], r11
    mov rax, [rsi + rbp*8 - 8]
    mov r11, rdx
    adc r11, 0
.mul1_l0:
    mul r9
    mov [rdi + rbp*8 - 16], r10
    add r11, rax
    mov r10, rdx
    mov rax, [rsi + rbp*8]
    adc r10, 0
.mul1_l3:
    mul r9
    mov [rdi + rbp*8 - 8], r11
    mov r11, rdx
    add r10, rax
    mov rax, [rsi + rbp*8 + 8]
    adc r11, 0
.mul1_l2:
    mul r9
    mov [rdi + rbp*8], r10
    add rbp, 4
    jnc .mul1_tp

.mul1_ed:
    add r11, rax
    adc rdx, 0
    mov [rdi - 8], r11         # 最后一个有效 limb dst[n-1]
    # 不存储 [rdi]（进位丢弃）

    dec r8                     # r8 = n - 1
    jz  .Lret

    # --------------------------------------------------
    # 准备外循环 : j = 1 .. n-1
    #    rdi 保持 dst + n (不变)
    #    rsi 指向 numa + (n-1)  (后续每次递减 8)
    #    rbx = -(n-1)           (当前长度 len = n-1)
    # --------------------------------------------------
    lea rcx, [rcx + 8]         # numb++
    inc rbx                    # rbx = -(n-1)
    sub rsi, 8                 # rsi = numa + (n-1)

    PUSH_EXTRA

    # ==================== 外循环 (单乘数累加) ====================
.Louter:
    mov r9, [rcx]              # r9 = numb[j]

    # ---- 第一个 limb : 仅计算乘积，不累加 dst，由循环统一处理 ----
    mov rax, [rsi + rbx*8]     # rax = numa[0]
    mul r9                     # rdx:rax = numa[0] * numb[j]

    test bl, 1
    jnz  .a1_odd

    # ~~~~~~~~ 长度为偶数 ~~~~~~~~
.a1_even:
    mov r10, rax               # r10 = prod_lo
    mov r11, rdx               # r11 = prod_hi
    # 预取 numa[1]（不破坏 r10/r11）
    mov rax, [rsi + rbx*8 + 8]
    test bl, 2
    jnz  .a1_even_2mod4

    # len ≡ 0 mod 4
    lea rbp, [rbx + 2]
    jmp  .a1_l0

.a1_even_2mod4:
    # len ≡ 2 mod 4
    lea rbp, [rbx]
    jmp  .a1_l2

    # ~~~~~~~~ 长度为奇数 ~~~~~~~~
.a1_odd:
    mov r11, rax               # r11 = prod_lo
    mov r10, rdx               # r10 = prod_hi
    test bl, 2
    jz   .a1_odd_3mod4         # bit1=0 → len ≡ 3 mod 4

    # len ≡ 1 mod 4
.a1_odd_1mod4:
    lea rbp, [rbx + 3]
    test rbp, rbp
    js   .a1_l1                # rbp < 0 → 进入循环
    # len == 1 时，手动累加并存储
    add r11, [rdi - 8]         # 累加 dst[n-1] 旧值
    adc r10, 0
    mov [rdi - 8], r11
    # 进位丢弃
    jmp  .Lnext

.a1_odd_3mod4:
    # len ≡ 3 mod 4
    lea rbp, [rbx + 1]
    mov rax, [rsi + rbx*8 + 8] # 预取 numa[1]
    jmp  .a1_l3

    # 4 路展开的累加乘法循环 (addmul_1)
.p2align 4
.a1_tp:
    mov r10, rdx
    add r11, rax
.a1_l1:
    mov rax, [rsi + rbp*8 - 16]
    adc r10, 0
    mul r9

    # --- store r11 前累加旧值 ---
    add r11, [rdi + rbp*8 - 24]
    adc r10, 0
    mov [rdi + rbp*8 - 24], r11

    add r10, rax
    mov rax, [rsi + rbp*8 - 8]
    mov r11, rdx
    adc r11, 0
.a1_l0:
    mul r9

    # --- store r10 前累加旧值 ---
    add r10, [rdi + rbp*8 - 16]
    adc r11, 0
    mov [rdi + rbp*8 - 16], r10

    add r11, rax
    mov r10, rdx
    mov rax, [rsi + rbp*8]
    adc r10, 0
.a1_l3:
    mul r9

    # --- store r11 前累加旧值 ---
    add r11, [rdi + rbp*8 - 8]
    adc r10, 0
    mov [rdi + rbp*8 - 8], r11

    mov r11, rdx
    add r10, rax
    mov rax, [rsi + rbp*8 + 8]
    adc r11, 0
.a1_l2:
    mul r9

    # --- store r10 前累加旧值 ---
    add r10, [rdi + rbp*8]
    adc r11, 0
    mov [rdi + rbp*8], r10

    add rbp, 4
    jnc .a1_tp

    # ---- 循环结束，处理剩余部分 ----
.a1_ed:
    add r11, rax
    adc rdx, 0
    add r11, [rdi - 8]         # 累加 dst[n-1] 旧值
    adc rdx, 0
    mov [rdi - 8], r11         # 写入 dst[n-1]
    # 进位 rdx 丢弃

.Lnext:
    dec r8
    jz  .Ldone
    lea rcx, [rcx + 8]         # numb++
    inc rbx                    # len-- (rbx = -(len))
    sub rsi, 8                 # numa 有效段末尾前移
    jmp .Louter

.Ldone:
    POP_EXTRA

.Lret:
    POP_ALL
    ret
