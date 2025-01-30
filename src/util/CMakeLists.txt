
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

add_library(util STATIC
    arrays.c
    char_repr.c
    list.c
    map.c
    refcount.c
    strong_malloc.c
    utf8.c
)
target_include_directories(util PUBLIC .)
