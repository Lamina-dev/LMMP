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
#  License along with this library.  If not, see
#  <https://www.gnu.org/licenses/>.
#
#  SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.20)

include(cmake/LMMPOptions.cmake)

# ==============================================================================
# 工具链选择与限制（编译器选择须在 project() 之前裁决）
# ==============================================================================
# 生成器检查：拒绝 Visual Studio 系生成器
if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    message(FATAL_ERROR
        "The '${CMAKE_GENERATOR}' generator always builds with the MSVC toolchain and "
        "ignores CMAKE_C_COMPILER / LMMP_COMPILER_DIR, but MSVC is not supported. "
        "Configure with a GNU-compatible generator instead: -G Ninja (recommended) or "
        "-G \"MinGW Makefiles\". Note the generator is cached per build directory: "
        "reconfigure into a fresh directory after switching.")
endif()

# 编译器在首次配置时固化进构建树（CMake 缓存），同树切换会静默沿用旧编译器，
# 故此前配置过目录而本次清空时同样显式拒绝。
if(NOT LMMP_COMPILER_DIR AND LMMP_RESOLVED_C_COMPILER)
    message(FATAL_ERROR
        "LMMP_COMPILER_DIR was cleared in an existing build directory previously "
        "configured with '${LMMP_RESOLVED_C_COMPILER}'. CMake cannot switch compilers "
        "in place; configure into a fresh build directory.")
endif()

if(LMMP_COMPILER_DIR)
    if(NOT IS_DIRECTORY "${LMMP_COMPILER_DIR}")
        message(FATAL_ERROR "LMMP_COMPILER_DIR is not a directory: ${LMMP_COMPILER_DIR}")
    endif()

    # 只在该目录内探测（不回落 PATH），探测缓存每次配置前清空。
    # 切换编译器必须换用全新构建目录：下方守卫对同树换目录显式报错。
    foreach(_pair "gcc;g++" "clang;clang++")
        list(GET _pair 0 _cc_name)
        list(GET _pair 1 _cxx_name)
        unset(LMMP_PROG_${_cc_name} CACHE)
        find_program(LMMP_PROG_${_cc_name} NAMES ${_cc_name} PATHS "${LMMP_COMPILER_DIR}" NO_DEFAULT_PATH)
        mark_as_advanced(LMMP_PROG_${_cc_name})
        unset(LMMP_PROG_${_cc_name}_CXX CACHE)
        find_program(LMMP_PROG_${_cc_name}_CXX NAMES ${_cxx_name} PATHS "${LMMP_COMPILER_DIR}" NO_DEFAULT_PATH)
        mark_as_advanced(LMMP_PROG_${_cc_name}_CXX)
    endforeach()

    if(LMMP_PROG_gcc AND LMMP_PROG_gcc_CXX)
        set(_lmmp_cc "${LMMP_PROG_gcc}")
        set(_lmmp_cxx "${LMMP_PROG_gcc_CXX}")
    elseif(LMMP_PROG_clang AND LMMP_PROG_clang_CXX)
        set(_lmmp_cc "${LMMP_PROG_clang}")
        set(_lmmp_cxx "${LMMP_PROG_clang_CXX}")
    elseif(LMMP_PROG_gcc OR LMMP_PROG_clang)
        message(FATAL_ERROR
            "LMMP_COMPILER_DIR has a C compiler but no matching C++ compiler (g++ / clang++): ${LMMP_COMPILER_DIR}")
    else()
        message(FATAL_ERROR
            "No GNU/Clang compiler pair (gcc+g++ or clang+clang++) found in LMMP_COMPILER_DIR: ${LMMP_COMPILER_DIR}")
    endif()

    if(LMMP_RESOLVED_C_COMPILER AND NOT LMMP_RESOLVED_C_COMPILER STREQUAL "${_lmmp_cc}")
        message(FATAL_ERROR
            "LMMP_COMPILER_DIR switched from '${LMMP_RESOLVED_C_COMPILER}' to '${_lmmp_cc}' "
            "in an existing build directory. CMake cannot switch compilers in place "
            "(it would silently keep the old one); configure into a fresh build directory.")
    endif()

    set(CMAKE_C_COMPILER "${_lmmp_cc}")
    set(CMAKE_CXX_COMPILER "${_lmmp_cxx}")
    set(LMMP_RESOLVED_C_COMPILER "${_lmmp_cc}" CACHE STRING
        "Compiler path resolved from LMMP_COMPILER_DIR (internal guard)" FORCE)
    mark_as_advanced(LMMP_RESOLVED_C_COMPILER)
    unset(_lmmp_cc)
    unset(_lmmp_cxx)
    # .S 汇编经 C 预处理器处理，与 C 使用同一驱动。
    set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
    message(STATUS "Compiler (LMMP_COMPILER_DIR): ${CMAKE_C_COMPILER}")
