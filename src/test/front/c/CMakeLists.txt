
cmake_minimum_required(VERSION 3.16.0)

add_library(c-frontend-test STATIC
    c_parser.c
    c_tokenizer.c
)
target_link_libraries(c-frontend-test PRIVATE test-common c-frontend)
