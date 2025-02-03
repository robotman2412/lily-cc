
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

add_library(c-frontend-test STATIC
    c_compiler_test.c
    c_parser_test.c
    c_tokenizer_test.c
)
target_link_libraries(c-frontend-test PRIVATE test-common c-frontend)
