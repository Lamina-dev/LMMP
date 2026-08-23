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
# void lmmp_div_2_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_ptr numb);
#
# mp_limb_t lmmp_div_2_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_srcptr numb);


#include "lammp_asm.h"

.text

#ifdef LMMP_WINDOWS
#else
#endif


.text

#======================================================================
# void lmmp_div_2_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_ptr numb)
#======================================================================
.globl ASM_GSYM(lmmp_div_2_)
.p2align 4
ASM_GSYM(lmmp_div_2_):
    test rx0, rx0                      # 检测 dstq==NULL
    jz .mod_path                       # 若是，仅计算余数

    # ---- 正常除法（存储商） ----
win push rdi
win push rsi
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push rx3                           # 保存 numb 指针

    lea rbx, [rx2-1]                   # rbx = na-1
win mov rdi, rcx                       # rdi = dstq
win mov rsi, rdx                       # rsi = numa
    mov r8, [rx3]                      # 除数低 64 位
    mov r9, [rx3+8]                    # 除数高 64 位

    # ---- 被除数与除数比较 ----
    mov r11, [rsi+rbx*8-8]             # 次高位
    mov r12, [rsi+rbx*8]               # 最高位
    xor r13, r13
    cmp r12, r9
    ja .lab_2
    setb dl
    cmp r11, r8
    setb al
    or dl, al
    jz .lab_2

    # qh = 0
    mov [rdi+rbx*8-8], r13             # 存商 0
    dec rbx
    mov r13, r12
    mov r12, r11
    jz .lab_retnsh
    mov r11, [rsi+rbx*8-8]

.lab_2:
    # ---- 除数规整化 ----
    bsr rcx, r9
    not rcx
    add rcx, 64
    shld r9, r8, cl
    shl r8, cl
    push rcx                           # 保存移位量
lin mov r10, rdi                       # 保护 rdi（Linux 下 call 会破坏）
    push r8
    push r9
    mov rx0, r9                        # 参数：除数高位

    # 栈对齐与影子空间（尽管并未使用）
win sub rsp, 32
    CALL(lmmp_inv_1_)
win add rsp, 32

lin mov rdi, r10                       # 恢复 rdi
    pop r9
    pop r8
    mov rbp, rax                       # rbp = inv - 2^64

    # ---- 修正逆元（与原始完全一致） ----
    mov r10, r9
    imul r10, rax
    xor rcx, rcx
    mul r8
    add r10, r8
    adc rcx, -1
    add r10, rdx
    adc rcx, 0
    js .lab_2f
.lab_1b:
    dec rbp
    sub rax, r8
    sbb r10, r9
    sbb rcx, 0
    jns .lab_1b
.lab_2f:
    pop rcx                            # 恢复移位量
    # rbp = (2^192-1) div b - 2^64
    shld r13, r12, cl
    shld r12, r11, cl
    jmp .lab_ent

.p2align 4
.lab_top:
    mov r10, [rsi+rbx*8-8]
    shld r11, r10, cl
.lab_last:                             # r13:r12 = ah < b = r9:r8
    mov rax, rbp
    mul r13
    mov r14, r12
    add r14, rax
    adc rdx, r13
    mov r13, r14                       # r13 = ql
    mov r14, rdx                       # r14 = qh
    imul rdx, r9
    xchg r13, r12                      # r12=ql, r13=a1
    mov rax, r8
    sub r13, rdx
    xchg r12, r11                      # r11=ql, r12=a0
    mul r14
    sub r12, r8
    sbb r13, r9
    sub r12, rax
    sbb r13, rdx                       # r13:r12 = z
    xor rax, rax
    xor rdx, rdx
    cmp r13, r11
    cmovnc rax, r8
    cmovnc rdx, r9
    adc r14, 0                         # 修正 qh（存储版本需要）
    add r12, rax
    adc r13, rdx
    cmp r13, r9
    jae .lab_fx
.lab_ok:
    mov [rdi+rbx*8], r14               # 存储商
    mov r11, r10
.lab_ent:
    sub rbx, 1
    ja .lab_top
    jb .lab_ret
    shl r11, cl
    jmp .lab_last

