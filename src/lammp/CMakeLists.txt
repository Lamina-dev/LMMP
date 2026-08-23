#
#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#   by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY. 
#
#  See <https://www.gnu.org/licenses/>.


set(LMMP_CORE_VERSION "4.2.0")


file(GLOB_RECURSE LMMP_CORE_SRCS
    LIST_DIRECTORIES FALSE
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

if(LMMP_ASM_ENABLED)
    # 启用汇编时，Windows 下使用 .def 文件导出汇编实现的函数。
    set(DEF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/lmmpn/asm/lmmp_asm.def)
    if(TARGET_SYSTEM STREQUAL "WIN")
        list(APPEND LMMP_CORE_SRCS ${DEF_FILE})
    endif()

    # 过滤掉 generic 目录下的源文件，避免与汇编实现重复定义。
    list(FILTER LMMP_CORE_SRCS EXCLUDE REGEX ".*/generic/.*")

    # 收集目标架构的 GAS/LLVM 汇编文件。
    file(GLOB ASM_SRCS
        LIST_DIRECTORIES FALSE
        ${LMMP_ASM_DIR}/*.S
    )

    if(NOT ASM_SRCS)
        message(FATAL_ERROR "LMMP_ASM=${LMMP_ASM} is enabled but no assembly files found in ${LMMP_ASM_DIR}!")
    endif()

    list(APPEND LMMP_CORE_SRCS ${ASM_SRCS})
    message(STATUS "LammpCore: Using ASM (${LMMP_ASM_ARCH} .S, GAS/LLVM)")
else()
    # 不启用汇编则使用纯 C 通用实现
    message(STATUS "LammpCore: Using generic C implementation")
endif()

add_library(LammpCore SHARED ${LMMP_CORE_SRCS})
add_library(Lammp::Core ALIAS LammpCore)

if(TARGET LammpCore AND UNIX AND NOT APPLE)
    target_link_options(LammpCore PRIVATE -Wl,-Bsymbolic-functions)
endif()

if(TARGET_SYSTEM STREQUAL "WIN")
    target_compile_definitions(LammpCore PRIVATE LMMP_CORE_EXPORTS)
elseif(TARGET_SYSTEM STREQUAL "LIN")
    target_link_libraries(LammpCore PRIVATE m)
endif()

set_target_properties(LammpCore PROPERTIES
    OUTPUT_NAME "LammpCore"
    VERSION ${LMMP_CORE_VERSION}
    SOVERSION 1
    PDB_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
)

target_compile_definitions(LammpCore PRIVATE
    LMMP_VERSION_STR="${LMMP_CORE_VERSION}"
    LMMP_BUILD_TYPE_STR="${CMAKE_BUILD_TYPE}"
)

lammp_target_setup(LammpCore)

message(STATUS "LammpCore: configured successfully (shared library)")
