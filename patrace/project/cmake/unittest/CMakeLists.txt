add_custom_target (copy_unittest_data ALL)
add_custom_command (
    TARGET copy_unittest_data
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data
)

include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/tool
    ${THIRDPARTY_INCLUDE_DIRS}/cppunit/include
)

include(src.cmake)
add_executable(testharness
    ${SRC_UNITTEST}
)

set (APP_LIBS
    common_eglstate
    common_image
    common_system
    tiff
    pthread
    z_bundled
)

target_link_libraries(testharness
    ${CPPUNITLIB}
    ${APP_LIBS}
    common 
    md5
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
)
