include(src.cmake)

include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/../../common
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
    common_eglstate
    collector
    ${SNAPPY_LIBRARIES}
    md5
    ${LIBRARY_FASTFORWARD_SYSTEM}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    jsoncpp
    hwcpipe
)

set(FASTFORWARD_DEPENDENCIES
    retrace_gles_auto_src_generation
    eglproc_auto_src_generation
    glxml_header
)
if(TARGET call_parser_src_generation)
    list(APPEND FASTFORWARD_DEPENDENCIES call_parser_src_generation)
endif()

add_dependencies(fastforward ${FASTFORWARD_DEPENDENCIES})

set_target_properties(fastforward PROPERTIES LINK_FLAGS "-pthread -z max-page-size=16384" COMPILE_FLAGS "-pthread")
install(TARGETS fastforward
    RUNTIME DESTINATION bin)
