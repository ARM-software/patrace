include(src.cmake)
include_directories(
	${SRC_ROOT}
)

add_library(EGL SHARED
	${SRC_FAKEDRIVER_EGL}
)
add_dependencies(EGL egl_auto_src_generation)

target_link_libraries (EGL
    dl
    rt
)
set_target_properties(EGL PROPERTIES LINK_FLAGS "-z max-page-size=16384")
add_library(GLESv1_CM SHARED
	${SRC_FAKEDRIVER_GLES1}
)
add_dependencies(GLESv1_CM gles1_auto_src_generation)

target_link_libraries (GLESv1_CM
    dl
    rt
)
set_target_properties(GLESv1_CM PROPERTIES LINK_FLAGS "-z max-page-size=16384")
add_library(GLESv2 SHARED
	${SRC_FAKEDRIVER_GLES2_GLES3}
)
add_dependencies(GLESv2 gles2_auto_src_generation)

target_link_libraries (GLESv2
    dl
    rt
    common
)
set_target_properties(GLESv2 PROPERTIES LINK_FLAGS "-z max-page-size=16384")
install(TARGETS EGL GLESv1_CM GLESv2 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
