cmake_minimum_required(VERSION 3.10)
project(main C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED raylib)
pkg_check_modules(RAQM REQUIRED raqm)
pkg_check_modules(FREETYPE2 REQUIRED freetype2)

include_directories(
    ${RAYLIB_INCLUDE_DIRS}
    ${RAQM_INCLUDE_DIRS}
    ${FREETYPE2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(
    ${RAYLIB_LIBRARY_DIRS}
    ${RAQM_LIBRARY_DIRS}
    ${FREETYPE2_LIBRARY_DIRS}
)

add_executable(main main.c)

target_link_libraries(main
    ${RAYLIB_LIBRARIES}
    ${RAQM_LIBRARIES}
    ${FREETYPE2_LIBRARIES}
)

if (WIN32)
    target_link_libraries(main opengl32 gdi32 winmm)
endif()