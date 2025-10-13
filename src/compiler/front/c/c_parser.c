
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_parser.h"

#include "arrays.h"
#include "c_tokenizer.h"
#include "compiler.h"
#include "strong_malloc.h"
#include "tokenizer.h"



// Parse a direct (abstract) declaration.
static token_t c_parse_ddecl(c_parser_t *ctx, bool allows_name, bool is_typedef);
// Parse an (abstract) declaration.
static token_t c_parse_decl(c_parser_t *ctx, bool allows_name, bool is_typedef);
// Parse a type qualifier list.
static token_t c_parse_type_qual_list(c_parser_t *ctx);
// Parse a pointer and its qualifier list.
static token_t c_parse_pointer(c_parser_t *ctx);
// Parse one or more C expressions separated by commas or a type.
static token_t c_parse_exprs_or_type(c_parser_t *ctx, bool *is_type_out);

// Parse a switch statement.
static token_t c_parse_switch(c_parser_t *ctx);
// Parse a do...while statement.
static token_t c_parse_do_while(c_parser_t *ctx);
// Parse a while statement.
static token_t c_parse_while(c_parser_t *ctx);
// Parse a for statement.
static token_t c_parse_for(c_parser_t *ctx);
// Parse a if statement.
static token_t c_parse_if(c_parser_t *ctx);
// Parse a goto statement.
static token_t c_parse_goto(c_parser_t *ctx);
// Parse a return statement.
static token_t c_parse_return(c_parser_t *ctx);

// Eat tokens up to an including the next delimiter,
// or stop before next curly bracket.
static void c_eat_delim(tokenizer_t *ctx, bool include_comma);


#ifndef NDEBUG
// Get enum name of `c_asttype_t` value.
char const *const c_asttype_name[] = {
    [C_AST_GARBAGE]        = "C_AST_GARBAGE",
    [C_AST_EXPRS]          = "C_AST_EXPRS",
    [C_AST_EXPR_INFIX]     = "C_AST_EXPR_INFIX",
    [C_AST_EXPR_PREFIX]    = "C_AST_EXPR_PREFIX",
    [C_AST_EXPR_SUFFIX]    = "C_AST_EXPR_SUFFIX",
    [C_AST_EXPR_INDEX]     = "C_AST_EXPR_INDEX",
    [C_AST_EXPR_CALL]      = "C_AST_EXPR_CALL",
    [C_AST_TYPE_ARRAY]     = "C_AST_TYPE_ARRAY",
    [C_AST_TYPE_PTR_TO]    = "C_AST_TYPE_PTR_TO",
    [C_AST_TYPE_FUNC]      = "C_AST_TYPE_FUNC",
    [C_AST_NAMED_STRUCT]   = "C_AST_NAMED_STRUCT",
    [C_AST_ANON_STRUCT]    = "C_AST_ANON_STRUCT",
    [C_AST_TYPE_NAME]      = "C_AST_TYPE_NAME",
    [C_AST_SPEC_QUAL_LIST] = "C_AST_SPEC_QUAL_LIST",
    [C_AST_DECLS]          = "C_AST_DECLS",
    [C_AST_ASSIGN_DECL]    = "C_AST_ASSIGN_DECL",
    [C_AST_ENUM_VARIANT]   = "C_AST_ENUM_VARIANT",
    [C_AST_FUNC_DEF]       = "C_AST_FUNC_DEF",
    [C_AST_STMTS]          = "C_AST_STMTS",
    [C_AST_FOR_LOOP]       = "C_AST_FOR_LOOP",
    [C_AST_WHILE]          = "C_AST_WHILE",
    [C_AST_DO_WHILE]       = "C_AST_DO_WHILE",
    [C_AST_IF_ELSE]        = "C_AST_IF_ELSE",
    [C_AST_SWITCH]         = "C_AST_SWITCH",
    [C_AST_CASE_LABEL]     = "C_AST_CASE_LABEL",
    [C_AST_LABEL]          = "C_AST_LABEL",
    [C_AST_RETURN]         = "C_AST_RETURN",
    [C_AST_GOTO]           = "C_AST_GOTO",
    [C_AST_NOP]            = "C_AST_NOP",
};
#endif


