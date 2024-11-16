
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include "c_tokenizer.h"
#include "compiler.h"



// Parse a C compilation unit into an AST.
token_t c_parse(tokenizer_t *tkn_ctx);

// Parse a function call expression.
token_t c_parse_funccall(tokenizer_t *tkn_ctx, token_t funcname);
// Parse one or more C expressions separated by commas.
token_t c_parse_exprs(tokenizer_t *tkn_ctx);
// Parse a C expression.
token_t c_parse_expr(tokenizer_t *tkn_ctx);
// Try to parse a C statement.
bool    c_parse_stmt(tokenizer_t *tkn_ctx, token_t *tkn_out);
// Try to parse a C variable declaration.
bool    c_parse_vardecl(tokenizer_t *tkn_ctx, token_t *tkn_out);
// Try to parse a C if statement.
bool    c_parse_if_stmt(tokenizer_t *tkn_ctx, token_t *tkn_out);
// Try to parse a C for loop.
bool    c_parse_for_loop(tokenizer_t *tkn_ctx, token_t *tkn_out);
// Try to parse a C while loop.
bool    c_parse_while_loop(tokenizer_t *tkn_ctx, token_t *tkn_out);
