#  Copyright (C) 2026 HJimmyK (Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as
#  published by the Free Software Foundation; either version 3 of
#  the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, see
#  <https://www.gnu.org/licenses/>.
#
#  SPDX-License-Identifier: LGPL-3.0-or-later


set(LIBLMMP_VERSION "4.3.0")

# 全部模块的 C 源统一递归收集（含各模块 arch/generic/ 的 C 回退；GENERIC 模式下
# 回退实现即为正式实现）。
file(GLOB_RECURSE LIBLMMP_SRCS CONFIGURE_DEPENDS
    LIST_DIRECTORIES FALSE
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

if(LMMP_ASM_ENABLED)
    # arch/ 布局由模块自描述，构建系统按形态自动发现
    #   <module>/arch/<arch>/*.S   目标架构汇编（<arch> = x64 | arm64）
    #   <module>/arch/generic/*.c  与汇编同名符号的 C 回退，启用汇编时整体排除
    #   <module>/arch/extern.def   该模块汇编的导出符号表（Windows .def）
    # 新增模块（如 numth）按同一形态建立 arch/ 子树即被自动纳入。
    list(FILTER LIBLMMP_SRCS EXCLUDE REGEX "/arch/generic/")

    file(GLOB ASM_SRCS CONFIGURE_DEPENDS
        LIST_DIRECTORIES FALSE
        ${CMAKE_CURRENT_SOURCE_DIR}/*/arch/${LMMP_ASM_ARCH}/*.S
    )
    if(NOT ASM_SRCS)
        message(FATAL_ERROR
            "LMMP_ASM=${LMMP_ASM} is enabled but no assembly files found under "
            "${CMAKE_CURRENT_SOURCE_DIR}/*/arch/${LMMP_ASM_ARCH}/")
    endif()
    list(APPEND LIBLMMP_SRCS ${ASM_SRCS})

    if(TARGET_SYSTEM STREQUAL "WIN")
        # 各模块的 extern.def 逐个交给链接器
        file(GLOB DEF_FILES CONFIGURE_DEPENDS
            LIST_DIRECTORIES FALSE
            ${CMAKE_CURRENT_SOURCE_DIR}/*/arch/extern.def
        )
        list(APPEND LIBLMMP_SRCS ${DEF_FILES})
    endif()

    list(LENGTH ASM_SRCS ASM_SRC_COUNT)
    message(STATUS "liblmmp: Using ASM (${LMMP_ASM_ARCH} .S, GAS/LLVM, ${ASM_SRC_COUNT} files)")
else()
    message(STATUS "liblmmp: Using generic C implementation")
endif()

if(LMMP_TUNE_MODE)
    # 调优模式：生成静态核心。阈值在 LMMP_TUNE 下绑定到可写全局变量，
    # 调优驱动会先设置这些变量再运行基准测试。
    add_library(lmmp_tune_core STATIC ${LIBLMMP_SRCS})
    add_library(lmmp ALIAS lmmp_tune_core)

    if(TARGET_SYSTEM STREQUAL "LIN")
        target_link_libraries(lmmp_tune_core PRIVATE m)
    endif()

    set_target_properties(lmmp_tune_core PROPERTIES
        OUTPUT_NAME "lmmp_tune_core"
        PDB_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
    )

    target_compile_definitions(lmmp_tune_core PRIVATE
        LMMP_TUNE=1
        LMMP_VERSION_STR="${LIBLMMP_VERSION}"
        LMMP_BUILD_TYPE_STR="Release"
    )

    lmmp_target_setup(lmmp_tune_core)
    message(STATUS "liblmmp: configured successfully (static tuning core)")
else()
    add_library(liblmmp SHARED ${LIBLMMP_SRCS})
    add_library(lmmp ALIAS liblmmp)

    if(TARGET_SYSTEM STREQUAL "WIN")
        target_compile_definitions(liblmmp PRIVATE LIBLMMP_EXPORTS)
        # Windows下无lib前缀
        set_target_properties(liblmmp PROPERTIES PREFIX "")
    elseif(TARGET_SYSTEM STREQUAL "LIN")
        target_link_options(liblmmp PRIVATE -Wl,-Bsymbolic-functions)
        target_link_libraries(liblmmp PRIVATE m)
    endif()

    set_target_properties(liblmmp PROPERTIES
        OUTPUT_NAME "lmmp"
        VERSION ${LIBLMMP_VERSION}
        SOVERSION 1
        PDB_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
    )

    target_compile_definitions(liblmmp PRIVATE
        LMMP_VERSION_STR="${LIBLMMP_VERSION}"
        LMMP_BUILD_TYPE_STR="${CMAKE_BUILD_TYPE}"
    )

    lmmp_target_setup(liblmmp)

    if(LMMP_OUTPUT_DIR)
        # 统一产物目录：随库同步对外头文件（仅 include/lmmp 顶层公共头，
        # impl/ 内部头不外发）。
        # （动态库 + 导入库 + include/ 头文件）。头文件变更而库未重链时，
        # 删除产物目录或重新构建 liblmmp 可强制刷新。
        file(GLOB LMMP_PUBLIC_HEADERS CONFIGURE_DEPENDS
            LIST_DIRECTORIES FALSE
            "${CMAKE_SOURCE_DIR}/include/lmmp/*.h"
        )
        add_custom_command(TARGET liblmmp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_BIN}/include/lmmp"
            COMMAND ${CMAKE_COMMAND} -E copy
                ${LMMP_PUBLIC_HEADERS} "${OUTPUT_BIN}/include/lmmp"
            COMMENT "liblmmp: copying public headers to ${OUTPUT_BIN}/include"
        )
    endif()

    message(STATUS "liblmmp: configured successfully (shared library)")
endif()
