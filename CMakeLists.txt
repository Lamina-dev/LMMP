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

cmake_minimum_required(VERSION 3.20)

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

# 编译器目录：传入包含 GNU/Clang 编译器对（gcc+g++ 或 clang+clang++）的文件夹
set(LMMP_COMPILER_DIR "" CACHE PATH
    "Directory containing a GNU/Clang compiler pair (gcc+g++ or clang+clang++). Empty = default compiler.")

# 链接器目录：传入包含链接器的文件夹，按文件名识别风味
set(LMMP_LINKER_DIR "" CACHE PATH
    "Directory containing the linker (ld.lld | ld.mold | ld, flavor by name). Empty = toolchain default linker.")

if(LMMP_COMPILER_DIR)
    if(NOT IS_DIRECTORY "${LMMP_COMPILER_DIR}")
        message(FATAL_ERROR "LMMP_COMPILER_DIR is not a directory: ${LMMP_COMPILER_DIR}")
    endif()

    # 只在该目录内探测；每次配置前清缓存，
    # 保证改动 LMMP_COMPILER_DIR 后重新探测而非沿用旧结果。
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
        set(CMAKE_C_COMPILER "${LMMP_PROG_gcc}")
        set(CMAKE_CXX_COMPILER "${LMMP_PROG_gcc_CXX}")
    elseif(LMMP_PROG_clang AND LMMP_PROG_clang_CXX)
        set(CMAKE_C_COMPILER "${LMMP_PROG_clang}")
        set(CMAKE_CXX_COMPILER "${LMMP_PROG_clang_CXX}")
    elseif(LMMP_PROG_gcc OR LMMP_PROG_clang)
        message(FATAL_ERROR
            "LMMP_COMPILER_DIR has a C compiler but no matching C++ compiler (g++ / clang++): ${LMMP_COMPILER_DIR}")
    else()
        message(FATAL_ERROR
            "No GNU/Clang compiler pair (gcc+g++ or clang+clang++) found in LMMP_COMPILER_DIR: ${LMMP_COMPILER_DIR}")
    endif()
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
# 可配置选项
# ==============================================================================
set(LMMP_X86_64 FALSE)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|x64|X64)$")
    set(LMMP_X86_64 TRUE)
endif()

set(LMMP_ARM64 FALSE)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64|AARCH64)$")
    set(LMMP_ARM64 TRUE)
endif()

# 汇编模式：GENERIC | X64 | ARM64 | AUTO
#   GENERIC  不使用汇编，使用通用 C 实现。
#   X64      使用 x86_64 GAS/LLVM 汇编（Windows/Linux）。
#   ARM64    使用 arm64 GAS/LLVM 汇编（Linux/macOS，ARMv8.2-A 基线）。
#   AUTO     按当前平台自动选择最合适的模式；不支持时回退 GENERIC。
set(LMMP_ASM "AUTO" CACHE STRING "Assembly mode: GENERIC | X64 | ARM64 | AUTO")
set_property(CACHE LMMP_ASM PROPERTY STRINGS GENERIC X64 ARM64 AUTO)
# 取值大小写不敏感（统一归一大写，用户可传小写）
string(TOUPPER "${LMMP_ASM}" _lmmp_asm_norm)
if(NOT _lmmp_asm_norm STREQUAL "${LMMP_ASM}")
    set(LMMP_ASM "${_lmmp_asm_norm}" CACHE STRING "Assembly mode: GENERIC | X64 | ARM64 | AUTO" FORCE)
endif()
unset(_lmmp_asm_norm)

# 构建目标（默认只构建动态库；test/benchmark/example 为附加构建，开启后
# 与动态库输出到同一目录，便于以统一产物目录整体引入）
option(LMMP_BUILD_TESTS "Build LMMP unit tests" OFF)
option(LMMP_BUILD_BENCHMARKS "Build LMMP benchmarks" OFF)
option(LMMP_BUILD_EXAMPLES "Build LMMP examples" OFF)

# 统一产物目录：传入后所有构建产物（动态库、导入库与可选的
# test/benchmark/example 可执行文件）平铺到该目录，不区分配置子目录，
# 并随库同步对外头文件（include/lmmp），目录即完整可引入形态；
# 为空时维持源内 dist 布局。建议使用绝对路径。
set(LMMP_OUTPUT_DIR "" CACHE PATH
    "Flat output directory for all artifacts (shared lib, import lib, headers, optional tests/benchmarks). Empty = in-tree dist/lmmp/bin/<config>.")

# 调优模式：构建静态可调阈值核心 + 调优驱动；默认关闭且独立于正常构建。
option(LMMP_TUNE_MODE "Build tuning core and tune driver" OFF)

