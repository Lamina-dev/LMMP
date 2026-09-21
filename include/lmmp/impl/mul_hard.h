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

#ifndef __LMMP_IMPL_MUL_HARD_H__
#define __LMMP_IMPL_MUL_HARD_H__

#include "../lmmpn.h"

/*
        硬编码平衡乘法/平方内部接口 (仿 FLINT flint_mpn_mul_N_N / sqr_N)

        为 n = 1..19 的每种规模提供编译期定长的展开实现:
        x64  mul: N<=8 FLINT 式 mulx+adcx/adox 寄存器整行累加;
              N>=9 numb 分块(<=6)多趟寄存器方案, RMW 终化列以 pc 寄存器
              (<=3) 跨行吸收列完成进位, 尾部线性 adc 链消化;
        arm64 mul: mul/umulh 展开 addmul_1, 每列进位即时以 adc 吸收进 pending;
        sqr: 交叉乘行累加 -> 倍增 -> 对角折叠三段式。
        调度策略见 mul_hard.c: mul 全面走硬编码(实测快于 basecase 8%~47%),
        sqr 仅 n<=3 走硬编码(与库内 basecase 小规模分支同源),
        其余回落 basecase。仅内部使用, 不对外导出。
*/

#define LMMP_MUL_HARD_MAX_N 19

/* 生成 38 个定长函数声明: lmmp_mul_hard_1_ .. lmmp_mul_hard_19_,
                            lmmp_sqr_hard_1_ .. lmmp_sqr_hard_19_ */

#define LMMP_DECL_MUL_HARD(n) \
    void lmmp_mul_hard_##n##_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb);
#define LMMP_DECL_SQR_HARD(n) void lmmp_sqr_hard_##n##_(mp_ptr dst, mp_srcptr numa);

LMMP_DECL_MUL_HARD(1)
LMMP_DECL_MUL_HARD(2)
LMMP_DECL_MUL_HARD(3)
LMMP_DECL_MUL_HARD(4)
LMMP_DECL_MUL_HARD(5)
LMMP_DECL_MUL_HARD(6)
LMMP_DECL_MUL_HARD(7)
LMMP_DECL_MUL_HARD(8)
LMMP_DECL_MUL_HARD(9)
LMMP_DECL_MUL_HARD(10)
LMMP_DECL_MUL_HARD(11)
LMMP_DECL_MUL_HARD(12)
LMMP_DECL_MUL_HARD(13)
LMMP_DECL_MUL_HARD(14)
LMMP_DECL_MUL_HARD(15)
LMMP_DECL_MUL_HARD(16)
LMMP_DECL_MUL_HARD(17)
LMMP_DECL_MUL_HARD(18)
LMMP_DECL_MUL_HARD(19)

LMMP_DECL_SQR_HARD(1)
LMMP_DECL_SQR_HARD(2)
LMMP_DECL_SQR_HARD(3)
LMMP_DECL_SQR_HARD(4)
LMMP_DECL_SQR_HARD(5)
LMMP_DECL_SQR_HARD(6)
LMMP_DECL_SQR_HARD(7)
LMMP_DECL_SQR_HARD(8)
LMMP_DECL_SQR_HARD(9)
LMMP_DECL_SQR_HARD(10)
LMMP_DECL_SQR_HARD(11)
LMMP_DECL_SQR_HARD(12)
LMMP_DECL_SQR_HARD(13)
LMMP_DECL_SQR_HARD(14)
LMMP_DECL_SQR_HARD(15)
LMMP_DECL_SQR_HARD(16)
LMMP_DECL_SQR_HARD(17)
LMMP_DECL_SQR_HARD(18)
LMMP_DECL_SQR_HARD(19)

/**
 * @brief 硬编码平衡乘法调度器 [dst,2n] = [numa,n] * [numb,n]
 * @param dst 结果输出指针
 * @param numa 第一个乘数
 * @param numb 第二个乘数
 * @param n 操作数 limb 长度
 * @warning 0 < n, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL;
 *          n <= LMMP_MUL_HARD_MAX_N 时走硬编码分支, 其余回落 mul_basecase
 * @return 无
 */
void lmmp_mul_hard_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 硬编码平衡平方调度器 [dst,2n] = [numa,n]^2
 * @param dst 结果输出指针
 * @param numa 输入操作数
 * @param n 操作数 limb 长度
 * @warning 0 < n, sep(dst,numa), dst!=NULL, numa!=NULL;
 *          n <= LMMP_MUL_HARD_MAX_N 时走硬编码分支, 其余回落 sqr_basecase
 * @return 无
 */
void lmmp_sqr_hard_n_(mp_ptr dst, mp_srcptr numa, mp_size_t n);

#endif /* __LMMP_IMPL_MUL_HARD_H__ */
