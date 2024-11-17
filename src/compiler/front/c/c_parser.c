
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "c_parser.h"

#include "arrays.h"
#include "strong_malloc.h"



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
        case TOKENTYPE_OTHER: return tkn.subtype >= C_TKN_LBRAC;
        default: return false;
    }
}

// Get operator precedence of infix operators.
// Returns -1 if not an operator token.
static int oper_precedence_infix(token_t token) {
    if (token.type != TOKENTYPE_OTHER) {
        return -1;
    }
    // TODO: sizeof, alignof, compount literal.
    switch (token.subtype) {
        case C_TKN_MUL:
        case C_TKN_DIV:
        case C_TKN_MOD: return 10;

        case C_TKN_ADD:
        case C_TKN_SUB: return 9;

        case C_TKN_SHL:
        case C_TKN_SHR: return 8;

        case C_TKN_LT:
        case C_TKN_LE:
        case C_TKN_GT:
        case C_TKN_GE: return 7;

        case C_TKN_NE:
        case C_TKN_EQ: return 6;

        case C_TKN_AND: return 5;

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



// Parse a C compilation unit into an AST.
token_t c_parse(tokenizer_t *tkn_ctx);


// Parse a function call expression.
token_t c_parse_funccall(tokenizer_t *tkn_ctx, token_t funcname);

// Parse one or more C expressions separated by commas.
token_t c_parse_exprs(tokenizer_t *tkn_ctx) {
    size_t   exprs_len = 0;
    size_t   exprs_cap = 1;
    token_t *exprs     = strong_malloc(exprs_cap * sizeof(token_t));
    *exprs             = c_parse_expr(tkn_ctx);

    // While the next token is a comma, more expressions can be parsed.
    token_t tkn = tkn_peek(tkn_ctx);
    while (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_COMMA) {
        tkn_next(tkn_ctx);
        token_t expr = c_parse_expr(tkn_ctx);
        array_lencap_insert_strong(&exprs, sizeof(token_t), &exprs_len, &exprs_cap, &expr, exprs_len);
    }

    // When the next token is not a comma, there are no more expressions to parse.
    return ast_from(C_AST_EXPRS, exprs_len, exprs);
}

// Parse a C expression.
token_t c_parse_expr(tokenizer_t *tkn_ctx) {
    // Assert that it starts with a token valid for the beginning of an expr.
    if (!is_first_expr_tkn(tkn_peek(tkn_ctx))) {
        cctx_diagnostic(tkn_ctx->cctx, tkn_peek(tkn_ctx).pos, DIAG_ERR, "Expected expression");
        return ast_from_va(C_AST_GARBAGE, 1, tkn_next(tkn_ctx));
    }

    size_t   stack_len = 0;
    size_t   stack_cap = 0;
    token_t *stack     = NULL;

#define push(thing)                                                                                                    \
    do {                                                                                                               \
        token_t tmp = thing;                                                                                           \
        array_lencap_insert_strong(&stack, sizeof(token_t), &stack_len, &stack_cap, &tmp, stack_len);                  \
    } while (0)
#define pop()                                                                                                          \
    ({                                                                                                                 \
        token_t tmp = stack[stack_len - 1];                                                                            \
        stack_len--;                                                                                                   \
        tmp;                                                                                                           \
    })
#define is_tkn(depth, _subtype)                                                                                        \
    (stack_len > depth && stack[stack_len - depth - 1].type == TOKENTYPE_OTHER                                         \
     && stack[stack_len - depth - 1].subtype == _subtype)
#define is_operand(depth) (stack_len > depth && is_operand_tkn(stack[stack_len - depth - 1]))

    // Start by pushing the first token onto the stack.
    push(tkn_next(tkn_ctx));
    while (1) {
        token_t peek     = tkn_peek(tkn_ctx);
        bool    can_push = is_pushable_expr_tkn(peek);

        if (is_operand(1) && (is_tkn(0, C_TKN_INC) || is_tkn(0, C_TKN_DEC))) {
            // Reduce suffix.
            token_t op  = pop();
            token_t val = pop();
            push(ast_from_va(C_AST_EXPR_SUFFIX, 2, op, val));

        } else if (is_operand(0) && stack_len >= 2 && is_prefix_oper_tkn(stack[stack_len - 1])) {
            // Reduce prefix.
            token_t val = pop();
            token_t op  = pop();
            push(ast_from_va(C_AST_EXPR_PREFIX, 2, op, val));

        } else if (is_operand(2) && is_operand(0) && oper_precedence_infix(stack[stack_len - 2]) >= 0
                   && (!can_push || oper_precedence_infix(stack[stack_len - 2]) >= oper_precedence_infix(peek))) {
            // Reduce infix.
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

    if (stack_len > 1) {
        // Invalid expression.
        cctx_diagnostic(tkn_ctx->cctx, stack[1].pos, DIAG_ERR, "Expected end of expression or operator");
        return ast_from(C_AST_GARBAGE, stack_len, stack);
    } else {
        // Valid expression.
        token_t tmp = *stack;
        free(stack);
        return tmp;
    }
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
