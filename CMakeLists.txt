cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_COMPILER clang++)

project(RomeSocket LANGUAGES CXX)

add_executable(romesocket
    src/iouring_backend.cpp
    src/rome_server.cpp
    src/utils.cpp
    test/echo_server.cpp
)

target_include_directories(
    romesocket
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(romesocket PRIVATE cxx_std_20)
target_link_libraries(romesocket PRIVATE uring)