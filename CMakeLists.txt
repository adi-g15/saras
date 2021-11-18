cmake_minimum_required(VERSION 3.16)

project(Kaleidoscope√A
    VERSION 0.0.1
    LANGUAGES CXX
    )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

file(GLOB SOURCES src/*.cpp)
# kra - Kaleidoscope Root A
add_executable(kra ${SOURCES})

include_directories(include)
install(TARGETS kra)
