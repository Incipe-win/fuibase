cmake_minimum_required(VERSION 3.20)
project(fuibase VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only library
add_library(fuibase INTERFACE)
target_include_directories(fuibase INTERFACE include)

# Demo
add_executable(fuibase-demo examples/demo.cpp)
target_link_libraries(fuibase-demo fuibase)
