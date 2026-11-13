cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(APP_NAME amaze)
project(${APP_NAME})
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Graphics backend. SFML is being phased out in favour of Qt; it's kept
# selectable for now to allow comparison.
set(AMAZE_BACKEND "Qt" CACHE STRING "Graphics backend: Qt or SFML")
set_property(CACHE AMAZE_BACKEND PROPERTY STRINGS Qt SFML)
if(NOT AMAZE_BACKEND MATCHES "^(Qt|SFML)$")
    message(FATAL_ERROR "AMAZE_BACKEND must be Qt or SFML")
endif()
message(STATUS "Graphics backend: ${AMAZE_BACKEND}")

# SDL is used for gamepad support with either backend
find_package(SDL3 REQUIRED)

if(AMAZE_BACKEND STREQUAL "Qt")
    find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets OpenGLWidgets Multimedia)
else()
    find_package(SFML 3.0 COMPONENTS Graphics Window System Audio REQUIRED)

    include(FetchContent)
    FetchContent_Declare(ImGui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.91.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(ImGui)
    FetchContent_GetProperties(ImGui SOURCE_DIR IMGUI_DIR)
    set(IMGUI_SFML_FIND_SFML OFF)
    FetchContent_Declare(ImGui-SFML
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml
        GIT_TAG v3.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(ImGui-SFML)

    # Silence warnings produced while compiling the fetched dependencies themselves.
    # SYSTEM (above) only suppresses warnings when *we* include their headers;
    # it doesn't touch the compilation of their own .cpp files.
    foreach(dep_target IN ITEMS imgui ImGui-SFML)
        if(TARGET ${dep_target})
            target_compile_options(${dep_target} PRIVATE -w)
        endif()
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(${APP_NAME}
    src/configreader.cpp
    src/controller.cpp
    src/gamemodel.cpp
    src/gameshape.cpp
    src/gamepad.cpp
    src/levelfilelister.cpp
    src/main.cpp
    src/scheduler.cpp
    src/shape.cpp
    src/shipmodel.cpp
    src/utils.cpp
    src/view.cpp
)

if(AMAZE_BACKEND STREQUAL "Qt")
    target_sources(${APP_NAME} PRIVATE
        src/qtadapter.cpp
        src/qtgamewidget.cpp
        src/qtmenudialog.cpp
    )
    target_compile_definitions(${APP_NAME} PRIVATE AMAZE_QT)
    target_link_libraries(${APP_NAME} PRIVATE
        Qt6::Widgets
        Qt6::OpenGLWidgets
        Qt6::Multimedia
    )
else()
    target_sources(${APP_NAME} PRIVATE
        src/imgui.cpp
        src/sfmladapter.cpp
    )
    target_link_libraries(${APP_NAME} PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
        ImGui-SFML::ImGui-SFML
    )
endif()

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

    if(AMAZE_BACKEND STREQUAL "Qt")
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
    else()
        find_program(DYLIBBUNDLER dylibbundler REQUIRED)

        # Write a quiet wrapper for dylibbundler at configure time
        file(WRITE "${CMAKE_BINARY_DIR}/dylibbundler.cmake" "
            execute_process(
                COMMAND ${DYLIBBUNDLER}
                    -od -b
                    -x \${BINARY}
                    -d \${FRAMEWORKS}
                    -p @executable_path/../Frameworks
                OUTPUT_QUIET
                ERROR_QUIET
            )
        ")

        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DBINARY=$<TARGET_FILE:${APP_NAME}>
                -DFRAMEWORKS=$<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Frameworks
                -P "${CMAKE_BINARY_DIR}/dylibbundler.cmake"
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