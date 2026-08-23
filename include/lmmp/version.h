/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#ifndef LMMP_VERSION_H
#define LMMP_VERSION_H

#include "lmmp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取 LMMP 的版本字符串.
 */
LMMP_API const char* lmmp_get_version(void);

/**
* @brief 获取 LMMP 的构建类型字符串.
*/
LMMP_API const char* lmmp_get_build_type(void);

#ifdef __cplusplus
}
#endif

#endif /* LMMP_VERSION_H */
