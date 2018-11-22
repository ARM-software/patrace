include(src.cmake)
include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/common
    ${SRC_ROOT}/dispatch
    ${THIRDPARTY}
    ${THIRDPARTY}/libcollector
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DRETRACE -DGLES_CALLCONVENTION=_stdcall)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DRETRACE -DGLES_CALLCONVENTION= -std=gnu++0x)
endif ()

add_executable(paretrace
    ${SRC_EGLRETRACE}
)

if (${WINDOWSYSTEM} MATCHES "x11")
    message(STATUS "link with x11: " ${X11_X11_LIB})
    target_link_libraries (paretrace
        ${X11_X11_LIB}
    )
endif()

if (${WINDOWSYSTEM} MATCHES "udriver")
   message(STATUS "windowsystem set as UDriver-WindowSystem")
   target_link_libraries (paretrace
       udriver_winsys
   )
endif()

if (${WINDOWSYSTEM} MATCHES "wayland")
    message(STATUS "link with wayland: " ${wayland_LIBRARIES})
    target_compile_options(paretrace
        PRIVATE
        ${wayland_CFLAGS}
    )
    target_link_libraries (paretrace
        ${wayland_LDFLAGS}
    )
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set (LIBRARY_EGLRETRACE_SYSTEM
        Opengl32
    )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (LIBRARY_EGLRETRACE_SYSTEM
        dl
        rt
    )
endif ()

target_link_libraries(paretrace
    common
    ${SNAPPY_LIBRARIES}
    md5
    dl
    ${LIBRARY_EGLRETRACE_SYSTEM}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    jsoncpp
    ${CMAKE_CURRENT_BINARY_DIR}/../libcollector/libcollector.a
)
set_target_properties(paretrace PROPERTIES LINK_FLAGS "-pthread" COMPILE_FLAGS "-pthread")
add_dependencies(paretrace
    retrace_gles_auto_src_generation
    eglproc_auto_src_generation
    glxml_header
)

install(TARGETS paretrace RUNTIME DESTINATION bin)
install(PROGRAMS eglretrace DESTINATION bin)
