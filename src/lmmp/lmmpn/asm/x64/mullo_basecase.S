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
    mov     rdi, P_DST             # dst  -> rdi
    mov     rsi, P_NUMA            # numa -> rsi
    mov     r8,  P_NUMB            # numb -> r8
    mov     r9,  P_N               # n    -> r9
    mov     rcx, r8                # numb -> rcx
    mov     rdx, r9                # n    -> rdx
    mov     r8,  r9                # n    -> r8  (外循环计数)
#else
    mov     r8,  rcx               # n    -> r8
    mov     rcx, rdx               # numb -> rcx
    mov     rdx, r8                # n    -> rdx
#endif

    test    rdx, rdx
    jz      .Lret                  # n == 0

    mov     rbx, rdx
    neg     rbx                    # rbx = -n
    lea     rsi, [rsi + rdx*8]     # rsi = numa + n
    lea     rdi, [rdi + rdx*8]     # rdi = dst  + n

    mov     r9, [rcx]              # r9 = numb[0]
    mov     rax, [rsi + rbx*8]     # rax = numa[0]
    mul     r9                     # rdx:rax = numa[0] * numb[0]

    test    bl, 1                  # n 为奇数？
    jnz    .Lmul1_odd

    # ~~~~~~~~ n 为偶数 ~~~~~~~~
.Lmul1_even:
    mov     r10, rax
    mov     r11, rdx
    mov     rax, [rsi + rbx*8 + 8] # 预取 numa[1]
    test    bl, 2                  # n ≡ 2 mod 4 ?
    jnz     .Lmul1_even_2mod4

    # n ≡ 0 mod 4
    lea     rbp, [rbx + 2]     # rbp = -n + 2
    jmp     .Lmul1_l0

.Lmul1_even_2mod4:
    lea     rbp, [rbx]         # rbp = -n
    jmp     .Lmul1_l2

    # ~~~~~~~~ n 为奇数 ~~~~~~~~
.Lmul1_odd:
    mov     r11, rax
    mov     r10, rdx
    test    bl, 2              # n ≡ 3 mod 4 ?
    jz      .Lmul1_odd_3mod4

    # n ≡ 1 mod 4
    lea     rbp, [rbx + 3]     # rbp = -n + 3
    test    rbp, rbp
    js      .Lmul1_l1          # rbp < 0 → 进入循环
    # n == 1 直接返回
    mov     [rdi - 8], rax     # dst[0] = 低位
    # 进位丢弃
    POP_ALL
    ret

.Lmul1_odd_3mod4:
    lea     rbp, [rbx + 1]         # rbp = -n + 1
    mov     rax, [rsi + rbx*8 + 8] # 预取 numa[1]
    jmp     .Lmul1_l3

.p2align 4
.Lmul1_tp:
    mov     r10, rdx
    add     r11, rax
.Lmul1_l1:
    mov     rax, [rsi + rbp*8 - 16]
    adc     r10, 0
    mul     r9
    add     r10, rax
    mov     [rdi + rbp*8 - 24], r11
    mov     rax, [rsi + rbp*8 - 8]
    mov     r11, rdx
    adc     r11, 0
.Lmul1_l0:
    mul     r9
    mov     [rdi + rbp*8 - 16], r10
    add     r11, rax
    mov     r10, rdx
    mov     rax, [rsi + rbp*8]
    adc     r10, 0
.Lmul1_l3:
    mul     r9
    mov     [rdi + rbp*8 - 8], r11
    mov     r11, rdx
    add     r10, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r11, 0
.Lmul1_l2:
    mul     r9
    mov     [rdi + rbp*8], r10
    add     rbp, 4
    jnc     .Lmul1_tp

