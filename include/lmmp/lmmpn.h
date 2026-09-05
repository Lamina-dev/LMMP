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

#ifndef LMMP_LMMPN_H
#define LMMP_LMMPN_H

#include <stdbool.h>
#include "lmmp.h"

#define INLINE_ static inline

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 运行时判断端序
 * @return true 表示小端序，false 表示大端序
 */
INLINE_ bool lmmp_endian(void) {
    int num = 1;
    return (*(char*)&num) != 0;
}

/**
 * @brief 计算满足 2^k > x 的最小自然数k
 * @param x 输入的64位无符号整数
 * @return 满足条件的最小自然数k
 */
LMMP_API int lmmp_limb_bits_(mp_limb_t x);

/**
 * @brief 计算一个64位无符号整数中1的个数
 * @param x 输入的64位无符号整数
 * @return 1的个数
 */
LMMP_API int lmmp_limb_popcnt_(mp_limb_t x);

/**
 * @brief 计算一个单精度数(limb)中前导零的个数
 * @param x 输入的64位无符号整数
 * @return 前导零的位数（范围：0~64）
 */
LMMP_API int lmmp_leading_zeros_(mp_limb_t x);

/**
 * @brief 计算一个单精度数(limb)中末尾零的个数
 * @param x 输入的64位无符号整数
 * @return 末尾零的位数（范围：0~64）
 */
LMMP_API int lmmp_tailing_zeros_(mp_limb_t x);

/**
 * @brief 计算两个64位无符号整数相乘的高位结果 (a*b)/B
 * @param a 第一个64位无符号整数
 * @param b 第二个64位无符号整数
 * @return 乘积的高64位结果
 */
LMMP_API mp_limb_t lmmp_mulh_(mp_limb_t a, mp_limb_t b);

/**
 * @brief 计算两个64位无符号整数相乘的128位结果 (a*b)
 * @param dst 输出结果缓冲区，存储乘积结果，长度为 2
 * @param a 第一个64位无符号整数
 * @param b 第二个64位无符号整数
 * @warning dst!=NULL
 * @return 无返回值
 */
LMMP_API void lmmp_mullh_(mp_limb_t a, mp_limb_t b, mp_ptr dst);

/**
 * @brief 带进位的n位加法 [dst,n] = [numa,n] + [numb,n] + c
 * @param dst 结果输出指针
 * @param numa 第一个加数指针
 * @param numb 第二个加数指针
 * @param n limb长度
 * @param c 初始进位值 [0|1]
 * @warning c=[0|1], n>0, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 运算后的最终进位值 [0|1]
 */
LMMP_API mp_limb_t lmmp_add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);

/**
 * @brief 无进位的n位加法 [dst,n] = [numa,n] + [numb,n]
 * @param dst 结果输出指针
 * @param numa 第一个加数指针
 * @param numb 第二个加数指针
 * @param n limb长度
 * @warning n>0, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 运算后的最终进位值 [0|1]
 */
LMMP_API mp_limb_t lmmp_add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 带借位的n位减法 [dst,n] = [numa,n] - [numb,n] - c
 * @param dst 结果输出指针
 * @param numa 被减数指针
 * @param numb 减数指针
 * @param n limb长度
 * @param c 初始借位值 [0|1]
 * @warning c=[0|1], n>0, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 运算后的最终借位值 [0|1]
 */
LMMP_API mp_limb_t lmmp_sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);

/**
 * @brief 无借位的n位减法 [dst,n] = [numa,n] - [numb,n]
 * @param dst 结果输出指针
 * @param numa 被减数指针
 * @param numb 减数指针
 * @param n limb长度
 * @warning n>0, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 运算后的最终借位值 [0|1]
 */
LMMP_API mp_limb_t lmmp_sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 同时执行n位加法和减法 ([dsta,n],[dstb,n]) = ([numa,n]+[numb,n],[numa,n]-[numb,n])
 * @param dsta 加法结果输出指针
 * @param dstb 减法结果输出指针
 * @param numa 第一个操作数指针（被加数/被减数）
 * @param numb 第二个操作数指针（加数/减数）
 * @param n limb长度
 * @warning n>0, eqsep(dsta,[numa|numb]), eqsep(dstb,[numa|numb]), dsta!=NULL, dstb!=NULL, numa!=NULL, numb!=NULL
 * @return 组合返回值 cb = 2*c + b (c为加法进位, b为减法借位)
 *         返回值范围: 0(无进位无借位),1(无进位有借位),2(有进位无借位),3(有进位有借位)
 */
LMMP_API mp_limb_t lmmp_add_n_sub_n_(mp_ptr dsta, mp_ptr dstb, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 加法后右移1位 [dst,n] = ([numa,n] + [numb,n]) >> 1
 * @param dst 结果输出指针
 * @param numa 第一个加数指针
 * @param numb 第二个加数指针
 * @param n limb长度
 * @warning n>0, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 右移操作产生的进位值 [0|1]
 */
LMMP_API mp_limb_t lmmp_shr1add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 带进位加法后右移1位 [dst,n] = ([numa,n] + [numb,n] + c) >> 1
 * @param dst 结果输出指针
 * @param numa 第一个加数指针
 * @param numb 第二个加数指针
 * @param n limb长度
 * @param c 初始进位值 [0|1]
 * @warning n>0, c=[0|1], eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 右移操作产生的进位值 [0|1]
 */
LMMP_API mp_limb_t lmmp_shr1add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);

/**
 * @brief 减法后右移1位 [dst,n] = ([numa,n] - [numb,n]) >> 1
 * @param dst 结果输出指针
 * @param numa 被减数指针
 * @param numb 减数指针
 * @param n 操作数的位数（limb数量）
 * @warning n>0, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 右移操作产生的进位值 (0或1)
 */
