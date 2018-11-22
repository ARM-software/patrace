cmake_minimum_required (VERSION 2.8)

project (common)

set (THIRDPARTY_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/../thirdparty)

include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${THIRDPARTY_INCLUDE_DIRS}
    ${THIRDPARTY_INCLUDE_DIRS}/khronos
    ${THIRDPARTY_INCLUDE_DIRS}/cppunit/include
)

find_library(CPPUNITLIB cppunit PATHS ${THIRDPARTY_INCLUDE_DIRS}/cppunit/lib/gcc-x86_64-linux)

add_subdirectory (${THIRDPARTY_INCLUDE_DIRS}/libpng ${CMAKE_CURRENT_BINARY_DIR}/libpng)
add_subdirectory (${THIRDPARTY_INCLUDE_DIRS}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
add_subdirectory (system)
add_subdirectory (image)
add_subdirectory (eglstate)

set (APP_LIBS
    common_eglstate
    common_image
    common_system
    tiff
    pthread
    z_bundled
)

add_subdirectory (tool/imageconv)
add_subdirectory (unittest)
