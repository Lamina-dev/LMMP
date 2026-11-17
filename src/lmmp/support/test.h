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

#ifndef LMMP_TEST_H
#define LMMP_TEST_H

#include <stdbool.h>
#include <stdio.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void lmmp_test_check(bool ok, const char *file, int line, const char *expr, const char *message);
void lmmp_test_run(const char *name, void (*test)(void));
int lmmp_test_finish(void);

#define TEST_CASE(category, name) static void test_fn_##name(void)
#define TEST_RUN(category, name) lmmp_test_run(category "/" #name, test_fn_##name)
#define TEST_CHECK(cond) lmmp_test_check((cond), __FILE__, __LINE__, #cond, NULL)
#define TEST_CHECK_MSG(cond, msg) lmmp_test_check((cond), __FILE__, __LINE__, #cond, (msg))
#define TEST_CHECK_EQ(a, b) lmmp_test_check((a) == (b), __FILE__, __LINE__, #a " == " #b, NULL)

#endif
