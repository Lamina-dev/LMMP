/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __LMMP_TRIALDIV_H__
#define __LMMP_TRIALDIV_H__

#include "../numth.h"
#include "../lmmpn.h"

/**
 * @brief 试除法：判定 [np,nn] 是否存在小奇素数因子
 * @param np 被除数（长度为 nn 个limb）
 * @param nn 被除数的 limb 长度
 * @param N 试除上界提示
 * @warning np!=NULL, nn>0
 * @note 试除素数自 3 起（因子 2 由调用方自理）；N 仅为上界提示——实际试除
 *       范围为所有含 <=N 素数的块之并（块粒度向上取整，可能试除 N 之后的
 *       素数），需要精确 N 边界者勿用
 * @return 存在能整除 [np,nn] 的试除素数则返回true，否则返回false
 */
bool lmmp_trialdiv_(mp_srcptr np, mp_size_t nn, ushort N);

/* 全局试除块表释放（lmmp_global_entries 钩子；表本身首次试除时惰性构建） */
void lmmp_trialdiv_table_free_(void);

/**
 * @brief 4 链折叠取模：r = [numa,na] mod d
 * @param numa 被除数（长度为 na 个limb）
 * @param na 被除数的 limb 长度
 * @param d 除数（单个 limb）
 * @param dinv 除数的逆元（d*dinv==1 mod 2^64）
 * @param r2 蒙域预处理（r2=B^2 mod d）
 * @param base 蒙域预处理（base=B mod d）
 * @param mi 蒙域预处理（mi=-dinv mod 2^64）
 * @warning na>0, numa!=NULL, d%2==1, d<2^63, d*dinv==1 mod 2^64,
 *          dinv+mi==0 (mod B), r2=B^2 mod d, base=B mod d
 * @return [numa,na] mod d
 */
mp_limb_t lmmp_mod_4chunk_1_(mp_srcptr numa, mp_size_t na, mp_limb_t d, mp_limb_t dinv,
                             mp_limb_t r2, mp_limb_t base, mp_limb_t mi);

/**
 * @brief 8 链折叠取模：r = [numa,na] mod d
 * @param numa 被除数（长度为 na 个limb）
 * @param na 被除数的 limb 长度
 * @param d 除数（单个 limb）
 * @param dinv 除数的逆元（d*dinv==1 mod 2^64）
 * @param r2 蒙域预处理（r2=B^2 mod d）
 * @param base 蒙域预处理（base=B mod d）
 * @param mi 蒙域预处理（mi=-dinv mod 2^64）
 * @warning na>0, numa!=NULL, d%2==1, d<2^63, d*dinv==1 mod 2^64,
 *          dinv+mi==0 (mod B), r2=B^2 mod d, base=B mod d
 * @return [numa,na] mod d
 */
mp_limb_t lmmp_mod_8chunk_1_(mp_srcptr numa, mp_size_t na, mp_limb_t d, mp_limb_t dinv,
                             mp_limb_t r2, mp_limb_t base, mp_limb_t mi);

#endif /* __LMMP_TRIALDIV_H__ */
