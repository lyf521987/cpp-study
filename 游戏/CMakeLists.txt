cmake_minimum_required(VERSION 3.10)
project(FlappyBird)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置vcpkg工具链（如果存在）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

# 查找SFML库
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sfml_config.cmake")
    include(${CMAKE_CURRENT_SOURCE_DIR}/sfml_config.cmake)
else()
    find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
endif()

# 创建可执行文件
add_executable(FlappyBird main.cpp)

# 链接SFML库
target_link_libraries(FlappyBird sfml-graphics sfml-window sfml-system)

# 设置输出目录
set_target_properties(FlappyBird PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Windows特定设置
if(WIN32)
    # 复制SFML DLL到输出目录
    add_custom_command(TARGET FlappyBird POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sfml-graphics>
        $<TARGET_FILE:sfml-window>
        $<TARGET_FILE:sfml-system>
        $<TARGET_FILE_DIR:FlappyBird>
    )
endif()