// Is this a valid token for the start of an expression?
static bool is_first_expr_tkn(c_parser_t *ctx, token_t tkn) {
    switch (tkn.type) {
        default: return false;
        case TOKENTYPE_CCONST:
        case TOKENTYPE_ICONST:
        case TOKENTYPE_SCONST: return true;
        case TOKENTYPE_IDENT:
            return !set_contains(&ctx->type_names, tkn.strval)
                   && (!ctx->func_body || !set_contains(&ctx->local_type_names, tkn.strval));
        case TOKENTYPE_KEYWORD: return tkn.subtype == C_KEYW_alignof || tkn.subtype == C_KEYW_sizeof;
        case TOKENTYPE_OTHER:
            switch (tkn.subtype) {
                case C_TKN_AND:
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
        case C_KEYW_typedef:
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

// Is this a valid token for a specifier/qualifier list?
static bool is_spec_qual_list_tkn(c_parser_t *ctx, token_t token) {
    if (token.type == TOKENTYPE_IDENT) {
        return set_contains(&ctx->type_names, token.strval)
               || (ctx->func_body && set_contains(&ctx->local_type_names, token.strval));
    } else if (token.type != TOKENTYPE_KEYWORD) {
        return false;
    }
    return is_type_specifier(token) || is_type_qualifier(token) || token.subtype == C_KEYW_enum
           || token.subtype == C_KEYW_struct || token.subtype == C_KEYW_union;
}

// Is this AST node a type?
static bool is_type_node(c_parser_t *ctx, token_t node) {
    return (node.type == TOKENTYPE_AST
            && (node.subtype == C_AST_TYPE_NAME || node.subtype == C_AST_TYPE_PTR_TO
                || node.subtype == C_AST_SPEC_QUAL_LIST || node.subtype == C_AST_TYPE_FUNC))
           || is_spec_qual_list_tkn(ctx, node);
}



// Parse a C compilation unit into an AST.
token_t c_parse(c_parser_t *ctx);


// Parse one or more C expressions separated by commas.
token_t c_parse_exprs(c_parser_t *ctx) {
    size_t   exprs_len = 1;
    size_t   exprs_cap = 1;
    token_t *exprs     = strong_malloc(exprs_cap * sizeof(token_t));
    *exprs             = c_parse_expr(ctx);
    bool is_garbage    = exprs->type == TOKENTYPE_AST && exprs->subtype == C_AST_GARBAGE;

    // While the next token is a comma, more expressions can be parsed.
    token_t tkn = tkn_peek(ctx->tkn_ctx);
    while (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_COMMA) {
        tkn_next(ctx->tkn_ctx);
        token_t expr = c_parse_expr(ctx);
        if (expr.type == TOKENTYPE_AST && expr.subtype == C_AST_GARBAGE) {
            is_garbage = true;
        }
        array_lencap_insert_strong(&exprs, sizeof(token_t), &exprs_len, &exprs_cap, &expr, exprs_len);
        tkn = tkn_peek(ctx->tkn_ctx);
    }

    // When the next token is not a comma, there are no more expressions to parse.
    return ast_from(is_garbage ? C_AST_GARBAGE : C_AST_EXPRS, exprs_len, exprs);
}

// Parse a C expression.
token_t c_parse_expr(c_parser_t *ctx) {
    // Assert that it starts with a token valid for the beginning of an expr.
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (!is_first_expr_tkn(ctx, peek)) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected expression");
        return ast_from_va(C_AST_GARBAGE, 1, tkn_next(ctx->tkn_ctx));
    }

    size_t   stack_len = 0;
    size_t   stack_cap = 0;
    token_t *stack     = NULL;

    // Push a node/token to the stack.
#define push(thing)                                                                                                    \
    do {                                                                                                               \
        token_t push_temporary_value = thing;                                                                          \
        array_lencap_insert_strong(&stack, sizeof(token_t), &stack_len, &stack_cap, &push_temporary_value, stack_len); \
    } while (0)
    // Pop a node/token from the stack.
#define pop()                                                                                                          \
    ({                                                                                                                 \
        token_t pop_temporary_value = stack[stack_len - 1];                                                            \
        stack_len--;                                                                                                   \
        pop_temporary_value;                                                                                           \
    })
    // Is this a specific type of token?
#define is_tkn(depth, subtype_)                                                                                        \
    (stack_len > (depth) && stack[stack_len - (depth) - 1].type == TOKENTYPE_OTHER                                     \
     && stack[stack_len - (depth) - 1].subtype == (subtype_))
    // Is this a specific type of AST node?
#define is_ast(depth, subtype_)                                                                                        \
    (stack_len > (depth) && stack[stack_len - (depth) - 1].type == TOKENTYPE_AST                                       \
     && stack[stack_len - (depth) - 1].subtype == (subtype_))
    // Is this eligible as an operand?
#define is_operand(depth) (stack_len > (depth) && is_operand_tkn(stack[stack_len - (depth) - 1]))

    while (1) {
        peek          = tkn_peek(ctx->tkn_ctx);
        bool can_push = is_pushable_expr_tkn(peek);

        if (is_tkn(0, C_TKN_LBRAC)) { // Recursively parse indexing.
            token_t idx = c_parse_expr(ctx);
            if (idx.type == TOKENTYPE_AST && idx.subtype == C_AST_GARBAGE) {
                push(idx);
                goto err;
            }
            if (idx.type == TOKENTYPE_AST && idx.subtype == C_AST_GARBAGE) {
                push(idx);
                goto err;
            }
            token_t tmp = tkn_peek(ctx->tkn_ctx);
            if (tmp.type != TOKENTYPE_OTHER || tmp.subtype != C_TKN_RBRAC) {
                cctx_diagnostic(ctx->tkn_ctx->cctx, tmp.pos, DIAG_ERR, "Expected ]");
                push(idx);
                goto err;
            }
            tkn_delete(tkn_next(ctx->tkn_ctx));
            tkn_delete(pop());
            token_t arr = pop();
            push(ast_from_va(C_AST_EXPR_INDEX, 2, arr, idx));

        } else if (is_tkn(0, C_TKN_LPAR)) { // Recursively parse exprs.
            if (is_operand(1) && peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
                // Function call may have zero params.
                token_t lpar = pop();
                push(tkn_with_pos(ast_from_va(C_AST_EXPRS, 0), pos_including(lpar.pos, peek.pos)));
                tkn_delete(lpar);
            } else {
                // If not a function call, then it must have something in the parentheses.
                bool    is_type;
                token_t tmp = c_parse_exprs_or_type(ctx, &is_type);
                if (tmp.type == TOKENTYPE_AST && tmp.subtype == C_AST_GARBAGE) {
                    push(tmp);
                    goto err;
                }
                tkn_delete(pop());
                push(tmp);
            }
            token_t tmp = tkn_peek(ctx->tkn_ctx);
            if (tmp.type != TOKENTYPE_OTHER || tmp.subtype != C_TKN_RPAR) {
                cctx_diagnostic(ctx->tkn_ctx->cctx, tmp.pos, DIAG_ERR, "Expected )");
                goto err;
            }
            tkn_next(ctx->tkn_ctx);

        } else if (stack_len >= 2 && (is_operand(1) || is_type_node(ctx, stack[stack_len - 2]))
                   && is_ast(0, C_AST_EXPRS)) { // Reduce call.
            token_t params = pop();
            token_t func   = pop();
            push(ast_from_va(C_AST_EXPR_CALL, 2, func, params));

        } else if (is_operand(1) && (is_tkn(0, C_TKN_INC) || is_tkn(0, C_TKN_DEC))) { // Reduce suffix.
            token_t op  = pop();
            token_t val = pop();
            push(ast_from_va(C_AST_EXPR_SUFFIX, 2, op, val));

        } else if (!is_operand(2) && is_operand(0) && stack_len >= 2 && is_prefix_oper_tkn(stack[stack_len - 2])
                   && oper_precedence(stack[stack_len - 2], true) >= oper_precedence(peek, false)) { // Reduce prefix.
            token_t val = pop();
            token_t op  = pop();
            push(ast_from_va(C_AST_EXPR_PREFIX, 2, op, val));

        } else if (is_operand(2) && is_operand(0) && oper_precedence(stack[stack_len - 2], false) >= 0
                   && (!can_push
                       || oper_precedence(stack[stack_len - 2], false)
                              >= oper_precedence(peek, false))) { // Reduce infix.
            token_t rhs = pop();
            token_t op  = pop();
            token_t lhs = pop();
            push(ast_from_va(C_AST_EXPR_INFIX, 3, op, lhs, rhs));

        } else if (can_push) { // Push next token.
            token_t next = tkn_next(ctx->tkn_ctx);
            push(next);

        } else { // Can't reduce anything.
            break;
        }
    }

#undef push
#undef pop
#undef is_tkn
#undef is_operand

    if (stack_len > 1) {
        // Invalid expression.
        cctx_diagnostic(ctx->tkn_ctx->cctx, stack[1].pos, DIAG_ERR, "Expected end of expression or operator");
    err:
        return ast_from(C_AST_GARBAGE, stack_len, stack);
    } else {
        // Valid expression.
        token_t tmp = *stack; // NOLINT.
        free(stack);
        return tmp;
    }
}

// Parse a type name.
token_t c_parse_type_name(c_parser_t *ctx) {
    bool    is_typedef;
    token_t spec_qual = c_parse_spec_qual_list(ctx, &is_typedef);
    if (is_typedef) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, spec_qual.pos, DIAG_ERR, "`typedef` not allowed here");
    }
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if ((peek.type == TOKENTYPE_OTHER
         && (peek.subtype == C_TKN_MUL || peek.subtype == C_TKN_LBRAC || peek.subtype == C_TKN_LPAR))
        || peek.subtype == TOKENTYPE_IDENT) {
        token_t decl       = c_parse_decl(ctx, false, false);
        bool    is_garbage = spec_qual.subtype == C_AST_GARBAGE || decl.subtype == C_AST_GARBAGE;
        return ast_from_va(is_garbage ? C_AST_GARBAGE : C_AST_TYPE_NAME, 2, spec_qual, decl);
    } else {
        return spec_qual;
    }
}

