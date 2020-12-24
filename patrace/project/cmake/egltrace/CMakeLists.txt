include(src.cmake)
include(config.cmake)

include_directories(
	${SRC_ROOT}
	${SRC_ROOT}/common
)

add_definitions(-DGLES_CALLCONVENTION=)

add_library(egltrace SHARED
	${SRC_EGLTRACE}
)

target_link_libraries (egltrace
    common
    ${SNAPPY_LIBRARIES}
    md5
    dl
    jsoncpp
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
)
add_dependencies(egltrace eglproc_auto_src_generation glxml_header)
set_target_properties(egltrace PROPERTIES LINK_FLAGS "-pthread" COMPILE_FLAGS "-pthread")
install(TARGETS egltrace LIBRARY DESTINATION lib)
