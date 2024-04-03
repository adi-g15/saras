cmake_minimum_required(VERSION 3.16)

project(saras
    VERSION 0.0.1
    LANGUAGES CXX C
    )

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(CMAKE_GENERATOR STREQUAL "Ninja")
    message("Enabling color in Ninja color")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake")
include(cmake/CPM.cmake)

CPMAddPackage("gh:adi-g15/rang@3.2")
CPMAddPackage("gh:p-ranav/tabulate@1.4")
CPMAddPackage(
  GITHUB_REPOSITORY "jarro2783/cxxopts"
  VERSION 3.0.0
  OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL NO"
)

add_compile_definitions("DEBUG")

find_package(LLVM REQUIRED)

# Hardcode all builds to be debug build
set(CMAKE_BUILD_TYPE "Debug")

file(GLOB SOURCES src/*.cpp)
add_executable(saras ${SOURCES})
target_link_libraries(saras tabulate rang cxxopts LLVM)

target_include_directories(saras PRIVATE ${LLVM_INCLUDE_DIRS})

include_directories(include)
install(TARGETS saras)