// Parse a direct (abstract) declaration.
static token_t c_parse_ddecl(c_parser_t *ctx, bool allows_name, bool is_typedef) {
    token_t peek  = tkn_peek(ctx->tkn_ctx);
    token_t peek1 = tkn_peek_n(ctx->tkn_ctx, 1);
    token_t inner;

    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LPAR && peek1.type == TOKENTYPE_OTHER
        && (peek1.subtype == C_TKN_MUL || peek1.subtype == C_TKN_LPAR || peek1.subtype == C_TKN_LBRAC)) {
        // Parenthesized declarator.
        token_t lpar = peek;
        tkn_next(ctx->tkn_ctx);

        inner = c_parse_decl(ctx, allows_name, is_typedef);
        if (inner.type == TOKENTYPE_AST && inner.subtype == C_AST_GARBAGE) {
            return inner;
        }
        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
            return ast_from_va(C_AST_GARBAGE, 2, lpar, inner);
        }
        tkn_next(ctx->tkn_ctx);
        inner.pos = pos_including(lpar.pos, peek.pos);

    } else if (peek.type == TOKENTYPE_IDENT && allows_name) {
        // Identifier.
        tkn_next(ctx->tkn_ctx);
        inner = peek;
        if (is_typedef) {
            if (ctx->func_body) {
                set_add(&ctx->local_type_names, inner.strval);
            } else {
                set_add(&ctx->type_names, inner.strval);
            }
        }

    } else if (peek.type != TOKENTYPE_OTHER || (peek.subtype != C_TKN_LBRAC && peek.subtype != C_TKN_LPAR)) {
        // Garbaj.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ( or [");
        pos_t pos = peek.pos;
        pos.len   = 0;
        return ast_empty(C_AST_GARBAGE, pos);
    } else {
        inner = ast_empty(C_AST_NOP, peek.pos);
    }

    peek = tkn_peek(ctx->tkn_ctx);
    while (peek.type == TOKENTYPE_OTHER && (peek.subtype == C_TKN_LBRAC || peek.subtype == C_TKN_LPAR)) {
        tkn_next(ctx->tkn_ctx);
        if (peek.subtype == C_TKN_LBRAC) {
            // Array type.
            peek  = tkn_peek(ctx->tkn_ctx);
            peek1 = tkn_peek_n(ctx->tkn_ctx, 1);

            if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_MUL && peek1.type == TOKENTYPE_OTHER
                && peek1.subtype == C_TKN_RBRAC) {
                // [*] style undimensioned array.
                peek = peek1;
                tkn_next(ctx->tkn_ctx);
            }

            if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RBRAC) {
                // Undimensioned array.
                inner = ast_from_va(C_AST_TYPE_ARRAY, 1, inner);
                tkn_next(ctx->tkn_ctx);
            } else {
                // Dimensioned array.
                token_t expr = c_parse_exprs(ctx);
                peek         = tkn_peek(ctx->tkn_ctx);
                if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RBRAC) {
                    tkn_next(ctx->tkn_ctx);
                } else {
                    cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ]");
                }
                inner = ast_from_va(C_AST_TYPE_ARRAY, 2, inner, expr);
            }

        } else {
            // Function type.
            peek              = tkn_peek(ctx->tkn_ctx);
            size_t   args_len = 1;
            size_t   args_cap = 2;
            token_t *args     = strong_malloc(args_cap * sizeof(token_t));
            *args             = inner;

            if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
                // Parse function args.
                while (1) {
                    bool    is_typedef;
                    token_t param_qual = c_parse_spec_qual_list(ctx, &is_typedef);
                    if (is_typedef) {
                        cctx_diagnostic(ctx->tkn_ctx->cctx, param_qual.pos, DIAG_ERR, "`typedef` not allowed here");
                    }
                    peek = tkn_peek(ctx->tkn_ctx);
                    if (peek.type == TOKENTYPE_OTHER && (peek.subtype == C_TKN_RPAR || peek.subtype == C_TKN_COMMA)) {
                        array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &param_qual, args_len);
                    } else {
                        token_t param_decl = c_parse_decl(ctx, true, false);
                        token_t param      = ast_from_va(C_AST_DECLS, 2, param_qual, param_decl);
                        array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &param, args_len);
                        peek = tkn_peek(ctx->tkn_ctx);
                    }
                    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_COMMA) {
                        tkn_next(ctx->tkn_ctx);
                    } else if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
                        break;
                    }
                }
            }

            if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
                tkn_next(ctx->tkn_ctx);
            } else {
                cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
            }
            inner = ast_from(C_AST_TYPE_FUNC, args_len, args);
        }

        peek = tkn_peek(ctx->tkn_ctx);
    }

    return inner;
}

