cmake_minimum_required(VERSION 3.16)

project(Kaleidoscope√A
    VERSION 0.0.1
    LANGUAGES CXX
    )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(cmake/CPM.cmake)

CPMAddPackage("gh:p-ranav/tabulate@1.4")
add_compile_definitions("DEBUG")

# Hardcode all builds to be debug build
set(CMAKE_BUILD_TYPE "Debug")

file(GLOB SOURCES src/*.cpp)
# kra - Kaleidoscope Root A
add_executable(kra ${SOURCES})
target_link_libraries(kra tabulate)

include_directories(include)
install(TARGETS kra)
