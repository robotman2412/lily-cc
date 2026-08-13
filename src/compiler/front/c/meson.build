
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

front_c_srcs = [
    'c_grammar/c_ast.c',
    'c_grammar/c_parser2.c',
    'c_lexicon/c_preproc.c',
    'c_lexicon/c_tokenizer.c',
    'c_semantic/c_compile_expr.c',
    'c_semantic/c_compile_stmt.c',
    'c_semantic/c_compile.c',
    'c_semantic/c_lower.c',
    'c_semantic/c_ir.c',
    'c_types/c_types.c',
    'c_compiler.c',
    'c_parser.c',
    'c_prepass.c',
    'c_types1.c',
    'c_values.c',
]

front_c_include = [
    '.',
    'c_grammar',
    'c_lexicon',
    'c_semantic',
    'c_types',
]

front_c_target = shared_library(
    'lily-front-c',
    front_c_srcs,
    install: true,
    include_directories: front_c_include,
    dependencies: [ common_dep ],
)

front_c_dep = declare_dependency(
    link_with: front_c_target,
    include_directories: front_c_include,
    dependencies: [ common_dep ],
)
