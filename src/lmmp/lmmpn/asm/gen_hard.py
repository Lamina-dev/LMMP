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
  x64  sqr, N<=3 : 库内 sqr_basecase.S 小规模分支特化; N>=4: 交叉行累加->倍增->对角
  arm64 mul      : 寄存器整行累加(mul/umulh + adds/adcs 单进位链, 列 c 固定映射 ring[c%n])
  arm64 sqr      : 交叉行累加 -> 倍增 -> 对角 (单进位链)

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


def x64_mul_regpass(b, ring, scr, zero, pcreg, rreg, nrows, s, off, inner_b, rmw):
    """发射一个寄存器方案 pass (整行累加).

    inner_b: True -> inner=numb(rcx), rows=numa(rsi); False -> inner=numa, rows=numb.
    ring/scr 为可变列表(行间轮转), 返回 (ring, scr).
    rmw (pass>=1): 终化列读加写; pc(pcreg)<=3 跨行累积列完成进位,
    rreg 捕获 cf1, pass 尾部以线性 adc 链消化 pc. (已通过 2 万次模拟验证)
    """
    inner, ioff = ("rcx", 8 * off) if inner_b else ("rsi", 0)
    rows = "rsi" if inner_b else "rcx"
    n = s
    pcb = r8name(pcreg)
    rb = r8name(rreg)

    # ---- 行 0: m 序列 ----
    b.append("    mov     rdx, [%s+%d]            // 行乘数 0" % (rows, 0))
    b.append("    mulx    %s, %s, [%s+%d]       // hi0->%s lo0->%s"
             % (ring[0], scr, inner, ioff, ring[0], scr))
    if rmw:
        b.append("    mov     rdx, [rdi+%d]" % (8 * off))
        b.append("    add     rdx, %s               // dst + lo0" % scr)
        b.append("    mov     [rdi+%d], rdx" % (8 * off))
        b.append("    setc    %s" % pcb)
        b.append("    movzx   %s, %s              // pc = CF" % (pcreg, pcb))
        b.append("    xor     %s, %s              // R = 0" % (e32(rreg), e32(rreg)))
        b.append("    mov     rdx, [%s+%d]            // 重载行乘数" % (rows, 0))
    else:
        b.append("    mov     [rdi+%d], %s           // dst[%d] = lo0" % (8 * off, scr, off))
    for k in range(1, n):
        b.append("    mulx    %s, %s, [%s+%d]     // hi%d->%s lo%d->%s"
                 % (ring[k], scr, inner, ioff + 8 * k, k, ring[k], k, scr))
        b.append("    adcx    %s, %s                // col%d = hi%d + lo%d"
                 % (ring[k - 1], scr, off + k, k - 1, k))
    b.append("    adcx    %s, %s                  // col%d = hi%d + CF" % (ring[n - 1], zero, off + n, n - 1))

    # ---- 行 j = 1..nrows-1: am 序列 ----
    for j in range(1, nrows):
        col = off + j
        b.append("    mov     rdx, [%s+%d]            // 行乘数 %d" % (rows, 8 * j, j))
        b.append("    mulx    %s, %s, [%s+%d]       // hi0->%s lo0->%s"
                 % (ring[n], scr, inner, ioff, ring[n], scr))
        b.append("    adcx    %s, %s                // col%d 完成, CF=cf1" % (ring[0], scr, col))
        if rmw:
            b.append("    setc    %s                  // R = cf1" % rb)
            b.append("    mov     rdx, [rdi+%d]" % (8 * col))
            b.append("    add     rdx, %s              // += pc" % pcreg)
            b.append("    setc    %s" % pcb)
            b.append("    movzx   %s, %s              // pc = gamma_a" % (pcreg, pcb))
            b.append("    add     rdx, %s              // += ring0" % ring[0])
            b.append("    mov     [rdi+%d], rdx" % (8 * col))
            b.append("    adc     %s, 0                // pc += gamma_b" % pcreg)
            b.append("    add     %s, %s               // pc += cf1" % (pcreg, rreg))
            b.append("    xor     %s, %s              // R = 0" % (e32(rreg), e32(rreg)))
            b.append("    mov     rdx, [%s+%d]            // 重载行乘数" % (rows, 8 * j))
        else:
            b.append("    mov     [rdi+%d], %s           // dst[%d]" % (8 * col, ring[0], col))
        hi_dest = ring[n]
        for k in range(1, n):
            hi_dest = scr if k % 2 == 1 else ring[n]
            hi_prev = ring[n] if k % 2 == 1 else scr
            b.append("    mulx    %s, %s, [%s+%d]     // hi%d->%s lo%d->ring0"
                     % (hi_dest, ring[0], inner, ioff + 8 * k, k, hi_dest, k))
            b.append("    adcx    %s, %s               // CF 链: += hi%d" % (ring[k], hi_prev, k - 1))
            b.append("    adox    %s, %s               // OF 链: += lo%d" % (ring[k], ring[0], k))
        b.append("    adcx    %s, %s                 // 顶列 = hi%d + 进位" % (hi_dest, zero, n - 1))
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
        b.append("    add     rdx, %s                // += pc" % pcreg)
        b.append("    mov     [rdi+%d], rdx" % (8 * (off + nrows)))
        for k in range(1, n):
            b.append("    mov     rdx, %s" % ring[k])
            b.append("    adc     rdx, 0               // 链式消化进位")
            b.append("    mov     [rdi+%d], rdx" % (8 * (off + nrows + k)))
    else:
        for k in range(n):
            b.append("    mov     [rdi+%d], %s" % (8 * (off + nrows + k), ring[k]))
    return ring, scr


