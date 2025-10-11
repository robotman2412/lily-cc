
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

add_library(compiler-common-test STATIC
    arith128_test.c
    compiler_test.c
    ir_test.c
    set_test.c
)
target_link_libraries(compiler-common-test PRIVATE test-common compiler-common)
