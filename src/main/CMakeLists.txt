
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

# The part of Lily-CC that parses arguments, opens source files and hands them over to the compiler.
add_executable(lilycc
    main.c
)
target_include_directories(lilycc PUBLIC main)
target_link_libraries(lilycc PUBLIC lilycc-lib)
