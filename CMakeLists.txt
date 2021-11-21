cmake_minimum_required(VERSION 3.16)

project(saras
    VERSION 0.0.1
    LANGUAGES CXX
    )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(CMAKE_GENERATOR STREQUAL "Ninja")
    message("Enabling color in Ninja color")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
endif()

include(cmake/CPM.cmake)

CPMAddPackage("gh:p-ranav/tabulate@1.4")
add_compile_definitions("DEBUG")

# Hardcode all builds to be debug build
set(CMAKE_BUILD_TYPE "Debug")

file(GLOB SOURCES src/*.cpp)
add_executable(saras ${SOURCES})
target_link_libraries(saras tabulate)

include_directories(include)
install(TARGETS saras)