LMMP_API mp_limb_t lmmp_shr1sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 带借位减法后右移1位 [dst,n] = ([numa,n] - [numb,n] - c) >> 1
 * @param dst 结果输出指针
 * @param numa 被减数指针
 * @param numb 减数指针
 * @param n limb长度
 * @param c 初始借位值 [0|1]
 * @warning n>0, c=[0|1], eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 右移操作产生的进位值 [0|1]
 */
LMMP_API mp_limb_t lmmp_shr1sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c);

/**
 * @brief 右移操作 [dst,na] = [numa,na]>>shr，dst的高shr位填充0
 * @param dst 结果输出指针
 * @param numa 源操作数指针
 * @param na limb长度
 * @param shr 右移的位数 (0~63)
 * @warning na>0, 0<=shr<64, eqsep(dst,numa), dst!=NULL, numa!=NULL
 *          允许dst指针地址小于numa（即支持原地长移位操作）
 * @return 其最高shr个比特位填充[numa,na]被移出的shr个最低位，其余比特位为0
 */
LMMP_API mp_limb_t lmmp_shr_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr);

/**
 * @brief 带进位的右移操作 [dst,na] = [numa,na]>>shr，dst的高shr位填充c的高shr位
 * @param dst 结果输出指针
 * @param numa 源操作数指针
 * @param na limb长度
 * @param shr 右移的位数 (0~63)
 * @param c 进位值（其低(64-shr)位必须为0）
 * @warning na>0, 0<=shr<64, eqsep(dst,numa), dst!=NULL, numa!=NULL
 *          c的低(64-shr)位必须为0
 *          允许dst指针地址小于numa（即支持原地长移位操作）
 * @return 其最高shr个比特位填充[numa,na]被移出的shr个最低位，其余比特位为0
 */
LMMP_API mp_limb_t lmmp_shr_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c);

/**
 * @brief 左移操作 [dst,na] = [numa,na]<<shl，dst的低shl位填充0
 * @param dst 结果输出指针
 * @param numa 源操作数指针
 * @param na limb长度
 * @param shl 左移的位数 (0~63)
 * @warning na>0, 0<=shl<64, eqsep(dst,numa), dst!=NULL, numa!=NULL
 *          允许dst指针地址大于numa（即支持原地长移位操作）
 * @return 其最低shl个比特位填充[numa,na]被移出的shl个最高位，其余比特位为0
 */
LMMP_API mp_limb_t lmmp_shl_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl);

/**
 * @brief 带进位的左移操作 [dst,na] = [numa,na]<<shl，dst的低shl位填充c的低shl位
 * @param dst 结果输出指针
 * @param numa 源操作数指针
 * @param na limb长度
 * @param shl 左移的位数 (0~63)
 * @param c 进位值（其高(64-shl)位必须为0）
 * @warning na>0, 0<=shl<64, eqsep(dst,numa), dst!=NULL, numa!=NULL
 *          c的高(64-shl)位必须为0
 *          允许dst指针地址大于numa（即支持原地长移位操作）
 * @return 其最低shl个比特位填充[numa,na]被移出的shl个最高位，其余比特位为0
 */
LMMP_API mp_limb_t lmmp_shl_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl, mp_limb_t c);

/**
 * @brief 按位取反操作 [dst,na] = ~[numa,na] (对每个limb执行按位非操作)
 * @param dst 结果输出指针
 * @param numa 源操作数指针
 * @param na limb长度
 * @warning na>0, eqsep(dst,numa), dst!=NULL, numa!=NULL
 */
LMMP_API void lmmp_not_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief 左移后按位取反操作 [dst,na] = ~([numa,na] << shl)，dst的低shl位填充1
 * @param dst 结果输出指针
 * @param numa 源操作数指针
 * @param na limb长度
 * @param shl 左移的位数 (0~63)
 * @warning na>0, 0<=shl<64, eqsep(dst,numa), dst!=NULL, numa!=NULL
 * @return 其最低shl个比特位填充[numa,na]被移出的shl个最高位，其余比特位为0
 */
LMMP_API mp_limb_t lmmp_shlnot_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl);

/**
 * @brief 加法结合左移1位操作 [dst,n] = [numa,n] + ([numb,n] << 1)
 * @param dst 结果输出指针
 * @param numa 被加数指针
 * @param numb 加数指针（先左移1位）
 * @param n limb长度
 * @warning n>0, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL
 * @return 运算后的进位值 [0|1|2]
 */
LMMP_API mp_limb_t lmmp_addshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 减法结合左移1位操作 [dst,n] = [numa,n] - ([numb,n] << 1)
 * @param dst 结果输出指针
 * @param numa 被减数指针
 * @param numb 减数指针（先左移1位）
 * @param n limb长度
 * @warning n>0, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL
 * @return 运算后的借位值 [0|1|2]
 */
LMMP_API mp_limb_t lmmp_subshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 乘以单limb并累加操作 [numa,n] += [numb,n] * b
 * @param numa 被加数指针（结果也存储在此）
 * @param numb 乘数指针
 * @param n limb长度
 * @param b 乘数
 * @warning n>0, eqsep(numa,numb)), numa!=NULL, numb!=NULL
 * @return 运算后的进位limb值
 */
LMMP_API mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);

/**
 * @brief 乘以单limb并累减操作 [numa,n] -= [numb,n] * b
 * @param numa 被减数指针（结果也存储在此）
 * @param numb 乘数指针
 * @param n limb长度
 * @param b 乘数
 * @warning n>0, eqsep(numa,numb)), numa!=NULL, numb!=NULL
 * @return 运算后的借位limb值
 */
