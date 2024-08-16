# logservice/CMakeLists.txt

# Create an executable from source files
add_executable(logservice
    main.c
    logrecv.c
    logwriter.c
)

# Link the static library to the executable
target_link_libraries(logservice PRIVATE mylib)

# Include directories for the executable
target_include_directories(logservice PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(logservice PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
