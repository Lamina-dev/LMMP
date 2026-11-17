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

# 编译器目录：传入包含 GNU/Clang 编译器对（gcc+g++ 或 clang+clang++）的文件夹
set(LMMP_COMPILER_DIR "" CACHE PATH
    "Directory containing a GNU/Clang compiler pair (gcc+g++ or clang+clang++). Empty = default compiler.")

# 链接器目录：传入包含链接器的文件夹，按文件名识别风味
set(LMMP_LINKER_DIR "" CACHE PATH
    "Directory containing the linker (ld.lld | ld.mold | ld, flavor by name). Empty = toolchain default linker.")

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

# 构建目标（默认只构建动态库；测试和示例可按构建选项加入，
# 与动态库输出到同一目录，便于以统一产物目录整体引入）
option(LMMP_BUILD_TESTS "Build LMMP unit tests" OFF)
option(LMMP_BUILD_EXAMPLES "Build LMMP examples" OFF)

# 统一产物目录：传入后所有构建产物（动态库、导入库与可选的
# 附属可执行文件）平铺到该目录，不区分配置子目录，
# 并随库同步对外头文件（include/lmmp），目录即完整可引入形态；
# 为空时维持源内 dist 布局。建议使用绝对路径。
set(LMMP_OUTPUT_DIR "" CACHE PATH
    "Flat output directory for all artifacts (shared lib, import lib, headers, optional executables). Empty = in-tree dist/lmmp/bin/<config>.")

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
if(NOT TARGET_SYSTEM MATCHES "^(AUTO|WIN|LIN|MAC)$")
    message(FATAL_ERROR "Invalid TARGET_SYSTEM value: ${TARGET_SYSTEM}. Use AUTO, WIN, LIN, or MAC.")
endif()