LMMP_API mp_limb_t lmmp_submul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);

/**
 * @brief 乘以单limb操作 [dst,na] = [numa,na] * x
 * @param dst 结果输出指针
 * @param numa 被乘数指针
 * @param na 操作数的位数（limb数量）
 * @param x 单个limb乘数
 * @warning na>0, eqsep(dst,numa), dst!=NULL, numa!=NULL
 * @return 运算后的进位limb值
 */
LMMP_API mp_limb_t lmmp_mul_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x);

/**
 * @brief 基础平方运算 [dst,2*na] = [numa,na]^2
 * @param dst 输出结果缓冲区，长度至少为2*na
 * @param numa 输入操作数，长度为na
 * @param na 输入操作数的 limb 长度
 * @warning 0<na, sep(dst,numa), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_sqr_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief Toom-2平方运算 [dst,2*na] = [numa,na]^2
 * @param dst 输出结果缓冲区，长度至少为 2*na
 * @param numa 输入操作数，长度为na
 * @param na 输入操作数的 limb 长度
 * @warning ??<na, sep(dst,numa), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_sqr_toom2_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief Toom-3平方运算 [dst,2*na] = [numa,na]^2
 * @param dst 输出结果缓冲区，长度至少为2*na
 * @param numa 输入操作数，长度为na
 * @param na 输入操作数的单精度数(limb)长度
 * @warning ??<na, sep(dst,numa), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_sqr_toom3_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief Toom-4平方运算 [dst,2*na] = [numa,na]^2
 * @param dst 输出结果缓冲区，长度至少为2*na
 * @param numa 输入操作数，长度为na
 * @param na 输入操作数的单精度数(limb)长度
 * @warning ??<na, sep(dst,numa), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_sqr_toom4_(mp_ptr dst, mp_srcptr numa, mp_size_t an);

/**
 * @brief 基础乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 0<nb<=na, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief 基础不平衡乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 0<nb<=na, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_basecase_unbalanced_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-22乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 4/5<=nb/na<=1, nb>=5, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom22_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-32乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 5/9<=nb/na<=4/5, nb>=12, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom32_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-33乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 4/5<=nb/na<=1, nb>=26, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom33_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-42乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 1/3<=nb/na<=5/9, nb>=20, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom42_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-42不平衡乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning na>=3*nb, nb>=20, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom42_unbalance_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-43乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 3/5<=nb/na<=4/5, nb>=??, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom43_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-44乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 4/5<=nb/na<=1, nb>=??, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom44_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-52乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 1/3<=nb/na<=9/20, nb>=??, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom52_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-53乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 9/20<=nb/na<=3/5, nb>=??, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom53_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-62乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning 1/5<=nb/na<=1/3, nb>=??, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom62_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief Toom-62不平衡乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning na>=5*nb, nb>=??, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_toom62_unbalance_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief 计算满足 >=n 的最小费马/梅森乘法可行尺寸
 * @param n 输入的目标尺寸
 * @return 满足条件的SSA乘法最小尺寸
 */
LMMP_API mp_size_t lmmp_fft_next_size_(mp_size_t n);

/**
 * @brief 费马数模乘法 [dst,rn+1]=[numa,na]*[numb,nb] mod B^rn+1
 * @param dst 输出结果缓冲区，长度至少为 rn+1
 * @param rn 模运算的阶数参数，rn = lmmp_fft_next_size_((na + nb + 1) >> 1)
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning eqsep(dst,[numa|numb]), 0<=[numa,na]<2*B^rn, 0<=[numb,nb]<2*B^rn, rn = lmmp_fft_next_size_((na+nb+1)>>1),
 *          dst!=NULL, numa!=NULL, numb!=NULL
 * @note 如果[numa,na]==[numb,nb]，请使用lmmp_sqr_fermat_函数，此函数不会检查操作数是否一致并跳转到lmmp_sqr_fermat_函数
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_fermat_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief 梅森数模乘法 [dst,rn] = [numa,na]*[numb,nb] mod B^rn-1
 * @param dst 输出结果缓冲区，长度至少为 rn
 * @param rn 模运算的阶数参数，rn = lmmp_fft_next_size_((na + nb + 1) >> 1)
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning eqsep(dst,[numa|numb]), 0<=[numa,na]<B^rn, 0<=[numb,nb]<B^rn, rn = lmmp_fft_next_size_((na+nb+1)>>1),
 *          dst!=NULL, numa!=NULL, numb!=NULL
 * @note 如果[numa,na]==[numb,nb]，请使用lmmp_sqr_mersenne_函数，此函数不会检查操作数是否一致并跳转到lmmp_sqr_mersenne_函数
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_mersenne_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief 费马数模平方 [dst,rn+1]=[numa,na]^2 mod B^rn+1
 * @param dst 输出结果缓冲区，长度至少为 rn+1
 * @param rn 模运算的阶数参数，rn = lmmp_fft_next_size_((2*na + 1) >> 1)
 * @param numa 操作数，长度为 na
 * @param na 操作数的 limb 长度
 * @warning eqsep(dst,numa), 0<=[numa,na]<2*B^rn, rn = lmmp_fft_next_size_((2*na+1)>>1), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_sqr_fermat_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na);

/**
 * @brief 梅森数模平方 [dst,rn] = [numa,na]^2 mod B^rn-1
 * @param dst 输出结果缓冲区，长度至少为 rn
 * @param rn 模运算的阶数参数，rn = lmmp_fft_next_size_((2*na + 1) >> 1)
 * @param numa 输入操作数，长度为 na
 * @param na 操作数的 limb 长度
 * @warning eqsep(dst,numa), 0<=[numa,na]<B^rn, rn = lmmp_fft_next_size_((2*na+1)>>1), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_sqr_mersenne_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na);

/**
 * @brief FFT乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning ???<=nb<=na, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_fft_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief FFT平方运算 [dst,2*na] = [numa,na]^2
 * @param dst 输出结果缓冲区，长度至少为 2*na
 * @param numa 输入操作数指针，长度为 na
 * @param na 输入操作数的 limb 长度
 * @warning ???<=na, sep(dst,numa), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_sqr_fft_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief FFT不平衡乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param hn FFT模域参数
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @warning ???<=nb<=na, na>=3*nb, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_fft_unbalance_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief 平方操作 [dst,2*na] = [numa,na]^2
 * @param dst 平方结果输出指针（需要2*na的limb长度）
 * @param numa 输入操作数指针
 * @param na 输入的 limb 长度
 * @warning na>0, sep(dst,numa), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_sqr_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief 等长乘法操作 [dst,2*n] = [numa,n] * [numb,n]
 * @param dst 乘积结果输出指针（需要 2*n 的 limb 长度）
 * @param numa 第一个乘数指针
 * @param numb 第二个乘数指针
 * @param n limb长度
 * @warning n>0, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 不等长乘法操作 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 乘积结果输出指针（需要 na+nb 的 limb 长度）
 * @param numa 第一个乘数指针（较长的操作数）
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个乘数指针（较短的操作数）
 * @param nb 第二个操作数的 limb 长度
 * @warning 0<nb<=na, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中
 */
