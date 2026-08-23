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

#
# mp_limb_t lmmp_div_1_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_limb_t x);
#
# mp_limb_t lmmp_div_1_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_limb_t x);


#include "lammp_asm.h"

.text

#ifdef LMMP_WINDOWS
#else
#endif


.text

#======================================================================
# mp_limb_t lmmp_div_1_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_limb_t x)
#======================================================================
.globl ASM_GSYM(lmmp_div_1_)
.p2align 4
ASM_GSYM(lmmp_div_1_):
    test rx0, rx0
    jz .mod_path                 # dstq == NULL → 仅计算余数

    # ---- 正常除法（存储商） ----
win push rdi
win push rsi
    push rbp
    push rbx
    push r12

    mov rbx, rx2                 # rbx = na
win mov rdi, rcx                 # rdi = dstq
win mov rsi, rdx                 # rsi = numa
    mov r8, rx3                  # r8 = x

    xor eax, eax
    bsr rcx, r8
    not rcx
    add rcx, 64
    shl r8, cl                   # 规整化除数

lin mov r10, rdi                 # 保存 rdi (dstq) 到 r10，call 会破坏它
    mov r11, rax                 # r11 = 0
    mov rbp, rcx                 # rbp = 移位量
    mov r12, r8                  # r12 = 规整化除数
    mov rx0, r8                  # 参数：x → rcx/rdi

    # 栈对齐与影子空间（尽管并未使用）
win sub rsp, 32
    CALL(lmmp_inv_1_)
win add rsp, 32

lin mov rdi, r10                 # 恢复 rdi
    mov r9, rax                  # r9 = 逆
    mov r8, r12
    mov rcx, rbp
    mov rax, r11                 # rax = 0

    mov r10, [rsi+rbx*8-8]
    shld rax, r10, cl
    jmp .lab_ent

.p2align 4
.lab_top:
    mov r12, [rsi+rbx*8-8]
    shld r10, r12, cl
.lab_last:                       # rax = xh < r8, rbp = xh+1
    mul r9                       # rdx:rax = xh * (inv-2^64)
    add rax, r10
    adc rdx, rbp
    mov rbp, rax                 # rbp = ql
    mov r11, rdx                 # r11 = qh
    imul rdx, r8
    sub r10, rdx                 # r10 = z
    lea rax, [r8+r10]
    cmp r10, rbp
    cmovc rax, r10
    adc r11, -1
    cmp rax, r8
    jae .lab_fx
.lab_ok:
    mov [rdi+rbx*8], r11         # 存储商
    mov r10, r12
.lab_ent:
    sub rbx, 1
    lea rbp, [rax+1]
    ja .lab_top
    jb .lab_ret

    shl r10, cl
    jmp .lab_last

.lab_fx:
    sub rax, r8
    inc r11
    jmp .lab_ok

.lab_ret:
    shr rax, cl                  # 余数在 rax
    pop r12
    pop rbx
    pop rbp
win pop rsi
win pop rdi
    ret

#------------------------ dstq == NULL：仅计算余数 -------------------------
.mod_path:
#ifdef LMMP_WINDOWS
    # 参数重映射：lmmp_mod_1_(numa,na,x) → rcx=numa, rdx=na, r8=x
    mov rcx, rdx
    mov rdx, r8
    mov r8, r9
    # 入口 rsp 仅含返回地址（8 字节，未对齐），需 40 字节调整
    sub rsp, 40
    CALL(lmmp_mod_1_)
    add rsp, 40
#else
    # Linux 入口 rsp 未对齐，映射参数并调整栈
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    sub rsp, 8
    CALL(lmmp_mod_1_)
    add rsp, 8
#endif
    ret

#======================================================================
# mp_limb_t lmmp_div_1_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_limb_t x)
#======================================================================
.globl ASM_GSYM(lmmp_div_1_s_)
.p2align 4
ASM_GSYM(lmmp_div_1_s_):
    push rbp
    push rbx
win push rdi
win push rsi

    mov rbx, rx2
win mov rdi, rcx
win mov rsi, rdx
lin mov r10, rdi
    mov rbp, rx3
    mov rx0, rx3                  # 参数：x → rcx/rdi

    # 栈对齐及影子空间
#ifdef LMMP_WINDOWS
    # 已 push 4 个寄存器 (32 字节)，rsp ≡ 8，需 32+8=40
    sub rsp, 40
#else
    # 已 push 2 个寄存器，rsp ≡ 8，对齐需 8
    sub rsp, 8
#endif
    CALL(lmmp_inv_1_)
#ifdef LMMP_WINDOWS
    add rsp, 40
#else
    add rsp, 8
#endif

lin mov rdi, r10
    mov r8, rbp
    mov r9, rax
    xor rcx, rcx
    mov r10, [rsi+rbx*8-8]
    mov rax, r10
    sub rax, r8
    setnc cl
    cmovc rax, r10
    dec rbx
.p2align 4
.lab_stop:
    lea rbp, [rax+1]
    mov r10, [rsi+rbx*8-8]
    mul r9
    add rax, r10
    adc rdx, rbp
    mov rbp, rax
    mov r11, rdx
    imul rdx, r8
    sub r10, rdx
    lea rax, [r8+r10]
    cmp r10, rbp
    cmovc rax, r10
    adc r11, -1
    cmp rax, r8
    jae .lab_sfx
.lab_sok:
    mov [rdi+rbx*8-8], r11
    dec rbx
    jnz .lab_stop
    mov [rsi], rax
    mov rax, rcx

win pop rsi
win pop rdi
    pop rbx
    pop rbp
    ret
.lab_sfx:
    sub rax, r8
    inc r11
    jmp .lab_sok
