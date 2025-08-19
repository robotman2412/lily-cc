
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "tokenizer.h"

// Parse a memory operand.
token_t ir_parse_memoperand(tokenizer_t *tkn_ctx);
// Parse an instruction.
token_t ir_parse_insn(tokenizer_t *tkn_ctx);
// Parse a variable definition.
token_t ir_parse_var(tokenizer_t *tkn_ctx);
// Parse a code block definition.
token_t ir_parse_code(tokenizer_t *tkn_ctx);
// Parse an argument definition.
token_t ir_parse_arg(tokenizer_t *tkn_ctx);
// Parse an entrypoint definition.
token_t ir_parse_entry(tokenizer_t *tkn_ctx);
// Parse a stack frame definition.
token_t ir_parse_frame(tokenizer_t *tkn_ctx);
// Parse a combinator instruction binding.
token_t ir_parse_bind(tokenizer_t *tkn_ctx);
// Parse a returns list.
token_t ir_parse_ret_list(tokenizer_t *tkn_ctx);
// Parse an operands list.
token_t ir_parse_operand_list(tokenizer_t *tkn_ctx);
// Parse a combinator bindings list.
token_t ir_parse_bind_list(tokenizer_t *tkn_ctx);
// Parse an IR operand.
token_t ir_parse_operand(tokenizer_t *tkn_ctx);
// Parse an IR statement.
token_t ir_parse_stmt(tokenizer_t *tkn_ctx);
// Parse an IR function.
token_t ir_parse_func(tokenizer_t *tkn_ctx);
