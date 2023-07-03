cmake_minimum_required (VERSION 2.8.4)

project (patrace)
set (PYTHON_EXECUTABLE python)

message(STATUS "main ar: ${CMAKE_AR}")

set (PATRACE_VERSION_MAJOR 5)
set (PATRACE_VERSION_MINOR 1)
set (PATRACE_VERSION_PATCH 0)

IF(DEFINED ENV{SVN_REVISION})
    set (PATRACE_REVISION $ENV{SVN_REVISION})
ELSE(DEFINED ENV{SVN_REVISION})
    set (PATRACE_REVISION "unofficial")
ENDIF(DEFINED ENV{SVN_REVISION})

IF(DEFINED ENV{VERSION_TYPE})
    set (PATRACE_VERSION_TYPE $ENV{VERSION_TYPE})
ELSE(DEFINED ENV{VERSION_TYPE})
    set (PATRACE_VERSION_TYPE "dev")
ENDIF(DEFINED ENV{VERSION_TYPE})

IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

IF(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "*** DEBUG BUILD ***")
    add_definitions(-DREPORT_GL_ERRORS -DDEBUG -gdwarf)
ENDIF()

if (FFI7)
    if (WINDOWSYSTEM MATCHES "wayland")
        if (ARCH MATCHES "aarch64")
            set(ENV{PKG_CONFIG_PATH} "/usr/lib/aarch64-linux-gnu/pkgconfig/ffi7")
        elseif (ARCH MATCHES "armhf")
            set(ENV{PKG_CONFIG_PATH} "/usr/lib/arm-linux-gnueabihf/pkgconfig/ffi7")
        endif()
    endif()
endif()

include(FindPkgConfig)

set (THIRDPARTY_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/../../../thirdparty)
set (COMMON_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/../../../common)
set (SRC_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/../../src)

set (SRC_ROOT ${PROJECT_SOURCE_DIR}/../../src)
set (SPECS_SCRIPTS
    ${SRC_ROOT}/specs/debug.py
    ${SRC_ROOT}/specs/stdapi.py
    ${SRC_ROOT}/specs/gltypes.py
    ${SRC_ROOT}/specs/gles12api.py
    ${SRC_ROOT}/specs/glxml_header.py
    ${SRC_ROOT}/specs/eglapi.py
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC -fvisibility=hidden -Werror -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -std=c++11 -fvisibility=hidden -Werror -Wno-deprecated-declarations")
    set(CMAKE_C_FLAGS_RELEASE "-Os")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os")
    if (SANITIZELIB)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fstack-protector-all -fsanitize=${SANITIZELIB} -fsanitize-undefined-trap-on-error -fuse-ld=gold")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fstack-protector-all -fsanitize=${SANITIZELIB} -fsanitize-undefined-trap-on-error -fuse-ld=gold")
    endif()

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions (-DPLATFORM_64BIT)
    endif()

    if (WINDOWSYSTEM MATCHES "x11")
        find_package(X11 REQUIRED)
        include_directories(${X11_INCLUDE_DIR})
        add_definitions (-DENABLE_X11)
    endif()

    if (WINDOWSYSTEM MATCHES "udriver")
        set (UDRIVER_WINSYS_INCLUDE_DIR ${THIRDPARTY_INCLUDE_DIRS}/udriver_winsys/include)
        include_directories (${UDRIVER_WINSYS_INCLUDE_DIR})
        set (SRC_UDRIVER_WINSYS
	 ${THIRDPARTY_INCLUDE_DIRS}/udriver_winsys/src/WindowSystem.cpp
	)
	add_library (udriver_winsys STATIC
	 ${SRC_UDRIVER_WINSYS}
	)
        # No official support for the Mali FBDEV defitions, so hack it with the Ozone ones
        add_definitions (-DENABLE_FBDEV -DUSE_OZONE)
    endif()

    if (WINDOWSYSTEM MATCHES "wayland")
        pkg_check_modules(wayland REQUIRED wayland-client wayland-egl)
        # No official support for the Mali FBDEV defitions, so hack it with the Ozone ones
        add_definitions (-DENABLE_WAYLAND -DUSE_OZONE)
        message(STATUS "windowsystem set as wayland: ${wayland_VERSION}, ${wayland_LIBRARIES}, ${wayland_LDFLAGS}, ${wayland_CFLAGS}, ${wayland_CPPFLAGS}")
    endif()

    if (WINDOWSYSTEM MATCHES "fbdev")
       message(STATUS "windowsystem set as fbdev")
        # No official support for the Mali FBDEV defitions, so hack it with the Ozone ones
       add_definitions (-DENABLE_FBDEV -DUSE_OZONE)
    endif()

else ()
    # uncomment the following line if want to use the drawcall_states plugin
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS /EHsc")
endif()

