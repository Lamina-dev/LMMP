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
# mp_limb_t lmmp_inv_1_(mp_limb_t x);
#
# mp_limb_t lmmp_inv_2_1_(mp_limb_t xh, mp_limb_t xl);


#include "lammp_asm.h"

.text

.globl ASM_GSYM(lmmp_inv_1_)
.globl ASM_GSYM(lmmp_inv_2_1_)

.p2align 4
ASM_GSYM(lmmp_inv_1_table_):
    # for(i=256#i<512#++i){round(0x80000/(i+1/2))}
    .word 2044,2036,2028,2020,2013,2005,1997,1990,1982,1975,1967,1960,1953,1945,1938,1931
    .word 1924,1917,1910,1903,1896,1889,1883,1876,1869,1862,1856,1849,1843,1836,1830,1824
    .word 1817,1811,1805,1799,1792,1786,1780,1774,1768,1762,1756,1751,1745,1739,1733,1727
    .word 1722,1716,1711,1705,1699,1694,1689,1683,1678,1672,1667,1662,1657,1651,1646,1641
    .word 1636,1631,1626,1621,1616,1611,1606,1601,1596,1591,1586,1582,1577,1572,1567,1563
    .word 1558,1553,1549,1544,1540,1535,1531,1526,1522,1517,1513,1509,1504,1500,1496,1492
    .word 1487,1483,1479,1475,1471,1467,1462,1458,1454,1450,1446,1442,1438,1434,1431,1427
    .word 1423,1419,1415,1411,1407,1404,1400,1396,1393,1389,1385,1382,1378,1374,1371,1367
    .word 1364,1360,1357,1353,1350,1346,1343,1339,1336,1332,1329,1326,1322,1319,1316,1312
    .word 1309,1306,1303,1299,1296,1293,1290,1287,1283,1280,1277,1274,1271,1268,1265,1262
    .word 1259,1256,1253,1250,1247,1244,1241,1238,1235,1232,1229,1226,1224,1221,1218,1215
    .word 1212,1209,1207,1204,1201,1198,1196,1193,1190,1188,1185,1182,1180,1177,1174,1172
    .word 1169,1166,1164,1161,1159,1156,1154,1151,1148,1146,1143,1141,1139,1136,1134,1131
    .word 1129,1126,1124,1121,1119,1117,1114,1112,1110,1107,1105,1103,1100,1098,1096,1093
    .word 1091,1089,1087,1084,1082,1080,1078,1075,1073,1071,1069,1067,1065,1062,1060,1058
    .word 1056,1054,1052,1050,1048,1045,1043,1041,1039,1037,1035,1033,1031,1029,1027,1025

# ----------------------------------------------
# lmmp_inv_1_
# 参数: x (64位) 在 rcx (Windows) 或 rdi (Linux)
# 返回: 结果在 rax
# ----------------------------------------------
.p2align 4
ASM_GSYM(lmmp_inv_1_):
#ifdef LMMP_WINDOWS
    mov r9, rcx
    mov rax, rcx
#else
    mov r9, rdi
    mov rax, rdi
#endif
    shr rax, 55
    lea rcx, [rip + lmmp_inv_1_table_]
    sub rcx, 512
    movzx rcx, WORD PTR [rcx + rax*2]
    # rcx = v0(10,9) = round(0x80000/(1/2+(x>>55)))

    mov r8, r9
    mov rax, rcx
    imul ecx, ecx
    shr r8, 24
    imul rcx, r8
    shr rcx, 41
    shl rax, 10
    sub rax, rcx
    # rax = v1(19,17) = (v0<<10) - (v0*v0*(x>>24)>>41)

    mov rcx, 0x800000000000000
    imul r8, rax
    sub rcx, r8
    imul rcx, rax
    shl rax, 15
    sar rcx, 44
    add rcx, rax
    # rcx = v2(34,33) = (v1<<15) + (v1*(2^59-v1*(x>>24))>>44)

    mov rax, r9
    mul rcx
    shrd rax, rdx, 2
    not rax
    imul rcx
    shl rcx, 30
    sar rdx, 2
    add rcx, rdx
    # rcx = v3(64,64) = (v2<<30) + (v2*(2^96-(v2*x>>2)-1)>>66)
    # we strictly proved that: -9/8 < v3+2^64 - 2^128/x < 0

    mov rax, r9
    mul rcx
    add rax, r9
    mov rax, rcx
    adc rdx, r9
    sub rax, rdx
    ret

# ----------------------------------------------
# lmmp_inv_2_1_
# 参数: xh (高64位) 在 rcx (Windows) 或 rdi (Linux)
#       xl (低64位) 在 rdx (Windows) 或 rsi (Linux)
# 返回: 结果在 rax
# ----------------------------------------------
.p2align 4
ASM_GSYM(lmmp_inv_2_1_):
#ifdef LMMP_WINDOWS
    # Windows : xh = rcx, xl = rdx
    mov r11, rdx          # 保存 xl
    mov r10, rcx          # 保存 xh
    call lmmp_inv_1_      # 参数在 rcx (xh)
    mov r8, rax
    mov r9, r10           # r9 = xh
    imul r9, rax          # r9 = xh * inv (低64位)
    xor rcx, rcx
    mul r11               # rdx:rax = xl * inv
    add r9, r11           # r9 = xh*inv + xl (低64位)
    adc rcx, -1           # rcx:r9 = 128位结果
    add r9, rdx
    adc rcx, 0
    js .lab_2f_win
.lab_1b_win:
    dec r8
    sub rax, r11
    sbb r9, r10
    sbb rcx, 0
    jns .lab_1b_win
.lab_2f_win:
    mov rax, r8
    ret
#else
    # Linux 调用约定: xh = rdi, xl = rsi
    call lmmp_inv_1_      # 参数在 rdi (xh)
    mov r8, rax
    mov r9, rdi           # r9 = xh
    imul r9, rax          # r9 = xh * inv
    xor rcx, rcx
    mul rsi               # rdx:rax = xl * inv
    add r9, rsi           # r9 = xh*inv + xl
    adc rcx, -1
    add r9, rdx
    adc rcx, 0
    js .lab_2f_lin
.lab_1b_lin:
    dec r8
    sub rax, rsi
    sbb r9, rdi
    sbb rcx, 0
    jns .lab_1b_lin
.lab_2f_lin:
    mov rax, r8
    ret
#endif
