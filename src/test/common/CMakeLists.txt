
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

add_library(compiler-common-test STATIC
    compiler_test.c
)
target_link_libraries(compiler-common-test PRIVATE test-common compiler-common)
