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

#ifndef LMMP_INTERNAL_H
#define LMMP_INTERNAL_H

// 封装名包含所属模块与源码作用域，声明与实现归原模块。
#if defined(LMMP_INTERNAL_BUILD)
#define LMMP_INTERNAL_ENTRY(type, name, parameters, ...) \
    type name parameters { __VA_ARGS__ }
#else
#define LMMP_INTERNAL_ENTRY(type, name, parameters, ...)
#endif

#endif
