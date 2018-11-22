add_custom_command (
    OUTPUT ${SRC_ROOT}/helper/paramsize.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/helper/paramsize.py
    DEPENDS
        ${SPECS_SCRIPTS}
        ${SRC_ROOT}/specs/glesparams.py
        ${SRC_ROOT}/helper/paramsize.py
    WORKING_DIRECTORY ${SRC_ROOT}/helper
)

add_custom_command (
    OUTPUT ${SRC_ROOT}/tracer/egltrace_auto.cpp ${SRC_ROOT}/tracer/sig_enum.hpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/tracer/trace.py
    DEPENDS
        ${SPECS_SCRIPTS}
        ${SRC_ROOT}/tracer/trace.py
    WORKING_DIRECTORY ${SRC_ROOT}/tracer
)

set(SRC_EGLTRACE
    ${SRC_ROOT}/dispatch/eglproc_auto.hpp
    ${SRC_ROOT}/dispatch/eglproc_auto.cpp
    ${SRC_ROOT}/dispatch/eglproc_trace.cpp
    ${SRC_ROOT}/helper/paramsize.cpp
    ${SRC_ROOT}/helper/eglsize.cpp
    ${SRC_ROOT}/helper/states.cpp
    ${SRC_ROOT}/tracer/egltrace.cpp
    ${SRC_ROOT}/tracer/egltrace_auto.cpp
    ${SRC_ROOT}/tracer/tracerparams.cpp
    ${SRC_ROOT}/tracer/interactivecmd.cpp
    ${SRC_ROOT}/tracer/glstate_images.cpp
    ${SRC_ROOT}/tracer/path.cpp
)

set_source_files_properties (
    ${SRC_ROOT}/dispatch/eglproc_auto.hpp
    ${SRC_ROOT}/dispatch/eglproc_auto.cpp
    PROPERTIES
        GENERATED True
)
