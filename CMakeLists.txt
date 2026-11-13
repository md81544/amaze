cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(APP_NAME amaze)
project(${APP_NAME})
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt for graphics, sound and UI; SDL for gamepad support
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets OpenGLWidgets Multimedia)
find_package(SDL3 REQUIRED)

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(${APP_NAME}
    src/audio.cpp
    src/configreader.cpp
    src/controller.cpp
    src/gamemodel.cpp
    src/gamepad.cpp
    src/gameshape.cpp
    src/gameshell.cpp
    src/gamewidget.cpp
    src/levelfilelister.cpp
    src/main.cpp
    src/menudialog.cpp
    src/scheduler.cpp
    src/shape.cpp
    src/shipmodel.cpp
    src/utils.cpp
    src/view.cpp
)


# ---------------------------------------------------------------------------
# macOS app bundle (Darwin only)
# ---------------------------------------------------------------------------
if(APPLE)
    set_target_properties(${APP_NAME} PROPERTIES
        MACOSX_BUNDLE                      TRUE
        MACOSX_BUNDLE_BUNDLE_NAME          "${APP_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER       "com.example.${APP_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION       "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_ICON_FILE            "amaze.icns"
    )

    target_sources(${APP_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/amaze.icns)
    set_source_files_properties(${CMAKE_SOURCE_DIR}/amaze.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/amaze.cfg
            $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Resources/amaze.cfg
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data
            $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Resources/data
        COMMENT "Copying resources into app bundle"
    )

    # macdeployqt copies Qt (including its plugins) and other dylibs into the
    # bundle. It's slow, so only do it for release builds; debug builds run
    # against the installed libraries.
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        find_program(MACDEPLOYQT macdeployqt REQUIRED
            HINTS "${Qt6_DIR}/../../../bin")
        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${MACDEPLOYQT} $<TARGET_BUNDLE_DIR:${APP_NAME}>
            COMMENT "Bundling app"
        )
    endif()
endif()

target_compile_options(${APP_NAME} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -ffast-math
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)
target_link_libraries(${APP_NAME} PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Multimedia
    SDL3::SDL3
)

# Symlink to the app bundle (macOS) or binary (other platforms)
add_custom_command(
    TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<IF:$<BOOL:${APPLE}>,$<TARGET_BUNDLE_DIR:${APP_NAME}>,$<TARGET_FILE:${APP_NAME}>>
        ${CMAKE_SOURCE_DIR}/$<IF:$<BOOL:${APPLE}>,$<TARGET_BUNDLE_DIR_NAME:${APP_NAME}>,$<TARGET_FILE_NAME:${APP_NAME}>>
    COMMENT "Updating symlink in source dir"
)
add_custom_command(
    TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Updating symlink: compile_commands.json"
)