LMMP_API void lmmp_mul_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief 基础低位乘法 [dst,n] = [numa,n] * [numb,n] mod B^n
 * @param dst 输出结果缓冲区，长度至少为 n
 * @param numa 第一个输入操作数，长度为 n
 * @param numb 第二个输入操作数，长度为 n
 * @param n limb长度
 * @warning n>0, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中，[dst,n]=[numa,n] * [numb,n] mod B^n
 */
LMMP_API void lmmp_mullo_basecase_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 低位乘法 [dst,n] = [numa,n] * [numb,n] mod B^n
 * @param dst 输出结果缓冲区，长度至少为 n
 * @param numa 第一个输入操作数，长度为 n
 * @param numb 第二个输入操作数，长度为 n
 * @param n limb长度
 * @warning n>0, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中，[dst,n]=[numa,n] * [numb,n] mod B^n
 */
LMMP_API void lmmp_mullo_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 低位乘法 [dst,n] = [numa,n] * [numb,n] mod B^n
 * @param dst 输出结果缓冲区，长度至少为 n
 * @param numa 第一个输入操作数，长度为 n
 * @param numb 第二个输入操作数，长度为 n
 * @param tp 临时缓冲区，长度至少为 2*n
 * @param n limb长度
 * @warning n>0, sep(dst,[numa|numb],tp), dst!=NULL, numa!=NULL, numb!=NULL
 * @return 无返回值，结果存储在dst中，[dst,n]=[numa,n] * [numb,n] mod B^n
 */
LMMP_API void lmmp_mullo_dc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_ptr tp, mp_size_t n);

/**
 * @brief 低位平方 [dst,n] = [numa,n]^2 mod B^n
 * @param dst 输出结果缓冲区，长度至少为 n
 * @param numa 第一个输入操作数，长度为 n
 * @param tp 临时缓冲区，长度至少为 2*n
 * @param n limb长度
 * @warning n>0, sep(dst,numa,tp), dst!=NULL, numa!=NULL, tp!=NULL
 * @return 无返回值，结果存储在dst中，[dst,n]=[numa,n]^2 mod B^n
 */
LMMP_API void lmmp_sqrlo_dc_(mp_ptr dst, mp_srcptr numa, mp_ptr tp, mp_size_t n);

/**
 * @brief 低位FFT乘法 [dst,n] = [numa,n] * [numb,n] mod B^n
 * @param dst 输出结果缓冲区，长度至少为 n
 * @param numa 第一个输入操作数，长度为 n
 * @param numb 第二个输入操作数，长度为 n
 * @param scratch 临时缓冲区，长度至少为 2*n
 * @param n 缓冲区 limb 长度
 * @warning ???<n, sep(scratch,[numa|numb]), eqsep(dst,scratch)
 * @return 无返回值，结果存储在dst中，[dst,n]=[numa,n] * [numb,n] mod B^n
 */
LMMP_API void lmmp_mullo_fft_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_ptr scratch);

/**
 * @brief 1阶逆元计算 (inv1)
 * @param x 输入的64位无符号整数，最高位为1(MSB(x)=1)
 * @return 计算结果：(B^2-1)/x - B
 * @warning MSB(x)=1, 即x>=2^63
 */
LMMP_API mp_limb_t lmmp_inv_1_(mp_limb_t x);

/**
 * @brief 2-1阶逆元计算 (inv21)
 * @param xh 输入数的高64位部分
 * @param xl 输入数的低64位部分
 * @return 计算结果：(B^3-1)/(xh*B+xl) - B
 * @warning MSB(xh)=1, 即xh>=2^63
 */
LMMP_API mp_limb_t lmmp_inv_2_1_(mp_limb_t xh, mp_limb_t xl);

/**
 * @brief 近似逆元计算
 * @param dst 输出结果缓冲区，长度为na
 * @param numa 输入操作数，长度为na
 * @param na 输入操作数的 limb 长度
 * @warning na>0, MSB(numa)=1, sep(dst,numa), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中，[dst,na]=(B^(2*na)-1)/[numa,na] - B^na
 */
