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

# 调优驱动输出到 tune/lmmp/bin，便于直接运行。
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(lmmp_tune
    src/lmmp_tune.c
    src/lmmp_tune_params.c
)

target_include_directories(lmmp_tune
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(lmmp_tune PRIVATE lmmp)
if(TARGET_SYSTEM STREQUAL "LIN")
    target_link_libraries(lmmp_tune PRIVATE m)
endif()

target_compile_definitions(lmmp_tune PRIVATE
    LMMP_TUNE=1
    LMMP_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

set_target_properties(lmmp_tune PROPERTIES
    OUTPUT_NAME "lmmp_tune"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

lmmp_target_setup(lmmp_tune)

message(STATUS "lmmp_tune: tuning driver configured (output: tune/lmmp/bin)")
