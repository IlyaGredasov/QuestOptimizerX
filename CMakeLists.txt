cmake_minimum_required(VERSION 3.28)
project(QuestOptimizerX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(QuestOptimizerX ${SOURCES})

if (WIN32)
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(RUNTIME_DLLS
            "libstdc++-6.dll"
            "libwinpthread-1.dll"
    )
    foreach (dll ${RUNTIME_DLLS})
        add_custom_command(TARGET QuestOptimizerX POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${COMPILER_DIR}/${dll}"
                $<TARGET_FILE_DIR:QuestOptimizerX>
        )
    endforeach ()
endif ()