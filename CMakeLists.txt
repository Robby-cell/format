cmake_minimum_required(VERSION 3.28)
project(format VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(format INTERFACE)
target_include_directories(format INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(formatexe
    src/main.cc)

target_link_libraries(formatexe PRIVATE format)