LMMP_API void lmmp_inv_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief 近似逆元计算（牛顿迭代法）
 * @param dst 输出结果缓冲区，长度为na
 * @param numa 输入操作数，长度为na
 * @param na 输入操作数的 limb 长度
 * @warning na>4, MSB(numa)=1, sep(dst,numa), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中，[dst,na]=(B^(2*na)-1)/[numa,na]-B^na+[0|-1]
 */
LMMP_API void lmmp_invappr_newton_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief 近似逆元计算 (invappr)
 * @param dst 输出结果缓冲区，长度为na
 * @param numa 输入操作数，长度为na
 * @param na 输入操作数的 limb 长度
 * @warning na>0, MSB(numa)=1, sep(dst,numa), dst!=NULL, numa!=NULL
 * @return 无返回值，结果存储在dst中，[dst,na] = (B^(2*na)-1)/[numa,na] - B^na + [0|-1]
 */
LMMP_API void lmmp_invappr_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief 3/2位除法运算 [numa,2]=[numa,3] mod [numb,2]
 * @param numa 输入被除数（长度3），运算后存储余数（长度2）
 * @param numb 输入除数（长度2）
 * @param inv21 除数的2-1阶逆元（提前计算好的inv21([numb,2])）
 * @warning [numa,3]<[numb,2]*B, MSB(numb)=1, inv21=inv21([numb,2]), eqsep(numa,numb), numb!=NULL, numa!=NULL
 * @return 商值（单精度数）
 */
LMMP_API mp_limb_t lmmp_div_3_2_(mp_ptr numa, mp_srcptr numb, mp_limb_t inv21);

/**
 * @brief 单精度数除法
 * @param dstq 输出商的缓冲区（可为NULL，此时仅计算余数）
 * @param numa 输入被除数，长度为na
 * @param na 被除数的 limb 长度
 * @param x 除数（单个 limb ）
 * @return 除法余数（单个 limb ）
 * @warning na>0, x!=0, eqsep(dstq,numa),
 * @warning [numa,3]<[numb,2]*B, MSB(numb)=1, inv21=inv21([numb,2]), eqsep(numa,numb), numa!=NULL,
 *          dstq>=numa-1 是可以接受的
 * @note if (dstq!=NULL) [dstq,na] = [numa,na] div x
 */
LMMP_API mp_limb_t lmmp_div_1_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_limb_t x);

/**
 * @brief 单精度数取余
 * @param numa 输入被除数，长度为na
 * @param na 被除数的 limb 长度
 * @param x 除数（单个 limb ）
 * @return 除法余数（单个 limb ）
 * @warning na>0, x!=0, numa!=NULL
 */
LMMP_API mp_limb_t lmmp_mod_1_(mp_srcptr numa, mp_size_t na, mp_limb_t x);

/**
 * @brief 双精度数除法 (除数为2个limb)
 * @param dstq 输出商的缓冲区，长度至少为na-1（可为NULL，此时仅计算余数）
 * @param numa 输入被除数（长度na）
 * @param na 被除数的 limb 长度
 * @param numb 输入除数（长度2）[numb,2]=[numa,na] mod [numb,2]
 * @warning na>=2, numb[1]!=0, eqsep(dstq,numa), numa!=NULL, numb!=NULL, dstq>=numa 是可以接受的
 * @note if (dstq!=NULL) [dstq,na-1]=[numa,na] div [numb,2]
 */
LMMP_API void lmmp_div_2_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_ptr numb);

/**
 * @brief 双精度数取余 (除数为2个limb)
 * @param numa 输入被除数（长度na）
 * @param na 被除数的 limb 长度
 * @param numb 输入除数（长度2）[numb,2]=[numa,na] mod [numb,2]
 * @warning na>=2, numb[1]!=0, numb!=NULL, numa!=NULL
 */
LMMP_API void lmmp_mod_2_(mp_srcptr numa, mp_size_t na, mp_ptr numb);

/**
 * @brief 基础除法运算
 * @param dstq 输出商的缓冲区，长度至少为na-nb
 * @param numa 输入被除数（长度na），运算后存储余数（长度nb）
 * @param na 被除数的 limb 长度
 * @param numb 输入除数，长度为nb
 * @param nb 除数的 limb 长度
 * @param inv21 除数的2-1阶逆元（inv21([numb+nb-2,2])）
 * @return 商的最高位（qh）
 * @warning na>=nb>=3, MSB(numb)=1, inv21=inv21([numb+nb-2,2]), sep(dstq,numa,numb),
 *          dstq!=NULL, numa!=NULL, numb!=NULL
 * @note qh:[dstq,na-nb]=[numa,na] div [numb,nb], [numa,na-nb]=[numa,na] mod [numb,nb], return qh
 */
LMMP_API mp_limb_t lmmp_div_basecase_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb,
                                      mp_limb_t inv21);

/**
 * @brief 分治除法运算
 * @param dstq 输出商的缓冲区，长度至少为na-nb
 * @param numa 输入被除数（长度na），运算后存储余数（长度nb）
 * @param na 被除数的 limb 长度
 * @param numb 输入除数，长度为nb
 * @param nb 除数的 limb 长度
 * @param inv21 除数的2-1阶逆元（inv21([numb+nb-2,2])）
 * @return 商的最高位（qh）
 * @warning na>=2*nb, nb>=6, MSB(numb)=1, inv21=inv21([numb+nb-2,2]), sep(dstq,numa,numb),
 *          dstq!=NULL, numa!=NULL, numb!=NULL
 * @note qh:[dstq,na-nb]=[numa,na] div [numb,nb], [numa,na-nb]=[numa,na] mod [numb,nb], return qh
 */
LMMP_API mp_limb_t lmmp_div_divide_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb,
                                    mp_limb_t inv21);

