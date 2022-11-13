cmake_minimum_required(VERSION 3.14)

set(PROJECT CellBakery)

project(${PROJECT})

set(SOURCES
    CellBakeryAlternative/Main.cpp
    CellBakeryAlternative/OpenGL/glad.cpp
)

set_source_files_properties(CellBakeryAlternative/OpenGL/glad.cpp PROPERTIES COMPILE_FLAGS -Wno-pedantic)

add_executable(${PROJECT} ${SOURCES})

target_include_directories(${PROJECT} PRIVATE "./CellBakeryAlternative/OpenGL/include")
target_compile_options(${PROJECT} PRIVATE -O3 -std=c++14 -Wall -Wextra -Wpedantic)
target_link_options(${PROJECT} PRIVATE -lglfw)

add_custom_command(TARGET ${PROJECT} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/CellBakeryAlternative/Shaders $<TARGET_FILE_DIR:${PROJECT}>/Shaders
)