// Parse an (abstract) declaration.
static token_t c_parse_decl(c_parser_t *ctx, bool allows_name, bool is_typedef) {
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_MUL) {
        // If no `*`, there is required to be a direct decl.
        return c_parse_ddecl(ctx, allows_name, is_typedef);
    }

    // Parse the pointer before the ddecl.
    token_t pointer = c_parse_pointer(ctx);
    peek            = tkn_peek(ctx->tkn_ctx);
    bool empty      = peek.type != TOKENTYPE_OTHER
                 || (peek.subtype != C_TKN_MUL && peek.subtype != C_TKN_LPAR && peek.subtype != C_TKN_LBRAC);
    if (allows_name && peek.type == TOKENTYPE_IDENT) {
        // Non-abstract decls can have idents here too.
        empty = false;
    }
    if (empty) {
        return pointer;
    }

    // Something after the pointer; parse the ddecl and add it.
    ast_append_param(&pointer, c_parse_ddecl(ctx, allows_name, is_typedef));
    return pointer;
}

// Parse a type qualifier list.
static token_t c_parse_type_qual_list(c_parser_t *ctx) {
    size_t   args_len = 0;
    size_t   args_cap = 2;
    token_t *args     = strong_malloc(args_cap * sizeof(token_t));

    token_t peek = tkn_peek(ctx->tkn_ctx);
    while (is_type_qualifier(peek)) {
        tkn_next(ctx->tkn_ctx);
        array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &peek, args_len);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    return ast_from(C_AST_SPEC_QUAL_LIST, args_len, args);
}

