# httppostclient/CMakeLists.txt

# Create an executable from source files

add_executable(httppostclient
    main.c
    dbclient.c
    httpclient.c
    httprequest.c
    linkedlist.c
    message.c
    recvworker.c
    sendworker.c
    textdb.c
)

# Link the static library to the executable
target_link_libraries(httppostclient PRIVATE httppostlib)

# Include directories for the executable
target_include_directories(httppostclient PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(httppostclient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
