cmake_minimum_required(VERSION 3.13)

project (amaze)

add_executable(amaze
    configreader.cpp
    controller.cpp
    gamemodel.cpp
    gameshape.cpp
    helperfunctions.cpp
    log.cpp
    main.cpp
    scheduler.cpp
    sfmladapter.cpp
    shape.cpp
    shipmodel.cpp
    view.cpp
)

target_link_libraries(amaze PRIVATE
    #sfml-graphics
    #sfml-window
    #sfml-system
    #sfml-audio
    # Temporary location of SFML libs I've manually fixed until 2.6.3 comes out
    # correcting the issue in my latest bug report
    ${CMAKE_SOURCE_DIR}/tmplibs/libsfml-graphics.2.6.2.dylib
    ${CMAKE_SOURCE_DIR}/tmplibs/libsfml-window.2.6.2.dylib
    ${CMAKE_SOURCE_DIR}/tmplibs/libsfml-system.2.6.2.dylib
    ${CMAKE_SOURCE_DIR}/tmplibs/libsfml-audio.2.6.2.dylib

)

target_compile_options(amaze PRIVATE -std=c++2b -Wall -Wextra -Werror -Wpedantic)


ADD_CUSTOM_TARGET(debug
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
    COMMENT "Switch CMAKE_BUILD_TYPE to Debug"
)

ADD_CUSTOM_TARGET(release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
    COMMENT "Switch CMAKE_BUILD_TYPE to Release"
)
