
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

add_library(compiler-common STATIC
    compiler.c
    tokenizer.c
)
target_include_directories(compiler-common PUBLIC .)
target_link_libraries(compiler-common PUBLIC util)
