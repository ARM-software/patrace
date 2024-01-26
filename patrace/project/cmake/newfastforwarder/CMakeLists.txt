include(src.cmake)
include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/common
    ${SRC_ROOT}/dispatch
)

add_executable(newfastforwarder
    ${SRC_NEWFASTFORWARDER}
)

if (${WINDOWSYSTEM} MATCHES "x11")
    message(STATUS "link with x11: " ${X11_X11_LIB})
    target_link_libraries (newfastforwarder
        ${X11_X11_LIB}
    )
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (LIBRARY_NEWFASTFORWARDER_SYSTEM
        dl
        rt
    )
endif ()

target_link_libraries(newfastforwarder
    common
    ${SNAPPY_LIBRARIES}
    ${LIBRARY_NEWFASTFORWARDER_SYSTEM}
    jsoncpp
)
set_target_properties(newfastforwarder PROPERTIES LINK_FLAGS "-pthread" COMPILE_FLAGS "-pthread")
add_dependencies(newfastforwarder
    retrace_gles_auto_src_generation
    eglproc_auto_src_generation
    glxml_header
)

install(TARGETS newfastforwarder RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