// Parse a pointer and its qualifier list.
static token_t c_parse_pointer(c_parser_t *ctx) {
    token_t ptr  = tkn_next(ctx->tkn_ctx);
    token_t list = c_parse_type_qual_list(ctx);
    token_t peek = tkn_peek(ctx->tkn_ctx);

    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_MUL) {
        return ast_from_va(C_AST_TYPE_PTR_TO, 3, ptr, list, c_parse_pointer(ctx));
    } else {
        return ast_from_va(C_AST_TYPE_PTR_TO, 2, ptr, list);
    }
}

// Parse one or more C expressions separated by commas or a type.
static token_t c_parse_exprs_or_type(c_parser_t *ctx, bool *is_type_out) {
    token_t tkn  = tkn_peek(ctx->tkn_ctx);
    *is_type_out = is_spec_qual_list_tkn(ctx, tkn);
    if (*is_type_out) {
        return c_parse_type_name(ctx);
    } else {
        return c_parse_exprs(ctx);
    }
}

// Parse a type specifier/qualifier list.
token_t c_parse_spec_qual_list(c_parser_t *ctx, bool *is_typedef_out) {
    tokenizer_t *tkn_ctx = ctx->tkn_ctx;
    size_t       len     = 0;
    size_t       cap     = 0;
    token_t     *list    = NULL;
    *is_typedef_out      = false;

    while (1) {
        token_t peek = tkn_peek(tkn_ctx);
        if (is_type_specifier(peek) || is_type_qualifier(peek)
            || (peek.type == TOKENTYPE_IDENT
                && (set_contains(&ctx->type_names, peek.strval)
                    || (ctx->func_body && set_contains(&ctx->local_type_names, peek.strval))))) {
            if (peek.type == TOKENTYPE_KEYWORD && peek.subtype == C_KEYW_typedef && is_typedef_out) {
                *is_typedef_out = true;
            }

            // Token added verbatim.
            array_lencap_insert_strong(&list, sizeof(token_t), &len, &cap, &peek, len);
            tkn_next(tkn_ctx);

        } else if (peek.type == TOKENTYPE_KEYWORD && (peek.subtype == C_KEYW_struct || peek.subtype == C_KEYW_union)) {
            // Parse a struct/union specifier.
            token_t struct_spec = c_parse_struct_spec(ctx);
            array_lencap_insert_strong(&list, sizeof(token_t), &len, &cap, &struct_spec, len);

        } else if (peek.type == TOKENTYPE_KEYWORD && (peek.subtype == C_KEYW_enum)) {
            // Parse an enum specifier.
            token_t enum_spec = c_parse_enum_spec(ctx);
            array_lencap_insert_strong(&list, sizeof(token_t), &len, &cap, &enum_spec, len);

        } else {
            // Not valid in a specifier/qualifier list.
            break;
        }
    }

    return ast_from(C_AST_SPEC_QUAL_LIST, len, list);
}

