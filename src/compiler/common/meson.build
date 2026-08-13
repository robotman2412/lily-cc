
# SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

common_srcs = [
    'back/asm_print.c',
    'back/backend.c',
    'back/codegen.c',
    'back/match_tree.c',
    'back/cand_tree.c',
    'back/regalloc.c',
    'back/insn_proto.c',
    'back/sub_tree.c',
    'front/compiler.c',
    'front/tokenizer.c',
    'ir/ir_interpreter.c',
    'ir/ir_parser.c',
    'ir/ir_optimizer.c',
    'ir/ir_serialization.c',
    'ir/ir_tokenizer.c',
    'ir/ir_types.c',
    'ir/ir.c',
]

common_include = [
    'back',
    'front',
    'ir',
]

common_target = shared_library(
    'lilycc',
    common_srcs,
    install: true,
    include_directories: common_include,
    dependencies: [ util_dep ],
)

common_dep = declare_dependency(
    link_with: common_target,
    dependencies: [ util_headers_dep ],
    include_directories: common_include,
)
