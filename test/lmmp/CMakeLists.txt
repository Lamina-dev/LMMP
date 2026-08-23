#
#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#  by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY.
#
#  See <https://www.gnu.org/licenses/>.
#

set(LMMP_TEST_SRCS
    main.cpp
    src/test_low_limb.cpp
    src/test_asm_boundary.cpp
    src/test_asm_functions.cpp
    src/test_mul.cpp
    src/test_div.cpp
    src/test_str.cpp
    src/test_numth_gcd.cpp
    src/test_numth_pow.cpp
    src/test_numth_divexact.cpp
    src/test_numth_binvert.cpp
    src/test_numth_sqrt_cbrt.cpp
    src/test_numth_combin.cpp
    src/test_numth_prime.cpp
    src/test_rng_hash.cpp
)

add_executable(LmmpTest ${LMMP_TEST_SRCS})

target_include_directories(LmmpTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(LmmpTest PRIVATE lmmp)

lmmp_target_setup(LmmpTest)

set_target_properties(LmmpTest PROPERTIES
    OUTPUT_NAME "LmmpTest"
    PDB_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

add_test(NAME LmmpTest COMMAND LmmpTest)
