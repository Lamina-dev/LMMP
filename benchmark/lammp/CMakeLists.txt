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

# 新版 LMMP 性能基准目标（C++17，无第三方依赖）。

set(LMMP_BENCH_SRCS
    main.cpp
    src/bench_basic.cpp
    src/bench_asm.cpp
)

add_executable(LammpBenchmark ${LMMP_BENCH_SRCS})

target_include_directories(LammpBenchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(LammpBenchmark PRIVATE LammpCore)

lammp_target_setup(LammpBenchmark)

set_target_properties(LammpBenchmark PROPERTIES
    OUTPUT_NAME "LammpBenchmark"
    PDB_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
