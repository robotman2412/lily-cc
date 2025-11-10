
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

add_library(util STATIC
    arith128.c
    arrays.c
    char_repr.c
    color.c
    hash.c
    list.c
    map.c
    refcount.c
    set.c
    strong_malloc.c
    unreachable.c
    utf8.c
)
target_include_directories(util PUBLIC .)
