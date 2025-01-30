
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

# The part of Lily-CC that parses arguments, opens source files and hands them over to the compiler.
add_executable(lily-cc
    main.c
)
target_include_directories(lily-cc PUBLIC main)
target_link_libraries(lily-cc PUBLIC lily-cc-lib)
