cmake_minimum_required(VERSION 3.8)
project(example)

set(CMAKE_CXX_STANDARD 11)

include_directories(../src/)

set(SOURCE_FILES main.cpp)
add_executable(example ${SOURCE_FILES})