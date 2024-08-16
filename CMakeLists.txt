cmake_minimum_required(VERSION 3.10)

# Project name and version
project(pov_postclient VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wno-unused-value -Wno-incompatible-pointer-types)
endif()


# Add subdirectories
add_subdirectory(lib)
add_subdirectory(httppostclient)
add_subdirectory(logservice)
