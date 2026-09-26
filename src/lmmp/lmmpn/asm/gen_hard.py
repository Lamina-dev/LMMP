#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_hard.py -- LMMP 硬编码乘法/平方汇编生成器

算法结构:
  x64  mul, N<=8 : 寄存器整行累加(mulx+adcx/adox 双进位链, 行间寄存器环轮转)
  x64  mul, N>=9 : 种子 mul_1 + 展开 addmul_2(双乘数交错列) + 尾部直存
  x64  sqr, N<=3 : 库内 sqr_basecase.S 小规模分支特化; N>=4: 交叉积列累加
                   (8 列寄存器窗口+adcx/adox 双链, 计划式发射) -> 倍增 -> 对角
  arm64 mul      : 种子 mul_1 + 展开 addmul_1, 双链批结构
                   (链1: s_k=lo_k+hi_{k-1}+C; 链2: dst+=s; carry 走 x15)
  arm64 sqr, N<=3 : 库内 sqr_basecase.S 小规模分支特化; N>=4: 2a 乘数单趟
                   (m_i = 2a_i + ov_{i-1}, ov 由 bit63 现场生成; 对角即时
                   折叠, 无整体倍增/对角 pass)

约定: intel 语法 mulx HI, LO, src (HI=第一目的操作数)。

