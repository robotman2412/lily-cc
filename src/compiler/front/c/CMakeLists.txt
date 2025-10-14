
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

add_library(c-frontend STATIC
    c_prepass.c
    c_compiler.c
    c_parser.c
    c_tokenizer.c
    c_types.c
    c_values.c
)
target_include_directories(c-frontend PUBLIC .)
target_link_libraries(c-frontend PUBLIC compiler-common)
