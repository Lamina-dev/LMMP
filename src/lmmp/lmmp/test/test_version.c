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

#include "lmmp/lmmpn.h"
#include "lmmp/mprand.h"
#include "lmmp/secret.h"
#include "lmmp/version.h"
#include "cases.h"
#include "support/lmmp_test_utils.h"

#include <string.h>

TEST_CASE("misc/version", version_strings) {
    const char* v = lmmp_get_version();
    const char* b = lmmp_get_build_type();
    TEST_CHECK_MSG(v != NULL && strlen(v) > 0, "version string nonempty");
    TEST_CHECK_MSG(b != NULL && strlen(b) > 0, "build type string nonempty");
}

void test_version(void) {
    TEST_RUN("misc/version", version_strings);
}