用法: python gen_hard.py  (在 asm/ 目录下执行, 生成 x64/ 与 arm64/ 下的 .S)
"""

import os

MAXN = 19

COPYRIGHT = """\
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
//  This program is distributed WITHOUT ANY WARRANTY.
//
//  See <https://www.gnu.org/licenses/>.
//
//  本文件由 gen_hard.py 生成, 请勿手工修改; 重新生成: python gen_hard.py
"""


def r8name(r):
    """64位寄存器名 -> 8位名 (setc/movzx 用)"""
    m = {"rax": "al", "rbx": "bl", "rcx": "cl", "rdx": "dl",
         "rsi": "sil", "rdi": "dil", "rbp": "bpl", "rsp": "spl"}
    return m.get(r, r + "b")


def e32(r):
    """64位寄存器名 -> 32位名 (xor 清零用)"""
    m = {"rax": "eax", "rbx": "ebx", "rcx": "ecx", "rdx": "edx", "rsi": "esi", "rdi": "edi", "rbp": "ebp"}
    return m.get(r, r + "d")


CALLEE_X64 = ["rbx", "rbp", "r12", "r13", "r14", "r15"]


# =============================================================================
# x64 乘法
# =============================================================================

X64_RING_POOL = ["rax", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]


def x64_prologue(used, remap_args):
    """生成函数序言. used: 需保存的被调用者寄存器集合.
    remap_args: 2 -> (dst,numa) 两参(sqr); 3 -> (dst,numa,numb) 三参(mul).
    内部约定 rdi=dst rsi=numa [rcx=numb]: Win 下 rdi/rsi 为被调用者保存,
    压栈后自 (rcx,rdx[,r8]) 重映射; SysV 前两参天然在位, 三参时 numb
    仍需自 rdx 移入 rcx."""
    out = []
    if remap_args:
        out.append("#ifdef LMMP_WINDOWS")
        out.append("    push    rdi")
        out.append("    push    rsi")
        out.append("    mov     rdi, rcx                // dst")
        out.append("    mov     rsi, rdx                // numa")
        if remap_args == 3:
            out.append("    mov     rcx, r8                 // numb")
            out.append("#else")
            out.append("    mov     rcx, rdx                // numb")
        out.append("#endif")
    for r in CALLEE_X64:
        if r in used:
            out.append("    push    " + r)
    return "\n".join(out)


def x64_epilogue(used, remap_args):
    out = []
    for r in reversed(CALLEE_X64):
        if r in used:
            out.append("    pop     " + r)
    if remap_args:
        out.append("#ifdef LMMP_WINDOWS")
        out.append("    pop     rsi")
        out.append("    pop     rdi")
        out.append("#endif")
    out.append("    ret")
    return "\n".join(out)


def x64_mul_n1():
    b = []
    b.append("    # SysV: rdi=dst rsi=numa rdx=numb | Win: rcx=dst rdx=numa r8=numb")
    b.append("#ifdef LMMP_WINDOWS")
    b.append("    mov     r10, [rdx]              // a0")
    b.append("    mov     rdx, [r8]               // b0 -> rdx (mulx 隐式乘数)")
    b.append("    mulx    r11, rax, r10           // a0*b0: r11=hi rax=lo")
    b.append("    mov     [rcx], rax")
    b.append("    mov     [rcx+8], r11")
    b.append("#else")
    b.append("    mov     rdx, [rdx]              // b0 -> rdx")
    b.append("    mulx    r11, rax, [rsi]         // a0*b0")
    b.append("    mov     [rdi], rax")
    b.append("    mov     [rdi+8], r11")
    b.append("#endif")
    b.append("    ret")
    return "\n".join(b)


def x64_mul_regpass(b, ring, scr, zero, pcreg, rreg, nrows, s, off, inner_b, rmw, spare=None):
    """发射一个寄存器方案 pass (整行累加).

    inner_b: True -> inner=numb(rcx), rows=numa(rsi); False -> inner=numa, rows=numb.
    ring/scr 为可变列表(行间轮转), 返回 (ring, scr).
    spare: 第二 lo 传送带寄存器 (mulx 成对批排用), None 则逐积单排。
    rmw (pass>=1): 终化列读加写; pc(pcreg)<=3 跨行累积列完成进位,
    rreg 捕获 cf1, pass 尾部以线性 adc 链消化。 (已通过 2 万次模拟验证)
    """
    inner, ioff = ("rcx", 8 * off) if inner_b else ("rsi", 0)
    rows = "rsi" if inner_b else "rcx"
    n = s
    pcb = r8name(pcreg)
    rb = r8name(rreg)

    # ---- 行 0: m 序列 (mulx 成对批排: 乘端口连供, 单 CF 链消费) ----
    b.append("    mov     rdx, [%s+%d]" % (rows, 0))
    b.append("    mulx    %s, %s, [%s+%d]" % (ring[0], scr, inner, ioff))
    if rmw:
        b.append("    mov     rdx, [rdi+%d]" % (8 * off))
        b.append("    add     rdx, %s" % scr)
        b.append("    mov     [rdi+%d], rdx" % (8 * off))
        b.append("    setc    %s" % pcb)
        b.append("    movzx   %s, %s" % (pcreg, pcb))
        # 清残留 CF (add rdx,lo0 的进位已收进 pc, 不清除会被本行 k 循环
        # 首个 adcx 重复消费); 兼清 r15 (作行内 lo 传送带)
        b.append("    xor     %s, %s" % (e32(rreg), e32(rreg)))
        b.append("    mov     rdx, [%s+%d]" % (rows, 0))
    else:
        b.append("    mov     [rdi+%d], %s" % (8 * off, scr))
    k = 1
    while k <= n - 1:
        if spare is not None and k + 1 <= n - 1:
            b.append("    mulx    %s, %s, [%s+%d]" % (ring[k], scr, inner, ioff + 8 * k))
            b.append("    mulx    %s, %s, [%s+%d]" % (ring[k + 1], spare, inner, ioff + 8 * (k + 1)))
            b.append("    adcx    %s, %s" % (ring[k - 1], scr))
            b.append("    adcx    %s, %s" % (ring[k], spare))
            k += 2
        else:
            b.append("    mulx    %s, %s, [%s+%d]" % (ring[k], scr, inner, ioff + 8 * k))
            b.append("    adcx    %s, %s" % (ring[k - 1], scr))
            k += 1
    b.append("    adcx    %s, %s" % (ring[n - 1], zero))

    # ---- 行 j = 1..nrows-1: am 序列 (CF 链收 hi, OF 链收 lo; mulx 成对批排) ----
    for j in range(1, nrows):
        col = off + j
        b.append("    mov     rdx, [%s+%d]" % (rows, 8 * j))
        if spare == rreg:
            # rreg 行内借作第二 lo 传送带, 行首清零 (setc 仅写低字节,
            # 上 56 位残留会污染 add pcreg, rreg; mov 不触标志)
            b.append("    mov     %s, 0" % e32(rreg))
        b.append("    mulx    %s, %s, [%s+%d]" % (ring[n], scr, inner, ioff))
        b.append("    adcx    %s, %s" % (ring[0], scr))
        if rmw:
            b.append("    setc    %s" % rb)
            b.append("    mov     rdx, [rdi+%d]" % (8 * col))
            b.append("    add     rdx, %s" % pcreg)
            b.append("    setc    %s" % pcb)
            b.append("    movzx   %s, %s" % (pcreg, pcb))
            b.append("    add     rdx, %s" % ring[0])
            b.append("    mov     [rdi+%d], rdx" % (8 * col))
            b.append("    adc     %s, 0" % pcreg)
            b.append("    add     %s, %s" % (pcreg, rreg))
            # 清 CF/OF: 普通 add 会统一两条进位链的依赖, 若不清, 本行
            # k 循环的 adcx/adox 双链退化为合并串行 (实测慢 ~25%)
            b.append("    xor     %s, %s" % (e32(rreg), e32(rreg)))
            b.append("    mov     rdx, [%s+%d]" % (rows, 8 * j))
        else:
            b.append("    mov     [rdi+%d], %s" % (8 * col, ring[0]))
        # k 成对 (k 奇): hi 滞后一个积才被 CF 链消费, 故第二个 mulx 的 hi
        # 目的须待 pending hi 被消费后释放 ("先消费后复用" 交错式)
        k = 1
        while k <= n - 1:
            if spare is not None and k + 1 <= n - 1:
                h1 = scr if k % 2 == 1 else ring[n]
                h1p = ring[n] if k % 2 == 1 else scr
                h2 = h1p                          # 消费 h1p 后复用其寄存器
                b.append("    mulx    %s, %s, [%s+%d]" % (h1, ring[0], inner, ioff + 8 * k))
                b.append("    adcx    %s, %s" % (ring[k], h1p))
                b.append("    mulx    %s, %s, [%s+%d]" % (h2, spare, inner, ioff + 8 * (k + 1)))
                b.append("    adox    %s, %s" % (ring[k], ring[0]))
                b.append("    adcx    %s, %s" % (ring[k + 1], h1))
                b.append("    adox    %s, %s" % (ring[k + 1], spare))
                k += 2
            else:
                hi_dest = scr if k % 2 == 1 else ring[n]
                hi_prev = ring[n] if k % 2 == 1 else scr
                b.append("    mulx    %s, %s, [%s+%d]" % (hi_dest, ring[0], inner, ioff + 8 * k))
                b.append("    adcx    %s, %s" % (ring[k], hi_prev))
                b.append("    adox    %s, %s" % (ring[k], ring[0]))
                k += 1
        hi_dest = scr if (n - 1) % 2 == 1 else ring[n]
        b.append("    adcx    %s, %s" % (hi_dest, zero))
        b.append("    adox    %s, %s" % (hi_dest, zero))
        if n % 2 == 1:
            ring = ring[1:] + ring[:1]
        else:
            old_n = ring[n]
            ring = ring[1:n] + [scr, ring[0]]
            scr = old_n

    # ---- 尾部: 存 dst[off+nrows ..], rmw 时 pc 以线性链消化 ----
    if rmw:
        b.append("    mov     rdx, %s" % ring[0])
        b.append("    add     rdx, %s" % pcreg)
        b.append("    mov     [rdi+%d], rdx" % (8 * (off + nrows)))
        for k in range(1, n):
            b.append("    mov     rdx, %s" % ring[k])
            b.append("    adc     rdx, 0")
            b.append("    mov     [rdi+%d], rdx" % (8 * (off + nrows + k)))
    else:
        for k in range(n):
            b.append("    mov     [rdi+%d], %s" % (8 * (off + nrows + k), ring[k]))
    return ring, scr


def x64_mul_tier1(n):
    """2 <= n <= 8: 单趟 FLINT 寄存器整行累加 (mulx 成对批排; n=8 寄存器
    已满 15 个, 无第二 lo 传送带, 退化为逐积单排)."""
    ring = X64_RING_POOL[: n + 1]
    rest = X64_RING_POOL[n + 1:]
    spare = None
    if len(rest) >= 3:
        scr, zero, spare = rest[0], rest[1], rest[2]
    elif len(rest) == 2:
        scr, zero = rest[0], rest[1]
        spare = "rbx"
    elif len(rest) == 1:
        scr, zero = rest[0], "rbx"
        spare = "rbp"
    else:
        scr, zero = "rbx", "rbp"
    used = set(X64_RING_POOL) | {scr, zero}
    if spare is not None:
        used.add(spare)

    b = []
    b.append("    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数")
    b.append("    # ring=[%s] scr=%s zero=%s spare=%s"
             % (",".join(ring), scr, zero, spare if spare else "-"))
    b.append(x64_prologue(used, 3))
    b.append("    xor     %s, %s                  // 清 CF/OF" % (e32(zero), e32(zero)))
    ring, scr = x64_mul_regpass(b, ring, scr, zero, zero, zero, n, n, 0, False, False, spare)
    b.append(x64_epilogue(used, 3))
    return "\n".join(b)


def x64_mul_tier2(n):
    """n >= 9: numb 分块, 每块一趟寄存器方案 (乘法总量恰为 n^2).
    pass 0 纯写; pass >= 1 终化列 RMW, 列完成进位累积进 pc 寄存器 (<=3),
    pass 尾部以线性 adc 链消化. am 序列保持逐积单排: 无空闲第二 lo 传送
    带, 借 rreg 需行首清零且 mulx 对中须夹入消费指令 (hi 滞后一积才被
    CF 链消费), 实测交错式配对反慢 7%~25%, 弃."""
    ring = X64_RING_POOL[:7]  # 7 个, 最大块 6 用满
    scr, rreg = X64_RING_POOL[7], X64_RING_POOL[8]
    zero, pcreg = "rbx", "rbp"
    used = set(X64_RING_POOL) | {scr, zero, pcreg}

    b = []
    b.append("    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数; numb 分块多趟寄存器方案")
    b.append(x64_prologue(used, 3))
    b.append("    xor     %s, %s                  // 清 CF/OF" % (e32(zero), e32(zero)))
    # 均衡分块: 每块 <= 6 (n>=9 时每块 >= 3)
    nparts = (n + 5) // 6
    base, extra = divmod(n, nparts)
    offs = []
    off = 0
    for q in range(nparts):
        s = base + (1 if q < extra else 0)
        offs.append((off, s))
        off += s
    for p, (off, s) in enumerate(offs):
        b.append("    # === pass %d: numb[%d..%d) x numa, 环宽 %d ===" % (p, off, off + s, s))
        ring, scr = x64_mul_regpass(b, ring, scr, zero, pcreg, rreg, n, s, off, True, p > 0, None)
    b.append(x64_epilogue(used, 3))
    return "\n".join(b)


def gen_x64_mul():
    parts = [COPYRIGHT, """
//
// void lmmp_mul_hard_N_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb);
//
//   平衡硬编码乘法: [dst,2N] = [numa,N] * [numb,N], N 为编译期常量。
//   N <= 8 : FLINT 式寄存器整行累加(mulx+adcx/adox 双进位链)
//   N >= 9 : 种子 mul_1 + 展开 addmul_2(双乘数交错列)
//

