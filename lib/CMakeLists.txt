# lib/CMakeLists.txt

# Define the static library
add_library(httppostlib STATIC
    buffer.c
    log.c
    worker.c
)

# Specify include directories for the library
target_include_directories(httppostlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)

target_link_libraries(httppostlib PRIVATE ${CMAKE_THREAD_LIBS_INIT})
