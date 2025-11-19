add_custom_command (
    OUTPUT ${SRC_ROOT}/fakedriver/egl/auto.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/fakedriver/autogencode.py
    DEPENDS
        ${SRC_FAKEDRIVER_EGL}
        ${SRC_ROOT}/fakedriver/autogencode.py
    WORKING_DIRECTORY ${SRC_ROOT}/fakedriver
)
add_custom_target(egl_auto_src_generation DEPENDS ${SRC_ROOT}/fakedriver/egl/auto.cpp)

add_custom_command (
    OUTPUT ${SRC_ROOT}/fakedriver/gles1/auto.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/fakedriver/autogencode.py
    DEPENDS
        ${SRC_FAKEDRIVER_GLES1}
        ${SRC_ROOT}/fakedriver/autogencode.py
    WORKING_DIRECTORY ${SRC_ROOT}/fakedriver
)
add_custom_target(gles1_auto_src_generation DEPENDS ${SRC_ROOT}/fakedriver/gles1/auto.cpp)

add_custom_command (
    OUTPUT ${SRC_ROOT}/fakedriver/gles2/auto.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/fakedriver/autogencode.py
    DEPENDS
        ${SRC_FAKEDRIVER_GLES2_GLES3}
        ${SRC_ROOT}/fakedriver/autogencode.py
    WORKING_DIRECTORY ${SRC_ROOT}/fakedriver
)
add_custom_target(gles2_auto_src_generation DEPENDS ${SRC_ROOT}/fakedriver/gles2/auto.cpp)

set(SRC_FAKEDRIVER_EGL
    ${SRC_ROOT}/fakedriver/common.cpp
    ${SRC_ROOT}/fakedriver/egl/auto.cpp
    ${SRC_ROOT}/fakedriver/egl/manual.cpp
    ${SRC_ROOT}/fakedriver/egl/fps_log.cpp
    ${SRC_ROOT}/fakedriver/egl/proc.cpp
)

set(SRC_FAKEDRIVER_GLES1
    ${SRC_ROOT}/fakedriver/common.cpp
    ${SRC_ROOT}/fakedriver/gles1/auto.cpp
    ${SRC_ROOT}/fakedriver/gles1/proc.cpp
)

set(SRC_FAKEDRIVER_GLES2_GLES3
    ${SRC_ROOT}/fakedriver/common.cpp
    ${SRC_ROOT}/fakedriver/gles2/auto.cpp
    ${SRC_ROOT}/fakedriver/gles2/proc.cpp
)
