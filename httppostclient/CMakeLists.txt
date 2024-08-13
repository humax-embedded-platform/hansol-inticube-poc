# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(pov_postclient VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Add the executable target
add_executable(pov_postclient
    dbclient.c
    textdb.c
    main.c
)

target_include_directories(pov_postclient PRIVATE ${CMAKE_SOURCE_DIR})

target_compile_options(pov_postclient PRIVATE -Wall -Wextra)

