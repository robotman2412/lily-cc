
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16.0)

add_library(c-frontend STATIC
    c_grammar/c_ast.c
    c_grammar/c_parser2.c
    c_semantic/c_compile_expr.c
    c_semantic/c_compile_stmt.c
    c_semantic/c_compile.c
    c_semantic/c_ir.c
    c_types/c_types.c
    c_compiler.c
    c_parser.c
    c_prepass.c
    c_preproc.c
    c_tokenizer.c
    c_types1.c
    c_values.c
)
target_include_directories(c-frontend PUBLIC . c_grammar c_semantic c_types)
target_link_libraries(c-frontend PUBLIC compiler-common)
