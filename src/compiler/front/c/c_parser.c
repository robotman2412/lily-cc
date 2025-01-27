
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "c_parser.h"

#include "arrays.h"
#include "char_repr.h"
#include "strong_malloc.h"



// Parse a C expression or type.
static token_t c_parse_expr_or_type(
    tokenizer_t *tkn_ctx, bool *restrict allow_expr, bool *restrict allow_type, bool *restrict allow_ddecl
);
// Parse one or more C expressions separated by commas or a type.
static token_t c_parse_exprs_or_type(
    tokenizer_t *tkn_ctx,
    bool *restrict allow_expr,
    bool *restrict allow_type,
    bool *restrict allow_ddecl,
    bool *restrict allow_type_list
);



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
    [C_AST_TYPE_PTR_QUAL]  = "C_AST_TYPE_PTR_QUAL",
    [C_AST_TYPE_PTR_TO]    = "C_AST_TYPE_PTR_TO",
    [C_AST_STRUCT]         = "C_AST_STRUCT",
    [C_AST_TYPE_NAME]      = "C_AST_TYPE_NAME",
    [C_AST_SPEC_QUAL_LIST] = "C_AST_SPEC_QUAL_LIST",
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

// Is this a valid token for a specifier/qualifier list?
static bool is_spec_qual_list_tkn(token_t token) {
    if (token.type == TOKENTYPE_IDENT) {
        return true;
    } else if (token.type == TOKENTYPE_AST && token.subtype == C_AST_STRUCT) {
        return true;
    } else if (token.type != TOKENTYPE_KEYWORD) {
        return false;
    }
    return is_type_specifier(token) || is_type_qualifier(token);
}



// Parse a C compilation unit into an AST.
token_t c_parse(tokenizer_t *tkn_ctx);


// Parse one or more C expressions separated by commas.
token_t c_parse_exprs(tokenizer_t *tkn_ctx) {
    bool allow_expr      = true;
    bool allow_type      = false;
    bool allow_ddecl     = false;
    bool allow_type_list = false;
    return c_parse_exprs_or_type(tkn_ctx, &allow_expr, &allow_type, &allow_ddecl, &allow_type_list);
}

// Parse a C expression.
token_t c_parse_expr(tokenizer_t *tkn_ctx) {
    bool allow_expr  = true;
    bool allow_type  = false;
    bool allow_ddecl = false;
    return c_parse_expr_or_type(tkn_ctx, &allow_expr, &allow_type, &allow_ddecl);
}

// Parse a type name.
token_t c_parse_type_name(tokenizer_t *tkn_ctx) {
    bool allow_expr  = false;
    bool allow_type  = true;
    bool allow_ddecl = false;
    return c_parse_expr_or_type(tkn_ctx, &allow_expr, &allow_type, &allow_ddecl);
}

