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

#include "cases.h"
#include "lmmp/lmmp.h"

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    lmmp_global_init();
    test_combin_boundaries();
    test_binom();
    test_binvert();
    test_cbrt();
    test_combin();
    test_divexact();
    test_fib();
    test_gcd();
    test_gcd_lehmer();
    test_gcdext();
    test_modinv();
    test_nthroot();
    test_pow();
    test_powmod();
    test_prime2();
    test_prime_n();
    test_prime_uint();
    test_proth();
    test_sqrt();
    lmmp_global_deinit();
    return lmmp_test_finish();
}
