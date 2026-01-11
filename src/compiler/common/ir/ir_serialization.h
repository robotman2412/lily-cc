
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "backend.h"
#include "ir_tokenizer.h"
#include "ir_types.h"

#include <stdio.h>

// Serialize an IR constant.
void ir_const_serialize(ir_const_t iconst, FILE *to);
// Serialize an IR operand.
void ir_operand_serialize(
    ir_operand_t const *operand, backend_profile_t const *profile_opt, bool show_memop_type, FILE *to
);
// Serialize an IR instruction.
void ir_insn_serialize(ir_insn_t const *insn, backend_profile_t const *profile_opt, FILE *to);
// Serialize an IR code block.
void ir_code_serialize(ir_code_t const *code, backend_profile_t const *profile_opt, FILE *to);
// Serialize an IR function.
void ir_func_serialize(ir_func_t const *func, backend_profile_t const *profile_opt, FILE *to);

// Deserialize a single IR function from the file.
// Call multiple times on a single file if you want all the functions.
ir_func_t *ir_func_deserialize(tokenizer_t *from);
// Helper to deserialize a single IR function from a string.
// Returns NULL if there are any syntax errors.
ir_func_t *ir_func_deserialize_str(char const *data, size_t data_len, char const *virt_filename);
