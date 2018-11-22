
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions (-DHAVE_CONFIG_H)
else ()
    add_definitions (-DHAVE_CONFIG_H -fPIC)
endif ()

# Adjust warnings
if (CMAKE_COMPILER_IS_GNUCXX)
    add_definitions (-Wno-unused-function)
endif ()

set (SNAPPY_LIBRARIES snappy_bundled PARENT_SCOPE)
add_library (snappy_bundled STATIC
    snappy.cc
    snappy-sinksource.cc
    snappy-stubs-internal.cc
    snappy-c.cc
)

set_target_properties (${SNAPPY_LIBRARIES} PROPERTIES
    COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_CXX_FLAGS}"
)
