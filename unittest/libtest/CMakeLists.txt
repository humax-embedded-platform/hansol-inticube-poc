cmake_minimum_required(VERSION 3.10)

# Find CMocka package
find_package(CMocka REQUIRED)

# Define the test executables
add_executable(test_buffer test_buffer.c)
add_executable(test_worker test_worker.c)
#add_executable(test_log test_log.c)

# Link CMocka
target_link_libraries(test_buffer PRIVATE cmocka::cmocka)
target_link_libraries(test_buffer PRIVATE httppostlib)
target_link_libraries(test_worker PRIVATE cmocka::cmocka)
target_link_libraries(test_worker PRIVATE httppostlib)
#target_link_libraries(test_log PRIVATE cmocka::cmocka)
#target_link_libraries(test_log PRIVATE httppostlib)

# Add include directories for header files
target_include_directories(test_buffer PRIVATE ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(test_worker PRIVATE ${PROJECT_SOURCE_DIR}/lib)
#target_include_directories(test_log PRIVATE ${PROJECT_SOURCE_DIR}/lib)

# Add tests
add_test(NAME test_buffer COMMAND test_buffer)
add_test(NAME test_worker COMMAND test_worker)
#add_test(NAME test_log COMMAND test_log)