#include "lmmp_asm.h"

.text
"""]
    for n in range(1, MAXN + 1):
        parts.append(".globl ASM_GSYM(lmmp_mul_hard_%d_)\n.p2align 4\n" % n)
        parts.append("ASM_GSYM(lmmp_mul_hard_%d_):\n" % n)
        if n == 1:
            parts.append(x64_mul_n1())
        elif n <= 8:
            parts.append(x64_mul_tier1(n))
        else:
            parts.append(x64_mul_tier2(n))
        parts.append("")
    return "\n".join(parts)


# =============================================================================
# x64 平方
# =============================================================================

def x64_sqr_n1():
    return """\
    mov     rax, [rx1]
    mul     rax
    mov     [rx0], rax
    mov     [rx0+8], rdx
    ret"""


def x64_sqr_n2():
    # 摘自库内 sqr_basecase.S 的 na==2 分支
    return """\
    mov     rax, [rx1]
    mov     r11, [rx1+8]
    mov     r8, rax
    mul     rax
    mov     [rx0], rax
    mov     rax, r11
    mov     r9, rdx
    mul     rax
    mov     r10, rax
    mov     rax, r11
    mov     r11, rdx
    mul     r8
    xor     r8, r8
    add     r9, rax
    adc     r10, rdx
    adc     r11, r8
    add     r9, rax
    mov     [rx0+8], r9
    adc     r10, rdx
    mov     [rx0+16], r10
    adc     r11, r8
    mov     [rx0+24], r11
    ret"""


def x64_sqr_n3():
    # 摘自库内 sqr_basecase.S 的 na==3 分支 (含 Windows rsi/rdi 保存)
    return """\
    PUSH_RSI
    PUSH_RDI
    mov     rsi, rx1
    mov     rdi, rx0
    mov     rax, [rsi]
    mov     r10, rax
    mul     rax
    mov     r11, [rsi+8]
    mov     [rdi], rax
    mov     rax, r11
    mov     [rdi+8], rdx
    mul     rax
    mov     rcx, [rsi+16]
    mov     [rdi+16], rax
    mov     rax, rcx
    mov     [rdi+24], rdx
    mul     rax
    mov     [rdi+32], rax
    mov     [rdi+40], rdx
    mov     rax, r11
    mul     r10
    mov     r8, rax
    mov     rax, rcx
    mov     r9, rdx
    mul     r10
    xor     r10, r10
    add     r9, rax
    mov     rax, r11
    mov     r11, r10
    adc     r10, rdx
    mul     rcx
    add     r10, rax
    adc     rdx, r11
    add     r8, r8
    adc     r9, r9
    adc     r10, r10
    adc     rdx, rdx
    adc     r11, r11
    add     [rdi+8], r8
    adc     [rdi+16], r9
    adc     [rdi+24], r10
    adc     [rdi+32], rdx
    adc     [rdi+40], r11
    POP_RDI
    POP_RSI
    ret"""


X64_WIN8 = ["rbx", "rbp", "r8", "r9", "r10", "r12", "r13", "r14"]  # 列窗口寄存器, 列 c -> WIN[c % 8]
X64_SQ_T = ["rax", "rcx"]   # 越窗段双临时 (载入-累加-回写)
X64_SQ_SH = "r11"           # 段/尾 mulx 高位目的; 兼零寄存器(使用后以 mov 恢复 0)
X64_SQ_SL = "r15"           # 段/尾 mulx 低位目的 (折叠临时复用)


def x64_sqr_plan(n):
    """n >= 4: 交叉积列累加指令计划 (op 元组序列, 渲染与模拟共用同一计划).

    dst = 2*T + D,  T = sum_{i<j} a_i*a_j*B^(i+j) (交叉列 1..2n-2),  D = sum a_i^2*B^2i.
    逐行扫描 i = 0..n-2, 积 a_i*a_j 的 lo/hi 分别累入列 i+j / i+j+1:
      - 8 列寄存器窗口 [2i+1, 2i+8], 完成列溢写栈数组 M, 越窗积经双临时处理;
      - adcx(CF)/adox(OF) 双进位链, 每条链的目的列严格递增, 积的进位由下一列
        同链指令消费 (权重 B 正确); 行尾两个余进位收入顶列 i+n/i+n+1 并可证终止
        (每行结束 CF=OF=0);
      - 行 0 种子: mulx 直写新鲜列 (仅单链);
      - 倍增 T *= 2 (寄存器链或 M 波动扫描), setc 收顶;
      - 对角折叠: 纯 adc 链 R_k = T'_k + D_k, a_i^2 由 mulx 即时产生.
        (曾实测融合倍增方案: 省 M 往返但每 limb 双链串行 2 周期, 慢 ~2.5%, 弃)
    返回 (ops, prezero): prezero 为必须预清零的 M 列 (会在 RMW 前被读).
    """
    W = 8
    mtop = 2 * n - 2
    full = mtop <= W          # n <= 5: 全寄存器, 无 M
    ops = []
    prezero = set()
    minit = set()             # M 列已初始化(可安全 RMW/读取)

    def R(c):
        return ("r", X64_WIN8[c % W])

    def rmw(c):
        if c not in minit:
            minit.add(c)
            prezero.add(c)

    def store_m(c, src):
        ops.append(("mov", ("M", c), src))
        minit.add(c)

    # ================= 行扫描: 交叉积列累加 =================
    for i in range(n - 1):
        ops.append(("#", "--- 行 %d: 乘数 a_%d ---" % (i, i)))
        # 窗口滑动: 退役完成列 (2i-1, 2i 在行 i-1 完成), 录入新列 (2i+7, 2i+8)
        if (not full) and i >= 1:
            for c in (2 * i - 1, 2 * i):
                store_m(c, R(c))
            for c in (2 * i + W - 1, 2 * i + W):
                if c <= mtop:
                    if c in minit:
                        ops.append(("mov", R(c), ("M", c)))
                    else:
                        ops.append(("mov", R(c), ("imm", 0)))
        ops.append(("mov", ("r", "rdx"), ("a", i)))
        if i == 0:
            # 行 0 种子: 未直写的窗口列清零
            for c in range(min(n - 1, W) + 1, min(mtop, W) + 1):
                ops.append(("mov", R(c), ("imm", 0)))
            ops.append(("mulx", R(2), R(1), ("a", 1)))       # lo01->列1, hi01->列2 (直写)
            ops.append(("xor0", ("r", X64_SQ_SH)))            # CF=OF=0
            j = 2
            while j <= min(n - 2, W - 1):                    # hi 直写新鲜列 j+1, mulx 成对批排
                if j + 1 <= min(n - 2, W - 1):
                    ops.append(("mulx", R(j + 1), ("r", "rax"), ("a", j)))
                    ops.append(("mulx", R(j + 2), ("r", "r15"), ("a", j + 1)))
                    ops.append(("adcx", R(j), ("r", "rax")))
                    ops.append(("adcx", R(j + 1), ("r", "r15")))
                    j += 2
                else:
                    ops.append(("mulx", R(j + 1), ("r", "rax"), ("a", j)))
                    ops.append(("adcx", R(j), ("r", "rax")))
                    j += 1
        else:
            j = i + 1
            hi = min(n - 2, i + W - 1)
            while j <= hi:                                    # mulx 成对批排 (仿 FLINT am 结构)
                if j + 1 <= hi:
                    ops.append(("mulx", ("r", "rcx"), ("r", "rax"), ("a", j)))
                    ops.append(("mulx", ("r", X64_SQ_SH), ("r", X64_SQ_SL), ("a", j + 1)))
                    ops.append(("adcx", R(i + j), ("r", "rax")))
                    ops.append(("adox", R(i + j + 1), ("r", "rcx")))
                    ops.append(("adcx", R(i + j + 1), ("r", X64_SQ_SL)))
                    ops.append(("adox", R(i + j + 2), ("r", X64_SQ_SH)))
                    j += 2
                else:
                    ops.append(("mulx", ("r", "rcx"), ("r", "rax"), ("a", j)))
                    ops.append(("adcx", R(i + j), ("r", "rax")))
                    ops.append(("adox", R(i + j + 1), ("r", "rcx")))
                    j += 1
        # ---- 越窗段: 积 j = i+8..n-2, 双临时 (t0/t1) 逐列滚动 ----
        seg = (not full) and (i + W <= n - 2)
        tH = None
        if seg:
            base = 2 * i + W        # 首段积 lo 列 = 窗口边列
            L = (n - 2) - (i + W) + 1
            t = X64_SQ_T
            ops.append(("mov", ("r", t[0]), ("M", base + 1)))
            rmw(base + 1)
            if L >= 2:
                ops.append(("mov", ("r", t[1]), ("M", base + 2)))
                rmw(base + 2)
            for k in range(L):
                j = i + W + k
                ops.append(("mulx", ("r", X64_SQ_SH), ("r", X64_SQ_SL), ("a", j)))
                tx = R(base + k) if k == 0 else ("r", t[(k - 1) % 2])
                ops.append(("adcx", tx, ("r", X64_SQ_SL)))
                ty = ("r", t[k % 2])
                ops.append(("adox", ty, ("r", X64_SQ_SH)))
                if k >= 1:
                    store_m(base + k, tx)
                    if base + k + 2 <= i + n:     # 供后续段积或尾部 c_hi 使用
                        ops.append(("mov", tx, ("M", base + k + 2)))
                        rmw(base + k + 2)
        # ---- 尾积 j = n-1: hi 入新鲜顶列 i+n, lo 入列 i+n-1, 行尾余进位入顶列 ----
        c_lo, c_hi, c_top = i + n - 1, i + n, i + n + 1
        last = (i == n - 2)
        ops.append(("mulx", ("r", X64_SQ_SH), ("r", X64_SQ_SL), ("a", n - 1)))
        # 顶列 c_hi 先收 hi + OF 余进位 (来自积 n-2 的 adox)
        if seg and L >= 2:
            tH = ("r", t[(L - 2) % 2])            # 段末重载的 c_hi
            ops.append(("adox", tH, ("r", X64_SQ_SH)))
        elif full or c_hi <= 2 * i + W:
            ops.append(("adox", R(c_hi), ("r", X64_SQ_SH)))
        else:
            ops.append(("mov", ("r", "rdx"), ("M", c_hi)))
            rmw(c_hi)
            ops.append(("adox", ("r", "rdx"), ("r", X64_SQ_SH)))
            tH = ("r", "rdx")
        # 列 c_lo 收 lo + CF 余进位 (来自积 n-2 的 adcx)
        if seg:
            tX = ("r", t[(L - 1) % 2])            # 段末 Y 临时持有 c_lo
            ops.append(("adcx", tX, ("r", X64_SQ_SL)))
            store_m(c_lo, tX)
        else:
            ops.append(("adcx", R(c_lo), ("r", X64_SQ_SL)))
        ops.append(("mov", ("r", X64_SQ_SH), ("imm", 0)))     # 恢复零寄存器 (不触标志)
        # 顶列 c_top 收 OF2 (行 0: 列 c_hi 初值 0, 可证无 OF2; 末行: 数学界无)
        kind_top = None
        if i >= 1 and not last:
            if full or c_top <= 2 * i + W:
                kind_top = R(c_top)
                ops.append(("adox", kind_top, ("r", X64_SQ_SH)))
            else:
                ops.append(("mov", ("r", X64_SQ_SL), ("M", c_top)))
                rmw(c_top)
                ops.append(("adox", ("r", X64_SQ_SL), ("r", X64_SQ_SH)))
                kind_top = ("r", X64_SQ_SL)
        # 顶列 c_hi 收 CF (列 c_lo 加法的余进位)
        if tH is not None:
            ops.append(("adcx", tH, ("r", X64_SQ_SH)))
            store_m(c_hi, tH)
        else:
            ops.append(("adcx", R(c_hi), ("r", X64_SQ_SH)))
        # 顶列 c_top 收 CF2 (末行: T < B^(2n-1) 可证无)
        if not last:
            if kind_top is None:                  # 行 0: 补载入 c_top
                if full or c_top <= 2 * i + W:
                    kind_top = R(c_top)
                else:
                    ops.append(("mov", ("r", X64_SQ_SL), ("M", c_top)))
                    rmw(c_top)
                    kind_top = ("r", X64_SQ_SL)
            ops.append(("adcx", kind_top, ("r", X64_SQ_SH)))
            if kind_top == ("r", X64_SQ_SL):      # SL 临时持有的 mem 列需回写
                store_m(c_top, kind_top)
    if not full:
        for c in (mtop - 1, mtop):                # 末两列退役
            store_m(c, R(c))

    # ================= 倍增 T *= 2 =================
    # 独立 add/adc 链 (1 周期/列) 与折叠的纯 adc 链互不依赖, 可被乱序引擎
    # 重叠; 实测融合倍增(每 limb adcx+adox 串行 2 周期)反而慢 ~2.5%, 故
    # 保留两段式。
    ops.append(("#", "--- 倍增 ---"))
    if full:
        ops.append(("add", R(1), R(1)))
        for c in range(2, mtop + 1):
            ops.append(("adc", R(c), R(c)))
        ops.append(("movzxc", ("r", X64_SQ_SH)))  # 倍增顶进位 (列 2n-1, 属 {0,1})
    else:
        if mtop >= 22:
            # AVX2 位移-或就地倍增 (仿 FLINT): 每组 4 列 ymm, 重叠读低邻列
            # 取 msb (vpsrlq $63 于 [c-1..c+2]) 与 vpsllq $1 于 [c..c+3]
            # 相或, 进位经位移在向量内传播, 消除标量 add/adc 串行链与寄存
            # 器往返; 组间自顶向下 (每组只读自身列与更低列的原始值, 低组尚
            # 未写); 最低 mtop%4 列 (<=2, mtop 恒偶) 标量收尾, 其 msb 由上方
            # 向量组的重叠读在覆写前捕获; 顶列 msb 预先标量取出。
            # 仅大 n 启用: 向量路径首列就绪有 ~10 周期固定延迟 (载入->移位
            # ->或->存->折叠读转发), mtop<22 时串行链短省不抵损 (实测
            # n=6..11 慢 2%~36%, n=12..19 快 4%~10.5%)。
            ops.append(("mov", ("r", X64_SQ_SH), ("M", mtop)))
            ops.append(("shr", ("r", X64_SQ_SH), 63))
            rem = mtop % 4
            if rem == 0 and 0 not in prezero:
                # 最低组自列 1 起, 重叠读触列 0 (恒 0), 需预清零 M[0]
                prezero.add(0)
                minit.add(0)
            c = mtop - 3
            while c >= 1 + rem:
                ops.append(("vdbl4", c))
                c -= 4
            if rem:
                ops.append(("mov", ("r", "rax"), ("M", 1)))
                ops.append(("add", ("r", "rax"), ("r", "rax")))
                ops.append(("mov", ("M", 1), ("r", "rax")))
                if rem == 2:
                    ops.append(("mov", ("r", "rax"), ("M", 2)))
                    ops.append(("adc", ("r", "rax"), ("r", "rax")))
                    ops.append(("mov", ("M", 2), ("r", "rax")))
            ops.append(("vzeroupper",))
        else:
            # 标量波扫描: 4 寄存器乒乓, 预取下一波, add/adc 链 1 周期/列
            TA = ["rax", "rcx", "rdx", "rbx"]
            TB = ["rbp", "r8", "r9", "r10"]
            cols = list(range(1, mtop + 1))
            waves = [cols[k:k + 4] for k in range(0, len(cols), 4)]
            cur = TA
            for idx, cc in enumerate(waves[0]):
                ops.append(("mov", ("r", cur[idx]), ("M", cc)))
            for wi, wave in enumerate(waves):
                for idx, cc in enumerate(wave):
                    ops.append(("add" if (wi == 0 and idx == 0) else "adc",
                                ("r", cur[idx]), ("r", cur[idx])))
                if wi + 1 < len(waves):
                    nxt = TB if cur is TA else TA
                    for idx, cc in enumerate(waves[wi + 1]):
                        ops.append(("mov", ("r", nxt[idx]), ("M", cc)))
                for idx, cc in enumerate(wave):
                    ops.append(("mov", ("M", cc), ("r", cur[idx])))
                cur = TB if cur is TA else TA
            ops.append(("movzxc", ("r", X64_SQ_SH)))

    # ================= 对角折叠: R = 2T + sum a_i^2 =================
    # 纯 adc 链 (1 周期/limb), 对角项由 mulx 即时产生, M 倍增值经寄存器加数
    ops.append(("#", "--- 对角折叠 ---"))
    src = (lambda c: R(c)) if full else (lambda c: ("M", c))
    T = ("r", X64_SQ_SL)
    ops.append(("mov", ("r", "rdx"), ("a", 0)))
    ops.append(("mulx", ("r", "rcx"), ("r", "rax"), ("r", "rdx")))   # a_0^2
    ops.append(("mov", ("A", 0), ("r", "rax")))                        # R_0 = lo (交叉列0恒0)
    ops.append(("mov", T, ("r", "rcx")))
    ops.append(("add", T, src(1)))                                     # R_1 = T'_1 + hi_0
    ops.append(("mov", ("A", 1), T))
    for i2 in range(1, n):
        ops.append(("mov", ("r", "rdx"), ("a", i2)))
        ops.append(("mulx", ("r", "rcx"), ("r", "rax"), ("r", "rdx")))
        ops.append(("mov", T, ("r", "rax")))                           # R_2i = lo(a_i^2) + ...
        ops.append(("adc", T, src(2 * i2)))
        ops.append(("mov", ("A", 2 * i2), T))
        if i2 < n - 1:
            ops.append(("mov", T, ("r", "rcx")))                       # R_2i+1 = hi(a_i^2) + ...
            ops.append(("adc", T, src(2 * i2 + 1)))
            ops.append(("mov", ("A", 2 * i2 + 1), T))
        else:
            ops.append(("mov", T, ("r", X64_SQ_SH)))                   # 倍增顶进位
            ops.append(("adc", T, ("r", "rcx")))
            ops.append(("mov", ("A", 2 * i2 + 1), T))
    all_ops = [("mov", ("M", c), ("imm", 0)) for c in sorted(prezero)] + ops
    return all_ops, sorted(prezero)


def x64_sqr_ref(x):
    k, v = x
    if k == "r":
        return v
    if k == "M":
        return "[rsp+%d]" % (8 * v)
    if k == "a":
        return "[rsi+%d]" % (8 * v)
    if k == "A":
        return "[rdi+%d]" % (8 * v)
    return "0"


def x64_sqr_asm(n):
    """渲染列累加计划为 .S 文本 (含序言/尾声/预清零合并)."""
    ops, prezero = x64_sqr_plan(n)
    used = set()
    need_m = False
    for op in ops:
        if op[0] == "#":
            continue
        for x in op[1:]:
            if isinstance(x, tuple):
                if x[0] == "r":
                    used.add(x[1])
                elif x[0] == "M":
                    need_m = True
    msize = 0
    if need_m:
        msize = (8 * (2 * n - 1) + 15) & ~15     # M[c] 偏移最大 8*(2n-2), 需再 +8

    b = []
    b.append("    # 列窗口 WIN[c%%8]=[%s]  rdx=行乘数  rax/rcx=段临时  r11=零(段内借作mulx)"
             % ",".join(X64_WIN8))
    b.append(x64_prologue(used, 2))
    if msize:
        b.append("    sub     rsp, %d               // M 交叉列数组, M[c]=[rsp+8c], c=1..%d" % (msize, 2 * n - 2))
    # 预清零 (计划开头连续的 M<-0 op, 合并为向量/标量存储)
    pz = list(prezero)
    if pz:
        if len(pz) >= 3:
            b.append("    xorps   xmm0, xmm0")
            k = 0
            while k + 1 < len(pz):
                if pz[k] + 1 == pz[k + 1]:
                    b.append("    movdqu  [rsp+%d], xmm0       // M[%d..%d] = 0" % (8 * pz[k], pz[k], pz[k] + 1))
                    k += 2
                else:
                    b.append("    mov     QWORD PTR [rsp+%d], 0" % (8 * pz[k]))
                    k += 1
            if k < len(pz):
                b.append("    mov     QWORD PTR [rsp+%d], 0" % (8 * pz[k]))
        else:
            for c in pz:
                b.append("    mov     QWORD PTR [rsp+%d], 0    // M[%d] 预清零" % (8 * c, c))
    # 主体 (跳过已渲染的预清零 op)
    body_start = len(pz)
    for op in ops[body_start:]:
        t = op[0]
        if t == "#":
            b.append("    // " + op[1])
        elif t == "mulx":
            b.append("    mulx    %s, %s, %s" % (x64_sqr_ref(op[1]), x64_sqr_ref(op[2]), x64_sqr_ref(op[3])))
        elif t in ("adcx", "adox", "add", "adc"):
            b.append("    %-7s %s, %s" % (t, x64_sqr_ref(op[1]), x64_sqr_ref(op[2])))
        elif t == "mov":
            d, s = op[1], op[2]
            if d[0] == "M" and s[0] == "imm":
                b.append("    mov     QWORD PTR [rsp+%d], 0    // M[%d] 预清零"
                         % (8 * d[1], d[1]))
            elif d[0] == "r" and s[0] == "imm":
                b.append("    mov     %s, 0" % e32(d[1]))
            else:
                b.append("    mov     %s, %s" % (x64_sqr_ref(d), x64_sqr_ref(s)))
        elif t == "xor0":
            b.append("    xor     %s, %s" % (e32(op[1][1]), e32(op[1][1])))
        elif t == "movzxc":
            r = op[1][1]
            b.append("    setc    %s" % r8name(r))
            b.append("    movzx   %s, %s" % (r, r8name(r)))
        elif t == "shr":
            b.append("    shr     %s, 63" % op[1][1])
        elif t == "vdbl4":
            c = op[1]
            b.append("    vmovdqu ymm0, [rsp+%d]" % (8 * c))
            b.append("    vmovdqu ymm1, [rsp+%d]" % (8 * (c - 1)))
            b.append("    vpsllq  ymm0, ymm0, 1")
            b.append("    vpsrlq  ymm1, ymm1, 63")
            b.append("    vpor    ymm0, ymm0, ymm1")
            b.append("    vmovdqu [rsp+%d], ymm0" % (8 * c))
        elif t == "vzeroupper":
            b.append("    vzeroupper")
    if msize:
        b.append("    add     rsp, %d" % msize)
    b.append(x64_epilogue(used, 2))
    return "\n".join(b)


def gen_x64_sqr():
    parts = [COPYRIGHT, """
