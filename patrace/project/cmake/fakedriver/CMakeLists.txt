include(src.cmake)
include_directories(
	${SRC_ROOT}
)

add_library(EGL SHARED
	${SRC_FAKEDRIVER_EGL}
)
add_dependencies(EGL ${SRC_ROOT}/fakedriver/egl/auto.cpp)

target_link_libraries (EGL
    dl
    rt
)

add_library(GLESv1_CM SHARED
	${SRC_FAKEDRIVER_GLES1}
)
add_dependencies(GLESv1_CM ${SRC_ROOT}/fakedriver/gles1/auto.cpp)

target_link_libraries (GLESv1_CM
    dl
    rt
)

add_library(GLESv2 SHARED
	${SRC_FAKEDRIVER_GLES2_GLES3}
)
add_dependencies(GLESv2 ${SRC_ROOT}/fakedriver/gles2/auto.cpp)

target_link_libraries (GLESv2
    dl
    rt
    common
)

install(TARGETS EGL GLESv1_CM GLESv2 LIBRARY DESTINATION lib)
