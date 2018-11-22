include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library (jsoncpp STATIC
    src/lib_json/json_writer.cpp
    src/lib_json/json_reader.cpp
    src/lib_json/json_value.cpp
)
