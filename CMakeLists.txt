cmake_minimum_required(VERSION 3.10)
project(quest_optimizer_x)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CLI11 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(quest_optimizer_x ${SOURCES})
target_include_directories(quest_optimizer_x PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(quest_optimizer_x PRIVATE CLI11::CLI11 yaml-cpp::yaml-cpp)
