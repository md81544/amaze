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
    src/levelfile.cpp
    src/levelformat.cpp
    src/levelfilelister.cpp
    src/main.cpp
    src/menudialog.cpp
    src/scheduler.cpp
    src/settings.cpp
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

    # The resources are copied after the app is linked, so relink it whenever
    # they change; otherwise the bundle keeps stale copies of them
    file(GLOB_RECURSE BUNDLED_DATA CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/data/*)
    set_property(TARGET ${APP_NAME} APPEND PROPERTY
        LINK_DEPENDS ${BUNDLED_DATA} ${CMAKE_SOURCE_DIR}/amaze.cfg)
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/amaze.cfg
            $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Resources/amaze.cfg
        # Start afresh, so files removed from data don't linger in the bundle
        COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Resources/data
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data
            $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Resources/data
        COMMENT "Copying resources into app bundle"
    )

    # macdeployqt copies Qt and other dylibs into the bundle. It's slow, so
    # only do it for release builds; debug builds run against the installed
    # libraries.
    #
    # By default macdeployqt copies every plugin for the Qt modules used, many
    # of which we don't need (and some of which it can't deploy cleanly), so
    # we copy in just the plugins we need, and have macdeployqt deploy their
    # dependencies too.
    #
    # macdeployqt reports 'Cannot resolve rpath "@rpath/libbrotlicommon.1.dylib"'
    # (used by libbrotlidec, used by FreeType). This is harmless: it only checks
    # libbrotlidec's own rpaths, whereas at run time the dynamic linker also
    # uses QtGui's, which point at the bundle's Frameworks directory.
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        find_program(MACDEPLOYQT macdeployqt REQUIRED
            HINTS "${Qt6_DIR}/../../../bin")
        # Pairs of plugin type (directory) and plugin target
        set(BUNDLED_QT_PLUGINS
            platforms Qt6::QCocoaIntegrationPlugin
            styles Qt6::QMacStylePlugin
            multimedia Qt6::QDarwinMediaPlugin
        )
        set(PLUGINS_DIR $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/PlugIns)
        set(COPY_PLUGINS)
        set(DEPLOY_PLUGINS)
        list(LENGTH BUNDLED_QT_PLUGINS count)
        math(EXPR last "${count} - 1")
        foreach(i RANGE 0 ${last} 2)
            math(EXPR j "${i} + 1")
            list(GET BUNDLED_QT_PLUGINS ${i} type)
            list(GET BUNDLED_QT_PLUGINS ${j} plugin)
            list(APPEND COPY_PLUGINS
                COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUGINS_DIR}/${type}
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${plugin}> ${PLUGINS_DIR}/${type}/
            )
            list(APPEND DEPLOY_PLUGINS
                -executable=${PLUGINS_DIR}/${type}/$<TARGET_FILE_NAME:${plugin}>
            )
        endforeach()
        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            # Start afresh, so nothing is left over from a previous deployment
            COMMAND ${CMAKE_COMMAND} -E rm -rf
                ${PLUGINS_DIR} $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Frameworks
            ${COPY_PLUGINS}
            COMMAND ${MACDEPLOYQT} $<TARGET_BUNDLE_DIR:${APP_NAME}> -no-plugins ${DEPLOY_PLUGINS}
            COMMENT "Bundling app"
        )
    endif()
endif()

set(AMAZE_COMPILE_OPTIONS
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -ffast-math
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)
target_compile_options(${APP_NAME} PRIVATE ${AMAZE_COMPILE_OPTIONS})
target_link_libraries(${APP_NAME} PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Multimedia
    SDL3::SDL3
)

# ---------------------------------------------------------------------------
# Unit tests (built if Catch2 is available; run with "make test")
# ---------------------------------------------------------------------------
find_package(Catch2 3 QUIET)
if(Catch2_FOUND)
    add_executable(${APP_NAME}_tests
        tests/shape_tests.cpp
        tests/levelfile_tests.cpp
        tests/levelformat_tests.cpp
        tests/viewtransform_tests.cpp
        tests/bounce_tests.cpp
        src/gamemodel.cpp
        src/gameshape.cpp
        src/levelfile.cpp
        src/levelfilelister.cpp
        src/levelformat.cpp
        src/scheduler.cpp
        src/shape.cpp
        src/shipmodel.cpp
        src/utils.cpp
    )
    target_include_directories(${APP_NAME}_tests PRIVATE src)
    target_compile_definitions(${APP_NAME}_tests PRIVATE AMAZE_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
    # Same options as the game, so the code under test behaves the same
    target_compile_options(${APP_NAME}_tests PRIVATE ${AMAZE_COMPILE_OPTIONS})
    # This file (shared with level_designer) uses brief initialisers, relying
    # on fields' defaults
    set_source_files_properties(tests/levelformat_tests.cpp PROPERTIES
        COMPILE_OPTIONS -Wno-missing-field-initializers)
    target_link_libraries(${APP_NAME}_tests PRIVATE Catch2::Catch2WithMain Qt6::Gui)
else()
    message(STATUS "Catch2 not found: unit tests won't be built")
endif()

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