cmake_minimum_required(VERSION 3.16)

project(Kaleidoscope√A
    VERSION 0.0.1
    LANGUAGES CXX
    )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

file(GLOB SOURCES src/*.cpp)
# kra - Kaleidoscope Root A
add_executable(kra ${SOURCES})

target_include_directories(kra PRIVATE include/)
install(TARGETS kra)
