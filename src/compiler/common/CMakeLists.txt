
add_library(compiler-common STATIC
    token/tokenizer.c
    frontend.c
)
target_include_directories(compiler-common PUBLIC .)
target_link_libraries(compiler-common PUBLIC util)