//
// void lmmp_sqr_hard_N_(mp_ptr dst, mp_srcptr numa);
//
//   平衡硬编码平方: [dst,2N] = [numa,N]^2。
//   N <= 3 : 库内 sqr_basecase.S 小规模分支的特化
//   N >= 4 : 交叉积列累加(FLINT 式, 8 列寄存器窗口 + adcx/adox 双链,
//            完成列溢写栈数组) -> 整体倍增 -> 对角平方单链折叠
//

#include "lmmp_asm.h"

.text

#ifdef LMMP_WINDOWS
    #define PUSH_RSI push rsi
    #define PUSH_RDI push rdi
    #define POP_RSI  pop rsi
    #define POP_RDI  pop rdi
#else
    #define PUSH_RSI
    #define PUSH_RDI
    #define POP_RSI
    #define POP_RDI
#endif
"""]
    for n in range(1, MAXN + 1):
        parts.append(".globl ASM_GSYM(lmmp_sqr_hard_%d_)\n.p2align 4\n" % n)
        parts.append("ASM_GSYM(lmmp_sqr_hard_%d_):\n" % n)
        if n == 1:
            parts.append(x64_sqr_n1())
        elif n == 2:
            parts.append(x64_sqr_n2())
        elif n == 3:
            parts.append(x64_sqr_n3())
        else:
            parts.append(x64_sqr_asm(n))
        parts.append("")
    return "\n".join(parts)


# =============================================================================
# arm64 乘法
# =============================================================================
#
# 双链批结构 (与库内 addmul_1.S 同构, 全展开 + 常量偏移):
#   链 1(部分积):  s_k = lo_k + hi_{k-1} + C, 一条 adcs 链跨整行,
#                  乘法对穿插链节点发射 (乘端口连续供数);
#   链 2(回写):    dst[col+k] += s_k, ldp 预读 + adds 就地折叠,
#                  adc 将链 2 溢出并入 carry.
# 批间 carry 经寄存器 x15 传递 (两链溢出同权, 数学合法).
# 寄存器分派:
#   x0=dst x1=numa x2=numb x3=v(行乘数) x4..x7=a 加载对(兼 hi)
#   x8..x11=lo/s 链节点  x12..x14=hi  x15=carry(兼第4积hi)  x16,x17=dst 读对
# 每积约 6 条指令, 关键链深 2 (旧即时吸收结构为 9 条/深 4).


def arm64_mul_n1():
    return """\
    ldr     x3, [x1]
    ldr     x4, [x2]
    mul     x5, x3, x4
    umulh   x6, x3, x4
    stp     x5, x6, [x0]
    ret"""


def arm64_row_batches(b, k0, m, col0, seed, cin0=None):
    """发射 m 个积 (a[k0..k0+m) * x3) 的批序列, 首列 col0。
    seed=True 种子直存 (无 dst 读); False 为 RMW 行。批间 carry 在 x15。
    cin0: 首批首节点 carry 加数 (默认 seed 用 xzr / RMW 用 x15)。"""
    if cin0 is None:
        cin0 = "xzr" if seed else "x15"
    k, c = k0, col0
    while m >= 4:
        cin = cin0 if k == k0 else "x15"
        b.append("    ldp     x4, x5, [x1, #%d]" % (8 * k))
        b.append("    mul     x8, x4, x3")
        b.append("    umulh   x12, x4, x3")
        b.append("    ldp     x6, x7, [x1, #%d]" % (8 * (k + 2)))
        b.append("    mul     x9, x5, x3")
        b.append("    umulh   x13, x5, x3")
        b.append("    adds    x8, x8, %s" % cin)
        b.append("    mul     x10, x6, x3")
        b.append("    umulh   x14, x6, x3")
        b.append("    adcs    x9, x9, x12")
        b.append("    mul     x11, x7, x3")
        b.append("    umulh   x15, x7, x3")
        b.append("    adcs    x10, x10, x13")
        b.append("    adcs    x11, x11, x14")
        b.append("    adc     x15, x15, xzr")
        if seed:
            b.append("    stp     x8, x9, [x0, #%d]" % (8 * c))
            b.append("    stp     x10, x11, [x0, #%d]" % (8 * (c + 2)))
        else:
            b.append("    ldp     x16, x17, [x0, #%d]" % (8 * c))
            b.append("    ldp     x12, x13, [x0, #%d]" % (8 * (c + 2)))
            b.append("    adds    x8, x16, x8")
            b.append("    adcs    x9, x17, x9")
            b.append("    adcs    x10, x12, x10")
            b.append("    adcs    x11, x13, x11")
            b.append("    adc     x15, x15, xzr")
            b.append("    stp     x8, x9, [x0, #%d]" % (8 * c))
            b.append("    stp     x10, x11, [x0, #%d]" % (8 * (c + 2)))
        k += 4
        c += 4
        m -= 4
    if m == 3:
        cin = cin0 if k == k0 else "x15"
        b.append("    ldp     x4, x5, [x1, #%d]" % (8 * k))
        b.append("    mul     x8, x4, x3")
        b.append("    umulh   x12, x4, x3")
        b.append("    ldr     x6, [x1, #%d]" % (8 * (k + 2)))
        b.append("    mul     x9, x5, x3")
        b.append("    umulh   x13, x5, x3")
        b.append("    adds    x8, x8, %s" % cin)
        b.append("    mul     x10, x6, x3")
        b.append("    umulh   x14, x6, x3")
        b.append("    adcs    x9, x9, x12")
        b.append("    adcs    x10, x10, x13")
        b.append("    adc     x15, x14, xzr")
        if seed:
            b.append("    stp     x8, x9, [x0, #%d]" % (8 * c))
            b.append("    str     x10, [x0, #%d]" % (8 * (c + 2)))
        else:
            b.append("    ldp     x16, x17, [x0, #%d]" % (8 * c))
            b.append("    ldr     x12, [x0, #%d]" % (8 * (c + 2)))
            b.append("    adds    x8, x16, x8")
            b.append("    adcs    x9, x17, x9")
            b.append("    adcs    x10, x12, x10")
            b.append("    adc     x15, x15, xzr")
            b.append("    stp     x8, x9, [x0, #%d]" % (8 * c))
            b.append("    str     x10, [x0, #%d]" % (8 * (c + 2)))
    elif m == 2:
        cin = cin0 if k == k0 else "x15"
        b.append("    ldp     x4, x5, [x1, #%d]" % (8 * k))
        b.append("    mul     x8, x4, x3")
        b.append("    umulh   x12, x4, x3")
        b.append("    mul     x9, x5, x3")
        b.append("    umulh   x13, x5, x3")
        b.append("    adds    x8, x8, %s" % cin)
        b.append("    adcs    x9, x9, x12")
        b.append("    adc     x15, x13, xzr")
        if seed:
            b.append("    stp     x8, x9, [x0, #%d]" % (8 * c))
        else:
            b.append("    ldp     x16, x17, [x0, #%d]" % (8 * c))
            b.append("    adds    x8, x16, x8")
            b.append("    adcs    x9, x17, x9")
            b.append("    adc     x15, x15, xzr")
            b.append("    stp     x8, x9, [x0, #%d]" % (8 * c))
    elif m == 1:
        cin = cin0 if k == k0 else "x15"
        b.append("    ldr     x4, [x1, #%d]" % (8 * k))
        b.append("    mul     x8, x4, x3")
        b.append("    umulh   x12, x4, x3")
        b.append("    adds    x8, x8, %s" % cin)
        b.append("    adc     x15, x12, xzr")
        if seed:
            b.append("    str     x8, [x0, #%d]" % (8 * c))
        else:
            b.append("    ldr     x16, [x0, #%d]" % (8 * c))
            b.append("    adds    x8, x16, x8")
            b.append("    adc     x15, x15, xzr")
            b.append("    str     x8, [x0, #%d]" % (8 * c))


def arm64_mul(n):
    """种子 mul_1(直存) + (n-1) 行展开 addmul_1(双链 RMW), 顶列直存。
    行 j 写列 [j, j+n-1] RMW + 顶列 j+n 直存 (此前行未触及, 安全)。
    全 caller-saved 寄存器, 无压栈。"""
    b = []
    b.append("    // x0=dst x1=numa x2=numb x3=v x15=carry; 种子 mul_1 + 展开 addmul_1")

    # ---- 种子行 j=0: dst[0..n] = numa * b_0 ----
    b.append("    ldr     x3, [x2]                 // b_0")
    arm64_row_batches(b, 0, n, 0, True)
    b.append("    str     x15, [x0, #%d]           // 顶列" % (8 * n))

    # ---- 行 j = 1..n-1: dst[j..j+n-1] += numa * b_j, 顶列直存 ----
    for j in range(1, n):
        b.append("    ldr     x3, [x2, #%d]            // b_%d" % (8 * j, j))
        b.append("    mov     x15, xzr               // carry = 0")
        arm64_row_batches(b, 0, n, j, False)
        b.append("    str     x15, [x0, #%d]           // 顶列" % (8 * (j + n)))

    b.append("    ret")
    return "\n".join(b)


def gen_arm64_mul():
    parts = [COPYRIGHT, """
