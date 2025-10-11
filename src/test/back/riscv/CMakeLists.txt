
# SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

add_library(riscv-backend-test STATIC
    codegen_test.c
)
target_link_libraries(riscv-backend-test PRIVATE riscv-backend test-common compiler-common)
