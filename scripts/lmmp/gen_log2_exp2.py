#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#  Copyright (C) 2026 HJimmyK (Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as
#  published by the Free Software Foundation, either version 3 of
#  the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, see
#  <https://www.gnu.org/licenses/>.
#
#  SPDX-License-Identifier: LGPL-3.0-or-later
"""
gen_log2_exp2.py -- LMMP log2/exp2 定点常量表生成器

生成 src/lmmp/global/log2_exp2.c 的全部六张表（纯整数运算，无外部依赖）：

  lg2_L32[32][3]  floor(log2(1+i/32)*2^192)，log2 查表约化主表（5bit 索引）
  lg2_R32[32][2]  floor(2^128/(32+i))，约简变量 u 的倒数乘子表
  lg2_c27[27][2]  floor(2^191/(m*ln2)) 的高 2 limb，m=1..27（log2 级数，
                  半尺度；低 limb 恒为死数据不生成）
  xp2_T16[16][2]  round((2^(k/16)-1)*2^128)，k=0..15（四舍五入存）
  xp2_T8[8][3]    round((2^(m/128)-1)*2^192)，m=0..7（四舍五入存，
                  运行时再舍入取高 64bit）
  xp2_c[13][2]    floor((ln2)^m/m!*2^192) 的高 2 limb，m=1..13（exp2 级数，
                  满尺度；低 limb 死数据不生成）

数学机制（与 .c 文件头注释一致）：
  ln2      = 2*atanh(1/3)
  log2(1+x)= 2*atanh(y)/ln2,  y = x/(2+x)
  2^x - 1 = e^(x*ln2) - 1    = sum_{m>=1} (x*ln2)^m/m!
全部以 768bit 定点求和、64bit 装配 guard 位后截断/舍入，保证低位确定。

用法:
  python scripts/lmmp/gen_log2_exp2.py            # 校验 .c 内表与再生成一致
  python scripts/lmmp/gen_log2_exp2.py --emit     # 打印全部表（C 源码形态）
"""

import re
import sys
from pathlib import Path

PREC = 768          # ln2 / 级数求和定点位数
GUARD = 64          # 输出装配 guard 位
SRC = Path(__file__).resolve().parents[2] / "src/lmmp/global/log2_exp2.c"


def _atanh_shiftadd(num, den, prec):
    """floor(2*atanh(num/den)*2^prec)，num/den < 1（移位加减无需除法）。"""
    total = 0
    m = 0
    while True:
        k = 2 * m + 1
        term = num ** k * (1 << prec) // (k * den ** k)
        if term == 0 and m > 4:
            break
        total += term
        m += 1
    return 2 * total


def _expm1_shiftadd(z_num, z_den, out_bits):
    """floor((e^(z_num/z_den)-1)*2^out_bits)，0 <= z < 1。"""
    total = 0
    m = 1
    fact = 1
    while True:
        fact *= m
        term = z_num ** m * (1 << (out_bits + GUARD)) // (fact * z_den ** m)
        if term == 0 and m > 4:
            break
        total += term
        m += 1
    return total >> GUARD


LN2_FP = _atanh_shiftadd(1, 3, PREC)   # ln2*2^prec


def log2_1px(p, k, out_bits):
    """floor(log2(1+p/2^k)*2^out_bits)。"""
    y_num, y_den = p, 2 * (1 << k) + p
    ln_fp = _atanh_shiftadd(y_num, y_den, PREC)
    return ln_fp * (1 << out_bits) // LN2_FP


def exp2m1(a, b, out_bits):
    """floor((2^(a/b)-1)*2^out_bits)。"""
    return _expm1_shiftadd(a * LN2_FP, b * (1 << PREC), out_bits)


def lg2_c_m(m):
    """floor(2^191/(m*ln2))。"""
    return (1 << (191 + PREC)) // (m * LN2_FP)


def xp2_c_m(m):
    """floor((ln2)^m/m!*2^192)。"""
    fact = 1
    for j in range(2, m + 1):
        fact *= j
    return (LN2_FP ** m << 192) // (fact * (1 << (m * PREC)))


# ---- 表值计算 ----