.Lmul1_ed:
    add     r11, rax
    adc     rdx, 0
    mov     [rdi - 8], r11     # 最后一个有效 limb dst[n-1]
    # 不存储 [rdi]（进位丢弃）

    dec     r8                 # r8 = n - 1
    jz      .Lret

    # --------------------------------------------------
    # 准备外循环 : j = 1 .. n-1
    #    rdi 保持 dst + n (不变)
    #    rsi 指向 numa + (n-1)  (长度递减时每次前移 16)
    #    rbx = -(n-1)           (当前长度 len = n-1)
    # --------------------------------------------------
    lea     rcx, [rcx + 8]     # numb++
    inc     rbx                # rbx = -(n-1)
    sub     rsi, 8             # rsi = numa + (n-1)

    PUSH_EXTRA                 # push r12, r13, r14
    push    r15
    push    r8                 # 计数器（剩余列数）入栈

    # ==================== 两列并行外循环 ====================
    # 状态：rdi=dst+n（不变），rsi=numa+L，rcx=numb+j，rbx=-L，
    #       [rsp]=剩余列数（== L）。每轮处理 numb[j]、numb[j+1] 两列，
    #       长度 L、L-1；只写低 n 个 limb，超出进位丢弃。
.p2align 4
.Louter2:
    cmp     rbx, -4
    jg      .Ltail              # L < 4 → 进入单乘数尾部

    mov     r9, [rcx]           # m0 = numb[j]
    mov     r8, [rcx + 8]       # m1 = numb[j+1]
    mov     rax, [rsi + rbx*8]  # rax = numa[0]
    mul     r9                  # rdx:rax = numa[0] * m0
    test    bl, 1
    jnz     .a2x1

.a2x0:
    mov     r14, [rdi + rbx*8]  # dst[j]
    xor     r10, r10
    mov     r11, rdx
    test    bl, 2
    jnz     .a210

.a200:
    lea     rbp, [rbx + 2]
    add     r14, rax
    adc     r11, 0
    mov     rax, [rsi + rbx*8]
    mul     r8
    mov     r15, [rdi + rbx*8 + 8]
    jmp     .a2lo0

.a210:
    lea     rbp, [rbx]
    xor     r13, r13
    jmp     .a2lo2

.a2x1:
    mov     r15, [rdi + rbx*8]
    xor     r12, r12
    xor     r11, r11
    test    bl, 2
    jz      .a211

.a201:
    lea     rbp, [rbx + 3]
    mov     r13, rdx
    add     r15, rax
    mov     rax, [rsi + rbx*8]
    adc     r13, 0
    jmp     .a2top

.a211:
    lea     rbp, [rbx + 1]
    jmp     .a2lo3

.p2align 4
.a2top:
    mov     r14, [rdi + rbp*8 - 16]
    mul     r8
    mov     r10, rdx
    add     r14, rax
    adc     r10, 0
    add     r15, r11
    adc     r13, 0
    add     r14, r12
    adc     r10, 0
    mov     rax, [rsi + rbp*8 - 16]
    mul     r9
    add     r14, rax
    mov     r11, rdx
    adc     r11, 0
    mov     rax, [rsi + rbp*8 - 16]
    mul     r8
    mov     [rdi + rbp*8 - 24], r15
    mov     r15, [rdi + rbp*8 - 8]
    add     r14, r13
    adc     r11, 0
.a2lo0:
    mov     r12, rdx
    mov     [rdi + rbp*8 - 16], r14
    add     r15, rax
    adc     r12, 0
    mov     rax, [rsi + rbp*8 - 8]
    add     r15, r10
    adc     r12, 0
    mul     r9
.a2lo3:
    add     r15, rax
    mov     r13, rdx
    adc     r13, 0
    mov     rax, [rsi + rbp*8 - 8]
    mul     r8
    add     r15, r11
    mov     r14, [rdi + rbp*8]
    adc     r13, 0
    mov     r10, rdx
    add     r14, rax
    adc     r10, 0
    mov     rax, [rsi + rbp*8]
    mul     r9
    add     r14, r12
    mov     [rdi + rbp*8 - 8], r15
    mov     r11, rdx
    adc     r10, 0
.a2lo2:
    add     r14, rax
    adc     r11, 0
    mov     rax, [rsi + rbp*8]
    add     r14, r13
    adc     r11, 0
    mul     r8
    mov     r15, [rdi + rbp*8 + 8]
    add     r15, rax
    mov     r12, rdx
    adc     r12, 0
    mov     rax, [rsi + rbp*8 + 8]
    mov     [rdi + rbp*8], r14
    mul     r9
    add     r15, r10
    mov     r13, rdx
    adc     r12, 0
    add     r15, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r13, 0
    add     rbp, 4
    jnc     .a2top

