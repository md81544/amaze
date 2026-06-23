cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(APP_NAME amaze)

project(${APP_NAME})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER clang++)

find_package(SDL3 REQUIRED)

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(${APP_NAME}
    configreader.cpp
    controller.cpp
    gamemodel.cpp
    gameshape.cpp
    gamepad.cpp
    main.cpp
    menu.cpp
    scheduler.cpp
    sfmladapter.cpp
    shape.cpp
    shipmodel.cpp
    utils.cpp
    view.cpp
)

target_compile_options(${APP_NAME} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

target_link_libraries(${APP_NAME} PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
    SDL3::SDL3
)

add_custom_command(
    TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE:${APP_NAME}>
        ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:${APP_NAME}>
    COMMENT "Updating symlink: $<TARGET_FILE_NAME:${APP_NAME}> -> $<TARGET_FILE:${APP_NAME}>"
)

add_custom_command(
    TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Updating symlink: compile_commands.json"
)


