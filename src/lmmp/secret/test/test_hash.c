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

TEST_CASE("hash/sanity", siphash_xxhash) {
    mp_limb_t in[8];
    u64 seed = 0x1234567890abcdefull;
    for (size_t x_index = 0; x_index < ARRAY_SIZE(in); ++x_index) {
        in[x_index] = xorshift64(&seed);
    }

    u64 key2[2] = {0x1111111111111111ull, 0x2222222222222222ull};
    u64 key1[1] = {0x3333333333333333ull};

    u64 h1 = lmmp_siphash24_(in, 8, key2);
    u64 h2 = lmmp_siphash24_(in, 8, key2);
    TEST_CHECK_EQ(h1, h2);

    u64 h3 = lmmp_siphash24_(in, 8, NULL);
    TEST_CHECK_MSG(h3 != h1, "siphash key changes hash");

    u64 x1 = lmmp_xxhash_(in, 8, key1);
    u64 x2 = lmmp_xxhash_(in, 8, key1);
    TEST_CHECK_EQ(x1, x2);

    u64 x3 = lmmp_xxhash_(in, 8, NULL);
    TEST_CHECK_MSG(x3 != x1, "xxhash key changes hash");

    // 空输入与 NULL key
    TEST_CHECK_MSG(lmmp_siphash24_(NULL, 0, NULL) != 0, "siphash empty nonzero");
    TEST_CHECK_MSG(lmmp_xxhash_(NULL, 0, NULL) != 0, "xxhash empty nonzero");

    // 变更输入应改变哈希
    in[0] ^= 1;
    TEST_CHECK_MSG(lmmp_xxhash_(in, 8, key1) != x1, "xxhash input change");
}

void test_hash(void) {
    TEST_RUN("hash/sanity", siphash_xxhash);
}
