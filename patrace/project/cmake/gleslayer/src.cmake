add_custom_command (
    OUTPUT ${SRC_ROOT}/fakedriver/egl/auto.cpp
        ${SRC_ROOT}/fakedriver/gles1/auto.cpp
        ${SRC_ROOT}/fakedriver/gles2/auto.cpp
        ${SRC_ROOT}/fakedriver/single/auto.cpp
        ${SRC_ROOT}/dispatch/gleslayer_helper.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/fakedriver/autogencode.py
    DEPENDS
        ${SRC_ROOT}/fakedriver/autogencode.py
    WORKING_DIRECTORY ${SRC_ROOT}/fakedriver ${SRC_ROOT}/dispatch
)

add_custom_command (
    OUTPUT ${SRC_ROOT}/tracer/egltrace_auto.cpp ${SRC_ROOT}/tracer/sig_enum.hpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/tracer/trace.py
    DEPENDS
        ${SPECS_SCRIPTS}
        ${SRC_ROOT}/tracer/trace.py
    WORKING_DIRECTORY ${SRC_ROOT}/tracer
)

set(SRC_GLES_LAYER
    ${SRC_ROOT}/fakedriver/common.cpp
    ${SRC_ROOT}/fakedriver/single/auto.cpp
    ${SRC_ROOT}/fakedriver/egl/fps_log.cpp
    ${SRC_ROOT}/fakedriver/single/proc.cpp
    ${SRC_ROOT}/dispatch/eglproc_auto.hpp
    ${SRC_ROOT}/dispatch/eglproc_auto.cpp
    ${SRC_ROOT}/dispatch/eglproc_trace.cpp
    ${SRC_ROOT}/dispatch/gleslayer_helper.cpp
    ${SRC_ROOT}/helper/paramsize.cpp
    ${SRC_ROOT}/helper/eglsize.cpp
    ${SRC_ROOT}/helper/shaderutility.cpp
    ${SRC_ROOT}/helper/states.cpp
    ${SRC_ROOT}/tracer/egltrace.cpp
    ${SRC_ROOT}/tracer/egltrace_auto.cpp
    ${SRC_ROOT}/tracer/tracerparams.cpp
    ${SRC_ROOT}/tracer/interactivecmd.cpp
    ${SRC_ROOT}/tracer/glstate_images.cpp
)
