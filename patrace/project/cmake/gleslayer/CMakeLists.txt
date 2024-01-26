include(src.cmake)
include(config.cmake)

include_directories(
	${SRC_ROOT}
)

add_library(GLES_LAYER SHARED
	${SRC_GLES_LAYER}
)
add_dependencies(GLES_LAYER ${SRC_ROOT}/fakedriver/single/auto.cpp)
add_dependencies(GLES_LAYER ${SRC_ROOT}/dispatch/gleslayer_helper.cpp)
add_dependencies(GLES_LAYER eglproc_auto_src_generation glxml_header)

target_link_libraries (GLES_LAYER
    common
    ${SNAPPY_LIBRARIES}
    md5
    dl
    jsoncpp
    rt
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

install(TARGETS EGL GLESv1_CM GLESv2 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