def x64_mul_tier1(n):
    """2 <= n <= 8: 单趟 FLINT 寄存器整行累加."""
    ring = X64_RING_POOL[: n + 1]
    rest = X64_RING_POOL[n + 1:]
    if len(rest) >= 2:
        scr, zero = rest[0], rest[1]
    elif len(rest) == 1:
        scr, zero = rest[0], "rbx"
    else:
        scr, zero = "rbx", "rbp"
    used = set(X64_RING_POOL) | {scr, zero}

    b = []
    b.append("    # 内部: rdi=dst rsi=numa rcx=numb rdx=行乘数")
    b.append("    # ring=[%s] scr=%s zero=%s" % (",".join(ring), scr, zero))
    b.append(x64_prologue(used, 3))
    b.append("    xor     %s, %s                  // 清 CF/OF" % (e32(zero), e32(zero)))
    ring, scr = x64_mul_regpass(b, ring, scr, zero, zero, zero, n, n, 0, False, False)
    b.append(x64_epilogue(used, 3))
    return "\n".join(b)


def x64_mul_tier2(n):
    """n >= 9: numb 分块, 每块一趟寄存器方案 (乘法总量恰为 n^2).
    pass 0 纯写; pass >= 1 终化列 RMW, 列完成进位累积进 pc 寄存器 (<=3),
    pass 尾部以线性 adc 链消化."""
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
        ring, scr = x64_mul_regpass(b, ring, scr, zero, pcreg, rreg, n, s, off, True, p > 0)
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


