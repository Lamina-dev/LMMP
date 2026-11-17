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

// cache 一次处理的位图数量
#define PRIME_CACHE_BLOCK_NUM 64
// cache 中质数最多可能的数量（取决于上面的PRIME_CACHE_BLOCK_NUM）。
// mod-30 位图下 64 字 = 512 字节 = 15360 个数，最坏为首块（含补入的 3、5）：
// pi(15359) - 1 = 1793，取 1794（与旧奇数编码取 pi(8191) = 1028 同风格 +1 松弛）
#define PRIME_CACHE_SIZE 1794
