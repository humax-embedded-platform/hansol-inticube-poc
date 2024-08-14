# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(pov_postclient VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wno-unused-value -Wno-incompatible-pointer-types)
endif()

# Add the executable target
add_executable(pov_postclient
    dbclient.c
    textdb.c
    httpclient.c
    httprequest.c
    linkedlist.c
    message.c
    sendworker.c
    recvworker.c
    worker.c
    main.c
)

target_include_directories(pov_postclient PRIVATE ${CMAKE_SOURCE_DIR})

target_compile_options(pov_postclient PRIVATE -Wall -Wextra)

