cmake_minimum_required(VERSION 3.15)
project(TP3_SE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(.)

add_executable(TP3
        block.cpp
        block.h
        disqueVirtuel.cpp
        disqueVirtuel.h
        main.cpp)