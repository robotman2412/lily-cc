
cmake_minimum_required(VERSION 3.16.0)

add_library(test-common STATIC
    testcase.c
)
target_include_directories(test-common PUBLIC .)
target_link_libraries(test-common PUBLIC util)