// Parse a variable/function declarations/definition.
token_t c_parse_decls(c_parser_t *ctx, bool allow_func_body) {
    size_t   args_len = 1;
    size_t   args_cap = 2;
    token_t *args     = strong_malloc(args_cap * sizeof(token_t));
    bool     is_typedef;
    *args = c_parse_spec_qual_list(ctx, &is_typedef);

    // Decls are actually allowed to be empty.
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_SEMIC) {
        tkn_next(ctx->tkn_ctx);
        return ast_from(C_AST_DECLS, args_len, args);
    }

    do {
        if (args_len >= 2) {
            tkn_next(ctx->tkn_ctx);
            allow_func_body = false;
        }
        token_t decl = c_parse_decl(ctx, true, is_typedef);
        peek         = tkn_peek(ctx->tkn_ctx);
        if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_ASSIGN) {
            tkn_next(ctx->tkn_ctx);
            token_t expr = c_parse_expr(ctx);
            decl         = ast_from_va(C_AST_ASSIGN_DECL, 2, decl, expr);
            peek         = tkn_peek(ctx->tkn_ctx);
        }

        array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &decl, args_len);
    } while (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_COMMA);

    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LCURL) {
        // Parse function body.
        if (!allow_func_body) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
        }

        tkn_next(ctx->tkn_ctx);
        token_t body = c_parse_stmts(ctx);
        array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &body, args_len);

        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected }");
        } else {
            tkn_next(ctx->tkn_ctx);
        }

        if (!allow_func_body) {
            return ast_from(C_AST_GARBAGE, args_len, args);
        }

        return ast_from(C_AST_FUNC_DEF, args_len, args);

    } else if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
        // Should have been a semicolon here.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
        c_eat_delim(ctx->tkn_ctx, false);
    } else {
        tkn_next(ctx->tkn_ctx);
    }

    return ast_from(C_AST_DECLS, args_len, args);
}

// Parse a struct or union specifier/definition.
token_t c_parse_struct_spec(c_parser_t *ctx) {
    size_t   args_len = 1;
    size_t   args_cap = 2;
    token_t *args     = strong_malloc(sizeof(token_t) * args_cap);
    *args             = tkn_next(ctx->tkn_ctx);
    token_t peek      = tkn_peek(ctx->tkn_ctx);
    bool    named     = false;

    if (peek.type == TOKENTYPE_IDENT) {
        array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &peek, args_len);
        tkn_next(ctx->tkn_ctx);
        peek  = tkn_peek(ctx->tkn_ctx);
        named = true;

        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LCURL) {
            // Just a struct/enum name; don't parse args.
            return ast_from(C_AST_NAMED_STRUCT, args_len, args);
        }
    }

    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LCURL) {
        // There should be a decl here since it's anonymous.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected {");
        return ast_from(C_AST_GARBAGE, args_len, args);
    }
    tkn_next(ctx->tkn_ctx);

    peek = tkn_peek(ctx->tkn_ctx);
    while (peek.type != TOKENTYPE_EOF && (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL)) {
        token_t decl = c_parse_decls(ctx, false);
        array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &decl, args_len);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL) {
        // There should be a decl here since it's anonymous.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected }");
        return ast_from(C_AST_GARBAGE, args_len, args);
    }
    tkn_next(ctx->tkn_ctx);

    return ast_from(named ? C_AST_NAMED_STRUCT : C_AST_ANON_STRUCT, args_len, args);
}

