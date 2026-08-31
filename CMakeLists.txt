cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_COMPILER clang++)

project(RomeSocket LANGUAGES CXX)

set(ROME_SOURCES
    src/rome_server.cpp
    src/iouring_backend.cpp
    src/utils.cpp
)

set(ROME_HEADERS
    include/rome_server.hpp
    include/connection.hpp
    include/log.hpp
)

add_library(romesocket
    ${ROME_SOURCES}
    ${ROME_HEADERS}
)

target_include_directories(
    romesocket
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(romesocket PRIVATE cxx_std_20)
target_link_libraries(romesocket PRIVATE uring)

## Echo test server

add_executable(echo_server
    test/echo_test/echo_server.cpp
)

target_link_libraries(echo_server
    PRIVATE
        romesocket
)

target_include_directories(echo_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)