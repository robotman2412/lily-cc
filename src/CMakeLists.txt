
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(lily-cc)
add_compile_options(
    -std=gnu17
    -O0 -ggdb
    -Wall -Wno-parentheses -Wno-unused-function -Wno-missing-braces
    -Werror=all -Werror=enum-conversion
    -lbsd
    -DDLIST_CONSISTENCY_CHECK
)


add_subdirectory(util)
add_subdirectory(compiler)
add_subdirectory(test)
add_subdirectory(main)
