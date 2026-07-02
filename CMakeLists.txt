cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(APP_NAME amaze)
project(${APP_NAME})
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(SFML COMPONENTS Graphics Window System Audio REQUIRED)

include(FetchContent)
find_package(SFML 3.0 COMPONENTS System Window Graphics REQUIRED)
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

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(${APP_NAME}
    src/configreader.cpp
    src/controller.cpp
    src/gamemodel.cpp
    src/gameshape.cpp
    src/gamepad.cpp
    src/main.cpp
    src/scheduler.cpp
    src/sfmladapter.cpp
    src/shape.cpp
    src/shipmodel.cpp
    src/utils.cpp
    src/view.cpp
    src/imgui.cpp
)

# ---------------------------------------------------------------------------
# macOS app bundle (Darwin only)
# ---------------------------------------------------------------------------
if(APPLE)
    find_program(DYLIBBUNDLER dylibbundler REQUIRED)

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
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/amaze.cfg
            $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Resources/amaze.cfg
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data
            $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Resources/data
        COMMAND ${CMAKE_COMMAND}
            -DBINARY=$<TARGET_FILE:${APP_NAME}>
            -DFRAMEWORKS=$<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME}>/Frameworks
            -P "${CMAKE_BINARY_DIR}/dylibbundler.cmake"
        COMMENT "Bundling app"
    )
endif()

target_compile_options(${APP_NAME} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)
target_link_libraries(${APP_NAME} PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SDL3::SDL3
    ImGui-SFML::ImGui-SFML
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