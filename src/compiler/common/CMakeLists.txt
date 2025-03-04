
# SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT

add_library(compiler-common STATIC
    ir/ir_interpreter.c
    ir/ir_optimizer.c
    ir/ir_serialization.c
    ir/ir_types.c
    ir/ir.c
    backend.c
    compiler.c
    tokenizer.c
)
target_include_directories(compiler-common PUBLIC . ir)
target_link_libraries(compiler-common PUBLIC util)
