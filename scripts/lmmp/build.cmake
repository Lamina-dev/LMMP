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

if(NOT LMMP_TARGET MATCHES "^(perf|tune)(_[a-z0-9]+)?$")
    message(FATAL_ERROR
        "Usage: cmake -DLMMP_BUILD_DIR=<build-dir> -DLMMP_TARGET=<perf|tune>[_<module>] "
        "[-DLMMP_CONFIG=<configuration>] -P scripts/lmmp/build.cmake")
endif()
if(NOT DEFINED LMMP_BUILD_DIR)
    set(LMMP_BUILD_DIR build)
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
# 由系统 shell 读取一行，保留终端及重定向输入。
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

set(config_arg)
if(DEFINED LMMP_CONFIG)
    set(config_arg --config "${LMMP_CONFIG}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${LMMP_BUILD_DIR}" ${config_arg} --target ${targets}
    COMMAND_ERROR_IS_FATAL ANY)
