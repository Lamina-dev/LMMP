#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_hard.py -- LMMP 硬编码乘法/平方汇编生成器

仿照 FLINT 的 x64/arm64 硬编码汇编思路, 生成平衡规模的
    lmmp_mul_hard_N_  (N = 1..19)   [dst,2N] = [numa,N] * [numb,N]
    lmmp_sqr_hard_N_  (N = 1..19)   [dst,2N] = [numa,N]^2
的 .S 源文件(纯指令展开, 不依赖汇编器宏, 保证 GAS/LLVM 兼容)。

算法结构:
  x64  mul, N<=8 : FLINT 式寄存器整行累加(mulx+adcx/adox 双进位链, 行间寄存器环轮转)
  x64  mul, N>=9 : 种子 mul_1 + 展开 addmul_2(双乘数交错列) + 尾部直存
  x64  sqr, N<=3 : 库内 sqr_basecase.S 小规模分支特化; N>=4: 交叉积列累加
                   (8 列寄存器窗口+adcx/adox 双链, 计划式发射) -> 倍增 -> 对角
  arm64 mul      : 寄存器整行累加(mul/umulh + adds/adcs 单进位链, 列 c 固定映射 ring[c%n])
  arm64 sqr      : 交叉行累加 -> 倍增 -> 对角 (单进位链)

x64 sqr 的指令计划 (x64_sqr_plan) 与外部模拟脚本共用 (hard_work/sim_sqr_plan.py),
发射与验证针对同一 op 序列, 修改调度前先过模拟。

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
    """生成函数序言. used: 需保存的被调用者寄存器集合."""
    out = []
    if remap_args:
        out.append("#ifdef LMMP_WINDOWS")
        out.append("    push    rdi")
        out.append("    push    rsi")
        out.append("    mov     rdi, rcx                // dst")
        out.append("    mov     rsi, rdx                // numa")
        if remap_args == 3:
            out.append("    mov     rcx, r8                 // numb")
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

def arm64_mul_n1():
    return """\
    ldr     x3, [x1]
    ldr     x4, [x2]
    mul     x5, x3, x4
    umulh   x6, x3, x4
    stp     x5, x6, [x0]
    ret"""


def arm64_mul(n):
    """内存喂入方案(与 x64 tier-2 同构): 种子 mul_1 + (n-1) 个展开 addmul_1.
    每列进位即时吸收: adds t,pl,c (γ1) -> adc c,h,xzr (c=hi+γ1)
                     -> adds v,v,t (γ2) -> str -> adc c,c,xzr (c+=γ2).
    全 caller-saved 寄存器, 无压栈."""
    ta, pl, h, c, t, v = "x3", "x4", "x5", "x6", "x7", "x8"
    b = []
    b.append("    // x0=dst x1=numa x2=numb; 种子 mul_1 + 展开 addmul_1")

    # ---- 种子: dst[0..n] = numa * b_0 ----
    b.append("    ldr     %s, [x2]                 // b_0" % ta)
    b.append("    ldr     %s, [x1]" % t)
    b.append("    mul     %s, %s, %s" % (pl, t, ta))
    b.append("    umulh   %s, %s, %s" % (h, t, ta))
    b.append("    str     %s, [x0]" % pl)
    b.append("    mov     %s, %s" % (c, h))
    for i in range(1, n):
        b.append("    ldr     %s, [x1, #%d]" % (t, 8 * i))
        b.append("    mul     %s, %s, %s" % (pl, t, ta))
        b.append("    umulh   %s, %s, %s" % (h, t, ta))
        b.append("    adds    %s, %s, %s" % (v, pl, c))
        b.append("    str     %s, [x0, #%d]" % (v, 8 * i))
        b.append("    adc     %s, %s, xzr" % (c, h))
    b.append("    str     %s, [x0, #%d]           // 顶列直存" % (c, 8 * n))

    # ---- 行 j = 1..n-1: dst[j..j+n-1] += numa * b_j ----
    for j in range(1, n):
        b.append("    ldr     %s, [x2, #%d]            // b_%d" % (ta, 8 * j, j))
        # 首列: v = dst[j] + lo (c 初值 0)
        b.append("    ldr     %s, [x1]" % t)
        b.append("    mul     %s, %s, %s" % (pl, t, ta))
        b.append("    umulh   %s, %s, %s" % (h, t, ta))
        b.append("    ldr     %s, [x0, #%d]" % (v, 8 * j))
        b.append("    adds    %s, %s, %s" % (v, v, pl))
        b.append("    str     %s, [x0, #%d]" % (v, 8 * j))
        b.append("    adc     %s, %s, xzr" % (c, h))
        for i in range(1, n):
            b.append("    ldr     %s, [x1, #%d]" % (t, 8 * i))
            b.append("    mul     %s, %s, %s" % (pl, t, ta))
            b.append("    umulh   %s, %s, %s" % (h, t, ta))
            b.append("    ldr     %s, [x0, #%d]" % (v, 8 * (j + i)))
            b.append("    adds    %s, %s, %s" % (t, pl, c))
            b.append("    adc     %s, %s, xzr" % (c, h))
            b.append("    adds    %s, %s, %s" % (v, v, t))
            b.append("    str     %s, [x0, #%d]" % (v, 8 * (j + i)))
            b.append("    adc     %s, %s, xzr" % (c, c))
        b.append("    str     %s, [x0, #%d]           // 顶列直存" % (c, 8 * (j + n)))

    b.append("    ret")
    return "\n".join(b)


