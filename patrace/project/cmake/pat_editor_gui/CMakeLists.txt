cmake_minimum_required(VERSION 2.8.11)

set(SRC_PAT_EDITOR_GUI
    ${SRC_ROOT}/tool/pat_editor/merge.cpp
    ${SRC_ROOT}/tool/pat_editor/extract.cpp
    ${SRC_ROOT}/tool/pat_editor/commonData.cpp
    ${SRC_ROOT}/tool/pat_editor_gui/dialog.cpp
    ${SRC_ROOT}/tool/pat_editor_gui/helpdialog.cpp
    ${SRC_ROOT}/tool/pat_editor_gui/mainwindow.cpp
    ${SRC_ROOT}/tool/pat_editor_gui/main.cpp
    ${SRC_ROOT}/tool/pat_editor_gui/open_thread.cpp
    ${SRC_ROOT}/tool/pat_editor_gui/percentage_thread.cpp
    ${SRC_ROOT}/tool/pat_editor_gui/icon.qrc
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

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5Widgets)

# Tell CMake to create the pat_editor_gui executable and add the icon to GUI
SET(pat_editor_gui_UIS
    ${SRC_ROOT}/tool/pat_editor_gui/mainwindow.ui
    ${SRC_ROOT}/tool/pat_editor_gui/dialog.ui
    ${SRC_ROOT}/tool/pat_editor_gui/helpdialog.ui
)
qt5_wrap_ui(pat_editor_gui_UIS_UIC ${pat_editor_gui_UIS})
SET(pat_editor_gui_RESOURCE ${SRC_ROOT}/tool/pat_editor_gui/icon.qrc)
qt5_add_resources(pat_editor_gui_RESOURCE_RCC ${pat_editor_gui_RESOURCE})
add_executable(pat_editor_gui ${SRC_PAT_EDITOR_GUI} ${pat_editor_gui_UIS_UIC} ${pat_editor_gui_RESOURCE_RCC})

# Use the Widgets module from Qt 5.
target_link_libraries (pat_editor_gui
    Qt5::Widgets
    common_eglstate
    common_image
    common_system
    common
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SNAPPY_LIBRARIES}
    jsoncpp
)

add_dependencies (pat_editor_gui call_parser_src_generation)

install (TARGETS pat_editor_gui DESTINATION tools)
