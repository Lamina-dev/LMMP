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

cmake_minimum_required(VERSION 3.20)
project(LMMP LANGUAGES C CXX)

# ==============================================================================
# 0. 工具链限制：仅支持 GNU/Clang 风格工具链
# ==============================================================================
if(MSVC OR CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # 原生 MSVC 与 clang-cl 均已停止支持。
    # Windows 下请使用 MinGW GCC 或 GNU 驱动的 clang。
    message(FATAL_ERROR "MSVC/clang-cl toolchain is not supported. Use MinGW GCC or GNU-driver clang on Windows.")
endif()

# ==============================================================================
# 1. 可配置选项
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

# 测试与基准目标默认开启，可独立关闭。
option(LMMP_BUILD_TESTS "Build LMMP unit tests" ON)
option(LMMP_BUILD_BENCHMARKS "Build LMMP benchmarks" ON)

# 调优模式：构建静态可调阈值核心 + 调优驱动；默认关闭且独立于正常构建。
option(LMMP_TUNE_MODE "Build tuning core and tune driver" OFF)

# 调试选项：由构建系统注入 LMMP_DEBUG_* 宏，不在头文件中直接定义。
option(LMMP_DEBUG_STACK_OVERFLOW_CHECK "Enable stack overflow check (high overhead)" OFF)
option(LMMP_DEBUG_ASSERT_CHECK "Enable debug_assert check (medium overhead)" OFF)
option(LMMP_DEBUG_PARAM_ASSERT_CHECK "Enable parameter assertion check (medium overhead)" OFF)
option(LMMP_DEBUG_MEMORY_CHECK "Enable full heap/stack memory check (very high overhead)" OFF)
set(LMMP_MEMORY_MORE_ALLOC_TIMES 1 CACHE STRING "Extra memory allocation factor in tenths (used by memory check)")
option(LMMP_DEBUG_MEMORY_LEAK "Enable allocation/free statistics (low overhead)" OFF)

# 目标系统：自动识别，也可手动指定 WIN | LIN | MAC
set(TARGET_SYSTEM "AUTO" CACHE STRING "Target system: AUTO | WIN | LIN | MAC")
set_property(CACHE TARGET_SYSTEM PROPERTY STRINGS AUTO WIN LIN MAC)

# 构建类型检查
set(VALID_BUILD_TYPES Debug Release RelWithDebInfo MinSizeRel)
if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST VALID_BUILD_TYPES)
    message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
endif()

# ==============================================================================
# 2. 目标系统
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
# 3. 汇编支持
# ==============================================================================
set(LMMP_ASM_ENABLED FALSE)
set(LMMP_ASM_ARCH "")
set(LMMP_ASM_DIR "")

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
    set(LMMP_ASM_DIR "${CMAKE_SOURCE_DIR}/src/lmmp/lmmpn/asm/x64")
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
    set(LMMP_ASM_DIR "${CMAKE_SOURCE_DIR}/src/lmmp/lmmpn/asm/arm64")
    enable_language(ASM)
    message(STATUS "ASM support: ENABLED (arm64 .S, GAS/LLVM)")
elseif(LMMP_ASM STREQUAL "GENERIC")
    message(STATUS "ASM support: DISABLED (generic C implementation)")
else()
    message(FATAL_ERROR "Invalid LMMP_ASM value: ${LMMP_ASM}. Use GENERIC, X64, ARM64, or AUTO.")
endif()

# ==============================================================================
# 4. 默认构建类型
# ==============================================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ==============================================================================
# 5. 通用编译选项函数
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

    if(TARGET_SYSTEM STREQUAL "WIN")
        target_compile_definitions(${target} PRIVATE LMMP_WINDOWS)
    elseif(TARGET_SYSTEM STREQUAL "LIN")
        target_compile_definitions(${target} PRIVATE LMMP_LINUX)
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:C>: -fno-plt>
            $<$<COMPILE_LANGUAGE:CXX>: -fno-plt>
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
    else()
        # Windows 上 GNU 驱动的 clang 等工具链，不附加平台特有链接选项。
        message(STATUS "Linker options: default for this toolchain.")
    endif()

endfunction()

# ==============================================================================
# 6. 输出目录
# ==============================================================================
if(LMMP_TUNE_MODE)
    # 调优产物全部输出到 tune/lmmp/bin，与 dist 分离。
    set(OUTPUT_ROOT ${CMAKE_SOURCE_DIR}/tune/lmmp)
else()
    set(OUTPUT_ROOT ${CMAKE_SOURCE_DIR}/dist/lmmp)
endif()

foreach(TYPE RUNTIME ARCHIVE LIBRARY)
    set(CMAKE_${TYPE}_OUTPUT_DIRECTORY ${OUTPUT_ROOT}/bin)
    if(LMMP_TUNE_MODE)
        # 调优模式不区分配置子目录，核心库与调优驱动放在一起。
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_ROOT}/bin)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_ROOT}/bin)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_ROOT}/bin)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_ROOT}/bin)
    else()
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_ROOT}/bin/debug)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_ROOT}/bin/release)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_ROOT}/bin/relwithdebinfo)
        set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_ROOT}/bin/minsizerel)
    endif()
endforeach()

# ==============================================================================
# 7. 构建子模块
# ==============================================================================
add_subdirectory(src/lmmp)

if(NOT LMMP_TUNE_MODE)
    if(EXISTS ${CMAKE_SOURCE_DIR}/main.c)
        add_executable(LmmpMain main.c)
        target_link_libraries(LmmpMain PRIVATE liblmmp)
        lmmp_target_setup(LmmpMain)
    endif()

    if(LMMP_BUILD_TESTS)
        enable_testing()
        add_subdirectory(test/lmmp)
    endif()

    add_subdirectory(example/lmmp)

    if(LMMP_BUILD_BENCHMARKS)
        add_subdirectory(benchmark/lmmp)
    endif()
else()
    add_subdirectory(tune/lmmp)
endif()

# ==============================================================================
# 8. 最终信息输出
# ==============================================================================
message(STATUS "=====================================")
message(STATUS "Lmmp configure summary")
message(STATUS "=====================================")
message(STATUS "Compiler:      ${CMAKE_C_COMPILER_ID}")
message(STATUS "Linker:        ${CMAKE_C_COMPILER_LINKER_ID}")
message(STATUS "Target system: ${TARGET_SYSTEM}")
message(STATUS "Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "ASM support:   ${LMMP_ASM}")
message(STATUS "Output dir:    ${OUTPUT_ROOT}")
message(STATUS "=====================================")