// Parse an enum specifier/definition.
token_t c_parse_enum_spec(c_parser_t *ctx) {
    size_t   args_len = 1;
    size_t   args_cap = 2;
    token_t *args     = strong_malloc(sizeof(token_t) * args_cap);
    *args             = tkn_next(ctx->tkn_ctx);
    token_t peek      = tkn_peek(ctx->tkn_ctx);
    bool    named     = false;

    if (peek.type == TOKENTYPE_IDENT) {
        array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &peek, args_len);
        tkn_next(ctx->tkn_ctx);
        peek  = tkn_peek(ctx->tkn_ctx);
        named = true;

        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LCURL) {
            // Just a enum/struct/union name; don't parse args.
            return ast_from(C_AST_NAMED_STRUCT, args_len, args);
        }
    }

    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LCURL) {
        // There should be a decl here since it's anonymous.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected {");
        return ast_from(C_AST_GARBAGE, args_len, args);
    }
    tkn_next(ctx->tkn_ctx);

    while (1) {
        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type == TOKENTYPE_EOF || (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RCURL)) {
            break;
        } else if (peek.type != TOKENTYPE_IDENT) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected identifier or }");
            break;
        } else {
            tkn_next(ctx->tkn_ctx);
            token_t ident = peek;
            peek          = tkn_peek(ctx->tkn_ctx);
            if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_ASSIGN) {
                // Enum variant with specific index.
                tkn_next(ctx->tkn_ctx);
                token_t expr = c_parse_expr(ctx);
                if (expr.type != TOKENTYPE_AST || expr.subtype != C_AST_GARBAGE) {
                    token_t ast = ast_from_va(C_AST_ENUM_VARIANT, 2, ident, expr);
                    array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &ast, args_len);
                }

                peek = tkn_peek(ctx->tkn_ctx);
            } else {
                // Enum variant with implicit index.
                token_t ast = ast_from_va(C_AST_ENUM_VARIANT, 1, ident);
                array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &ast, args_len);
            }
            if (peek.type != TOKENTYPE_OTHER || (peek.subtype != C_TKN_COMMA && peek.subtype != C_TKN_RCURL)) {
                cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ,");
                c_eat_delim(ctx->tkn_ctx, true);
            } else if (peek.subtype == C_TKN_COMMA) {
                tkn_next(ctx->tkn_ctx);
            }
        }
    }

    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL) {
        // There should be a decl here since it's anonymous.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected }");
        return ast_from(C_AST_GARBAGE, args_len, args);
    }
    tkn_next(ctx->tkn_ctx);

    return ast_from(named ? C_AST_NAMED_STRUCT : C_AST_ANON_STRUCT, args_len, args);
}

// Parse a switch statement.
static token_t c_parse_switch(c_parser_t *ctx) {
    (void)ctx;
    fprintf(stderr, "[TODO] Create C switch statement parser\n");
    abort();
}

// Parse a do...while statement.
static token_t c_parse_do_while(c_parser_t *ctx) {
    (void)ctx;
    fprintf(stderr, "[TODO] Create do...while statement parser\n");
    abort();
}

// Parse a while statement.
static token_t c_parse_while(c_parser_t *ctx) {
    token_t kw = tkn_next(ctx->tkn_ctx);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected (");
        return ast_from_va(C_AST_GARBAGE, 1, kw);
    }
    tkn_next(ctx->tkn_ctx);

    token_t cond = c_parse_exprs(ctx);

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
        return ast_from_va(C_AST_GARBAGE, 1, cond);
    }
    tkn_next(ctx->tkn_ctx);

    token_t body = c_parse_stmt(ctx);
    return ast_from_va(C_AST_WHILE, 2, cond, body);
}

// Parse a for statement.
static token_t c_parse_for(c_parser_t *ctx) {
    token_t kw = tkn_next(ctx->tkn_ctx);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected (");
        return ast_from_va(C_AST_GARBAGE, 1, kw);
    }
    tkn_next(ctx->tkn_ctx);

    peek = tkn_peek(ctx->tkn_ctx);
    token_t init;
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_SEMIC) {
        // No initializer.
        tkn_next(ctx->tkn_ctx);
        init = ast_empty(C_AST_NOP, peek.pos);
    } else if (is_spec_qual_list_tkn(ctx, peek)) {
        // Declaration as initializer.
        init = c_parse_decls(ctx, false);
    } else {
        // Expression as initializer.
        init = c_parse_exprs(ctx);

        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
            return ast_from_va(C_AST_GARBAGE, 1, init);
        }
        tkn_next(ctx->tkn_ctx);
    }

    peek = tkn_peek(ctx->tkn_ctx);
    token_t cond;
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_SEMIC) {
        // No initializer.
        tkn_next(ctx->tkn_ctx);
        cond = ast_empty(C_AST_NOP, peek.pos);
    } else {
        // Expression as condition.
        cond = c_parse_exprs(ctx);

        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
            return ast_from_va(C_AST_GARBAGE, 2, init, cond);
        }
        tkn_next(ctx->tkn_ctx);
    }

    peek = tkn_peek(ctx->tkn_ctx);
    token_t inc;
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
        // No increment.
        inc = ast_empty(C_AST_NOP, peek.pos);
    } else {
        // Expression as increment.
        inc  = c_parse_exprs(ctx);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
        return ast_from_va(C_AST_GARBAGE, 3, init, cond, inc);
    }
    tkn_next(ctx->tkn_ctx);

    token_t body = c_parse_stmt(ctx);
    return ast_from_va(C_AST_FOR_LOOP, 4, init, cond, inc, body);
}