include_directories(BEFORE
    ${THIRDPARTY_INCLUDE_DIRS}/egl-registry/api
    ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/api
    ${THIRDPARTY_INCLUDE_DIRS}/opencl-headers
    ${THIRDPARTY_INCLUDE_DIRS}/libcollector/external/jsoncpp/include
)
include_directories(${THIRDPARTY_INCLUDE_DIRS})
include_directories(${SRC_INCLUDE_DIRS})

set(HWCPIPE_INCLUDE_DIRS ${THIRDPARTY_INCLUDE_DIRS}/hwcpipe)
add_subdirectory(${THIRDPARTY_INCLUDE_DIRS}/hwcpipe ${CMAKE_CURRENT_BINARY_DIR}/hwcpipe EXCLUDE_FROM_ALL)
include_directories(${HWCPIPE_INCLUDE_DIRS})

set (ZLIB_INCLUDE_DIRS ${THIRDPARTY_INCLUDE_DIRS}/zlib)
add_subdirectory (${THIRDPARTY_INCLUDE_DIRS}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib EXCLUDE_FROM_ALL)
include_directories (${ZLIB_INCLUDE_DIRS})

set (SNAPPY_INCLUDE_DIRS ${THIRDPARTY_INCLUDE_DIRS}/snappy)
add_subdirectory (${THIRDPARTY_INCLUDE_DIRS}/snappy ${CMAKE_CURRENT_BINARY_DIR}/snappy EXCLUDE_FROM_ALL)
include_directories (${SNAPPY_INCLUDE_DIRS})

set (PNG_INCLUDE_DIR ${THIRDPARTY_INCLUDE_DIRS}/libpng)
add_subdirectory (${THIRDPARTY_INCLUDE_DIRS}/libpng ${CMAKE_CURRENT_BINARY_DIR}/libpng EXCLUDE_FROM_ALL)
include_directories (${PNG_INCLUDE_DIR})

if(NOT CC_HOST)
    set(CC_HOST "")
else()
    message(STATUS "cross-compile: setting --host=" ${CC_HOST})
endif()

add_custom_target(glxml_header
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/specs/glxml_header.py
    DEPENDS
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/specs/glxml_header.py
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)

add_custom_target (eglproc_auto_src_generation
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/dispatch/eglproc.py
    DEPENDS
        ${SPECS_SCRIPTS}
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/dispatch/dispatch.py
        ${SRC_ROOT}/dispatch/eglproc.py
    WORKING_DIRECTORY ${SRC_ROOT}/dispatch
)

add_subdirectory(common)
add_library(jsoncpp STATIC
        ${THIRDPARTY_INCLUDE_DIRS}/libcollector/external/jsoncpp/src/lib_json/json_reader.cpp
        ${THIRDPARTY_INCLUDE_DIRS}/libcollector/external/jsoncpp/src/lib_json/json_value.cpp
        ${THIRDPARTY_INCLUDE_DIRS}/libcollector/external/jsoncpp/src/lib_json/json_writer.cpp)
add_subdirectory(${THIRDPARTY_INCLUDE_DIRS}/md5 ${CMAKE_CURRENT_BINARY_DIR}/md5 EXCLUDE_FROM_ALL)

add_custom_target(retrace_gles_auto_src_generation
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/retracer/retrace.py
    DEPENDS
        ${SPECS_SCRIPTS}
        ${SRC_ROOT}/retracer/retrace.py
    WORKING_DIRECTORY ${SRC_ROOT}/retracer
)

add_subdirectory(fastforward)
add_subdirectory (${COMMON_INCLUDE_DIRS}/eglstate ${CMAKE_CURRENT_BINARY_DIR}/common_eglstate EXCLUDE_FROM_ALL)

if (NOT FF_ONLY)
    add_subdirectory(eglretrace)
    add_subdirectory(${THIRDPARTY_INCLUDE_DIRS}/libcollector ${CMAKE_BINARY_DIR}/libcollector)

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND NOT WINDOWSYSTEM MATCHES "udriver")
        add_subdirectory(egltrace)
        add_subdirectory(fakedriver)
    endif ()
endif ()