endif()

project(LMMP LANGUAGES C CXX)

# 工具链限制（第 0 节续）：仅支持 GNU/Clang 风格工具链，project() 识别完成后
# 拒绝 MSVC 系编译器（原生 MSVC、clang-cl）。
if(MSVC OR CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    message(FATAL_ERROR
        "MSVC toolchain is not supported (C compiler: ${CMAKE_C_COMPILER}, "
        "id '${CMAKE_C_COMPILER_ID}'). Use a GNU/Clang toolchain: on Windows "
        "MinGW GCC or GNU-driver clang, e.g. -G Ninja "
        "-DLMMP_COMPILER_DIR=<mingw64/bin>.")
endif()

# ==============================================================================
# 目标架构与构建类型
# ==============================================================================
set(LMMP_X86_64 FALSE)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|x64|X64)$")
    set(LMMP_X86_64 TRUE)
endif()

set(LMMP_ARM64 FALSE)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64|AARCH64)$")
    set(LMMP_ARM64 TRUE)
endif()

# 构建类型检查
set(VALID_BUILD_TYPES Debug Release RelWithDebInfo MinSizeRel)
if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST VALID_BUILD_TYPES)
    message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
endif()

# ==============================================================================
# 目标系统
# ==============================================================================
if(TARGET_SYSTEM STREQUAL "AUTO")
    if(WIN32)
        set(TARGET_SYSTEM "WIN")
    elseif(APPLE)
        set(TARGET_SYSTEM "MAC")
    else()
        set(TARGET_SYSTEM "LIN")
    endif()
endif()

string(TOUPPER ${TARGET_SYSTEM} TARGET_SYSTEM_UPPER)
message(STATUS "Target system: ${TARGET_SYSTEM_UPPER}")

# ==============================================================================
# 汇编支持
# ==============================================================================
set(LMMP_ASM_ENABLED FALSE)
set(LMMP_ASM_ARCH "")

if(LMMP_ASM STREQUAL "AUTO")
    if(LMMP_X86_64 AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT TARGET_SYSTEM STREQUAL "MAC")
        set(LMMP_ASM "X64")
    elseif(LMMP_ARM64 AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND (TARGET_SYSTEM STREQUAL "MAC" OR TARGET_SYSTEM STREQUAL "LIN"))
        set(LMMP_ASM "ARM64")
    else()
        set(LMMP_ASM "GENERIC")
    endif()
endif()

if(LMMP_ASM STREQUAL "X64")
    if(NOT LMMP_X86_64 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "LMMP_ASM=X64 requires an x86_64 64-bit toolchain.")
    endif()
    if(TARGET_SYSTEM STREQUAL "MAC")
        message(FATAL_ERROR "LMMP_ASM=X64 is not supported on macOS; use LMMP_ASM=GENERIC.")
    endif()
    set(LMMP_ASM_ENABLED TRUE)
    set(LMMP_ASM_ARCH "x64")
    enable_language(ASM)
    message(STATUS "ASM support: ENABLED (x64 .S, GAS/LLVM)")