/**
 * @brief 计算预计算逆元的尺寸
 * @param nq 商的 limb 长度
 * @param nb 除数的 limb 长度
 * @return 计算需要预计算逆元尺寸ni（ni<=nb）
 * @note 用于已归一化除法([nq+nb]/[nb]=[nq])的逆元 ni 尺寸
 */
INLINE_ mp_size_t lmmp_div_inv_size_(mp_size_t nq, mp_size_t nb) {
    mp_size_t ni, b;
    if (nq > nb) {
        b = (nq - 1) / nb + 1;  // ceil(nq/nb), number of blocks
        ni = (nq - 1) / b + 1;  // ceil(nq/b)
    } else if (3 * nq > nb) {
        ni = (nq - 1) / 2 + 1;  // b=2
    } else {
        ni = (nq - 1) / 1 + 1;  // b=1
    }
    return ni;
}

/**
 * @brief 除法前的逆元预计算，[dst,ni] = invappr( (ni+1 MSLs of numa) + 1 ) / B
 * @param dst 输出预计算逆元的缓冲区，长度为ni
 * @param numa 输入操作数，长度为na
 * @param na 输入操作数的 limb 长度
 * @param ni 预计算逆元的目标尺寸
 * @warning na>=ni>0, MSB(numa)=1, eqsep(dst,numa), dst!=NULL, numa!=NULL
 * @note if (ni=na) [dst,na] = (B^(2*na)-1) / [numa,na] - B^na
 */
LMMP_API void lmmp_inv_prediv_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t ni);

/**
 * @brief 求逆操作 [dst,na+nf+1] = (B^(2*(na+nf)) - 1) / ([numa,na]*B^nf) + [0|-1]
 * @param dst 逆元结果输出指针
 * @param numa 源操作数指针
 * @param na 操作数的 limb 长度
 * @param nf 精度因子
 * @warning na>0, numa[na-1]!=0, eqsep(dst,numa), dst!=NULL, numa!=NULL
 */
LMMP_API void lmmp_inv_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t nf);

/**
 * @brief 近似逆元计算 [dstq,na+ni+2] = B^(2*(na+ni)) / ([numa,na] * B^ni) + [0|-ept]
 * @param dstq 输出逆元的缓冲区，长度至少为na+ni+2
 * @param numa 输入指针（长度na）
 * @param na 输入指针的 limb 长度
 * @param ni 精度因子
 * @warning na>0, sep(dstq,numa), dstq!=NULL, numa[na-1]!=0, dstq!=NULL, numa!=NULL
 * @note 也就是计算 B^(2*na+ni) div [numa,na] + [0|-ept]，存在接近2^-64的绝对误差
 */
LMMP_API void lmmp_bninv_(mp_ptr dstq, mp_srcptr numa, mp_size_t na, mp_size_t ni);

/**
 * @brief 乘法逆元除法
 * @param dstq 输出商的缓冲区，长度至少为na-nb
 * @param numa 输入被除数（长度na），运算后存储余数（长度nb）
 * @param na 被除数的 limb 长度
 * @param numb 输入除数，长度为nb
 * @param nb 除数的 limb 长度
 * @param invappr 预计算的近似逆元，长度为ni
 * @param ni 预计算逆元的 limb 长度
 * @return 商的最高位（qh）
 * @warning na>=nb>=ni>0, MSB(numb)=1, [invappr,ni]=inv_prediv([numb,nb]), sep(dstq,numa,numb,invappr))
 *          dstq!=NULL, numa!=NULL, numb!=NULL, invappr!=NULL
 * @note qh:[dstq,na-1]=[numa,na] div x, [numa,1]=[numa,na] mod x, return qh
 */
LMMP_API mp_limb_t lmmp_div_mulinv_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb,
                                    mp_srcptr invappr, mp_size_t ni);

/**
 * @brief 单精度数除法（除数为1个limb）
 * @param dstq 输出商的缓冲区，长度至少为na-1
 * @param numa 输入被除数（长度na），运算后存储余数（长度1）
 * @param na 被除数的 limb 长度
 * @param x 除数（单个 limb ）
 * @return 商的最高位（qh）
 * @warning na>1, MSB(x)=1, sep(dstq,numa), dstq!=NULL, numa!=NULL
 * @note qh:[dstq,na-1]=[numa,na] div x, [numa,1]=[numa,na] mod x, return qh
 */
LMMP_API mp_limb_t lmmp_div_1_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_limb_t x);

/**
 * @brief 双精度数除法（除数为2个limb）
 * @param dstq 输出商的缓冲区，长度至少为na-2
 * @param numa 输入被除数（长度na），运算后存储余数（长度2）
 * @param na 被除数的 limb 长度
 * @param numb 输入除数，长度为2
 * @return 商的最高位（qh）
 * @warning na>2, MSB(numb)=1, sep(dstq,numa,numb), dstq!=NULL, numa!=NULL, numb!=NULL
 * @note qh:[dstq,na-2]=[numa,na] div [numb,2], [numa,2]=[numa,na] mod [numb,2], return qh
 */
LMMP_API mp_limb_t lmmp_div_2_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_srcptr numb);

/**
 * @brief 除法运算
 * @param dstq 输出商的缓冲区，长度至少为na-nb
 * @param numa 输入被除数（长度na），运算后存储余数（长度nb）
 * @param na 被除数的 limb 长度
 * @param numb 输入除数，长度为nb
 * @param nb 除数的 limb 长度
 * @return 商的最高位（qh）
 * @warning na>=nb>0, MSB(numb)=1, sep(dstq,numa,numb), dstq!=NULL, numa!=NULL, numb!=NULL
 * @note qh:[dstq,na-nb]=[numa,na] div [numb,nb], [numa,nb]=[numa,na] mod [numb,nb], return qh
 */
