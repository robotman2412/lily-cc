
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

add_library(compiler-common STATIC
    back/backend.c
    back/codegen.c
    back/match_tree.c
    back/cand_tree.c
    back/insn_proto.c
    back/sub_tree.c
    front/compiler.c
    front/tokenizer.c
    ir/ir_interpreter.c
    ir/ir_parser.c
    ir/ir_optimizer.c
    ir/ir_serialization.c
    ir/ir_tokenizer.c
    ir/ir_types.c
    ir/ir.c
)
target_include_directories(compiler-common PUBLIC . ir back front)
target_link_libraries(compiler-common PUBLIC util)
