project(BDD)

cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

SET( CMAKE_EXPORT_COMPILE_COMMANDS ON )
IF( EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
    EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
        )
ENDIF()

add_library(LBDD INTERFACE)
target_compile_features(LBDD INTERFACE cxx_std_17) 
target_compile_options(LBDD INTERFACE -march=native)
target_include_directories(LBDD INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()

add_subdirectory(src)
add_subdirectory(test)