def gen_arm64_mul():
    parts = [COPYRIGHT, """
//
// void lmmp_mul_hard_N_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb);
//
//   平衡硬编码乘法: [dst,2N] = [numa,N] * [numb,N]。
//   寄存器整行累加: 列 c 固定映射 ring[c%N], 每行存一列并回收为顶列。
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

def arm64_sqr_rows(n):
    """交叉乘行累加 -> 倍增 -> 对角 (行/对角与 mul 同构的即时进位吸收模式).
    行内 hi 前驱天然包含在 pending c 中: 每列 = dst + lo + c 三项."""
    ta, pl, h, c, t, v = "x3", "x4", "x5", "x6", "x7", "x8"
    b = []
    b.append("    // x0=dst x1=numa; 交叉乘行累加 -> 倍增 -> 对角")
    b.append("    str     xzr, [x0]                // dst[0] = 0 (交叉列0恒0)")
    b.append("    str     xzr, [x0, #%d]           // dst[2n-1] = 0 (交叉不触及)" % (8 * (2 * n - 1)))

    # ---- 行 0: dst[k] = a_k*a_0, k = 1..n-1, 顶列 n (纯写) ----
    b.append("    ldr     %s, [x1]                 // a_0" % ta)
    for idx, k in enumerate(range(1, n)):
        b.append("    ldr     %s, [x1, #%d]" % (t, 8 * k))
        b.append("    mul     %s, %s, %s" % (pl, t, ta))
        b.append("    umulh   %s, %s, %s" % (h, t, ta))
        if idx == 0:
            b.append("    mov     %s, %s" % (v, pl))
            b.append("    str     %s, [x0, #%d]" % (v, 8 * k))
            b.append("    mov     %s, %s" % (c, h))
        else:
            b.append("    adds    %s, %s, %s" % (v, pl, c))
            b.append("    str     %s, [x0, #%d]" % (v, 8 * k))
            b.append("    adc     %s, %s, xzr" % (c, h))
    if n >= 2:
        b.append("    str     %s, [x0, #%d]           // 顶列直存" % (c, 8 * n))

    # ---- 行 i = 1..n-2: dst[i+k] += a_k*a_i ----
    for i in range(1, n - 1):
        b.append("    ldr     %s, [x1, #%d]            // a_%d" % (ta, 8 * i, i))
        for idx, k in enumerate(range(i + 1, n)):
            col = i + k
            b.append("    ldr     %s, [x1, #%d]" % (t, 8 * k))
            b.append("    mul     %s, %s, %s" % (pl, t, ta))
            b.append("    umulh   %s, %s, %s" % (h, t, ta))
            b.append("    ldr     %s, [x0, #%d]" % (v, 8 * col))
            if idx == 0:
                b.append("    adds    %s, %s, %s" % (v, v, pl))
                b.append("    str     %s, [x0, #%d]" % (v, 8 * col))
                b.append("    adc     %s, %s, xzr" % (c, h))
            else:
                b.append("    adds    %s, %s, %s" % (t, pl, c))
                b.append("    adc     %s, %s, xzr" % (c, h))
                b.append("    adds    %s, %s, %s" % (v, v, t))
                b.append("    str     %s, [x0, #%d]" % (v, 8 * col))
                b.append("    adc     %s, %s, xzr" % (c, c))
        b.append("    str     %s, [x0, #%d]           // 顶列直存" % (c, 8 * (i + n)))

    # ---- 倍增: dst = 2*dst ----
    b.append("    // dst *= 2")
    for col in range(1, 2 * n):
        b.append("    ldr     %s, [x0, #%d]" % (v, 8 * col))
        op = "adds" if col == 1 else "adcs"
        b.append("    %s     %s, %s, %s" % (op, v, v, v))
        b.append("    str     %s, [x0, #%d]" % (v, 8 * col))

    # ---- 对角: 折叠 a_i^2 ----
    b.append("    // 对角平方折叠")
    b.append("    ldr     %s, [x1]" % t)
    b.append("    mul     %s, %s, %s" % (pl, t, t))
    b.append("    umulh   %s, %s, %s" % (h, t, t))
    b.append("    ldr     %s, [x0]" % v)
    b.append("    adds    %s, %s, %s" % (v, v, pl))
    b.append("    str     %s, [x0]" % v)
    b.append("    adc     %s, %s, xzr" % (c, h))
    if n >= 2:
        b.append("    ldr     %s, [x0, #8]" % v)
        b.append("    adds    %s, %s, %s" % (v, v, c))
        b.append("    str     %s, [x0, #8]" % v)
        b.append("    adc     %s, xzr, xzr          // c = CF" % c)
    for i in range(1, n):
        b.append("    ldr     %s, [x1, #%d]" % (t, 8 * i))
        b.append("    mul     %s, %s, %s" % (pl, t, t))
        b.append("    umulh   %s, %s, %s" % (h, t, t))
        b.append("    ldr     %s, [x0, #%d]" % (v, 8 * (2 * i)))
        b.append("    adds    %s, %s, %s" % (t, c, pl))
        b.append("    adc     %s, %s, xzr" % (c, h))
        b.append("    adds    %s, %s, %s" % (v, v, t))
        b.append("    str     %s, [x0, #%d]" % (v, 8 * (2 * i)))
        b.append("    adc     %s, %s, xzr" % (c, c))
        if i < n - 1:
            b.append("    ldr     %s, [x0, #%d]" % (v, 8 * (2 * i + 1)))
            b.append("    adds    %s, %s, %s" % (v, v, c))
            b.append("    str     %s, [x0, #%d]" % (v, 8 * (2 * i + 1)))
            b.append("    adc     %s, xzr, xzr          // c = CF" % c)
    b.append("    ldr     %s, [x0, #%d]" % (v, 8 * (2 * n - 1)))
    b.append("    adds    %s, %s, %s" % (v, v, c))
    b.append("    str     %s, [x0, #%d]" % (v, 8 * (2 * n - 1)))
    b.append("    ret")
    return "\n".join(b)


def gen_arm64_sqr():
    parts = [COPYRIGHT, """
//
// void lmmp_sqr_hard_N_(mp_ptr dst, mp_srcptr numa);
//
//   平衡硬编码平方: [dst,2N] = [numa,N]^2。
//   交叉乘逐行累加 -> 整体倍增 -> 对角平方折叠 (单进位链)。
//

#include "lmmp_asm.h"

.text
"""]
    for n in range(1, MAXN + 1):
        parts.append(".globl ASM_GSYM(lmmp_sqr_hard_%d_)\n.p2align 4\n" % n)
        parts.append("ASM_GSYM(lmmp_sqr_hard_%d_):\n" % n)
        parts.append(arm64_sqr_rows(n))
        parts.append("")
    return "\n".join(parts)


# =============================================================================
# 主流程
# =============================================================================

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(here, "x64", "mul_hard.S"), "w", newline="\n") as f:
        f.write(gen_x64_mul())
    with open(os.path.join(here, "x64", "sqr_hard.S"), "w", newline="\n") as f:
        f.write(gen_x64_sqr())
    with open(os.path.join(here, "arm64", "mul_hard.S"), "w", newline="\n") as f:
        f.write(gen_arm64_mul())
    with open(os.path.join(here, "arm64", "sqr_hard.S"), "w", newline="\n") as f:
        f.write(gen_arm64_sqr())
    print("generated: x64/mul_hard.S x64/sqr_hard.S arm64/mul_hard.S arm64/sqr_hard.S")


if __name__ == "__main__":
    main()
