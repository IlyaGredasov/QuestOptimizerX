cmake_minimum_required(VERSION 3.10)
project(quest_optimizer_x)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CLI11 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

file(GLOB quest_optimizer_core_sources ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM quest_optimizer_core_sources
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/quest_optimizer_tests.cpp
)
add_library(quest_optimizer_core STATIC ${quest_optimizer_core_sources})
target_include_directories(quest_optimizer_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(quest_optimizer_core PRIVATE CLI11::CLI11 yaml-cpp::yaml-cpp)

add_executable(quest_optimizer_x ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(quest_optimizer_x PRIVATE quest_optimizer_core)

option(QO_BUILD_TESTS "Build Tests" OFF)
if(QO_BUILD_TESTS)
    find_package(GTest REQUIRED)
    
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
    add_executable(quest_optimizer_tests ${CMAKE_SOURCE_DIR}/src/quest_optimizer_tests.cpp)
    target_include_directories(quest_optimizer_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
    
    target_link_libraries(quest_optimizer_tests PRIVATE
        quest_optimizer_core
        GTest::gtest_main
        GTest::gtest
        yaml-cpp::yaml-cpp
    )

    enable_testing()
    add_test(NAME QuestOptimizerTests COMMAND quest_optimizer_tests)
    set_tests_properties(QuestOptimizerTests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
