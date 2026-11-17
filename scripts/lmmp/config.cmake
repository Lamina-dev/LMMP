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

set(usage "Usage: cmake -P scripts/lmmp/config.cmake -- -B <build-dir> [CMake configure options]")
if(CMAKE_ARGC EQUAL 3 OR (CMAKE_ARGV3 STREQUAL "--" AND CMAKE_ARGV4 STREQUAL "--help"))
    message("${usage}")
    return()
endif()
if(NOT CMAKE_ARGV1 STREQUAL "-P" OR NOT CMAKE_ARGV3 STREQUAL "--"
    OR NOT CMAKE_ARGV4 STREQUAL "-B" OR CMAKE_ARGC LESS 6)
    message(FATAL_ERROR "${usage}")
endif()

set(args)
math(EXPR last_arg "${CMAKE_ARGC} - 1")
foreach(i RANGE 4 ${last_arg})
    string(REPLACE ";" "\\;" arg "${CMAKE_ARGV${i}}")
    list(APPEND args "${arg}")
endforeach()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}/../.." ${args}
    COMMAND_ERROR_IS_FATAL ANY)
