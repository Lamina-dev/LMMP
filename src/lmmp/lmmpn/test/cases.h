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

#ifndef LMMP_LMMPN_TEST_CASES_H
#define LMMP_LMMPN_TEST_CASES_H

#include "support/test.h"

void test_asm_boundary(void);
void test_asm_functions(void);
void test_deterministic_extra(void);
void test_div(void);
void test_low_limb(void);
void test_memory_layout(void);
void test_mul(void);
void test_mul_mersenne(void);
void test_mul_toom68(void);
void test_mul_toom93(void);
void test_mul_toom_contracts(void);
void test_str(void);
void test_threshold_boundaries(void);

#endif
