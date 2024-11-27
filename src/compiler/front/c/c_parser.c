
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "c_parser.h"

#include "arrays.h"
#include "strong_malloc.h"



// Parse a C expression or type.
static token_t c_parse_expr_or_type(tokenizer_t *tkn_ctx, bool *allow_expr, bool *allow_type, bool *allow_ddecl);
// Parse one or more C expressions separated by commas or a type.
static token_t c_parse_exprs_or_type(tokenizer_t *tkn_ctx, bool *allow_expr, bool *allow_type);



#ifndef NDEBUG
// Get enum name of `c_asttype_t` value.
char const *const c_asttype_name[] = {
    [C_AST_GARBAGE]     = "C_AST_GARBAGE",
    [C_AST_EXPRS]       = "C_AST_EXPRS",
    [C_AST_EXPR_INFIX]  = "C_AST_EXPR_INFIX",
    [C_AST_EXPR_PREFIX] = "C_AST_EXPR_PREFIX",
    [C_AST_EXPR_SUFFIX] = "C_AST_EXPR_SUFFIX",
    [C_AST_EXPR_INDEX]  = "C_AST_EXPR_INDEX",
    [C_AST_EXPR_CALL]   = "C_AST_EXPR_CALL",
    [C_AST_TYPE_QUAL]   = "C_AST_TYPE_QUAL",
    [C_AST_TYPE_PTR]    = "C_AST_TYPE_PTR",
};
#endif


// Is this a valid token for the start of an expression?
static bool is_first_expr_tkn(token_t tkn) {
    switch (tkn.type) {
        default: return false;
        case TOKENTYPE_CCONST:
        case TOKENTYPE_ICONST:
        case TOKENTYPE_SCONST:
        case TOKENTYPE_IDENT: return true;
        case TOKENTYPE_KEYWORD: return tkn.subtype == C_KEYW_alignof || tkn.subtype == C_KEYW_sizeof;
        case TOKENTYPE_OTHER:
            switch (tkn.subtype) {
                case C_TKN_ADD:
                case C_TKN_SUB:
                case C_TKN_MUL:
                case C_TKN_LPAR:
                case C_TKN_INC:
                case C_TKN_DEC: return true;
                default: return false;
            }
    }
}

// Is this a valid token or AST node for an operand?
static bool is_operand_tkn(token_t tkn) {
    switch (tkn.type) {
        case TOKENTYPE_CCONST:
        case TOKENTYPE_ICONST:
        case TOKENTYPE_SCONST:
        case TOKENTYPE_IDENT: return true;
        case TOKENTYPE_AST:
            switch (tkn.subtype) {
                case C_AST_EXPR_INFIX:
                case C_AST_EXPR_PREFIX:
                case C_AST_EXPR_SUFFIX:
                case C_AST_EXPR_CALL:
                case C_AST_EXPR_INDEX:
                case C_AST_EXPRS: return true;
                default: return false;
            }
        default: return false;
    }
}

// Is this a pushable token for `c_parse_expr`?
static inline bool is_pushable_expr_tkn(token_t tkn) {
    switch (tkn.type) {
        case TOKENTYPE_SCONST:
        case TOKENTYPE_CCONST:
        case TOKENTYPE_ICONST:
        case TOKENTYPE_IDENT: return true;
        case TOKENTYPE_KEYWORD:
            switch (tkn.subtype) {
                case C_KEYW_alignof:
                case C_KEYW_sizeof:
                case C_KEYW_true:
                case C_KEYW_false: return true;
                default: return false;
            }
        case TOKENTYPE_OTHER: return tkn.subtype >= C_TKN_LPAR;
        default: return false;
    }
}