elseif(LMMP_ASM STREQUAL "ARM64")
    if(NOT LMMP_ARM64 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "LMMP_ASM=ARM64 requires an arm64 64-bit toolchain.")
    endif()
    if(NOT (TARGET_SYSTEM STREQUAL "MAC" OR TARGET_SYSTEM STREQUAL "LIN"))
        message(FATAL_ERROR "LMMP_ASM=ARM64 is only supported on macOS and Linux.")
    endif()
    set(LMMP_ASM_ENABLED TRUE)
    set(LMMP_ASM_ARCH "arm64")
    enable_language(ASM)
    message(STATUS "ASM support: ENABLED (arm64 .S, GAS/LLVM)")
elseif(LMMP_ASM STREQUAL "GENERIC")
    message(STATUS "ASM support: DISABLED (generic C implementation)")
else()
    message(FATAL_ERROR "Invalid LMMP_ASM value: ${LMMP_ASM}. Use GENERIC, X64, ARM64, or AUTO.")
endif()

# ==============================================================================
# 链接器选择（LMMP_LINKER_DIR）
# ==============================================================================
# 在指定目录内按 ld.lld | lld → ld.mold | mold → ld 顺序探测链接器，按文件名
# 归类风味，经 GNU/Clang 驱动的 -B 前缀目录与 -fuse-ld 生效（对 Apple ld64
# 之外的链接器，macOS 下仅 lld 可用；mold 需较新驱动支持）。
set(LMMP_LINKER_FLAGS "")
if(LMMP_LINKER_DIR)
    if(NOT IS_DIRECTORY "${LMMP_LINKER_DIR}")
        message(FATAL_ERROR "LMMP_LINKER_DIR is not a directory: ${LMMP_LINKER_DIR}")
    endif()

    set(_lmmp_ld_flavor "")
    foreach(_name ld.lld lld ld.mold mold ld)
        unset(_lmmp_ld_prog CACHE)
        find_program(_lmmp_ld_prog NAMES ${_name} PATHS "${LMMP_LINKER_DIR}" NO_DEFAULT_PATH)
        if(_lmmp_ld_prog)
            if(_name MATCHES "lld")
                set(_lmmp_ld_flavor "lld")
            elseif(_name MATCHES "mold")
                set(_lmmp_ld_flavor "mold")
            else()
                set(_lmmp_ld_flavor "ld")
            endif()
            break()
        endif()
    endforeach()
    if(NOT _lmmp_ld_flavor)
        message(FATAL_ERROR
            "No linker (ld / ld.lld / ld.mold) found in LMMP_LINKER_DIR: ${LMMP_LINKER_DIR}")
    endif()

    # GNU/Clang 驱动的子程序搜索前缀，须以 / 结尾
    file(TO_CMAKE_PATH "${LMMP_LINKER_DIR}" _lmmp_ld_dir)
    if(NOT _lmmp_ld_dir MATCHES "/$")
        string(APPEND _lmmp_ld_dir "/")
    endif()
    set(LMMP_LINKER_FLAGS "-B${_lmmp_ld_dir}")
    if(_lmmp_ld_flavor STREQUAL "lld")
        list(APPEND LMMP_LINKER_FLAGS "-fuse-ld=lld")
    elseif(_lmmp_ld_flavor STREQUAL "mold")
        list(APPEND LMMP_LINKER_FLAGS "-fuse-ld=mold")
    endif()
    message(STATUS "Linker (LMMP_LINKER_DIR): ${_lmmp_ld_flavor} in ${LMMP_LINKER_DIR}")
    unset(_lmmp_ld_flavor)
    unset(_lmmp_ld_dir)
endif()

