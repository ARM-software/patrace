include(src.cmake)
include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/common
    ${THIRDPARTY_INCLUDE_DIRS}
    ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/api
    ${THIRDPARTY_INCLUDE_DIRS}/snappy
    ${THIRDPARTY_INCLUDE_DIRS}/libpng
    ${THIRDPARTY_INCLUDE_DIRS}/zlib
)

add_library(common STATIC
    ${SRC_COMMON}
    ${SRC_COMMON_SYSTEM}
)

# common/gl_extension_supported.cpp depends on eglproc_auto.hpp
add_dependencies(common eglproc_auto_src_generation)
