project(BDD)

cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_CXX_FLAGS "-std=c++17")
add_definitions(-march=native)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

SET( CMAKE_EXPORT_COMPILE_COMMANDS ON )
IF( EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
    EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
        )
ENDIF()


include_directories(${CMAKE_SOURCE_DIR}/include)

enable_testing()

add_subdirectory(src)
add_subdirectory(test)