// Get operator precedence.
// Returns -1 if not an operator token.
static int oper_precedence(token_t token, bool is_prefix) {
    if (token.type != TOKENTYPE_OTHER) {
        return -1;
    }
    // TODO: sizeof, alignof, compount literal.
    switch (token.subtype) {
        case C_TKN_LPAR:
        case C_TKN_LBRAC:
        case C_TKN_LCURL:
        case C_TKN_DOT:
        case C_TKN_ARROW: return 12;

        case C_TKN_INC:
        case C_TKN_DEC: return is_prefix ? 11 : 12;

        case C_TKN_MUL: return is_prefix ? 11 : 10;
        case C_TKN_DIV:
        case C_TKN_MOD: return 10;

        case C_TKN_ADD:
        case C_TKN_SUB: return is_prefix ? 11 : 9;

        case C_TKN_SHL:
        case C_TKN_SHR: return 8;

        case C_TKN_LT:
        case C_TKN_LE:
        case C_TKN_GT:
        case C_TKN_GE: return 7;

        case C_TKN_NE:
        case C_TKN_EQ: return 6;

        case C_TKN_AND: return is_prefix ? 11 : 5;

        case C_TKN_XOR: return 4;

        case C_TKN_OR: return 3;

        case C_TKN_LAND: return 2;

        case C_TKN_LOR: return 1;

        case C_TKN_ADD_S ... C_TKN_XOR_S:
        case C_TKN_ASSIGN: return 0;

        default: return -1;
    }
}

// Is this a valid prefix operator token?
static bool is_prefix_oper_tkn(token_t token) {
    if (token.type != TOKENTYPE_OTHER) {
        return false;
    }
    switch (token.subtype) {
        case C_TKN_MUL:
        case C_TKN_AND:
        case C_TKN_ADD:
        case C_TKN_SUB:
        case C_TKN_INC:
        case C_TKN_DEC:
        case C_TKN_NOT:
        case C_TKN_LNOT: return true;
        default: return false;
    }
}

// Is this a valid suffix operator token?
static bool is_suffix_oper_tkn(token_t token) {
    if (token.type != TOKENTYPE_OTHER) {
        return false;
    }
    switch (token.subtype) {
        case C_TKN_LPAR:
        case C_TKN_LBRAC:
        case C_TKN_DOT:
        case C_TKN_ARROW:
        case C_TKN_INC:
        case C_TKN_DEC: return true;
        default: return false;
    }
}

// Is this a valid type qualifier token?
static bool is_type_qualifier(token_t token) {
    if (token.type != TOKENTYPE_KEYWORD) {
        return false;
    }
    switch (token.subtype) {
        case C_KEYW__Atomic:
        case C_KEYW_restrict:
        case C_KEYW_const:
        case C_KEYW_volatile: return true;
        default: return false;
    }
}

// Is this a valid type specifier token?
static bool is_type_specifier(token_t token) {
    if (token.type != TOKENTYPE_KEYWORD) {
        return false;
    }
    switch (token.subtype) {
        case C_KEYW_void:
        case C_KEYW_char:
        case C_KEYW_short:
        case C_KEYW_long:
        case C_KEYW_int:
        case C_KEYW_signed:
        case C_KEYW_unsigned:
        case C_KEYW_bool: return true;
        default: return false;
    }
}



// Parse a C compilation unit into an AST.
token_t c_parse(tokenizer_t *tkn_ctx);


// Parse one or more C expressions separated by commas.
token_t c_parse_exprs(tokenizer_t *tkn_ctx) {
    bool allow_expr = true;
    bool allow_type = false;
    return c_parse_exprs_or_type(tkn_ctx, &allow_expr, &allow_type);
}

// Parse a C expression.
token_t c_parse_expr(tokenizer_t *tkn_ctx) {
    bool allow_expr  = true;
    bool allow_type  = false;
    bool allow_ddecl = false;
    return c_parse_expr_or_type(tkn_ctx, &allow_expr, &allow_type, &allow_ddecl);
}

