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

set(LIBLMMP_VERSION "4.3.0")

set(LMMP_MODULES lmmp lmmpn mprand numth secret)
set(LMMP_BACKEND generic)
if(LMMP_ASM_ENABLED)
    set(LMMP_BACKEND "${LMMP_ASM_ARCH}")
endif()

set(LMMP_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(mparam "#ifndef LMMP_MPARAM_H\n#define LMMP_MPARAM_H\n")
set(param_names)
set(asm_sources)
set(library_sources)
set(public_exports)
foreach(module IN LISTS LMMP_MODULES)
    file(GLOB module_sources CONFIGURE_DEPENDS "${module}/*.c")
    list(APPEND library_sources ${module_sources})

    # 一个模块的架构实现整体替换其 generic 实现。
    file(GLOB module_asm CONFIGURE_DEPENDS "${module}/arch/${LMMP_BACKEND}/*.S")
    if(module_asm)
        list(APPEND asm_sources ${module_asm})
        if(TARGET_SYSTEM STREQUAL "WIN" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${module}/arch/extern.def")
            list(APPEND public_exports "${module}/arch/extern.def")
        endif()
    else()
        file(GLOB module_generic CONFIGURE_DEPENDS "${module}/arch/generic/*.c")
        list(APPEND library_sources ${module_generic})
    endif()

    file(GLOB param_headers CONFIGURE_DEPENDS "${module}/arch/${LMMP_BACKEND}/mparam.h")
    if(param_headers)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${param_headers})
        file(READ "${param_headers}" module_params)
        string(REGEX MATCHALL "#[ \t]*define[ \t]+[A-Za-z_][A-Za-z_0-9]*" definitions "${module_params}")
        foreach(definition IN LISTS definitions)
            string(REGEX REPLACE ".*[ \t]" "" name "${definition}")
            if(name IN_LIST param_names)
                message(FATAL_ERROR "Duplicate machine parameter ${name} in ${param_headers}")
            endif()
            list(APPEND param_names "${name}")
        endforeach()
        string(APPEND mparam "\n${module_params}")
    endif()
endforeach()
if(LMMP_ASM_ENABLED AND NOT asm_sources)
    message(FATAL_ERROR "No LMMP assembly sources for backend ${LMMP_BACKEND}")
endif()
list(APPEND library_sources ${asm_sources})
string(APPEND mparam "\n#endif\n")
file(GENERATE OUTPUT "${LMMP_GENERATED_DIR}/mparam.h" CONTENT "${mparam}")

function(lmmp_shared_library target output_name)
    add_library(${target} SHARED ${library_sources} ${ARGN})
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${output_name}")
    target_include_directories(${target}
        PUBLIC "${PROJECT_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${LMMP_GENERATED_DIR}")
    target_compile_definitions(${target} PRIVATE
        LMMP_VERSION_STR="${LIBLMMP_VERSION}"
        LMMP_BUILD_TYPE_STR="$<CONFIG>")
    if(TARGET_SYSTEM STREQUAL "WIN")
        target_compile_definitions(${target} PRIVATE LIBLMMP_EXPORTS)
        set_target_properties(${target} PROPERTIES PREFIX "")
    elseif(TARGET_SYSTEM STREQUAL "LIN")
        target_link_libraries(${target} PRIVATE m)
    endif()
    lmmp_target_setup(${target})
endfunction()

lmmp_shared_library(liblmmp lmmp ${public_exports})
add_library(lmmp ALIAS liblmmp)
set_target_properties(liblmmp PROPERTIES
    C_VISIBILITY_PRESET hidden
    VERSION ${LIBLMMP_VERSION}
    SOVERSION 1)

if(LMMP_OUTPUT_DIR)
    file(GLOB public_headers CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/lmmp/*.h")
    add_custom_command(TARGET liblmmp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_BIN}/include/lmmp"
        COMMAND ${CMAKE_COMMAND} -E copy ${public_headers} "${OUTPUT_BIN}/include/lmmp"
        VERBATIM)
endif()

lmmp_shared_library(liblmmp_internal lmmp_internal)
set_target_properties(liblmmp_internal PROPERTIES
    C_VISIBILITY_PRESET default
    EXCLUDE_FROM_ALL TRUE)
target_compile_definitions(liblmmp_internal PRIVATE LMMP_INTERNAL_BUILD)
target_compile_definitions(liblmmp_internal INTERFACE LMMP_INTERNAL_USE)
if(TARGET_SYSTEM STREQUAL "WIN")
    if(CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
        set_target_properties(liblmmp_internal PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    else()
        target_link_options(liblmmp_internal PRIVATE -Wl,--export-all-symbols)
    endif()
endif()

function(lmmp_add_program target)
    add_executable(${target} ${ARGN})
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${LMMP_GENERATED_DIR}")
    target_link_libraries(${target} PRIVATE liblmmp_internal)
    if(TARGET_SYSTEM STREQUAL "LIN")
        target_link_libraries(${target} PRIVATE m)
    endif()
    lmmp_target_setup(${target})
endfunction()

if(LMMP_BUILD_TESTS)
    if(TARGET_SYSTEM STREQUAL "WIN")
        get_filename_component(compiler_bin "${CMAKE_C_COMPILER}" DIRECTORY)
    else()
        find_package(Threads REQUIRED)
    endif()
endif()

set(program_targets "")
foreach(module IN LISTS LMMP_MODULES)
    if(LMMP_BUILD_TESTS)
        file(GLOB tests CONFIGURE_DEPENDS "${module}/test/*.c")
        if("${CMAKE_CURRENT_SOURCE_DIR}/${module}/test/main.c" IN_LIST tests)
            set(target "test_${module}")
            lmmp_add_program(${target} ${tests} support/test.c)
            if(TARGET_SYSTEM STREQUAL "WIN")
                add_test(NAME ${target} COMMAND ${CMAKE_COMMAND} -E env
                    "PATH=${compiler_bin};$ENV{PATH}" $<TARGET_FILE:${target}>)
            else()
                if(module STREQUAL "lmmp")
                    target_link_libraries(${target} PRIVATE Threads::Threads)
                endif()
                add_test(NAME ${target} COMMAND ${target})
            endif()
        endif()
    endif()

    foreach(kind IN ITEMS perf tune)
        file(GLOB programs CONFIGURE_DEPENDS "${module}/${kind}/*.c")
        foreach(program IN LISTS programs)
            get_filename_component(name "${program}" NAME_WE)
            set(target "${kind}_${module}_${name}")
            lmmp_add_program(${target} "${program}")
            set_target_properties(${target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
            if(NOT TARGET_SYSTEM STREQUAL "WIN")
                target_compile_definitions(${target} PRIVATE _POSIX_C_SOURCE=200809L)
            endif()
            string(APPEND program_targets "${target}\n")
        endforeach()
    endforeach()
endforeach()

file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/lmmp-programs.txt" CONTENT "${program_targets}")
