
add_library(compiler-common STATIC
    token/tokenizer.c
    frontend.c
)
target_include_directories(compiler-common PUBLIC .)