// Parse a C expression or type.
static token_t c_parse_expr_or_type(tokenizer_t *tkn_ctx, bool *allow_expr, bool *allow_type, bool *allow_ddecl) {
    // Assert that it starts with a token valid for the beginning of an expr.
    if (!is_first_expr_tkn(tkn_peek(tkn_ctx))) {
        cctx_diagnostic(tkn_ctx->cctx, tkn_peek(tkn_ctx).pos, DIAG_ERR, "Expected expression");
        return ast_from_va(C_AST_GARBAGE, 1, tkn_next(tkn_ctx));
    }

    size_t   stack_len = 0;
    size_t   stack_cap = 0;
    token_t *stack     = NULL;

    // Push a node/token to the stack.
#define push(thing)                                                                                                    \
    do {                                                                                                               \
        token_t tmp = thing;                                                                                           \
        array_lencap_insert_strong(&stack, sizeof(token_t), &stack_len, &stack_cap, &tmp, stack_len);                  \
    } while (0)
    // Pop a node/token from the stack.
#define pop()                                                                                                          \
    ({                                                                                                                 \
        token_t tmp = stack[stack_len - 1];                                                                            \
        stack_len--;                                                                                                   \
        tmp;                                                                                                           \
    })
    // Is this a specific type of token?
#define is_tkn(depth, _subtype)                                                                                        \
    (stack_len > depth && stack[stack_len - depth - 1].type == TOKENTYPE_OTHER                                         \
     && stack[stack_len - depth - 1].subtype == _subtype)
    // Is this eligible as an operand?
#define is_operand(depth) (stack_len > depth && is_operand_tkn(stack[stack_len - depth - 1]))

    // Start by pushing the first token onto the stack.
    push(tkn_next(tkn_ctx));
    while (1) {
        token_t peek     = tkn_peek(tkn_ctx);
        bool    can_push = is_pushable_expr_tkn(peek);

        if (is_tkn(0, C_TKN_LBRAC)) {
            // Recursively parse indexing.
            pop();
            token_t arr = pop();
            token_t idx = c_parse_expr(tkn_ctx);
            token_t tmp = tkn_peek(tkn_ctx);
            if (tmp.type != TOKENTYPE_OTHER || tmp.subtype != C_TKN_RBRAC) {
                cctx_diagnostic(tkn_ctx->cctx, tmp.pos, DIAG_ERR, "Expected ]");
                tkn_delete(arr);
                tkn_delete(idx);
                goto err;
            }
            tkn_next(tkn_ctx);
            push(ast_from_va(C_AST_EXPR_INDEX, 2, arr, idx));

        } else if (is_tkn(0, C_TKN_LPAR)) {
            // Recursively parse exprs or type.
            if (*allow_expr && is_operand(1) && peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
                // Function call may have zero params.
                push(tkn_with_pos(ast_from_va(C_AST_EXPRS, 0), pos_between(pop().pos, peek.pos)));
            } else {
                // If not a function call, then it must have something in the parentheses.
                pop();
                bool is_expr = true;
                bool is_type = true;
                push(c_parse_exprs_or_type(tkn_ctx, &is_expr, &is_type));
            }
            token_t tmp = tkn_peek(tkn_ctx);
            if (tmp.type != TOKENTYPE_OTHER || tmp.subtype != C_TKN_RPAR) {
                cctx_diagnostic(tkn_ctx->cctx, tmp.pos, DIAG_ERR, "Expected )");
                goto err;
            }
            tkn_next(tkn_ctx);

        } else if (*allow_expr && is_operand(1) && stack[stack_len - 1].type == TOKENTYPE_AST
                   && stack[stack_len - 1].subtype == C_AST_EXPRS) {
            // Reduce call.
            *allow_type    = false;
            token_t params = pop();
            token_t func   = pop();
            push(ast_from_va(C_AST_EXPR_CALL, 2, func, params));

        } else if (*allow_expr && is_operand(1) && (is_tkn(0, C_TKN_INC) || is_tkn(0, C_TKN_DEC))) {
            // Reduce suffix.
            *allow_type = false;
            token_t op  = pop();
            token_t val = pop();
            push(ast_from_va(C_AST_EXPR_SUFFIX, 2, op, val));

        } else if (*allow_expr && !is_operand(2) && is_operand(0) && stack_len >= 2
                   && is_prefix_oper_tkn(stack[stack_len - 2])
                   && oper_precedence(stack[stack_len - 2], true) >= oper_precedence(peek, false)) {
            // Reduce prefix.
            *allow_type = false;
            token_t val = pop();
            token_t op  = pop();
            push(ast_from_va(C_AST_EXPR_PREFIX, 2, op, val));

        } else if (*allow_expr && is_operand(2) && is_operand(0) && oper_precedence(stack[stack_len - 2], false) >= 0
                   && (!can_push || oper_precedence(stack[stack_len - 2], false) >= oper_precedence(peek, false))) {
            // Reduce infix.
            *allow_type = false;
            token_t rhs = pop();
            token_t op  = pop();
            token_t lhs = pop();
            push(ast_from_va(C_AST_EXPR_INFIX, 3, op, lhs, rhs));

        } else if (can_push) {
            // Push next token.
            push(tkn_next(tkn_ctx));

        } else {
            // Can't reduce anything.
            break;
        }
    }

#undef push
#undef pop
#undef is_tkn
#undef is_operand

    if (stack_len > 1) {
        // Invalid expression.
        cctx_diagnostic(tkn_ctx->cctx, stack[1].pos, DIAG_ERR, "Expected end of expression or operator");
    err:
        tkn_arr_delete(stack_len, stack);
        return ast_from(C_AST_GARBAGE, stack_len, stack);
    } else {
        // Valid expression.
        token_t tmp = *stack;
        free(stack);
        return tmp;
    }
}

