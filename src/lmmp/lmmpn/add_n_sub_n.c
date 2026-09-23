/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/lmmpn.h"

/*
    汇编不可用时的朴素参考实现（eqsep 重叠安全：每肢双加载先于双存储）。
    汇编构建使用 asm/{x64,arm64}/add_n_sub_n.S：
      x64   为 adcx/adox 双进位链（差链经补码加法 d = a + ~b + k）；
      arm64 为每肢交错、进位寄存器化的简单版本。
*/
#ifndef LMMP_ASM

mp_limb_t lmmp_add_n_sub_n_(mp_ptr dsta, mp_ptr dstb, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_size_t i;
    mp_limb_t acyo, scyo;

    for (i = 0, acyo = 0, scyo = 0; i < n; i++) {
        mp_limb_t a, b, r;
        a = numa[i];
        b = numb[i];
        r = a + acyo;
        acyo = (r < acyo);
        r += b;
        acyo += (r < b);
        dsta[i] = r;

        b += scyo;
        scyo = (b < scyo);
        scyo += (a < b);
        dstb[i] = a - b;
    }
    return 2 * acyo + scyo;
}

#endif /* !LMMP_ASM */
