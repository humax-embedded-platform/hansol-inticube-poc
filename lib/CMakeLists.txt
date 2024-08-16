# lib/CMakeLists.txt

# Define the static library
add_library(mylib STATIC 
    buffer.c
    log.c
    worker.c
)

# Specify include directories for the library
target_include_directories(mylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