LMMP_API mp_limb_t lmmp_div_s_(mp_ptr dstq, mp_ptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief 除法和取模操作
 * @param dstq 商结果输出指针（NULL表示不计算商）
 * @param dstr 余数结果输出指针（NULL表示不计算余数）
 * @param numa 被除数指针
 * @param na 被除数的 limb 长度
 * @param numb 除数指针
 * @param nb 除数的 limb 长度
 * @warning 0<nb<=na, numb[nb-1]!=0, sep(dstq,[numa|numb]), eqsep(dstr,[numa|numb]))
 *          numa!=NULL, numb!=NULL
 *          特殊情况: nb==1时, dstq>=numa-1 是允许的
 *                   nb==2时, dstq>=numa 是允许的
 * @note 如果dstq不为NULL: [dstq,na-nb+1] = [numa,na] / [numb,nb] (商)
 *       如果dstr不为NULL: [dstr,nb] = [numa,na] mod [numb,nb] (余数)
 */
LMMP_API void lmmp_div_(mp_ptr dstq, mp_ptr dstr, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

/**
 * @brief加1宏（预期无进位）
 * @param p 指针
 * @note 从最低位开始加1，直到遇到非零值（预期无进位溢出）
 */
#define lmmp_inc(p)                \
    do {                           \
        mp_ptr _p_ = (p);          \
        while (++(*(_p_++)) == 0); \
    } while (0)

/**
 * @brief 加指定值宏（预期无进位）
 * @param p 指针
 * @param inc 要加的单精度数值
 * @note 先加最低位，若产生进位则逐位加1，直到无进位（预期无溢出）
 */
#define lmmp_inc_1(p, inc)             \
    do {                               \
        mp_ptr _p_ = (p);              \
        mp_limb_t _inc_ = (inc), _x_;  \
        _x_ = *_p_ + _inc_;            \
        *_p_ = _x_;                    \
        if (_x_ < _inc_)               \
            while (++(*(++_p_)) == 0); \
    } while (0)

/**
 * @brief 减1宏（预期无借位）
 * @param p 指针
 * @note 从最低位开始减1，直到遇到非零值（预期无借位溢出）
 */
#define lmmp_dec(p)                \
    do {                           \
        mp_ptr _p_ = (p);          \
        while ((*(_p_++))-- == 0); \
    } while (0)

/**
 * @brief 减指定值宏（预期无借位）
 * @param p 指针
 * @param dec 要减的单精度数值
 * @note 先减最低位，若产生借位则逐位减1，直到无借位（预期无溢出）
 */
#define lmmp_dec_1(p, dec)             \
    do {                               \
        mp_ptr _p_ = (p);              \
        mp_limb_t _dec_ = (dec), _x_;  \
        _x_ = *_p_;                    \
        *_p_ = _x_ - _dec_;            \
        if (_x_ < _dec_)               \
            while ((*(++_p_))-- == 0); \
    } while (0)

/**
 * @brief 比较函数（内联）
 * @param numa 第一个指针，长度为n
 * @param numb 第二个指针，长度为n
 * @param n 比较两数的 limb 长度
 * @return 1(numa>numb) / 0(numa==numb) / -1(numa<numb)
 * @warning n>0, numa!=NULL, numb!=NULL
 * @note 从最高位开始逐位比较，直到找到不同位
 */
INLINE_ int lmmp_cmp_(mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    lmmp_param_assert(n > 0);
    lmmp_param_assert(numa != NULL);
    lmmp_param_assert(numb != NULL);
    mp_ssize_t i = n;
    mp_limb_t x, y;
    while (--i >= 0) {
        x = numa[i];
        y = numb[i];
        if (x != y)
            return (x > y ? 1 : -1);
    }
    return 0;
}

/**
 * @brief 判零函数（内联）
 * @param p 指针
 * @param n 数的 limb 长度
 * @return 1(全零) / 0(非零)
 * @warning n>0, p!=NULL
 * @note 从最高位开始检查，只要有非零位则返回0
 */
INLINE_ int lmmp_zero_q_(mp_srcptr p, mp_size_t n) {
    do {
        if (p[--n] != 0)
            return 0;
    } while (n != 0);
    return 1;
}

#define LMMP_AORS_(FUNCTION, TEST)               \
    mp_limb_t _x_;                               \
    if (FUNCTION(dst, numa, numb, nb)) {         \
        do {                                     \
            if (nb >= na)                        \
                return 1;                        \
            _x_ = numa[nb];                      \
        } while (TEST);                          \
    }                                            \
    if (dst != numa && na != nb)                 \
        lmmp_copy(dst + nb, numa + nb, na - nb); \
    return 0

/**
 * @brief 加法静态内联函数 [dst,na]=[numa,na]+[numb,nb]
 * @param dst 输出结果缓冲区，存储numa + numb
 * @param numa 第一个加数，长度为na
 * @param na 第一个加数的 limb 长度
 * @param numb 第二个加数，长度为nb
 * @param nb 第二个加数的 limb 长度
 * @return 进位标志（1表示有进位，0表示无进位）
 * @warning 0<nb<=na, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 */
INLINE_ mp_limb_t lmmp_add_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb) {
    LMMP_AORS_(lmmp_add_n_, ((dst[nb++] = _x_ + 1) == 0));
}

/**
 * @brief 减法静态内联函数 [dst,na]=[numa,na]-[numb,nb]
 * @param dst 输出结果缓冲区，存储numa - numb
 * @param numa 被减数，长度为na
 * @param na 被减数的 limb 长度
 * @param numb 减数，长度为nb
 * @param nb 减数的 limb 长度
 * @return 借位标志（1表示有借位，0表示无借位）
 * @warning 0<nb<=na, eqsep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL
 */