// Parse one or more C expressions separated by commas or a type.
static token_t c_parse_exprs_or_type(tokenizer_t *tkn_ctx, bool *allow_expr, bool *allow_type) {
    size_t   exprs_len   = 1;
    size_t   exprs_cap   = 1;
    token_t *exprs       = strong_malloc(exprs_cap * sizeof(token_t));
    bool     allow_ddecl = false;
    *exprs               = c_parse_expr_or_type(tkn_ctx, allow_expr, allow_type, &allow_ddecl);

    // If this is unambiguously a type, don't bother trying to parse multiple exprs.
    token_t tkn = tkn_peek(tkn_ctx);
    if (!*allow_expr && tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_COMMA) {
        cctx_diagnostic(tkn_ctx->cctx, tkn.pos, DIAG_ERR, "Expected )");
        token_t tmp = *exprs;
        free(exprs);
        return tmp;
    }

    // While the next token is a comma, more expressions can be parsed.
    while (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_COMMA) {
        tkn_next(tkn_ctx);
        token_t expr = c_parse_expr(tkn_ctx);
        array_lencap_insert_strong(&exprs, sizeof(token_t), &exprs_len, &exprs_cap, &expr, exprs_len);
        tkn = tkn_peek(tkn_ctx);
    }

    // When the next token is not a comma, there are no more expressions to parse.
    return ast_from(C_AST_EXPRS, exprs_len, exprs);
}

// Try to parse a C statement.
bool c_parse_stmt(tokenizer_t *tkn_ctx, token_t *tkn_out);

// Try to parse a C variable declaration.
bool c_parse_vardecl(tokenizer_t *tkn_ctx, token_t *tkn_out);

// Try to parse a C if statement.
bool c_parse_if_stmt(tokenizer_t *tkn_ctx, token_t *tkn_out);

// Try to parse a C for loop.
bool c_parse_for_loop(tokenizer_t *tkn_ctx, token_t *tkn_out);

// Try to parse a C while loop.
bool c_parse_while_loop(tokenizer_t *tkn_ctx, token_t *tkn_out);


// Print a token.
void c_tkn_debug_print(token_t token) {
    tkn_debug_print(token, c_keyw_name, c_asttype_name, c_tokentype_name);
}
