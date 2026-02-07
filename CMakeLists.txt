cmake_minimum_required(VERSION 3.10)
project(quest_optimizer_x)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(quest_optimizer_x ${SOURCES})