.lab_fx:
    seta dl
    cmp r12, r8
    setae al
    or al, dl
    jz .lab_ok
    inc r14                            # 修正 qh
    sub r12, r8
    sbb r13, r9
    jmp .lab_ok

.lab_ret:
    shrd r12, r13, cl
    shr r13, cl
.lab_retnsh:
    pop rx3                            # 恢复 numb 指针
    mov [rx3], r12                     # 存储余数低 64 位
    mov [rx3+8], r13                   # 存储余数高 64 位
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
win pop rsi
win pop rdi
    ret

#------------------------ dstq == NULL ：调用 lmmp_mod_2_ -------------------------
.mod_path:
    # 参数映射：numa, na, numb -> rcx, rdx, r8 (Win) 或 rdi, rsi, rdx (Linux)
#ifdef LMMP_WINDOWS
    mov rcx, rdx                       # numa -> rcx
    mov rdx, r8                        # na   -> rdx
    mov r8, r9                         # numb -> r8
    sub rsp, 40                        # 入口 rsp 偏移 8，需 40 调整
    CALL(lmmp_mod_2_)
    add rsp, 40
#else
    mov rdi, rsi                       # numa -> rdi
    mov rsi, rdx                       # na   -> rsi
    mov rdx, rcx                       # numb -> rdx
    sub rsp, 8
    CALL(lmmp_mod_2_)
    add rsp, 8
#endif
    ret

#======================================================================
# mp_limb_t lmmp_div_2_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_srcptr numb)
#======================================================================
.globl ASM_GSYM(lmmp_div_2_s_)
.p2align 4
ASM_GSYM(lmmp_div_2_s_):
    push rbp
    push rbx
    push r12
    push r13
win push rdi
win push rsi

    lea rbx, [rx2-1]
win mov rdi, rcx
win mov rsi, rdx
    mov r12, [rx3]                     # 除数低
    mov r13, [rx3+8]                   # 除数高
lin mov r10, rdi                       # 保存 rdi
    mov rx0, r13                       # 参数

    # 栈对齐与影子空间（Win 已 push 6 个，偏移 56，需 40）
#ifdef LMMP_WINDOWS
    sub rsp, 40
#else
    sub rsp, 8
#endif
    CALL(lmmp_inv_1_)
#ifdef LMMP_WINDOWS
    add rsp, 40
#else
    add rsp, 8
#endif

lin mov rdi, r10
    mov r9, r13
    mov r8, r12
    mov rbp, rax

    imul r13, rax
    xor rcx, rcx
    mul r8
    add r13, r8
    adc rcx, -1
    add r13, rdx
    adc rcx, 0
    js .lab_2f_s
.lab_1b_s:
    dec rbp
    sub rax, r8
    sbb r13, r9
    sbb rcx, 0
    jns .lab_1b_s
.lab_2f_s:
    xor rcx, rcx
    mov rax, [rsi+rbx*8-8]
    mov rdx, [rsi+rbx*8]
    mov r12, rax
    mov r13, rdx
    sub r12, r8
    sbb r13, r9
    setnc cl
    cmovc r12, rax
    cmovc r13, rdx
    dec rbx
.p2align 4
.lab_top_s:
    mov r11, [rsi+rbx*8-8]
    mov rax, rbp
    mul r13
    mov r10, r12
    add r10, rax
    adc rdx, r13
    mov r13, r10
    mov r10, rdx
    imul rdx, r9
    xchg r13, r12
    mov rax, r8
    sub r13, rdx
    xchg r12, r11
    mul r10
    sub r12, r8
    sbb r13, r9
    sub r12, rax
    sbb r13, rdx
    xor rax, rax
    xor rdx, rdx
    cmp r13, r11
    cmovnc rax, r8
    cmovnc rdx, r9
    adc r10, 0
    add r12, rax
    adc r13, rdx
    cmp r13, r9
    jae .lab_fx_s
.lab_ok_s:
    mov [rdi+rbx*8-8], r10
    dec rbx
    jnz .lab_top_s
    mov [rsi], r12
    mov [rsi+8], r13
    mov rax, rcx
win pop rsi
win pop rdi
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
.lab_fx_s:
    seta dl
    cmp r12, r8
    setae al
    or al, dl
    jz .lab_ok_s
    inc r10
    sub r12, r8
    sbb r13, r9
    jmp .lab_ok_s
