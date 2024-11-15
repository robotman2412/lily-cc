
cmake_minimum_required(VERSION 3.16.0)

add_library(c-frontend STATIC
    token/c_tokenizer.c
)
target_include_directories(c-frontend PUBLIC .)
target_link_libraries(c-frontend PUBLIC compiler-common)