# 调试选项：由构建系统注入 LMMP_DEBUG_* 宏，不在头文件中直接定义。
option(LMMP_DEBUG_STACK_OVERFLOW_CHECK "Enable stack overflow check (high overhead)" OFF)
option(LMMP_DEBUG_ASSERT_CHECK "Enable debug_assert check (medium overhead)" OFF)
option(LMMP_DEBUG_PARAM_ASSERT_CHECK "Enable parameter assertion check (medium overhead)" OFF)
option(LMMP_DEBUG_MEMORY_CHECK "Enable full heap/stack memory check (very high overhead)" OFF)
set(LMMP_MEMORY_MORE_ALLOC_TIMES 1 CACHE STRING "Extra memory allocation factor in tenths (used by memory check)")
option(LMMP_DEBUG_MEMORY_LEAK "Enable allocation/free statistics (low overhead)" OFF)

# 目标系统：自动识别，也可手动指定 WIN | LIN | MAC（取值大小写不敏感）
set(TARGET_SYSTEM "AUTO" CACHE STRING "Target system: AUTO | WIN | LIN | MAC")
set_property(CACHE TARGET_SYSTEM PROPERTY STRINGS AUTO WIN LIN MAC)
string(TOUPPER "${TARGET_SYSTEM}" _lmmp_ts_norm)
if(NOT _lmmp_ts_norm STREQUAL "${TARGET_SYSTEM}")
    set(TARGET_SYSTEM "${_lmmp_ts_norm}" CACHE STRING "Target system: AUTO | WIN | LIN | MAC" FORCE)
endif()
unset(_lmmp_ts_norm)

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

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

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
            -fvisibility=hidden
            $<$<NOT:$<PLATFORM_ID:Windows>>:-fPIC>
            $<$<CONFIG:Debug>:-O0 -g -DDEBUG>
            $<$<CONFIG:Release>:-O3 -g0 -fno-math-errno -DNDEBUG>
            $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        >
        $<$<COMPILE_LANGUAGE:CXX>:
            -Wall -Wextra -Wpedantic
            -fvisibility=hidden
            $<$<NOT:$<PLATFORM_ID:Windows>>:-fPIC>
            $<$<CONFIG:Debug>:-O0 -g -DDEBUG>
            $<$<CONFIG:Release>:-O3 -g0 -fno-math-errno -DNDEBUG>
            $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        >
    )
    message(STATUS "Compile options: GNU/Clang standard options set.")

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
        message(STATUS "Linker options: Apple ld standard options set.")
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
        message(STATUS "Linker options: GNU ld style (Linux/MinGW/Windows-GNU-Clang).")

        # Linux 共享库：函数符号链接期本地绑定，库内互调免 PLT/GOT 间接，
        # 与编译期 -fno-semantic-interposition 配套（对符号插入式
        # interposition 的显式放弃，与库的 -fvisibility=hidden 策略一致）
        if(TARGET_SYSTEM STREQUAL "LIN")
            get_target_property(target_type ${target} TYPE)
            if(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
                target_link_options(${target} PRIVATE -Wl,-Bsymbolic-functions)
            endif()
        endif()
    else()
        # Windows 上 GNU 驱动的 clang 等工具链，不附加平台特有链接选项。
        message(STATUS "Linker options: default for this toolchain.")
    endif()

endfunction()

# ==============================================================================
# 输出目录
# ==============================================================================
# 三种形态：
#   LMMP_TUNE_MODE  -> tune/lmmp/bin（调优产物独立，不区分配置子目录）
#   LMMP_OUTPUT_DIR -> 传入目录平铺（动态库/导入库/头文件/附加构建产物同层，
#                     不区分配置子目录，供其他项目直接引入）
#   默认            -> dist/lmmp/bin/<config>（源内开发布局）
set(OUTPUT_SPLIT_CONFIG FALSE)
if(LMMP_TUNE_MODE)
    set(OUTPUT_BIN ${CMAKE_SOURCE_DIR}/tune/lmmp/bin)
elseif(LMMP_OUTPUT_DIR)
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
add_subdirectory(src/lmmp)

if(NOT LMMP_TUNE_MODE)
    if(LMMP_BUILD_TESTS)
        enable_testing()
        add_subdirectory(test/lmmp)
    endif()

    if(LMMP_BUILD_EXAMPLES)
        add_subdirectory(example/lmmp)
    endif()

    if(LMMP_BUILD_BENCHMARKS)
        add_subdirectory(benchmark/lmmp)
    endif()
else()
    add_subdirectory(tune/lmmp)
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
message(STATUS "Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "ASM support:   ${LMMP_ASM}")
message(STATUS "Output dir:    ${OUTPUT_BIN}")
if(NOT LMMP_TUNE_MODE)
    message(STATUS "Extras:        tests=${LMMP_BUILD_TESTS} benchmarks=${LMMP_BUILD_BENCHMARKS} examples=${LMMP_BUILD_EXAMPLES}")
endif()
message(STATUS "=====================================")
