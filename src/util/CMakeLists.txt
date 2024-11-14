
add_library(util STATIC
    arrays.c
    list.c
    map.c
    refcount.c
    strong_malloc.c
    utf8.c
)
target_include_directories(util PUBLIC .)