.a2end:
    mul     r8                  # 幻影乘（结果丢弃）
    add     r15, r11
    adc     r13, 0
    add     rax, r12
    adc     rdx, 0
    mov     [rdi - 8], r15      # dst[n-1]（最后有效 limb）
    add     rax, r13
    adc     rdx, 0
    # 丢弃 [rdi]、[rdi+8] 的进位（超出 n 个 limb）

    sub     QWORD PTR [rsp], 2  # 剩余列数 -= 2
    lea     rcx, [rcx + 16]     # numb += 2
    lea     rsi, [rsi - 16]     # numa -= 2（长度 L -= 2）
    add     rbx, 2              # rbx = -(L-2)
    jmp     .Louter2

    # ==================== 尾部（剩余 1~3 列，单乘数）====================
.Ltail:
    pop     r8                  # 恢复剩余列数到 r8
    pop     r15                 # 恢复 r15
    # 以下为原单乘数外循环

.Louter:
    mov     r9, [rcx]           # r9 = numb[j]
    mov     rax, [rsi + rbx*8]  # rax = numa[0]
    mul     r9                  # rdx:rax = numa[0] * numb[j]

    test    bl, 1
    jnz     .La1_odd

.La1_even:
    mov     r10, rax            # r10 = prod_lo
    mov     r11, rdx            # r11 = prod_hi
    mov     rax, [rsi + rbx*8 + 8]
    test    bl, 2
    jnz     .La1_even_2mod4

    lea     rbp, [rbx + 2]
    jmp     .La1_l0

.La1_even_2mod4:
    lea     rbp, [rbx]
    jmp     .La1_l2

.La1_odd:
    mov     r11, rax            # r11 = prod_lo
    mov     r10, rdx            # r10 = prod_hi
    test    bl, 2
    jz      .La1_odd_3mod4      # bit1=0 → len ≡ 3 mod 4

.La1_odd_1mod4:
    lea     rbp, [rbx + 3]
    test    rbp, rbp
    js      .La1_l1             # rbp < 0 → 进入循环
    add     r11, [rdi - 8]      # 累加 dst[n-1] 旧值
    adc     r10, 0
    mov     [rdi - 8], r11
    jmp     .Lnext

.La1_odd_3mod4:
    lea     rbp, [rbx + 1]
    mov     rax, [rsi + rbx*8 + 8]
    jmp     .La1_l3

.p2align 4
.La1_tp:
    mov     r10, rdx
    add     r11, rax
.La1_l1:
    mov     rax, [rsi + rbp*8 - 16]
    adc     r10, 0
    mul     r9
    add     r11, [rdi + rbp*8 - 24]
    adc     r10, 0
    mov     [rdi + rbp*8 - 24], r11
    add     r10, rax
    mov     rax, [rsi + rbp*8 - 8]
    mov     r11, rdx
    adc     r11, 0
.La1_l0:
    mul     r9
    add     r10, [rdi + rbp*8 - 16]
    adc     r11, 0
    mov     [rdi + rbp*8 - 16], r10
    add     r11, rax
    mov     r10, rdx
    mov     rax, [rsi + rbp*8]
    adc     r10, 0
.La1_l3:
    mul     r9
    add     r11, [rdi + rbp*8 - 8]
    adc     r10, 0
    mov     [rdi + rbp*8 - 8], r11
    mov     r11, rdx
    add     r10, rax
    mov     rax, [rsi + rbp*8 + 8]
    adc     r11, 0
.La1_l2:
    mul     r9
    add     r10, [rdi + rbp*8]
    adc     r11, 0
    mov     [rdi + rbp*8], r10
    add     rbp, 4
    jnc     .La1_tp

.La1_ed:
    add     r11, rax
    adc     rdx, 0
    add     r11, [rdi - 8]      # 累加 dst[n-1] 旧值
    adc     rdx, 0
    mov     [rdi - 8], r11      # 写入 dst[n-1]
    # 进位 rdx 丢弃

.Lnext:
    dec     r8
    jz      .Ldone
    lea     rcx, [rcx + 8]      # numb++
    inc     rbx                 # len-- (rbx = -(len))
    sub     rsi, 8              # numa 有效段末尾前移
    jmp     .Louter

.Ldone:
    POP_EXTRA

.Lret:
    POP_ALL
    ret