// Parse a C expression or type.
static token_t c_parse_expr_or_type(
    tokenizer_t *tkn_ctx, bool *restrict allow_expr, bool *restrict allow_type, bool *restrict allow_ddecl
) {
    // Assert that it starts with a token valid for the beginning of an expr.
    token_t peek = tkn_peek(tkn_ctx);
    if (!is_first_expr_tkn(peek) && (!*allow_expr || !is_type_qualifier(peek) && !is_type_specifier(peek))) {
        cctx_diagnostic(tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected expression");
        return ast_from_va(C_AST_GARBAGE, 1, tkn_next(tkn_ctx));
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
#define is_tkn(depth, _subtype)                                                                                        \
    (stack_len > depth && stack[stack_len - depth - 1].type == TOKENTYPE_OTHER                                         \
     && stack[stack_len - depth - 1].subtype == _subtype)
    // Is this a specific type of AST node?
#define is_ast(depth, _subtype)                                                                                        \
    (stack_len > depth && stack[stack_len - depth - 1].type == TOKENTYPE_AST                                           \
     && stack[stack_len - depth - 1].subtype == _subtype)
    // Is this eligible as an operand?
#define is_operand(depth) (stack_len > depth && is_operand_tkn(stack[stack_len - depth - 1]))

    while (1) {
        peek          = tkn_peek(tkn_ctx);
        bool can_push = is_pushable_expr_tkn(peek)
                        || (*allow_type || *allow_ddecl) && (is_type_specifier(peek) || is_type_qualifier(peek));

        if (is_tkn(0, C_TKN_LBRAC)) { // Recursively parse indexing.
            token_t tmp = tkn_peek(tkn_ctx);
            if (tmp.type == TOKENTYPE_OTHER && tmp.subtype == C_TKN_RBRAC && *allow_type || *allow_ddecl) {
                // Types can have empty array indices.
                tkn_delete(tkn_next(tkn_ctx));
                tkn_delete(pop());
                token_t arr = pop();
                push(ast_from_va(C_AST_EXPR_INDEX, 1, arr));
                continue;
            }
            bool    is_expr  = true;
            bool    is_type  = false;
            bool    is_ddecl = *allow_type || *allow_ddecl;
            token_t idx      = c_parse_expr_or_type(tkn_ctx, &is_expr, &is_type, &is_ddecl);
            if (idx.type == TOKENTYPE_AST && idx.subtype == C_AST_GARBAGE) {
                push(idx);
                goto err;
            }
            if (is_ddecl && (idx.type != TOKENTYPE_AST || idx.subtype != C_AST_TYPE_PTR_TO)) {
                cctx_diagnostic(tkn_ctx->cctx, idx.pos, DIAG_ERR, "Expected * or ] or expression");
                push(idx);
                goto err;
            }
            if (idx.type == TOKENTYPE_AST && idx.subtype == C_AST_GARBAGE) {
                push(idx);
                goto err;
            }
            tmp = tkn_peek(tkn_ctx);
            if (tmp.type != TOKENTYPE_OTHER || tmp.subtype != C_TKN_RBRAC) {
                cctx_diagnostic(tkn_ctx->cctx, tmp.pos, DIAG_ERR, "Expected ]");
                push(idx);
                goto err;
            }
            tkn_delete(tkn_next(tkn_ctx));
            tkn_delete(pop());
            token_t arr = pop();
            push(ast_from_va(C_AST_EXPR_INDEX, 2, arr, idx));

        } else if (is_tkn(0, C_TKN_LPAR)) { // Recursively parse exprs or type.
            if (*allow_expr && is_operand(1) && peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
                // Function call may have zero params.
                token_t lpar = pop();
                push(tkn_with_pos(ast_from_va(C_AST_EXPRS, 0), pos_between(lpar.pos, peek.pos)));
                tkn_delete(lpar);
            } else {
                // If not a function call, then it must have something in the parentheses.
                bool    is_expr      = *allow_expr;
                bool    is_type      = *allow_expr && !*allow_type;
                bool    is_ddecl     = *allow_type || *allow_ddecl;
                // TODO: This may not be the correct way to go about function types but I don't know what the correct
                // way could be.
                bool    is_type_list = *allow_type;
                token_t tmp          = c_parse_exprs_or_type(tkn_ctx, &is_expr, &is_type, &is_ddecl, &is_type_list);
                if (tmp.type == TOKENTYPE_AST && tmp.subtype == C_AST_GARBAGE) {
                    push(tmp);
                    goto err;
                }
                if (!is_ddecl) {
                    *allow_ddecl = false;
                    *allow_type  = false;
                }
                if (!is_expr && is_ddecl) {
                    *allow_expr = false;
                }
                tkn_delete(pop());
                push(tmp);
            }
            token_t tmp = tkn_peek(tkn_ctx);
            if (tmp.type != TOKENTYPE_OTHER || tmp.subtype != C_TKN_RPAR) {
                cctx_diagnostic(tkn_ctx->cctx, tmp.pos, DIAG_ERR, "Expected )");
                goto err;
            }
            tkn_next(tkn_ctx);

        } else if (*allow_expr && is_operand(1) && is_ast(0, C_AST_EXPRS)) { // Reduce call.
            *allow_type    = false;
            *allow_ddecl   = false;
            token_t params = pop();
            token_t func   = pop();
            push(ast_from_va(C_AST_EXPR_CALL, 2, func, params));

        } else if (*allow_expr && is_operand(1) && (is_tkn(0, C_TKN_INC) || is_tkn(0, C_TKN_DEC))) { // Reduce suffix.
            *allow_type  = false;
            *allow_ddecl = false;
            token_t op   = pop();
            token_t val  = pop();
            push(ast_from_va(C_AST_EXPR_SUFFIX, 2, op, val));

        } else if (*allow_expr && !is_operand(2) && is_operand(0) && stack_len >= 2
                   && is_prefix_oper_tkn(stack[stack_len - 2])
                   && oper_precedence(stack[stack_len - 2], true) >= oper_precedence(peek, false)) { // Reduce prefix.
            *allow_type  = false;
            *allow_ddecl = false;
            token_t val  = pop();
            token_t op   = pop();
            push(ast_from_va(C_AST_EXPR_PREFIX, 2, op, val));

        } else if (*allow_expr && is_operand(2) && is_operand(0) && oper_precedence(stack[stack_len - 2], false) >= 0
                   && (!can_push || oper_precedence(stack[stack_len - 2], false) >= oper_precedence(peek, false)
                   )) { // Reduce infix.
            *allow_type  = false;
            *allow_ddecl = false;
            token_t rhs  = pop();
            token_t op   = pop();
            token_t lhs  = pop();
            push(ast_from_va(C_AST_EXPR_INFIX, 3, op, lhs, rhs));

        } else if ((*allow_ddecl || *allow_type) && (is_tkn(1, C_TKN_MUL) || is_ast(1, C_AST_TYPE_PTR_QUAL))
                   && is_type_qualifier(stack[stack_len - 1])) { // Reduce qualified pointer.
            *allow_expr  = false;
            token_t qual = pop();
            if (stack[stack_len - 1].type == TOKENTYPE_OTHER) {
                // There is one asterisk, create new list.
                token_t ptr = pop();
                push(tkn_with_pos(ast_from_va(C_AST_TYPE_PTR_QUAL, 1, qual), pos_between(ptr.pos, qual.pos)));

            } else {
                // Add item to existing list.
                ast_append_param(&stack[stack_len - 1], qual);
            }

        } else if (*allow_type && stack_len >= 2 && is_spec_qual_list_tkn(stack[stack_len - 2])
                   && is_spec_qual_list_tkn(stack[stack_len - 1])) { // Reduce specifier/qualifier list.
            *allow_expr  = false;
            token_t tkn1 = pop();
            token_t tkn0 = pop();
            push(ast_from_va(C_AST_SPEC_QUAL_LIST, 2, tkn0, tkn1));

        } else if (*allow_type && is_ast(1, C_AST_SPEC_QUAL_LIST)
                   && is_spec_qual_list_tkn(stack[stack_len - 1])) { // Reduce specifier/qualifier list.
            *allow_expr = false;
            ast_append_param(stack + stack_len - 2, pop());

        } else if (can_push) { // Push next token.
            token_t next = tkn_next(tkn_ctx);
            push(next);
            if (!*allow_ddecl && !*allow_type) {
                continue;
            }
            if (next.type == TOKENTYPE_ICONST || next.type == TOKENTYPE_CCONST || next.type == TOKENTYPE_SCONST) {
                *allow_ddecl = false;
                *allow_type  = false;
            } else if (next.type == TOKENTYPE_OTHER && next.subtype != C_TKN_MUL && next.subtype != C_TKN_LBRAC
                       && next.subtype != C_TKN_LPAR) {
                *allow_ddecl = false;
                *allow_type  = false;
            } else if (next.type == TOKENTYPE_IDENT) {
                *allow_ddecl = false;
            }

        } else if ((*allow_ddecl || *allow_type) && (is_tkn(1, C_TKN_MUL) || is_ast(1, C_AST_TYPE_PTR_QUAL))
                   && (is_ast(0, C_AST_TYPE_PTR_TO) || is_ast(0, C_AST_EXPR_INDEX))) { // Reduce ptr type.
            *allow_expr  = false;
            token_t type = pop();
            token_t ptr  = pop();
            push(ast_from_va(C_AST_TYPE_PTR_TO, 2, ptr, type));

        } else if ((*allow_ddecl || *allow_type) && is_tkn(0, C_TKN_MUL)) { // Reduce inner-most ptr.
            *allow_expr = false;
            token_t tmp = pop();
            push(ast_from_va(C_AST_TYPE_PTR_TO, 1, tmp));
            tkn_delete(tmp);

        } else if (*allow_type && stack_len == 2
                   && (is_ast(1, C_AST_SPEC_QUAL_LIST) || is_spec_qual_list_tkn(stack[stack_len - 2]))
                   && (is_ast(0, C_AST_TYPE_PTR_TO) || is_ast(0, C_AST_EXPR_INDEX))) { // Reduce type name.
            token_t ptrs = pop();
            token_t spec = pop();
            push(ast_from_va(C_AST_TYPE_NAME, 2, spec, ptrs));

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
        cctx_diagnostic(tkn_ctx->cctx, stack[1].pos, DIAG_ERR, "Expected end of expression or operator");
    err:
        return ast_from(C_AST_GARBAGE, stack_len, stack);
    } else {
        // Valid expression.
        token_t tmp = *stack;
        free(stack);
        return tmp;
    }
}

// Parse one or more C expressions separated by commas or a type.
static token_t c_parse_exprs_or_type(
    tokenizer_t *tkn_ctx,
    bool *restrict allow_expr,
    bool *restrict allow_type,
    bool *restrict allow_ddecl,
    bool *restrict allow_type_list
) {
    size_t   exprs_len = 1;
    size_t   exprs_cap = 1;
    token_t *exprs     = strong_malloc(exprs_cap * sizeof(token_t));
    *exprs             = c_parse_expr_or_type(tkn_ctx, allow_expr, allow_type, allow_ddecl);
    bool is_garbage    = exprs->type == TOKENTYPE_AST && exprs->subtype == C_AST_GARBAGE;

    // If this is unambiguously a type, don't bother trying to parse multiple exprs.
    token_t tkn = tkn_peek(tkn_ctx);
    if (!*allow_expr && !*allow_type_list && tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_COMMA) {
        cctx_diagnostic(tkn_ctx->cctx, tkn.pos, DIAG_ERR, "Expected )");
        token_t tmp = *exprs;
        free(exprs);
        return tmp;
    }

    // While the next token is a comma, more expressions can be parsed.
    while (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_COMMA) {
        tkn_next(tkn_ctx);
        bool    is_expr  = *allow_expr;
        bool    is_type  = *allow_type_list;
        bool    is_ddecl = false;
        token_t expr     = c_parse_expr_or_type(tkn_ctx, &is_expr, &is_type, &is_ddecl);
        if (expr.type == TOKENTYPE_AST && expr.subtype == C_AST_GARBAGE) {
            is_garbage = true;
        }
        array_lencap_insert_strong(&exprs, sizeof(token_t), &exprs_len, &exprs_cap, &expr, exprs_len);
        tkn = tkn_peek(tkn_ctx);
        if (is_expr && !is_type) {
            *allow_type_list = false;
        } else if (!is_expr && is_type) {
            *allow_expr = false;
        }
    }

    // When the next token is not a comma, there are no more expressions to parse.
    return ast_from(is_garbage ? C_AST_GARBAGE : C_AST_EXPRS, exprs_len, exprs);
}

// Parse a type specifier/qualifier list.
token_t c_parse_spec_qual_list(tokenizer_t *tkn_ctx) {
    size_t   len  = 0;
    size_t   cap  = 0;
    token_t *list = NULL;

    token_t peek = tkn_peek(tkn_ctx);
    while (1) {
        if (is_type_specifier(peek) || is_type_qualifier(peek) || peek.type == TOKENTYPE_IDENT) {
            // Token added verbatim.
            array_lencap_insert_strong(&list, sizeof(token_t), &len, &cap, &peek, len - 1);
            tkn_next(tkn_ctx);

        } else if (peek.type == TOKENTYPE_KEYWORD && (peek.subtype == C_KEYW_struct || peek.subtype == C_KEYW_union)) {
            // TODO: Parse a struct/union specifier.
            break;

        } else if (peek.type == TOKENTYPE_KEYWORD && (peek.subtype == C_KEYW_enum)) {
            // TODO: Parse an enum specifier.
            break;

        } else {
            // Not valid in a specifier/qualifier list.
            break;
        }
    }

    return ast_from(C_AST_SPEC_QUAL_LIST, len, list);
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