# ==============================================================================
# 默认构建类型
# ==============================================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# ==============================================================================
# 通用编译选项函数
# ==============================================================================
function(lmmp_target_setup target)

    set_target_properties(${target} PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_compile_options(${target} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:
            -Wall -Wextra -Wpedantic
            -Wshadow
            -Wformat-security
            -Wnull-dereference
            -Wundef
            -Wmissing-prototypes
            -Wimplicit-fallthrough
            $<$<NOT:$<PLATFORM_ID:Windows>>:-fPIC>
            $<$<CONFIG:Debug>:-O0 -g -DDEBUG>
            $<$<CONFIG:Release>:-O3 -g0 -fno-math-errno -DNDEBUG>
            $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        >
        $<$<COMPILE_LANGUAGE:CXX>:
            -Wall -Wextra -Wpedantic
            $<$<NOT:$<PLATFORM_ID:Windows>>:-fPIC>
            $<$<CONFIG:Debug>:-O0 -g -DDEBUG>
            $<$<CONFIG:Release>:-O3 -g0 -fno-math-errno -DNDEBUG>
            $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        >
    )
    if(LMMP_ASM_ENABLED)
        # 自定义汇编宏：C/C++/ASM 统一使用，避免依赖 __APPLE__/_WIN32 等编译器宏。
        target_compile_definitions(${target} PRIVATE LMMP_ASM=1)
        if(LMMP_ASM_ARCH STREQUAL "x64")
            target_compile_definitions(${target} PRIVATE LMMP_ASM_X64=1)
            # x86-64-v3 基线 + adx 指令集
            target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:C>:-march=x86-64-v3;-madx>
                $<$<COMPILE_LANGUAGE:CXX>:-march=x86-64-v3;-madx>
                $<$<COMPILE_LANGUAGE:ASM>:-march=x86-64-v3;-madx>
            )
        elseif(LMMP_ASM_ARCH STREQUAL "arm64")
            target_compile_definitions(${target} PRIVATE LMMP_ASM_ARM64=1)
            # armv8.2-a 基线
            target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:C>:-march=armv8.2-a>
                $<$<COMPILE_LANGUAGE:CXX>:-march=armv8.2-a>
                $<$<COMPILE_LANGUAGE:ASM>:-march=armv8.2-a>
            )
        endif()
    endif()

    # 调试宏统一由 CMake 注入，保证头文件中的 #if 按配置展开。
    target_compile_definitions(${target} PRIVATE
        LMMP_DEBUG_STACK_OVERFLOW_CHECK=$<IF:$<BOOL:${LMMP_DEBUG_STACK_OVERFLOW_CHECK}>,1,0>
        LMMP_DEBUG_ASSERT_CHECK=$<IF:$<BOOL:${LMMP_DEBUG_ASSERT_CHECK}>,1,0>
        LMMP_DEBUG_PARAM_ASSERT_CHECK=$<IF:$<BOOL:${LMMP_DEBUG_PARAM_ASSERT_CHECK}>,1,0>
        LMMP_DEBUG_MEMORY_CHECK=$<IF:$<BOOL:${LMMP_DEBUG_MEMORY_CHECK}>,1,0>
        LMMP_DEBUG_MEMORY_LEAK=$<IF:$<BOOL:${LMMP_DEBUG_MEMORY_LEAK}>,1,0>
        LMMP_MEMORY_MORE_ALLOC_TIMES=${LMMP_MEMORY_MORE_ALLOC_TIMES}
    )

    # 链接器目录选择：经驱动的 -B 前缀与 -fuse-ld 生效（见 3.5 节）
    if(LMMP_LINKER_FLAGS)
        target_link_options(${target} PRIVATE ${LMMP_LINKER_FLAGS})
    endif()

    if(TARGET_SYSTEM STREQUAL "WIN")
        target_compile_definitions(${target} PRIVATE LMMP_WINDOWS)
    elseif(TARGET_SYSTEM STREQUAL "LIN")
        target_compile_definitions(${target} PRIVATE LMMP_LINUX)
        # 库内对导出函数的调用假定无语义插入（允许内联与直接调用），
        # 配合链接期 -Bsymbolic-functions（见下方链接选项段）取代 -fno-plt
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:C>: -fno-semantic-interposition>
            $<$<COMPILE_LANGUAGE:CXX>: -fno-semantic-interposition>
        )
    elseif(TARGET_SYSTEM STREQUAL "MAC")
        target_compile_definitions(${target} PRIVATE LMMP_MACOS)
    endif()

    if(TARGET_SYSTEM STREQUAL "MAC")
        target_link_options(${target} PRIVATE
            $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:
                -Wl,-dead_strip
            >
        )
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
            if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_STRIP} -x "$<TARGET_FILE:${target}>"
                    COMMENT "Stripping local symbols from ${target}"
                )
            endif()
        endif()
    elseif(  TARGET_SYSTEM STREQUAL "LIN"
          OR MINGW
          OR (TARGET_SYSTEM STREQUAL "WIN" AND
              CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
              NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))

        # Linux
        # MinGW 环境
        # Windows 上的 GNU 驱动 Clang（非 MSVC 兼容模式的 Clang）
        target_link_options(${target} PRIVATE
            -Wl,--no-undefined
            $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:
                -Wl,--strip-all
            >
        )
        # Linux 共享库：函数符号链接期本地绑定，库内互调免 PLT/GOT 间接，
        # 与编译期 -fno-semantic-interposition 配套（对符号插入式
        # interposition 的显式放弃，与库的 -fvisibility=hidden 策略一致）
        if(TARGET_SYSTEM STREQUAL "LIN")
            get_target_property(target_type ${target} TYPE)
            if(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
                target_link_options(${target} PRIVATE -Wl,-Bsymbolic-functions)
            endif()
        endif()
    endif()

endfunction()

# ==============================================================================
# 输出目录
# ==============================================================================
# 输出布局：
#   LMMP_OUTPUT_DIR -> 传入目录平铺（动态库/导入库/头文件/附加构建产物同层，
#                     不区分配置子目录，供其他项目直接引入）
#   默认            -> dist/lmmp/bin/<config>（源内开发布局）
set(OUTPUT_SPLIT_CONFIG FALSE)
if(LMMP_OUTPUT_DIR)
    get_filename_component(OUTPUT_BIN "${LMMP_OUTPUT_DIR}" ABSOLUTE)
else()
    set(OUTPUT_BIN ${CMAKE_SOURCE_DIR}/dist/lmmp/bin)
    set(OUTPUT_SPLIT_CONFIG TRUE)
endif()

foreach(TYPE RUNTIME ARCHIVE LIBRARY)
    set(CMAKE_${TYPE}_OUTPUT_DIRECTORY ${OUTPUT_BIN})
    if(OUTPUT_SPLIT_CONFIG)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_BIN}/debug)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_BIN}/release)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_BIN}/relwithdebinfo)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_BIN}/minsizerel)
    else()
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_BIN})
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_BIN})
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_BIN})
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_BIN})
    endif()