if (ENABLE_TOOLS AND NOT FF_ONLY)
    add_custom_command(
        OUTPUT ${SRC_ROOT}/common/call_parser.cpp
        COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/common/call_parser.py
        DEPENDS
            ${SPECS_SCRIPTS}
            ${SRC_ROOT}/common/call_parser.py
        WORKING_DIRECTORY ${SRC_ROOT}/common
    )
    add_custom_target(call_parser_src_generation DEPENDS ${SRC_ROOT}/common/call_parser.cpp)

    IF(NOT DEFINED ENV{NO_PYTHON_BUILD})

    set(PAT_PYTHON_DIR ${PROJECT_SOURCE_DIR}/../../python)

    set(PATRACE_VERSION_FULL ${PATRACE_VERSION_MAJOR}.${PATRACE_VERSION_MINOR}.${PATRACE_VERSION_PATCH})

    if (${PATRACE_VERSION_PATCH} EQUAL 0)
        set(PATRACE_VERSION_FULL ${PATRACE_VERSION_MAJOR}.${PATRACE_VERSION_MINOR})
    endif ()

    # Build source distribution for pypatrace
    set(PYPATRACE_TIMESTAMP "${PAT_PYTHON_DIR}/pypatrace/dist/timestamp")
    add_custom_command(
        OUTPUT  ${PYPATRACE_TIMESTAMP}
        COMMAND ${PYTHON_EXECUTABLE} setup.py sdist bdist_wheel
        COMMAND ${CMAKE_COMMAND} -E touch ${PYPATRACE_TIMESTAMP}
        DEPENDS
            call_parser_src_generation
            glxml_header
            common
            ${PAT_PYTHON_DIR}/pypatrace/patrace.i
        WORKING_DIRECTORY ${PAT_PYTHON_DIR}/pypatrace
    )
    add_custom_target(pypatrace ALL DEPENDS ${PYPATRACE_TIMESTAMP})

    install(DIRECTORY ${PAT_PYTHON_DIR}/pypatrace/dist/ DESTINATION python
        FILES_MATCHING PATTERN "pypatrace-${PATRACE_VERSION_FULL}.tar.gz"
        PATTERN "pypatrace-${PATRACE_VERSION_FULL}-*.whl")

    # Build source distribution for patracetools
    set(PATRACETOOLS_TIMESTAMP "${PAT_PYTHON_DIR}/patracetools/dist/timestamp")
    add_custom_command(
        OUTPUT  ${PATRACETOOLS_TIMESTAMP}
        COMMAND ${PYTHON_EXECUTABLE} setup.py sdist bdist_wheel
        COMMAND ${CMAKE_COMMAND} -E touch ${PATRACETOOLS_TIMESTAMP}
        DEPENDS
            call_parser_src_generation
            glxml_header
            common
        WORKING_DIRECTORY ${PAT_PYTHON_DIR}/patracetools
    )
    add_custom_target(patracetools ALL DEPENDS ${PATRACETOOLS_TIMESTAMP})

    install(DIRECTORY ${PAT_PYTHON_DIR}/patracetools/dist/ DESTINATION python
        FILES_MATCHING PATTERN "patracetools-${PATRACE_VERSION_FULL}.tar.gz"
        PATTERN "patracetools-${PATRACE_VERSION_FULL}-*.whl")
    install(FILES ${PAT_PYTHON_DIR}/README.rst DESTINATION python)

    ENDIF()

    include_directories (BEFORE
        ${COMMON_INCLUDE_DIRS}
    )

    add_subdirectory (${COMMON_INCLUDE_DIRS}/system ${CMAKE_CURRENT_BINARY_DIR}/common_system EXCLUDE_FROM_ALL)
    add_subdirectory (${COMMON_INCLUDE_DIRS}/image ${CMAKE_CURRENT_BINARY_DIR}/common_image EXCLUDE_FROM_ALL)

    add_subdirectory(drawstate)
    add_subdirectory(tool)
    if (WINDOWSYSTEM MATCHES "x11")
        add_subdirectory(pat_editor)
        add_subdirectory(pat_editor_gui)
    endif ()
endif ()

install(FILES ${PROJECT_SOURCE_DIR}/../../ARM_RELEASE_NOTE DESTINATION . OPTIONAL)

SET(CPACK_CMAKE_GENERATOR "Unix Makefiles")
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_PACKAGE_VERSION_MAJOR ${PATRACE_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${PATRACE_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PATRACE_VERSION_PATCH})
if (${PATRACE_VERSION_PATCH} EQUAL 0)
    SET(CPACK_PACKAGE_VERSION "r${PATRACE_VERSION_MAJOR}p${PATRACE_VERSION_MINOR}")
else ()
    SET(CPACK_PACKAGE_VERSION "r${PATRACE_VERSION_MAJOR}p${PATRACE_VERSION_MINOR}.${PATRACE_VERSION_PATCH}")
endif ()
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_FILE_NAME "patrace_${CPACK_PACKAGE_VERSION}_${CMAKE_SYSTEM_NAME}_${WINDOWSYSTEM}_${ARCH}")
SET(CPACK_SOURCE_GENERATOR "TGZ;TZ")
include(CPack)

# --- tracetooltests ---

if (ENABLE_TOOLS AND NOT FF_ONLY)
    set(CMAKE_SKIP_RPATH TRUE)
    set(NO_VULKAN "1")
    add_subdirectory(${THIRDPARTY_INCLUDE_DIRS}/tracetooltests ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()