L32 = [log2_1px(i, 5, 192) for i in range(32)]
R32 = [(1 << 128) // (32 + i) for i in range(32)]
C27 = [lg2_c_m(m) for m in range(1, 28)]
T16 = [(exp2m1(k, 16, 128 + GUARD) + (1 << (GUARD - 1))) >> GUARD for k in range(16)]
T8 = [(exp2m1(m, 128, 192 + GUARD) + (1 << (GUARD - 1))) >> GUARD for m in range(8)]
XC13 = [xp2_c_m(m) for m in range(1, 14)]


def _fmt3(v):
    return "{0x%016xull, 0x%016xull, 0x%016xull}" % (
        v & 0xFFFFFFFFFFFFFFFF, (v >> 64) & 0xFFFFFFFFFFFFFFFF, (v >> 128) & 0xFFFFFFFFFFFFFFFF)


def _fmt2(v):
    return "{0x%016xull, 0x%016xull}" % (v & 0xFFFFFFFFFFFFFFFF, (v >> 64) & 0xFFFFFFFFFFFFFFFF)


def emit():
    out = []
    out.append("static const uint64_t lg2_L32[32][3] = {")
    out.append(",\n".join("    " + _fmt3(v) for v in L32))
    out.append("};\n")
    out.append("static const uint64_t lg2_R32[32][2] = {")
    out.append(",\n".join("    " + _fmt2(v) for v in R32))
    out.append("};\n")
    out.append("static const uint64_t lg2_c27[27][2] = {")
    out.append(",\n".join("    " + _fmt2(v) for v in C27))
    out.append("};\n")
    out.append("static const uint64_t xp2_T16[16][2] = {")
    out.append(",\n".join("    " + _fmt2(v) for v in T16))
    out.append("};\n")
    out.append("static const uint64_t xp2_T8[8][3] = {")
    out.append(",\n".join("    " + _fmt3(v) for v in T8))
    out.append("};\n")
    out.append("static const uint64_t xp2_c[13][2] = {")
    out.append(",\n".join("    " + _fmt2(v) for v in XC13))
    out.append("};")
    return "\n".join(out)


# ---- 对源文件表的校验 ----

def _parse_tables(src_text):
    """按表名解析 .c 内的 uint64_t 常量表 -> {name: [row 值 list]}。"""
    tables = {}
    for m in re.finditer(
            r"static const uint64_t (\w+)\[(\d+)\]\[(\d+)\]\s*=\s*\{(.*?)\n\};",
            src_text, re.S):
        name, rows, cols, body = m.group(1), int(m.group(2)), int(m.group(3)), m.group(4)
        vals = []
        for row in re.findall(r"\{([^}]*)\}", body):
            nums = [int(t.strip().rstrip("ull").rstrip("ULL"), 16) for t in row.split(",") if t.strip()]
            assert len(nums) == cols, (name, nums)
            v = 0
            for limb in reversed(nums):
                v = (v << 64) | limb
            vals.append(v)
        assert len(vals) == rows, name
        tables[name] = vals
    return tables


def check():
    text = SRC.read_text(encoding="utf-8")
    got = _parse_tables(text)
    want = {
        "lg2_L32": (L32, False), "lg2_R32": (R32, False), "lg2_c27": (C27, True),
        "xp2_T16": (T16, False), "xp2_T8": (T8, False), "xp2_c": (XC13, True),
    }
    ok = True
    for name, (vals, trunc_hi2) in want.items():
        if name not in got:
            print("MISS  %s（源文件中不存在）" % name)
            ok = False
            continue
        for i, (g, w) in enumerate(zip(got[name], vals)):
            if trunc_hi2:
                w >>= 64  # 系数表只存全值的第 2/3 limb
            if g != w:
                print("DIFF  %s[%d]: src=%x gen=%x" % (name, i, g, w))
                ok = False
    print("gen_log2_exp2: " + ("all tables match" if ok else "MISMATCH"))
    return ok


if __name__ == "__main__":
    if "--emit" in sys.argv:
        print(emit())
        sys.exit(0)
    sys.exit(0 if check() else 1)