INLINE_ mp_limb_t lmmp_sub_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb) {
    LMMP_AORS_(lmmp_sub_n_, ((dst[nb++] = _x_ - 1), _x_ == 0));
}

#undef LMMP_AORS_

// 单精度加减运算通用宏：封装单精度加减的公共逻辑
#define LMMP_AORS_1_(OP, CB)                        \
    mp_size_t _i_ = 1;                              \
    mp_limb_t _x_ = numa[0], _r_ = _x_ OP x;        \
    dst[0] = _r_;                                   \
    if (CB(_r_, _x_, x)) {                          \
        do {                                        \
            if (_i_ >= na)                          \
                return 1;                           \
            _x_ = numa[_i_];                        \
            _r_ = _x_ OP 1;                         \
            dst[_i_] = _r_;                         \
            ++_i_;                                  \
        } while (CB(_r_, _x_, 1));                  \
    }                                               \
    if (numa != dst && na != _i_)                   \
        lmmp_copy(dst + _i_, numa + _i_, na - _i_); \
    return 0

// 加法进位判断宏：判断加法是否产生进位
#define LMMP_ADDCB_(r, x, y) ((r) < (y))
// 减法借位判断宏：判断减法是否产生借位
#define LMMP_SUBCB_(r, x, y) ((x) < (y))

/**
 * @brief 加单精度数静态内联函数 [dst,na]=[numa,na]+x
 * @param dst 输出结果缓冲区，存储numa + x
 * @param numa 被加数，长度为na
 * @param na 被加数的 limb 长度
 * @param x 加数（单个 limb ）
 * @return 进位标志（1表示有进位，0表示无进位）
 * @warning na>0, eqsep(dst,numa), dst!=NULL, numa!=NULL
 */
INLINE_ mp_limb_t lmmp_add_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x) { LMMP_AORS_1_(+, LMMP_ADDCB_); }

/**
 * @brief 减单精度数静态内联函数 [dst,na]=[numa,na]-x
 * @param dst 输出结果缓冲区，存储numa - x
 * @param numa 被减数，长度为na
 * @param na 被减数的 limb 长度
 * @param x 减数（单个 limb ）
 * @return 借位标志（1表示有借位，0表示无借位）
 * @warning na>0, eqsep(dst,numa), dst!=NULL, numa!=NULL
 */
INLINE_ mp_limb_t lmmp_sub_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x) { LMMP_AORS_1_(-, LMMP_SUBCB_); }

/**
 * @brief 计算转换为字节数组，字节数组需要的缓冲区长度
 * @param numa 输入指针，长度为na
 * @param na 输入的 limb 长度
 * @param base 目标基数（2~256）
 * @warning na>=0, 2<=base<=256
 * @note 将会忽略numa的前导零，
 *       1. if (numa!=NULL) 返回的长度可能会多分配一个字节
 *       2. if (numa==NULL) 返回na个limb长度的数的最大可能字节长度（最坏情况）
 * @return 在指定基数下的位数
 */
LMMP_API mp_size_t lmmp_to_str_len_(mp_srcptr numa, mp_size_t na, int base);

/**
 * @brief 计算字节数组转limb数组所需的 limb 缓冲区长度
 * @param src 输入字节数组指针
 * @param len 字节数组长度
 * @param base 字节数组的基数（2~256）
 * @return 存储该字节数组数值所需的 limb 缓冲区长度
 * @warning len>=0, 2<=base<=256
 * @note 将会忽略前导零，
 *       1. if (src!=NULL) 返回的长度可能会多分配一个 limb 空间
 *       2. if (src==NULL) 返回len位base进制数的最大可能 limb 长度（最坏情况）
 */
LMMP_API mp_size_t lmmp_from_str_len_(const mp_byte_t* src, mp_size_t len, int base);

/**
 * @brief 字节数组转limb数组操作 [src,len,base] to [dst,return value,B]
 * @param dst 结果输出指针
 * @param src 字节数组源指针
 * @param len 字节数组长度
 * @param base 字节数组的进制基数
 * @warning len>=0, 2<=base<=256, dst!=NULL, src!=NULL
 * @return 转换后的结果的 limb 长度
 */
LMMP_API mp_size_t lmmp_from_str_(mp_ptr dst, const mp_byte_t* src, mp_size_t len, int base);

/**
 * @brief limb数组转字节数组操作 [numa,na,B] to [dst,return value,base]
 * @param dst 字节数组结果输出指针
 * @param numa 输入指针
 * @param na 输入的 limb 长度
 * @param base 目标字节数组的进制基数
 * @warning na>=0, 2<=base<=256, dst!=NULL, numa!=NULL
 * @return 转换后的字节数组长度
 */
LMMP_API mp_size_t lmmp_to_str_(mp_byte_t* dst, mp_srcptr numa, mp_size_t na, int base);

/**
 * @brief 提取高位指定位数，并返回低位bits位数
 * @param num 待提取的指针
 * @param n num的 limb 长度
 * @param bits 待提取的位数(1-64)
 * @param ext 提取结果输出指针
 * @warning n>0, 1<=bits<=64, ext!=NULL, num!=NULL
 * @note 如果bits大于num的实际位数，则不会保证ext有效位数为bits位；
 *       如果bits小于等于num的实际位数，则ext将会有bits位有效位数。
 * @return 剩余的低位bits数量
 */
LMMP_API mp_bitcnt_t lmmp_extract_bits_(mp_srcptr num, mp_size_t n, mp_limb_t* ext, int bits);

#ifdef __cplusplus
}  // extern "C"
#endif

#undef LMMP_ADDCB_
#undef LMMP_SUBCB_
#undef LMMP_AORS_1_


#undef INLINE_

#endif  // LMMP_LMMPN_H