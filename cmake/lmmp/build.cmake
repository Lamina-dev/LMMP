#  Copyright (C) 2026 HJimmyK (Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as
#  published by the Free Software Foundation, either version 3 of
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

function(lmmp_build_help)
    message("LMMP 目标构建工具")
    message("")
    message("用法：")
    message("  cmake -P cmake/lmmp/build.cmake -- build -B <构建目录> -t <目标>")
    message("")
    message("参数：")
    message("  -B <目录>            构建树目录，默认 build")
    message("  -t <目标>            perf、tune、perf_<模块> 或 tune_<模块>")
    message("  help                 显示帮助")
endfunction()

if(CMAKE_ARGC LESS 5 OR CMAKE_ARGV4 STREQUAL "help"
        OR CMAKE_ARGV4 STREQUAL "-h" OR CMAKE_ARGV4 STREQUAL "--help")
    lmmp_build_help()
    return()
endif()
if(NOT CMAKE_ARGV4 STREQUAL "build")
    message(FATAL_ERROR "未知命令 '${CMAKE_ARGV4}'。请使用 help 查看用法。")
endif()

set(LMMP_BUILD_DIR build)
set(LMMP_TARGET "")
math(EXPR _last_arg "${CMAKE_ARGC} - 1")
set(_i 5)
while(_i LESS_EQUAL _last_arg)
    set(_arg "${CMAKE_ARGV${_i}}")
    if(_arg STREQUAL "-B" OR _arg STREQUAL "--build-dir")
        math(EXPR _i "${_i} + 1")
        if(_i GREATER _last_arg)
            message(FATAL_ERROR "${_arg} requires a directory.")
        endif()
        set(LMMP_BUILD_DIR "${CMAKE_ARGV${_i}}")
    elseif(_arg STREQUAL "-t" OR _arg STREQUAL "--target")
        math(EXPR _i "${_i} + 1")
        if(_i GREATER _last_arg)
            message(FATAL_ERROR "${_arg} requires a target.")
        endif()
        set(LMMP_TARGET "${CMAKE_ARGV${_i}}")
    else()
        message(FATAL_ERROR "未知参数 '${_arg}'。请使用 help 查看用法。")
    endif()
    math(EXPR _i "${_i} + 1")
endwhile()

if(NOT LMMP_TARGET MATCHES "^(perf|tune)(_[a-z0-9]+)?$")
    message(FATAL_ERROR "-t must be perf, tune, perf_<module>, or tune_<module>.")
endif()
if(NOT EXISTS "${LMMP_BUILD_DIR}/CMakeCache.txt")
    message(FATAL_ERROR "Configure an LMMP build first: ${LMMP_BUILD_DIR}")
endif()

# 重新生成清单，纳入自上次配置后增删的测量程序。
execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}/../.." -B "${LMMP_BUILD_DIR}"
    OUTPUT_QUIET
    COMMAND_ERROR_IS_FATAL ANY)
file(STRINGS "${LMMP_BUILD_DIR}/lmmp-programs.txt" targets)
list(FILTER targets INCLUDE REGEX "^${LMMP_TARGET}_")
list(LENGTH targets count)
if(count EQUAL 0)
    message(FATAL_ERROR "No LMMP programs for '${LMMP_TARGET}'.")
endif()

message("${count} LMMP programs selected:")
foreach(target IN LISTS targets)
    message("  ${target}")
endforeach()
message("Build these programs? [y/N]")
if(CMAKE_HOST_WIN32)
    execute_process(COMMAND cmd.exe /D /V:ON /C "set reply=& set /p reply=& echo(!reply!"
        OUTPUT_VARIABLE reply OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    execute_process(COMMAND /bin/sh -c [=[IFS= read -r reply; printf '%s' "$reply"]=]
        OUTPUT_VARIABLE reply OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(NOT reply STREQUAL "y")
    message("Build cancelled.")
    return()
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${LMMP_BUILD_DIR}" --target ${targets}
    COMMAND_ERROR_IS_FATAL ANY)

