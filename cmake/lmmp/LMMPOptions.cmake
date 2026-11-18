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

include("${CMAKE_CURRENT_LIST_DIR}/ConfigOptions.cmake")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/config.options")
lmmp_read_option_catalog(_lmmp_option_records)

foreach(_record IN LISTS _lmmp_option_records)
    string(REPLACE "|" ";" _fields "${_record}")
    list(GET _fields 1 _name)
    list(GET _fields 2 _type)
    list(GET _fields 3 _catalog_default)
    list(GET _fields 4 _description)
    lmmp_option_default("${_catalog_default}" _default)

    if(_type STREQUAL "bool")
        option(${_name} "${_description}" "${_default}")
    elseif(_type STREQUAL "path")
        set(${_name} "${_default}" CACHE PATH "${_description}")
    else()
        set(${_name} "${_default}" CACHE STRING "${_description}")
        lmmp_option_choices("${_type}" _choices)
        if(_choices)
            set_property(CACHE ${_name} PROPERTY STRINGS ${_choices})
        endif()
    endif()
endforeach()

set(_lmmp_build_types Debug Release RelWithDebInfo MinSizeRel)
foreach(_name IN ITEMS LMMP_BUILD_TYPE LMMP_LIBRARY_BUILD_TYPE LMMP_TEST_BUILD_TYPE
        LMMP_EXAMPLE_BUILD_TYPE LMMP_PERF_BUILD_TYPE LMMP_TUNE_BUILD_TYPE)
    if(${_name} STREQUAL "inherit")
        set(${_name} "" CACHE STRING "${_name}" FORCE)
    endif()
    if(NOT ${_name} STREQUAL "" AND NOT ${_name} IN_LIST _lmmp_build_types)
        message(FATAL_ERROR "Invalid ${_name}: ${${_name}}. Use Debug, Release, RelWithDebInfo, or MinSizeRel.")
    endif()
endforeach()
if(NOT LMMP_BUILD_TYPE)
    message(FATAL_ERROR "LMMP_BUILD_TYPE must be Debug, Release, RelWithDebInfo, or MinSizeRel.")
endif()

foreach(_name IN ITEMS LMMP_ASM LMMP_TARGET_SYSTEM)
    string(TOUPPER "${${_name}}" _normalized)
    if(NOT _normalized STREQUAL "${${_name}}")
        set(${_name} "${_normalized}" CACHE STRING "${_name}" FORCE)
    endif()
endforeach()
if(NOT LMMP_ASM MATCHES "^(AUTO|GENERIC|X64|ARM64)$")
    message(FATAL_ERROR "Invalid LMMP_ASM value: ${LMMP_ASM}. Use AUTO, GENERIC, X64, or ARM64.")
endif()
if(NOT LMMP_TARGET_SYSTEM MATCHES "^(AUTO|WIN|LIN|MAC)$")
    message(FATAL_ERROR "Invalid LMMP_TARGET_SYSTEM value: ${LMMP_TARGET_SYSTEM}. Use AUTO, WIN, LIN, or MAC.")
endif()

if(NOT LMMP_MEMORY_MORE_ALLOC_TIMES MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "LMMP_MEMORY_MORE_ALLOC_TIMES must be a positive integer.")
endif()