endforeach()

# ==============================================================================
# 构建子模块
# ==============================================================================
if(LMMP_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(src/lmmp)

if(LMMP_BUILD_EXAMPLES)
    add_subdirectory(example/lmmp)
endif()

# ==============================================================================
# 最终信息输出
# ==============================================================================
message(STATUS "=====================================")
message(STATUS "Lmmp configure summary")
message(STATUS "=====================================")
message(STATUS "Compiler:      ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} (${CMAKE_C_COMPILER})")
if(LMMP_LINKER_FLAGS)
    message(STATUS "Linker:        ${LMMP_LINKER_FLAGS}")
elseif(CMAKE_C_COMPILER_LINKER_ID)
    message(STATUS "Linker:        ${CMAKE_C_COMPILER_LINKER_ID} (toolchain default)")
else()
    message(STATUS "Linker:        toolchain default")
endif()
message(STATUS "Target system: ${TARGET_SYSTEM}")
if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Build types:   ${CMAKE_CONFIGURATION_TYPES}")
else()
    message(STATUS "Build type:    ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "ASM support:   ${LMMP_ASM}")
message(STATUS "Output dir:    ${OUTPUT_BIN}")
message(STATUS "Extras:        tests=${LMMP_BUILD_TESTS} examples=${LMMP_BUILD_EXAMPLES} (perf/tune by target)")
message(STATUS "=====================================")
