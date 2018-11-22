add_custom_command (
    OUTPUT ${SRC_ROOT}/common/api_info_auto.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/common/api_info.py
    DEPENDS
        ${SPECS_SCRIPTS}
        ${SRC_ROOT}/common/api_info.py
    WORKING_DIRECTORY ${SRC_ROOT}/common
)

set(SRC_COMMON
    ${SRC_ROOT}/common/memory.cpp
    ${SRC_ROOT}/common/trace_callset.cpp
    ${SRC_ROOT}/common/api_info_auto.cpp
    ${SRC_ROOT}/common/api_info.cpp
    ${SRC_ROOT}/common/in_file.cpp
    ${SRC_ROOT}/common/in_file_mt.cpp
    ${SRC_ROOT}/common/in_file_ra.cpp
    ${SRC_ROOT}/common/out_file.cpp
    ${SRC_ROOT}/common/image.cpp
    ${SRC_ROOT}/common/image_png.cpp
    ${SRC_ROOT}/common/image_bmp.cpp
    ${SRC_ROOT}/common/image_pnm.cpp
    ${SRC_ROOT}/common/base64.cpp
    ${SRC_ROOT}/common/library.cpp
    ${SRC_ROOT}/common/gl_extension_supported.cpp
    ${SRC_ROOT}/common/analysis_utility.cpp
    ${SRC_ROOT}/common/gl_utility.cpp
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set (SRC_COMMON_SYSTEM
        ${SRC_ROOT}/common/os_win.cpp
    )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (SRC_COMMON_SYSTEM
        ${SRC_ROOT}/common/os_posix.cpp
        ${SRC_ROOT}/common/memoryinfo.cpp
        ${SRC_ROOT}/common/os_thread_linux.cpp
    )
endif ()