def x64_sqr_rows(n):
    """n >= 4: 交叉乘(行0种子 + 配对行) -> 倍增 -> 对角.

    寄存器: rdi=dst rsi=numa r8=u r9=v rax/rdx=mul r13=D r14=cu r15=cv r10=Y
    配对行 (a_i, a_{i+1}): 与 mul tier-2 同构的即时进位吸收模式。
    """
    b = []
    b.append("    # 交叉乘(行0种子+配对行) -> 倍增 -> 对角平方折叠")
    b.append(x64_prologue({"rbx", "r13", "r14", "r15"}, 2))
    b.append("    mov     QWORD PTR [rdi], 0        // dst[0] = 0 (交叉列0恒为0)")
    b.append("    mov     QWORD PTR [rdi+%d], 0    // dst[2n-1] = 0 (交叉不触及)" % (8 * (2 * n - 1)))

    # ---- 行 0 种子: dst[k] = a_k*a_0 (k=1..n-1), 顶列 n (纯写) ----
    b.append("    mov     r8, [rsi]                // a_0")
    b.append("    xor     r14d, r14d              // c = 0")
    for k in range(1, n):
        b.append("    mov     rax, [rsi+%d]" % (8 * k))
        b.append("    mul     r8")
        b.append("    add     rax, r14               // lo + c")
        b.append("    mov     [rdi+%d], rax" % (8 * k))
        b.append("    mov     r14, rdx               // c = hi")
        b.append("    adc     r14, 0")
    b.append("    mov     [rdi+%d], r14           // 顶列直存" % (8 * n))

    # ---- 配对行 (u,v) = (a_i, a_{i+1}), i = 1,3,... ----
    for i in range(1, n - 2, 2):
        b.append("    # --- 行对: a_%d 与 a_%d ---" % (i, i + 1))
        b.append("    mov     r8, [rsi+%d]            // u = a_%d" % (8 * i, i))
        b.append("    mov     r9, [rsi+%d]            // v = a_%d" % (8 * (i + 1), i + 1))
        b.append("    xor     r15d, r15d             // cv = 0")
        # 首列 2i+1: 仅 u 项 (a_{i+1}*a_i)
        c0 = 2 * i + 1
        b.append("    mov     r13, [rdi+%d]          // D 预载" % (8 * c0))
        b.append("    mov     rax, [rsi+%d]          // a%d" % (8 * (i + 1), i + 1))
        b.append("    mul     r8")
        b.append("    add     r13, rax               // D + lo_u")
        b.append("    mov     r14, rdx               // cu = hu")
        b.append("    adc     r14, 0")
        b.append("    mov     [rdi+%d], r13" % (8 * c0))
        # c = 2i+2: 仅 u 项 (v 行此列为对角, 不存在)
        if 2 * i + 2 <= i + n - 1:
            c1 = 2 * i + 2
            b.append("    mov     r13, [rdi+%d]          // D 预载" % (8 * c1))
            b.append("    mov     rax, [rsi+%d]          // a%d*u" % (8 * (c1 - i), c1 - i))
            b.append("    mul     r8")
            b.append("    add     rax, r14               // lo_u + cu")
            b.append("    mov     r14, rdx               // cu = hu")
            b.append("    adc     r14, 0                 // += alpha")
            b.append("    add     r13, rax               // D + lo_u'")
            b.append("    adc     r14, 0                 // cu += gamma")
            b.append("    mov     [rdi+%d], r13" % (8 * c1))
        # cols c = 2i+3 .. i+n-1: u 项 a_{c-i}, v 项 a_{c-i-1}
        for c in range(2 * i + 3, i + n):
            b.append("    mov     r13, [rdi+%d]          // D 预载" % (8 * c))
            b.append("    mov     rax, [rsi+%d]          // a%d*u" % (8 * (c - i), c - i))
            b.append("    mul     r8")
            b.append("    add     rax, r14               // lo_u + cu")
            b.append("    mov     r14, rdx               // cu = hu")
            b.append("    adc     r14, 0                 // += alpha")
            b.append("    add     r13, rax               // D + lo_u'")
            b.append("    adc     r14, 0                 // cu += gamma1")
            b.append("    mov     rax, [rsi+%d]          // a%d*v" % (8 * (c - i - 1), c - i - 1))
            b.append("    mul     r9")
            b.append("    add     rax, r15               // lo_v + cv")
            b.append("    mov     r15, rdx               // cv = hv")
            b.append("    adc     r15, 0                 // += beta")
            b.append("    add     r13, rax               // + lo_v'")
            b.append("    mov     [rdi+%d], r13" % (8 * c))
            b.append("    adc     r15, 0                 // cv += gamma2")
        # 尾部: col i+n: X = lo(a_{n-1}v)+cu+cv; col i+n+1: Y = hv+bits
        b.append("    mov     rax, [rsi+%d]          // a%d*v 顶部积" % (8 * (n - 1), n - 1))
        b.append("    mul     r9")
        b.append("    add     rax, r14               // + cu")
        b.append("    mov     r10, rdx               // Y = hv")
        b.append("    adc     r10, 0                 // Y += alpha")
        b.append("    add     rax, r15               // + cv")
        b.append("    mov     [rdi+%d], rax" % (8 * (i + n)))
        b.append("    adc     r10, 0                 // Y += beta")
        b.append("    mov     [rdi+%d], r10" % (8 * (i + n + 1)))

    # ---- 剩余单行 (n 奇数): 行 n-2 ----
    if (n - 2) % 2 == 1:
        i = n - 2
        b.append("    # --- 单行: a_%d ---" % i)
        b.append("    mov     r8, [rsi+%d]            // u = a_%d" % (8 * i, i))
        b.append("    xor     r14d, r14d             // c = 0")
        for k in range(i + 1, n):
            c = i + k
            b.append("    mov     r13, [rdi+%d]" % (8 * c))
            b.append("    mov     rax, [rsi+%d]" % (8 * k))
            b.append("    mul     r8")
            b.append("    add     rax, r14               // lo + c")
            b.append("    mov     r14, rdx               // c = hi")
            b.append("    adc     r14, 0")
            b.append("    add     r13, rax               // D + lo'")
            b.append("    adc     r14, 0")
            b.append("    mov     [rdi+%d], r13" % (8 * c))
        b.append("    mov     [rdi+%d], r14          // 顶列直存" % (8 * (i + n)))

    # ---- 倍增: dst = 2*dst ----
    b.append("    # dst *= 2")
    for col in range(1, 2 * n):
        b.append("    mov     rax, [rdi+%d]" % (8 * col))
        b.append("    add     rax, rax" if col == 1 else "    adc     rax, rax")
        b.append("    mov     [rdi+%d], rax" % (8 * col))

    # ---- 对角: 折叠 a_i^2 (mulx, 与旧版相同的已验证模式) ----
    b.append("    # 对角平方折叠")
    b.append("    xor     ebx, ebx                // zero, 清 CF/OF")
    b.append("    xor     r8d, r8d                // c = 0")
    for i in range(n):
        b.append("    mov     rdx, [rsi+%d]" % (8 * i))
        b.append("    mov     rax, rdx")
        b.append("    mulx    r11, r10, rax          // a%d^2" % i)
        b.append("    mov     r9, [rdi+%d]" % (8 * (2 * i)))
        b.append("    adox    r9, r8")
        b.append("    adcx    r9, r10")
        b.append("    mov     [rdi+%d], r9" % (8 * (2 * i)))
        b.append("    mov     r8, r11")
        b.append("    adcx    r8, rbx")
        b.append("    adox    r8, rbx")
        if i < n - 1:
            b.append("    mov     r9, [rdi+%d]" % (8 * (2 * i + 1)))
            b.append("    adcx    r9, r8")
            b.append("    mov     [rdi+%d], r9" % (8 * (2 * i + 1)))
            b.append("    mov     r8, rbx")
            b.append("    adcx    r8, rbx               // c = CF")
    b.append("    mov     r9, [rdi+%d]" % (8 * (2 * n - 1)))
    b.append("    adcx    r9, r8                  // 最高列 (最终进位必为0)")
    b.append("    mov     [rdi+%d], r9" % (8 * (2 * n - 1)))

    b.append(x64_epilogue({"rbx", "r13", "r14", "r15"}, 2))
    return "\n".join(b)


