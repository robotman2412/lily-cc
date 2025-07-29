
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir_tokenizer.h"
#include "ir_types.h"

#include <stdio.h>

// Serialize an IR constant.
void ir_const_serialize(ir_const_t iconst, FILE *to);
// Serialize an IR operand.
void ir_operand_serialize(ir_operand_t operand, FILE *to);
// Serialize an IR instruction.
void ir_insn_serialize(ir_insn_t *insn, FILE *to);
// Serialize an IR code block.
void ir_code_serialize(ir_code_t *code, FILE *to);
// Serialize an IR function.
void ir_func_serialize(ir_func_t *func, FILE *to);

// Deserialize a single IR function from the file.
// Call multiple times on a single file if you want all the functions.
ir_func_t *ir_func_deserialize(tokenizer_t *from);