// Parse a if statement.
static token_t c_parse_if(c_parser_t *ctx) {
    token_t kw = tkn_next(ctx->tkn_ctx);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected (");
        return ast_from_va(C_AST_GARBAGE, 1, kw);
    }
    tkn_next(ctx->tkn_ctx);

    token_t cond = c_parse_exprs(ctx);

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
        return ast_from_va(C_AST_GARBAGE, 2, kw, cond);
    }
    tkn_next(ctx->tkn_ctx);

    token_t body = c_parse_stmt(ctx);

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_KEYWORD && peek.subtype == C_KEYW_else) {
        // An if...else statement.
        tkn_next(ctx->tkn_ctx);
        token_t else_body = c_parse_stmt(ctx);
        return ast_from_va(C_AST_IF_ELSE, 3, cond, body, else_body);
    } else {
        // Just if.
        return ast_from_va(C_AST_IF_ELSE, 2, cond, body);
    }
}

// Parse a goto statement.
static token_t c_parse_goto(c_parser_t *ctx) {
    (void)ctx;
    fprintf(stderr, "[TODO] Create goto statement parser\n");
    abort();
}

// Parse a return statement.
static token_t c_parse_return(c_parser_t *ctx) {
    token_t kw   = tkn_next(ctx->tkn_ctx);
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
        token_t expr = c_parse_exprs(ctx);
        peek         = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
            c_eat_delim(ctx->tkn_ctx, false);
        } else {
            tkn_next(ctx->tkn_ctx);
        }
        return ast_from_va(C_AST_RETURN, 1, expr);
    }
    tkn_next(ctx->tkn_ctx);
    return ast_empty(C_AST_RETURN, kw.pos);
}

// Parse a statment.
token_t c_parse_stmt(c_parser_t *ctx) {
    token_t peek = tkn_peek(ctx->tkn_ctx);

    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LCURL) {
        // Multi-statement parser will always continue until RCRUL token.
        tkn_next(ctx->tkn_ctx);
        token_t stmts = c_parse_stmts(ctx);
        tkn_next(ctx->tkn_ctx);
        return stmts;
    } else if (is_spec_qual_list_tkn(ctx, peek)) {
        return c_parse_decls(ctx, false);
    } else if (peek.type == TOKENTYPE_KEYWORD) {
        switch (peek.subtype) {
            case C_KEYW_switch: return c_parse_switch(ctx);
            case C_KEYW_do: return c_parse_do_while(ctx);
            case C_KEYW_while: return c_parse_while(ctx);
            case C_KEYW_for: return c_parse_for(ctx);
            case C_KEYW_if: return c_parse_if(ctx);
            case C_KEYW_goto: return c_parse_goto(ctx);
            case C_KEYW_return: return c_parse_return(ctx);
            default: {
                cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected a statement");
                c_eat_delim(ctx->tkn_ctx, false);
                pos_t pos = peek.pos;
                pos.len   = 0;
                return ast_empty(C_AST_GARBAGE, pos);
            }
        }
    } else {
        token_t expr = c_parse_exprs(ctx);
        peek         = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
            c_eat_delim(ctx->tkn_ctx, false);
        } else {
            tkn_next(ctx->tkn_ctx);
        }
        return expr;
    }
}

// Parse multiple statments.
token_t c_parse_stmts(c_parser_t *ctx) {
    size_t   args_len = 0;
    size_t   args_cap = 0;
    token_t *args     = NULL;

    token_t peek = tkn_peek(ctx->tkn_ctx);
    while (peek.type != TOKENTYPE_EOF && (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL)) {
        token_t stmt = c_parse_stmt(ctx);
        array_lencap_insert_strong(&args, sizeof(token_t), &args_len, &args_cap, &stmt, args_len);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    return ast_from(C_AST_STMTS, args_len, args);
}



// Eat tokens up to an including the next delimiter,
// or stop before next curly bracket.
static void c_eat_delim(tokenizer_t *tkn_ctx, bool include_comma) {
    token_t peek    = tkn_peek(tkn_ctx);
    bool    tooketh = false;
    while (1) {
        if (peek.type == TOKENTYPE_OTHER && (peek.subtype == C_TKN_RCURL || peek.subtype == C_TKN_LCURL)) {
            if (!tooketh) {
                tkn_next(tkn_ctx);
            }
            return;
        } else if (peek.type == TOKENTYPE_OTHER
                   && (peek.subtype == C_TKN_SEMIC || (include_comma && peek.subtype == C_TKN_COMMA))) {
            tkn_next(tkn_ctx);
            return;
        } else {
            tkn_next(tkn_ctx);
            peek = tkn_peek(tkn_ctx);
        }
        tooketh = true;
    }
}

#ifndef NDEBUG
// Print a token.
void c_tkn_debug_print(token_t token) {
    tkn_debug_print(token, c_keyw_name, c_asttype_name, c_tokentype_name);
}

// Build a test case that asserts an exact value for a token.
void c_tkn_debug_testcase(token_t token) {
    tkn_debug_testcase(token, c_keyw_name, c_asttype_name, c_tokentype_name);
}
#endif