def gen_x64_sqr():
    parts = [COPYRIGHT, """
//
// void lmmp_sqr_hard_N_(mp_ptr dst, mp_srcptr numa);
//
//   平衡硬编码平方: [dst,2N] = [numa,N]^2。
//   N <= 3 : 库内 sqr_basecase.S 小规模分支的特化
//   N >= 4 : 交叉乘逐行累加 -> 整体倍增 -> 对角平方折叠
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
            parts.append(x64_sqr_rows(n))
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
    b.append("    str     %s, [x0]                // dst[0] = lo" % pl)
    b.append("    mov     %s, %s                  // c = hi" % (c, h))
    for i in range(1, n):
        b.append("    ldr     %s, [x1, #%d]" % (t, 8 * i))
        b.append("    mul     %s, %s, %s" % (pl, t, ta))
        b.append("    umulh   %s, %s, %s" % (h, t, ta))
        b.append("    adds    %s, %s, %s             // lo + c" % (v, pl, c))
        b.append("    str     %s, [x0, #%d]" % (v, 8 * i))
        b.append("    adc     %s, %s, xzr            // c = hi + CF" % (c, h))
    b.append("    str     %s, [x0, #%d]           // dst[n] = c" % (c, 8 * n))

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
            b.append("    adds    %s, %s, %s             // t = lo + c" % (t, pl, c))
            b.append("    adc     %s, %s, xzr            // c = hi + γ1" % (c, h))
            b.append("    adds    %s, %s, %s             // v = dst + t" % (v, v, t))
            b.append("    str     %s, [x0, #%d]" % (v, 8 * (j + i)))
            b.append("    adc     %s, %s, xzr            // c += γ2" % (c, c))
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
    b.append("    str     xzr, [x0]                // dst[0] = 0")
    b.append("    str     xzr, [x0, #%d]           // dst[2n-1] = 0" % (8 * (2 * n - 1)))

    # ---- 行 0: dst[k] = a_k*a_0, k = 1..n-1, 顶列 n (纯写) ----
    b.append("    ldr     %s, [x1]                 // a_0" % ta)
    for idx, k in enumerate(range(1, n)):
        b.append("    ldr     %s, [x1, #%d]" % (t, 8 * k))
        b.append("    mul     %s, %s, %s" % (pl, t, ta))
        b.append("    umulh   %s, %s, %s" % (h, t, ta))
        if idx == 0:
            b.append("    mov     %s, %s" % (v, pl))
            b.append("    str     %s, [x0, #%d]" % (v, 8 * k))
            b.append("    mov     %s, %s                  // c = hi" % (c, h))
        else:
            b.append("    adds    %s, %s, %s             // lo + c" % (v, pl, c))
            b.append("    str     %s, [x0, #%d]" % (v, 8 * k))
            b.append("    adc     %s, %s, xzr            // c = hi + CF" % (c, h))
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
                b.append("    adds    %s, %s, %s             // t = lo + c" % (t, pl, c))
                b.append("    adc     %s, %s, xzr            // c = hi + γ1" % (c, h))
                b.append("    adds    %s, %s, %s             // v = dst + t" % (v, v, t))
                b.append("    str     %s, [x0, #%d]" % (v, 8 * col))
                b.append("    adc     %s, %s, xzr            // c += γ2" % (c, c))
        b.append("    str     %s, [x0, #%d]           // 顶列直存" % (c, 8 * (i + n)))

    # ---- 倍增: dst = 2*dst ----
    b.append("    # dst *= 2")
    for col in range(1, 2 * n):
        b.append("    ldr     %s, [x0, #%d]" % (v, 8 * col))
        op = "adds" if col == 1 else "adcs"
        b.append("    %s     %s, %s, %s" % (op, v, v, v))
        b.append("    str     %s, [x0, #%d]" % (v, 8 * col))

    # ---- 对角: 折叠 a_i^2 ----
    b.append("    # 对角平方折叠")
    b.append("    ldr     %s, [x1]" % t)
    b.append("    mul     %s, %s, %s" % (pl, t, t))
    b.append("    umulh   %s, %s, %s" % (h, t, t))
    b.append("    ldr     %s, [x0]" % v)
    b.append("    adds    %s, %s, %s" % (v, v, pl))
    b.append("    str     %s, [x0]" % v)
    b.append("    adc     %s, %s, xzr" % (c, h))
    if n >= 2:
        # i=0 的奇数列 (col 1): dst[1] += c, 新 c = CF
        b.append("    ldr     %s, [x0, #8]" % v)
        b.append("    adds    %s, %s, %s" % (v, v, c))
        b.append("    str     %s, [x0, #8]" % v)
        b.append("    adc     %s, xzr, xzr          // c = CF" % c)
    for i in range(1, n):
        b.append("    ldr     %s, [x1, #%d]" % (t, 8 * i))
        b.append("    mul     %s, %s, %s" % (pl, t, t))
        b.append("    umulh   %s, %s, %s" % (h, t, t))
        b.append("    ldr     %s, [x0, #%d]" % (v, 8 * (2 * i)))
        b.append("    adds    %s, %s, %s             // t = c + lo" % (t, c, pl))
        b.append("    adc     %s, %s, xzr            // c = hi + γ1" % (c, h))
        b.append("    adds    %s, %s, %s             // v = dst + t" % (v, v, t))
        b.append("    str     %s, [x0, #%d]" % (v, 8 * (2 * i)))
        b.append("    adc     %s, %s, xzr            // c += γ2" % (c, c))
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
