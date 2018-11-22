set(SRC_PAT_EDITOR
    ${SRC_ROOT}/tool/pat_editor/main.cpp
    ${SRC_ROOT}/tool/pat_editor/merge.cpp
    ${SRC_ROOT}/tool/pat_editor/extract.cpp
    ${SRC_ROOT}/tool/pat_editor/commonData.cpp
    ${SRC_ROOT}/common/trace_model.cpp
    ${SRC_ROOT}/common/call_parser.cpp
	${SRC_ROOT}/common/api_info.cpp
)

set_source_files_properties (
    ${SRC_ROOT}/common/call_parser.cpp
    PROPERTIES
        GENERATED True
)

include_directories(
	${SRC_ROOT}
	${SRC_ROOT}/common
)

add_executable(pat_editor
	${SRC_PAT_EDITOR}
)

target_link_libraries (pat_editor
    common_eglstate
    common_image
    common_system
    common
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SNAPPY_LIBRARIES}
    jsoncpp
)
add_dependencies (pat_editor call_parser_src_generation)
install (TARGETS pat_editor DESTINATION tools)
