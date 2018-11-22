set (SRC_SYSTEM
    path.cpp
    environment_variable.cpp
)

add_library (common_system STATIC
    ${SRC_SYSTEM}
)
