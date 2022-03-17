set (SRC_EGL_STATE
	common.cpp
    #dump.cpp
    #fbo.cpp
    #frame.cpp
    #frame_recorder.cpp
    program.cpp
    render_target.cpp
    context.cpp
    #vbo.cpp
)

include_directories(
    ${SRC_ROOT}/../../common
)

add_library (common_eglstate STATIC
    ${SRC_EGL_STATE}
)
