cmake_minimum_required(VERSION 3.23)
project(project4)

set(CMAKE_CXX_STANDARD 11)

add_executable(project4 main.cpp Instruction.h Hardware.h Instruction.cpp Hardware.cpp Ali.h Ali.cpp Subclass.h Subclass.cpp)
