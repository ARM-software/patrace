include(src.cmake)
include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/common
    ${SRC_ROOT}/dispatch
)

add_executable(fastforward
    ${SRC_FASTFORWARD}
)

target_compile_definitions(fastforward PRIVATE RETRACE GLES_CALLCONVENTION=  TOOL_BUILD)

if (${WINDOWSYSTEM} MATCHES "x11")
    message(STATUS "link with x11: " ${X11_X11_LIB})
    target_link_libraries (fastforward
        ${X11_X11_LIB}
    )
endif()

if (${WINDOWSYSTEM} MATCHES "udriver")
   message(STATUS "link with UDriver-WindowSystem")
   target_link_libraries (fastforward
        udriver_winsys
   )
endif()

if (${WINDOWSYSTEM} MATCHES "wayland")
    message(STATUS "link with wayland: " ${wayland_LIBRARIES})
    target_compile_options(fastforward
        PRIVATE
        ${wayland_CFLAGS}
    )
    target_link_libraries (fastforward
        ${wayland_LDFLAGS}
    )
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (LIBRARY_FASTFORWARD_SYSTEM
        dl
        rt
    )
endif ()

target_link_libraries (fastforward
    dl
    common
    ${CMAKE_BINARY_DIR}/libcollector/libcollector.a
    ${SNAPPY_LIBRARIES}
    md5
    ${LIBRARY_FASTFORWARD_SYSTEM}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    jsoncpp
    hwcpipe
)
add_dependencies(fastforward
    retrace_gles_auto_src_generation
    eglproc_auto_src_generation
    glxml_header
)
set_target_properties(fastforward PROPERTIES LINK_FLAGS "-pthread" COMPILE_FLAGS "-pthread")
install(TARGETS fastforward
    RUNTIME DESTINATION bin)
