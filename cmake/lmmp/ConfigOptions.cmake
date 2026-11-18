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

function(lmmp_read_option_catalog output_variable)
    set(_catalog_file "${CMAKE_CURRENT_LIST_DIR}/config.options")
    if(NOT EXISTS "${_catalog_file}")
        message(FATAL_ERROR "LMMP option catalog not found: ${_catalog_file}")
    endif()

    file(READ "${_catalog_file}" _contents)
    string(REPLACE "\n" ";" _lines "${_contents}")
    set(_records)
    foreach(_line IN LISTS _lines)
        string(STRIP "${_line}" _line)
        if(_line STREQUAL "" OR _line MATCHES "^#")
            continue()
        endif()
        string(REPLACE "|" ";" _fields "${_line}")
        list(LENGTH _fields _field_count)
        if(_field_count EQUAL 5)
            list(GET _fields 0 _category)
            list(GET _fields 1 _name)
            list(GET _fields 2 _type)
            list(GET _fields 3 _default)
            list(GET _fields 4 _description)
            string(STRIP "${_default}" _default)
            string(STRIP "${_description}" _description)
            list(APPEND _records "${_category}|${_name}|${_type}|${_default}|${_description}")
        else()
            message(FATAL_ERROR "Invalid LMMP option catalog line: ${_line}")
        endif()
    endforeach()
    set(${output_variable} "${_records}" PARENT_SCOPE)
endfunction()

function(lmmp_option_default value output_variable)
    set(_value "${value}")
    if(_value STREQUAL "\"\"")
        set(_value "")
    endif()
    set(${output_variable} "${_value}" PARENT_SCOPE)
endfunction()

function(lmmp_option_choices type output_variable)
    set(_choices "")
    if("${type}" MATCHES "^enum\\((.*)\\)$")
        set(_choices "${CMAKE_MATCH_1}")
        string(REPLACE "," ";" _choices "${_choices}")
    elseif("${type}" STREQUAL "build-type")
        set(_choices "inherit;Debug;Release;RelWithDebInfo;MinSizeRel")
    endif()
    set(${output_variable} "${_choices}" PARENT_SCOPE)
endfunction()

