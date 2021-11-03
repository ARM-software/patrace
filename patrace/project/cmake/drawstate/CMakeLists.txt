include(src.cmake)
include_directories(
    ${COMMON_INCLUDE_DIRS}
    ${SRC_ROOT}
    ${SRC_ROOT}/common
    ${SRC_ROOT}/dispatch
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DRETRACE -DGLES_CALLCONVENTION=)
endif ()

add_executable(drawstate ${SRC_MM})

if (${WINDOWSYSTEM} MATCHES "x11")
    message(STATUS "link with x11: " ${X11_X11_LIB})
    target_link_libraries(drawstate ${X11_X11_LIB})
endif()

if (${WINDOWSYSTEM} MATCHES "wayland")
    message(STATUS "link with wayland: " ${wayland_LIBRARIES})
    target_compile_options(drawstate PRIVATE ${wayland_CFLAGS})
    target_link_libraries(drawstate ${wayland_LDFLAGS})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LIBRARY_MM_SYSTEM dl rt)
endif ()

target_link_libraries(drawstate
    dl
    common
    common_eglstate
    collector
    ${SNAPPY_LIBRARIES}
    md5
    ${LIBRARY_MM_SYSTEM}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    jsoncpp
    hwcpipe
)
add_dependencies(drawstate
    retrace_gles_auto_src_generation
    eglproc_auto_src_generation
)
set_target_properties(drawstate PROPERTIES LINK_FLAGS "-pthread" COMPILE_FLAGS "-pthread")
install(TARGETS drawstate
    RUNTIME DESTINATION bin)
