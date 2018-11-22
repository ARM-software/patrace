#add_custom_command (
#    OUTPUT ${SRC_ROOT}/helper/paramsize.cpp
#    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/helper/paramsize.py
#    DEPENDS
#	${SPECS_SCRIPTS}
#        ${SRC_ROOT}/specs/glesparams.py
#        ${SRC_ROOT}/helper/paramsize.py
 #   WORKING_DIRECTORY ${SRC_ROOT}/helper
#)

set(SRC_NEWFASTFORWARDER
    ${SRC_ROOT}/dispatch/eglproc_auto.hpp
    ${SRC_ROOT}/dispatch/eglproc_auto.cpp
    ${SRC_ROOT}/dispatch/eglproc_retrace.cpp
    ${SRC_ROOT}/newfastforwarder/newfastforwad.cpp
    ${SRC_ROOT}/newfastforwarder/parser.cpp
#    ${SRC_ROOT}/newfastforwarder/retrace_api.cpp
    ${SRC_ROOT}/newfastforwarder/parse_gles.cpp
    ${SRC_ROOT}/newfastforwarder/parse_egl.cpp
    ${SRC_ROOT}/newfastforwarder/parse_main.cpp
    ${SRC_ROOT}/newfastforwarder/cutter.cpp
    ${SRC_ROOT}/newfastforwarder/framestore.cpp
)

set_source_files_properties (
    ${SRC_ROOT}/dispatch/eglproc_auto.hpp
    ${SRC_ROOT}/dispatch/eglproc_auto.cpp
    ${SRC_ROOT}/newfastforwarder/parse_gles.cpp
    PROPERTIES
        GENERATED True
)

#configure_file (
#    "${SRC_ROOT}/newfastforwarder/config.hpp.in"
#    "${SRC_ROOT}/newfastforwarder/config.hpp"
#    )
