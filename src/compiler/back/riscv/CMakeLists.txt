
# SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

add_library(riscv-backend STATIC
    rv_backend.c
    rv_instructions.c
    rv_isel.c
    rv_misc.c
)
target_include_directories(riscv-backend PUBLIC .)
target_link_libraries(riscv-backend PUBLIC compiler-common)
