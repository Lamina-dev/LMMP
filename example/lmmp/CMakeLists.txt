#
#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#   by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY. 
#
#  See <https://www.gnu.org/licenses/>.


# 收集所有示例文件
file(GLOB EXAMPLE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/example*.cpp)

# 遍历生成示例可执行文件
foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
    # 获取文件名（不含路径和扩展名）
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    # 生成可执行文件名称（如Lmmpexample1）
    set(TARGET_NAME "lmmp_${EXAMPLE_NAME}")
    
    # 添加可执行文件
    add_executable(${TARGET_NAME} ${EXAMPLE_FILE})
    
    # 头文件包含
    target_include_directories(${TARGET_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
    )
    
    # 链接核心动态库
    target_link_libraries(${TARGET_NAME} PRIVATE LmmpCore)
    
    # 设置编译器选项
    lmmp_target_setup(${TARGET_NAME})
    
    # 设置输出属性
    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME ${TARGET_NAME}
        PDB_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endforeach()