//
// void lmmp_mul_hard_N_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb);
//
//   平衡硬编码乘法: [dst,2N] = [numa,N] * [numb,N]。
//   种子 mul_1 + 展开 addmul_1, 双链结构:
//     链1: s_k = lo_k + hi_{k-1} + C (adcs 链跨整行)
//     链2: dst += s, 溢出并入 carry -> 顶列直存
//

#include "lmmp_asm.h"

.text
"""]
    for n in range(1, MAXN + 1):
        parts.append(".globl ASM_GSYM(lmmp_mul_hard_%d_)\n.p2align 4\n" % n)
        parts.append("ASM_GSYM(lmmp_mul_hard_%d_):\n" % n)
        if n == 1:
            parts.append(arm64_mul_n1())
        else:
            parts.append(arm64_mul(n))
        parts.append("")
    return "\n".join(parts)


# =============================================================================
# arm64 平方
# =============================================================================
#
# 三段式 (交叉行结构与 mul 同构):
#   段1 交叉行: dst = T = sum_{i<k} a_i*a_k*B^{i+k}
#        行 0 种子直存(积 a_0*a_k, k=1..n-1, 列 [1,n-1] + 顶列 n);
#        行 i=1..n-2 双链 RMW(积 a_i*a_k, k=i+1..n-1, 列 [2i+1, i+n-1] + 顶列 i+n);
#   段2 倍增: dst = 2T, 列 1..2n-1 的连续 adcs 长链 (ldp/stp 不触 CF 可穿插),
#        列 0 直存 0, 末 CF=0 由 2T < B^{2n} 保证;
#   段3 对角: dst += D = sum a_i^2, lo_i/hi_i 分离入列 2i/2i+1,
#        链节点单加数 (adds/adcs xd, xd, xzr), 批 2 积 4 limb + RMW 双链。
# 乘法总数 n(n+1)/2。


def arm64_sqr_n1():
    return """\
    ldr     x3, [x1]
    mul     x4, x3, x3
    umulh   x5, x3, x3
    stp     x4, x5, [x0]
    ret"""


def arm64_sqr_n2():
    # 摘自库内 sqr_basecase.S 的 Lsqr_2o3 分支 (na==2)
    return """\
    ldr     x3, [x1]                 // a_0
    ldr     x6, [x1,#8]              // a_1
    mul     x4, x3, x3               // lo(a0^2)
    umulh   x5, x3, x3               // hi(a0^2)
    mul     x7, x6, x6               // lo(a1^2)
    umulh   x8, x6, x6               // hi(a1^2)
    mul     x9, x3, x6               // lo(a0a1)
    umulh   x10, x3, x6              // hi(a0a1)
    adds    x5, x5, x9               // += cross terms
    adcs    x7, x7, x10
    adc     x8, x8, xzr
    adds    x5, x5, x9               // again: x2
    adcs    x7, x7, x10
    adc     x8, x8, xzr
    stp     x4, x5, [x0]
    stp     x7, x8, [x0,#16]
    ret"""


def arm64_sqr_n3():
    # 摘自库内 sqr_basecase.S 的 na==3 路径 (Lsqr_2o3 前半 + Lsqr_3)
    return """\
    ldr     x3, [x1]                 // a_0
    ldr     x6, [x1,#8]              // a_1
    mul     x4, x3, x3               // lo(a0^2)
    umulh   x5, x3, x3               // hi(a0^2)
    mul     x7, x6, x6               // lo(a1^2)
    umulh   x8, x6, x6               // hi(a1^2)
    mul     x9, x3, x6               // lo(a0a1)
    umulh   x10, x3, x6              // hi(a0a1)
    ldr     x11, [x1,#16]            // a_2
    mul     x12, x11, x11            // lo(a2^2)
    umulh   x13, x11, x11            // hi(a2^2)
    mul     x14, x3, x11             // lo(a0a2)
    umulh   x15, x3, x11             // hi(a0a2)
    mul     x16, x6, x11             // lo(a1a2)
    umulh   x17, x6, x11             // hi(a1a2)
    adds    x5, x5, x9               // cross x2
    adcs    x7, x7, x10
    adcs    x8, x8, x15
    adcs    x12, x12, x17
    adc     x13, x13, xzr
    adds    x5, x5, x9
    adcs    x7, x7, x10
    adcs    x8, x8, x15
    adcs    x12, x12, x17
    adc     x13, x13, xzr
    adds    x7, x7, x14              // += a0a2, a1a2
    adcs    x8, x8, x16
    adcs    x12, x12, xzr
    adc     x13, x13, xzr
    adds    x7, x7, x14
    adcs    x8, x8, x16
    adcs    x12, x12, xzr
    adc     x13, x13, xzr
    stp     x4, x5, [x0]
    stp     x7, x8, [x0,#16]
    stp     x12, x13, [x0,#32]
    ret"""


def arm64_sqr_rows_2a(n):
    """小 n 专用: 2a 乘数单趟结构 (与库内 sqr_basecase.S outer 同构)。
    行 i 乘数 m_i = 2a_i + ov_{i-1} (adds/adc 现场生成; 积自动补偿 ov 高位项,
    逐积零开销)。溢出等值恒等式 [2a_i+ov_{i-1} >= B] = [a_i >= B/2]
    (B 偶数, 2a_i 不可能等于 B-1) 保证 ov 恒可由 a_i 的 bit63 现场生成
    (and x, a_i, a_{i-1}, asr 63), 无需常驻掩码寄存器。
    对角 a_i^2 即时折叠: lo 经行头部机关入列 2i, hi 经 x15 融入行链首。
    强化 (n4v2 的通用化):
      1) 批残留直用: 上一行批 m_prev <= 4 (单批) 时, 批后 x4 = a_i,
         x9 = dst[2i] (首批 s1) 仍持值, 机关免 ldr 与存储转发;
      2) 掩码源恒 ldr x7 (批后恒空闲);
      3) 行 n-2 与尾融合: 行 n-2 不写顶列, 其 1 积链尾 carry 直接作
         尾列 2n-2 源, 列 [2n-2, 2n-1] 两段链直存 (每段 <=2 源,
         中间进位走寄存器, 避免多源列单 CF 链的进位回流)。"""
    b = []
    b.append("    // x0=dst x1=numa x3=m(2a+ov) x15=carry; 2a 乘数单趟 (强化)")

    # ---- 行 0 (种子, m_0 = 2a_0): D_0 折叠 + 直存列 [1, n-1] + 顶列 n ----
    b.append("    ldr     x4, [x1]                 // a_0")
    b.append("    mul     x9, x4, x4               // lo(a_0^2)")
    b.append("    umulh   x17, x4, x4              // hi(a_0^2) (种子行 x17 空闲)")
    b.append("    str     x9, [x0]                 // 列 0 = lo(a_0^2)")
    b.append("    adds    x3, x4, x4               // m_0 = 2a_0")
    arm64_row_batches(b, 1, n - 1, 1, True, "x17")
    b.append("    str     x15, [x0, #%d]           // 顶列" % (8 * n))

    # ---- 行 i = 1..n-3: 机关 + RMW 批 + 顶列 ----
    for i in range(1, n - 2):
        mp = n - i                      # 上一行积数 (批残留判定)
        b.append("    ldr     x7, [x1, #%d]             // a_%d (掩码源)" % (8 * (i - 1), i - 1))
        if mp > 4:                       # 双批: 残留被尾批覆写, 需加载
            b.append("    ldr     x4, [x1, #%d]             // a_%d" % (8 * i, i))
            b.append("    ldr     x16, [x0, #%d]            // dst[%d]" % (8 * 2 * i, 2 * i))
            src0 = "x16"
        else:                            # 单批: x4 = a_i, x9 = dst[2i] 残留直用
            src0 = "x9"
        # 累加器恒 x16 (x9 随即被 mul 覆写为 lo(D_i), 仅作首条源)
        b.append("    and     x17, x4, x7, asr 63      // ov_{%d} ? a_%d : 0" % (i - 1, i))
        b.append("    adds    x16, %s, x17             // 列 %d = 值 + 修正, C1" % (src0, 2 * i))
        b.append("    mul     x9, x4, x4               // lo(a_%d^2)" % i)
        b.append("    umulh   x12, x4, x4              // hi(a_%d^2)" % i)
        b.append("    adc     x12, x12, xzr            // hi' = hi + C1")
        b.append("    adds    x16, x16, x9             // += lo, C2")
        b.append("    str     x16, [x0, #%d]" % (8 * 2 * i))
        b.append("    adc     x15, x12, xzr            // 行种子 carry = hi' + C2")
        b.append("    adds    xzr, x7, x7              // C = ov_{%d}" % (i - 1))
        b.append("    adc     x3, x4, x4               // m_%d = 2a_%d + ov_{%d}" % (i, i, i - 1))
        arm64_row_batches(b, i + 1, n - 1 - i, 2 * i + 1, False)
        b.append("    str     x15, [x0, #%d]           // 顶列" % (8 * (i + n)))

    # ---- 行 n-2 + 尾融合: 机关(m_prev=2 直用) + 1 积 RMW + 两段链直存 ----
    i = n - 2
    b.append("    ldr     x7, [x1, #%d]             // a_%d (掩码源)" % (8 * (i - 1), i - 1))
    b.append("    and     x17, x4, x7, asr 63      // ov_{%d} ? a_%d : 0" % (i - 1, i))
    b.append("    adds    x16, x9, x17             // 列 %d = 残留值 + 修正, C1" % (2 * i))
    b.append("    mul     x9, x4, x4               // lo(a_%d^2)" % i)
    b.append("    umulh   x12, x4, x4")
    b.append("    adc     x12, x12, xzr")
    b.append("    adds    x16, x16, x9")
    b.append("    str     x16, [x0, #%d]" % (8 * 2 * i))
    b.append("    adc     x15, x12, xzr            // 行种子 carry")
    b.append("    adds    xzr, x7, x7              // C = ov_{%d}" % (i - 1))
    b.append("    adc     x3, x4, x4               // m_%d" % i)
    b.append("    mov     x6, x4                   // a_%d 备份 (尾掩码)" % i)
    b.append("    ldr     x4, [x1, #%d]            // a_%d" % (8 * (n - 1), n - 1))
    b.append("    mul     x8, x4, x3               // lo(a_%d*m_%d)" % (n - 1, i))
    b.append("    umulh   x12, x4, x3")
    b.append("    adds    x8, x8, x15              // s = lo + 种子")
    b.append("    adc     x15, x12, xzr            // carry = hi + C")
    b.append("    ldr     x16, [x0, #%d]           // dst[%d] (行 %d 顶列)" % (8 * (2 * n - 3), 2 * n - 3, n - 3))
    b.append("    adds    x8, x16, x8              // 列 %d = dst + s" % (2 * n - 3))
    b.append("    adc     x15, x15, xzr")
    b.append("    str     x8, [x0, #%d]" % (8 * (2 * n - 3)))
    b.append("    and     x17, x4, x6, asr 63      // ov_%d ? a_%d : 0" % (i, n - 1))
    b.append("    mul     x9, x4, x4               // D_%d" % (n - 1))
    b.append("    umulh   x12, x4, x4")
    b.append("    adds    x16, x15, x17            // 列 %d = carry + 修正, C1" % (2 * n - 2))
    b.append("    adc     x12, x12, xzr            // hi' = hi + C1")
    b.append("    adds    x16, x16, x9             // += lo, C2")
    b.append("    str     x16, [x0, #%d]" % (8 * (2 * n - 2)))
    b.append("    adc     x14, x12, xzr            // 列 %d = hi' + C2" % (2 * n - 1))
    b.append("    str     x14, [x0, #%d]" % (8 * (2 * n - 1)))

    b.append("    ret")
    return "\n".join(b)


def gen_arm64_sqr():
    parts = [COPYRIGHT, """
//
// void lmmp_sqr_hard_N_(mp_ptr dst, mp_srcptr numa);
//
//   平衡硬编码平方: [dst,2N] = [numa,N]^2。
//   N <= 3 : 库内 sqr_basecase.S 小规模分支的特化
//   N >= 4 : 交叉乘行累加(双链) -> 倍增(连续 adcs 长链) -> 对角折叠(单加数链)。
//

#include "lmmp_asm.h"

.text
"""]
    for n in range(1, MAXN + 1):
        parts.append(".globl ASM_GSYM(lmmp_sqr_hard_%d_)\n.p2align 4\n" % n)
        parts.append("ASM_GSYM(lmmp_sqr_hard_%d_):\n" % n)
        if n == 1:
            parts.append(arm64_sqr_n1())
        elif n == 2:
            parts.append(arm64_sqr_n2())
        elif n == 3:
            parts.append(arm64_sqr_n3())
        else:
            parts.append(arm64_sqr_rows_2a(n))
        parts.append("")
    return "\n".join(parts)


# =============================================================================
# 主流程
# =============================================================================

def arm64_strip_inline_comments(text):
    """剥离 arm64 输出中指令行的尾部 // 注释 (独立成行的段落注释保留)。"""
    out = []
    for line in text.split("\n"):
        if "//" in line and not line.lstrip().startswith("//"):
            line = line[:line.index("//")].rstrip()
        out.append(line)
    return "\n".join(out)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(here, "x64", "mul_hard.S"), "w", newline="\n") as f:
        f.write(gen_x64_mul())
    with open(os.path.join(here, "x64", "sqr_hard.S"), "w", newline="\n") as f:
        f.write(gen_x64_sqr())
    with open(os.path.join(here, "arm64", "mul_hard.S"), "w", newline="\n") as f:
        f.write(arm64_strip_inline_comments(gen_arm64_mul()))
    with open(os.path.join(here, "arm64", "sqr_hard.S"), "w", newline="\n") as f:
        f.write(arm64_strip_inline_comments(gen_arm64_sqr()))
    print("generated: x64/mul_hard.S x64/sqr_hard.S arm64/mul_hard.S arm64/sqr_hard.S")


if __name__ == "__main__":
    main()
