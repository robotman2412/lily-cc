
cmake_minimum_required(VERSION 3.16.0)

add_library(compiler-common-test STATIC
    compiler.c
)
target_link_libraries(compiler-common-test PRIVATE test-common compiler-common)
