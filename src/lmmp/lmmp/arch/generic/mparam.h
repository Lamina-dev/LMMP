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
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

// 线程局部栈与内存池的大小，单位为字节。
#define LMMP_DEFAULT_STACK_SIZE (320 * 1024)
#define LMMP_POOL_SIZE (512 * 1024)
#define MP_UCHAR_MAX (0xff)
#define MP_USHORT_MAX (0xffff)
#define MP_UINT_MAX (0xffffffff)
#define MP_ULONG_MAX (0xffffffffffffffffull)
#define MP_SIZE_MAX ((mp_size_t)0xffffffffffffffffULL)
#define MP_CHAR_BITS (8)
#define MP_SHORT_BITS (16)
#define MP_INT_BITS (32)
#define MP_LONG_BITS (64)
#define MP_CHAR_BYTES (1)
#define MP_SHORT_BYTES (2)
#define MP_INT_BYTES (4)
#define MP_LONG_BYTES (8)
// B / 2
#define LIMB_B_2 (0x8000000000000000ull)
// B / 4
#define LIMB_B_4 (0x4000000000000000ull)
// B / 8
#define LIMB_B_8 (0x2000000000000000ull)
