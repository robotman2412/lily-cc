
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_parser2.h"

#include "arith128.h"
#include "c_ast.h"
#include "c_parser.h"
#include "c_prim.h"
#include "c_tokenizer.h"
#include "compiler.h"
#include "lilycc_malloc.h"
#include "tokenizer.h"
#include "vec.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>



// Union tag of `lr_entry_t`.
typedef enum {
    // A C token.
    LR_ENTRY_TOKEN,
    // An expression AST node.
    LR_ENTRY_EXPR,
    // A type name AST node.
    LR_ENTRY_TYPE,
} lr_entry_type_t;

// Stack entry in the LR parser of `c_parse2_expr`.
typedef struct {
    // Union tag.
    lr_entry_type_t tag;
    union {
        token_t            token;
        c_ast_expr_t      *expr;
        c_ast_type_name_t *type;
    };
} lr_entry_t;
VEC_TYPE_DEF(vec_lr_entry_t, lr_entry_t)

// Destroy an LR parser stack entry.
// Does not free the memory of `entry` itself.
static void lr_entry_delete(lr_entry_t entry);

// Parse a direct (abstract) declaration.
static c_ast_decl_t           *c_parse2_ddecl(c_parser_t *ctx, bool allows_name, bool is_typedef);
// Parse an (abstract) declaration.
static c_ast_decl_t           *c_parse2_decl(c_parser_t *ctx, bool allows_name, bool is_typedef);
// Parse a type qualifier list.
static c_ast_spec_qual_list_t *c_parse2_type_qual_list(c_parser_t *ctx);
// Parse one or more C expressions separated by commas or a type.
// The return type is either `c_ast_type_name_t *` or `c_ast_exprs_t *`.
static void                   *c_parse2_exprs_or_type(c_parser_t *ctx, bool *is_type_out);
// Parse a compound initializer or expression.
static c_ast_initval_t        *c_parse2_compinit_or_expr(c_parser_t *ctx);

// Parse a switch statement.
static c_ast_stmt_t *c_parse2_switch(c_parser_t *ctx);
// Parse a do...while statement.
static c_ast_stmt_t *c_parse2_do_while(c_parser_t *ctx);
// Parse a while statement.
static c_ast_stmt_t *c_parse2_while(c_parser_t *ctx);
// Parse a for statement.
static c_ast_stmt_t *c_parse2_for(c_parser_t *ctx);
// Parse a if statement.
static c_ast_stmt_t *c_parse2_if(c_parser_t *ctx);
// Parse a goto statement.
static c_ast_stmt_t *c_parse2_goto(c_parser_t *ctx);
// Parse a return statement.
static c_ast_stmt_t *c_parse2_return(c_parser_t *ctx);

// Eat tokens up to an including the next delimiter,
// or stop before next curly bracket.
static void c_eat_delim(tokenizer_t *ctx, bool include_comma);



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
        case TOKENTYPE_KEYWORD:
            return tkn.subtype == C_KEYW_sizeof || tkn.subtype == C_KEYW_alignof || tkn.subtype == C_KEYW_true
                   || tkn.subtype == C_KEYW_false;
        case TOKENTYPE_OTHER:
            switch (tkn.subtype) {
                case C_TKN_AND:
                case C_TKN_ADD:
                case C_TKN_SUB:
                case C_TKN_MUL:
                case C_TKN_LPAR:
                case C_TKN_INC:
                case C_TKN_DEC:
                case C_TKN_LNOT:
                case C_TKN_NOT: return true;
                default: return false;
            }
    }
}

// Is this a pushable token for `c_parse2_expr`?
static inline bool is_pushable_expr_tkn(c_parser_t *ctx, token_t tkn) {
    (void)ctx;
    switch (tkn.type) {
        case TOKENTYPE_SCONST:
        case TOKENTYPE_CCONST:
        case TOKENTYPE_ICONST:
        case TOKENTYPE_IDENT: return true;
        case TOKENTYPE_KEYWORD:
            switch (tkn.subtype) {
                case C_KEYW_sizeof:
                case C_KEYW_alignof:
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
    switch (token.subtype) {
        case C_TKN_LPAR:
        case C_TKN_LBRAC:
        case C_TKN_LCURL:
        case C_TKN_DOT:
        case C_TKN_ARROW: return 13;

        case C_TKN_NOT:
        case C_TKN_LNOT: return is_prefix ? 12 : -1;

        case C_TKN_INC:
        case C_TKN_DEC: return is_prefix ? 12 : 13;

        case C_TKN_MUL: return is_prefix ? 12 : 11;
        case C_TKN_DIV:
        case C_TKN_MOD: return 11;

        case C_TKN_ADD:
        case C_TKN_SUB: return is_prefix ? 12 : 10;

        case C_TKN_SHL:
        case C_TKN_SHR: return 9;

        case C_TKN_LT:
        case C_TKN_LE:
        case C_TKN_GT:
        case C_TKN_GE: return 8;

        case C_TKN_NE:
        case C_TKN_EQ: return 7;

        case C_TKN_AND: return is_prefix ? 12 : 6;

        case C_TKN_XOR: return 5;

        case C_TKN_OR: return 4;

        case C_TKN_LAND: return 3;

        case C_TKN_LOR: return 2;

        case C_TKN_QUESTION:
        case C_TKN_COLON: return 1;

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

// Is this a valid infix operator token?
static bool is_infix_oper_tkn(token_t token) {
    if (token.type != TOKENTYPE_OTHER) {
        return -1;
    }
    switch (token.subtype) {
        case C_TKN_LPAR:
        case C_TKN_LBRAC:
        case C_TKN_LCURL:
        case C_TKN_DOT:
        case C_TKN_ARROW:

        case C_TKN_INC:
        case C_TKN_DEC:

        case C_TKN_MUL:
        case C_TKN_DIV:
        case C_TKN_MOD:

        case C_TKN_ADD:
        case C_TKN_SUB:

        case C_TKN_SHL:
        case C_TKN_SHR:

        case C_TKN_LT:
        case C_TKN_LE:
        case C_TKN_GT:
        case C_TKN_GE:

        case C_TKN_NE:
        case C_TKN_EQ:

        case C_TKN_AND:

        case C_TKN_XOR:

        case C_TKN_OR:

        case C_TKN_LAND:

        case C_TKN_LOR:

        case C_TKN_ADD_S ... C_TKN_XOR_S:
        case C_TKN_ASSIGN: return true;

        default: return false;
    }
}

// Is this a valid type qualifier token?
static bool is_type_qualifier(token_t token) {
    if (token.type != TOKENTYPE_KEYWORD) {
        return false;
    }
    switch (token.subtype) {
        case C_KEYW_inline:
        case C_KEYW_static:
        case C_KEYW_extern:
        case C_KEYW_register:
        case C_KEYW_typedef:
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
        case C_KEYW__Bool:
        case C_KEYW___int128: return true;
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

// Destroy an LR parser stack entry.
// Does not free the memory of `entry` itself.
static void lr_entry_delete(lr_entry_t entry) {
    switch (entry.tag) {
        case LR_ENTRY_TOKEN: tkn_delete(entry.token); break;
        case LR_ENTRY_EXPR: c_ast_expr_delete(entry.expr); break;
        case LR_ENTRY_TYPE: c_ast_type_name_delete(entry.type); break;
    }
}



// Parse a whole translation unit (all global declarations until EOF).
c_ast_def_list_t *c_parse2(c_parser_t *ctx) {
    vec_c_ast_def_t items = {0};
    token_t         peek  = tkn_peek(ctx->tkn_ctx);
    pos_t           pos   = peek.pos;
    pos.len               = 0;

    while (peek.type != TOKENTYPE_EOF) {
        c_ast_def_t *unit = c_parse2_def(ctx, true);
        pos               = pos_including(pos, unit->pos);
        vec_push(&items, unit);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    return c_ast_def_list_create(pos, items);
}



// Recursive implementation of `c_parse2_comp_init_field`.
static c_ast_init_t *c_parse2_comp_init_field_r(c_parser_t *ctx) {
    token_t peek = tkn_peek(ctx->tkn_ctx);

    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_ASSIGN) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
        return c_ast_init_create_val(c_parse2_compinit_or_expr(ctx));

    } else if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LBRAC) {
        token_t lbrac     = tkn_next(ctx->tkn_ctx);
        pos_t   lbrac_pos = lbrac.pos;
        tkn_delete(lbrac);
        c_ast_expr_t *index = c_parse2_expr(ctx);
        peek                = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RBRAC) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ]");
            pos_t pos = pos_including(lbrac_pos, index->pos);
            c_ast_expr_delete(index);
            return c_ast_init_create_garbage(c_ast_garbage_create(pos));
        }
        token_t rbrac     = tkn_next(ctx->tkn_ctx);
        pos_t   rbrac_pos = rbrac.pos;
        tkn_delete(rbrac);
        c_ast_init_t *inner = c_parse2_comp_init_field_r(ctx);
        if (inner->tag == C_AST_TAG_INIT_GARBAGE) {
            pos_t pos = inner->pos;
            c_ast_init_delete(inner);
            return c_ast_init_create_garbage(c_ast_garbage_create(pos));
        }
        return c_ast_init_create_indexed(c_ast_init_indexed_create(pos_including(lbrac_pos, rbrac_pos), index, inner));

    } else if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_DOT) {
        token_t dot     = tkn_next(ctx->tkn_ctx);
        pos_t   dot_pos = dot.pos;
        tkn_delete(dot);
        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_IDENT) {
            return c_ast_init_create_garbage(c_ast_garbage_create(dot_pos));
        }
        token_t        ident     = tkn_next(ctx->tkn_ctx);
        c_ast_ident_t *ident_ast = c_ast_ident_create(ident.pos, lilycc_strdup(ident.strval));
        tkn_delete(ident);
        c_ast_init_t *inner = c_parse2_comp_init_field_r(ctx);
        if (inner->tag == C_AST_TAG_INIT_GARBAGE) {
            pos_t pos = inner->pos;
            c_ast_init_delete(inner);
            return c_ast_init_create_garbage(c_ast_garbage_create(pos));
        }
        return c_ast_init_create_named(c_ast_init_named_create(dot_pos, ident_ast, inner));

    } else {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected initializer");
        return c_ast_init_create_garbage(c_ast_garbage_create(peek.pos));
    }
}

// Parse a compound initializer field.
c_ast_init_t *c_parse2_comp_init_field(c_parser_t *ctx) {
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || (peek.subtype != C_TKN_DOT && peek.subtype != C_TKN_LBRAC)) {
        return c_ast_init_create_val(c_parse2_compinit_or_expr(ctx));
    } else {
        return c_parse2_comp_init_field_r(ctx);
    }
}

// Parse a compound initializer.
c_ast_init_list_t *c_parse2_comp_init(c_parser_t *ctx) {
    token_t lcurl = tkn_next(ctx->tkn_ctx);
    pos_t   start = lcurl.pos;
    if (lcurl.type != TOKENTYPE_OTHER || lcurl.subtype != C_TKN_LCURL) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, start, DIAG_ERR, "Expected {");
        tkn_delete(lcurl);
        c_eat_delim(ctx->tkn_ctx, true);
        return c_ast_init_list_create(start, (vec_c_ast_init_t){0});
    }
    tkn_delete(lcurl);

    vec_c_ast_init_t fields = {0};
    pos_t            end    = start;

    while (1) {
        // Parse initializer.
        token_t peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type == TOKENTYPE_OTHER
            && (peek.subtype == C_TKN_SEMIC || peek.subtype == C_TKN_RBRAC || peek.subtype == C_TKN_RPAR)) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected expression");
            break;
        }
        if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RCURL) {
            token_t rcurl = tkn_next(ctx->tkn_ctx);
            end           = rcurl.pos;
            tkn_delete(rcurl);
            break;
        }

        c_ast_init_t *field = c_parse2_comp_init_field(ctx);
        end                 = field->pos;
        vec_push(&fields, field);

        // Expect `,` or `}`.
        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RCURL) {
            token_t rcurl = tkn_next(ctx->tkn_ctx);
            end           = rcurl.pos;
            tkn_delete(rcurl);
            break;
        } else if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_COMMA) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ,");
            break;
        }
        tkn_delete(tkn_next(ctx->tkn_ctx));
    }

    return c_ast_init_list_create(pos_including(start, end), fields);
}

// Parse one or more C expressions separated by commas.
c_ast_expr_list_t *c_parse2_exprs(c_parser_t *ctx) {
    vec_c_ast_expr_t args = {0};
    vec_push(&args, c_parse2_expr(ctx));
    pos_t pos = args.arr[0]->pos;

    // While the next token is a comma, more expressions can be parsed.
    token_t tkn = tkn_peek(ctx->tkn_ctx);
    while (tkn.type == TOKENTYPE_OTHER && tkn.subtype == C_TKN_COMMA) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
        c_ast_expr_t *expr = c_parse2_expr(ctx);
        pos                = pos_including(pos, expr->pos);
        vec_push(&args, expr);
        tkn = tkn_peek(ctx->tkn_ctx);
    }

    // When the next token is not a comma, there are no more expressions to parse.
    return c_ast_expr_list_create(pos, args);
}

// Parse a C expression.
c_ast_expr_t *c_parse2_expr(c_parser_t *ctx) {
    // Assert that it starts with a token valid for the beginning of an expr.
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (!is_first_expr_tkn(ctx, peek)) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected expression");
        pos_t pos = peek.pos;
        tkn_delete(tkn_next(ctx->tkn_ctx));
        return c_ast_expr_create_garbage(c_ast_garbage_create(pos));
    }

    vec_lr_entry_t stack = {0};

    // Push a node/token to the stack.
#define push(thing)        vec_push(&stack, thing)
    // Push a token to the stack.
#define push_token(token_) push(((lr_entry_t){.tag = LR_ENTRY_TOKEN, .token = (token_)}))
    // Push an expr to the stack.
#define push_expr(expr_)   push(((lr_entry_t){.tag = LR_ENTRY_EXPR, .expr = (expr_)}))
    // Push a type name to the stack.
#define push_type(type_)   push(((lr_entry_t){.tag = LR_ENTRY_TYPE, .type = (type_)}))
    // Pop a node/token from the stack.
#define pop()                                                                                                          \
    ({                                                                                                                 \
        lr_entry_t pop_temporary_value = stack.arr[stack.len - 1];                                                     \
        stack.len--;                                                                                                   \
        pop_temporary_value;                                                                                           \
    })
    // Pop a token off the stack.
#define pop_token()                                                                                                    \
    ({                                                                                                                 \
        lr_entry_t pop_temporary_value_2 = pop();                                                                      \
        assert(pop_temporary_value_2.tag == LR_ENTRY_TOKEN);                                                           \
        pop_temporary_value_2.token;                                                                                   \
    })
    // Pop an expr off the stack.
#define pop_expr()                                                                                                     \
    ({                                                                                                                 \
        lr_entry_t pop_temporary_value_2 = pop();                                                                      \
        assert(pop_temporary_value_2.tag == LR_ENTRY_EXPR);                                                            \
        pop_temporary_value_2.expr;                                                                                    \
    })
    // Pop a type name off the stack.
#define pop_type()                                                                                                     \
    ({                                                                                                                 \
        lr_entry_t pop_temporary_value_2 = pop();                                                                      \
        assert(pop_temporary_value_2.tag == LR_ENTRY_TYPE);                                                            \
        pop_temporary_value_2.type;                                                                                    \
    })
    // Is this a specific type of KEYWORD token?
#define is_keyw(depth, subtype_)                                                                                       \
    (stack.len > (depth) && stack.arr[stack.len - (depth) - 1].tag == LR_ENTRY_TOKEN                                   \
     && stack.arr[stack.len - (depth) - 1].token.type == TOKENTYPE_KEYWORD                                             \
     && stack.arr[stack.len - (depth) - 1].token.subtype == (subtype_))
    // Is this a specific type of OTHER token?
#define is_punct(depth, subtype_)                                                                                      \
    (stack.len > (depth) && stack.arr[stack.len - (depth) - 1].tag == LR_ENTRY_TOKEN                                   \
     && stack.arr[stack.len - (depth) - 1].token.type == TOKENTYPE_OTHER                                               \
     && stack.arr[stack.len - (depth) - 1].token.subtype == (subtype_))
    // Is this a certain kind of token?
#define is_token(depth, type_)                                                                                         \
    (stack.len > (depth) && stack.arr[stack.len - (depth) - 1].tag == LR_ENTRY_TOKEN                                   \
     && stack.arr[stack.len - (depth) - 1].token.type == (type_))
    // Is this an expression node?
#define is_expr(depth) (stack.len > (depth) && stack.arr[stack.len - (depth) - 1].tag == LR_ENTRY_EXPR)
    // Is this a type node?
#define is_type(depth) (stack.len > (depth) && stack.arr[stack.len - (depth) - 1].tag == LR_ENTRY_TYPE)

    // How many `?` there are without matching `:`.
    size_t tern_count = 0;
    while (1) {
        peek          = tkn_peek(ctx->tkn_ctx);
        bool can_push = is_pushable_expr_tkn(ctx, peek);

        if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_COLON && tern_count == 0) {
            can_push = false;
        }

        if (is_punct(0, C_TKN_LBRAC)) { // Recursively parse indexing.
            c_ast_expr_t *rhs = c_parse2_expr(ctx);
            if (rhs->tag == C_AST_TAG_EXPR_GARBAGE) {
                push_expr(rhs);
                goto err;
            }
            token_t peek2 = tkn_peek(ctx->tkn_ctx);
            if (peek2.type != TOKENTYPE_OTHER || peek2.subtype != C_TKN_RBRAC) {
                cctx_diagnostic(ctx->tkn_ctx->cctx, peek2.pos, DIAG_ERR, "Expected ]");
                push_expr(rhs);
                goto err;
            }
            if (!is_expr(1)) {
                cctx_diagnostic(ctx->tkn_ctx->cctx, peek2.pos, DIAG_ERR, "Expected expression before this [");
                push_expr(rhs);
                goto err;
            }
            pos_t rbrac_pos = peek2.pos;
            tkn_delete(tkn_next(ctx->tkn_ctx));
            lr_entry_delete(pop());
            c_ast_expr_t *lhs = pop_expr();
            push_expr(c_ast_expr_create_index(c_ast_expr_index_create(pos_including(lhs->pos, rbrac_pos), lhs, rhs)));

        } else if (is_punct(0, C_TKN_LPAR)) { // Recursively parse exprs.
            token_t lpar = pop_token();
            void   *res;
            bool    is_type = false;
            if (is_expr(0) && peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
                // Function call may have zero params.
                pos_t pos = peek.pos;
                pos.len   = 0;
                res       = c_ast_expr_list_create(pos, (vec_c_ast_expr_t){0});
            } else {
                // If not a function call, then it must have something in the parentheses.
                res = c_parse2_exprs_or_type(ctx, &is_type);
            }
            token_t rpar = tkn_peek(ctx->tkn_ctx);
            if (rpar.type != TOKENTYPE_OTHER || rpar.subtype != C_TKN_RPAR) {
                if (is_type) {
                    push_type(res);
                } else {
                    push_expr(c_ast_expr_create_exprs(res));
                }
                tkn_delete(lpar);
                cctx_diagnostic(ctx->tkn_ctx->cctx, rpar.pos, DIAG_ERR, "Expected )");
                goto err;
            }
            tkn_delete(tkn_next(ctx->tkn_ctx));
            pos_t pos = pos_including(lpar.pos, rpar.pos);
            if (is_type) {
                ((c_ast_type_name_t *)res)->pos = pos;
                push_type(res);
            } else {
                ((c_ast_expr_list_t *)res)->pos = pos;
                push_expr(c_ast_expr_create_exprs(res));
            }

        } else if (
            is_type(0) //
            && peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LCURL
        ) { // Recursively parse compound literals.
            c_ast_init_list_t *init     = c_parse2_comp_init(ctx);
            c_ast_type_name_t *typename = pop_type();
            push_expr(c_ast_expr_create_compliteral(
                c_ast_expr_compliteral_create(pos_including(init->pos, typename->pos), typename, init)
            ));

        } else if (
            is_expr(1) && is_expr(0) //
            && stack.arr[stack.len - 1].expr->tag == C_AST_TAG_EXPRS
        ) { // Reduce call.
            c_ast_expr_t      *wrapped_params = pop_expr();
            c_ast_expr_t      *func           = pop_expr();
            pos_t              pos            = pos_including(func->pos, wrapped_params->pos);
            c_ast_expr_list_t *params         = wrapped_params->expr_exprs;
            lilycc_free(wrapped_params);
            push_expr(c_ast_expr_create_call(c_ast_expr_call_create(pos, func, params)));

        } else if (is_expr(0) && is_type(1)) { // Reduce cast.
            c_ast_expr_t *val           = pop_expr();
            c_ast_type_name_t *typename = pop_type();
            push_expr(
                c_ast_expr_create_cast(c_ast_expr_cast_create(pos_including(val->pos, typename->pos), typename, val))
            );

        } else if (is_expr(1) && (is_punct(0, C_TKN_INC) || is_punct(0, C_TKN_DEC))) { // Reduce suffix.
            token_t       op       = pop_token();
            c_tokentype_t oper     = op.subtype;
            pos_t         oper_pos = op.pos;
            tkn_delete(op);
            c_ast_expr_t *val = pop_expr();
            push_expr(c_ast_expr_create_suffix(
                c_ast_expr_suffix_create(pos_including(oper_pos, val->pos), oper, oper_pos, val)
            ));

        } else if (
            !is_expr(2) && is_token(1, TOKENTYPE_OTHER) && is_expr(0)
            && is_prefix_oper_tkn(stack.arr[stack.len - 2].token)
            && oper_precedence(stack.arr[stack.len - 2].token, true) >= oper_precedence(peek, false)
        ) { // Reduce prefix.
            c_ast_expr_t *val      = pop_expr();
            token_t       op       = pop_token();
            c_tokentype_t oper     = op.subtype;
            pos_t         oper_pos = op.pos;
            tkn_delete(op);
            push_expr(c_ast_expr_create_prefix(
                c_ast_expr_prefix_create(pos_including(oper_pos, val->pos), oper, oper_pos, val)
            ));

        } else if (
            is_expr(2) && is_expr(0) && is_infix_oper_tkn(stack.arr[stack.len - 2].token)
            && (!can_push || oper_precedence(stack.arr[stack.len - 2].token, false) >= oper_precedence(peek, false))
        ) { // Reduce infix.
            c_ast_expr_t *rhs      = pop_expr();
            token_t       op       = pop_token();
            c_ast_expr_t *lhs      = pop_expr();
            c_tokentype_t oper     = op.subtype;
            pos_t         oper_pos = op.pos;
            tkn_delete(op);
            push_expr(c_ast_expr_create_infix(
                c_ast_expr_infix_create(pos_including(lhs->pos, rhs->pos), lhs, oper, oper_pos, rhs)
            ));

        } else if (
            is_expr(4) && is_punct(3, C_TKN_QUESTION) && is_expr(2) && is_punct(1, C_TKN_COLON) && is_expr(0)
            && oper_precedence(stack.arr[stack.len - 2].token, true) >= oper_precedence(peek, false)
        ) { // Reduce ternary.
            c_ast_expr_t *else_expr = pop_expr();
            token_t       colon     = pop_token();
            c_ast_expr_t *if_expr   = is_expr(0) ? pop_expr() : NULL;
            token_t       question  = pop_token();
            c_ast_expr_t *cond      = pop_expr();
            tkn_delete(colon);
            tkn_delete(question);
            push_expr(c_ast_expr_create_ternary(
                c_ast_expr_ternary_create(pos_including(cond->pos, else_expr->pos), cond, if_expr, else_expr)
            ));

        } else if (is_token(0, TOKENTYPE_ICONST) || is_token(0, TOKENTYPE_CCONST)) { // Reduce iconst / cconst to expr.
            token_t  tkn  = pop_token();
            pos_t    pos  = tkn.pos;
            c_prim_t prim = tkn.type == TOKENTYPE_CCONST ? C_PRIM_CHAR : tkn.subtype;
            i128_t   val  = i128_pack(tkn.ivalh, tkn.ival);
            tkn_delete(tkn);
            push_expr(c_ast_expr_create_iconst(c_ast_expr_iconst_create(pos, prim, val)));

        } else if (is_keyw(0, C_KEYW_true)) { // Reduce true.
            token_t  tkn  = pop_token();
            pos_t    pos  = tkn.pos;
            c_prim_t prim = C_PRIM_BOOL;
            i128_t   val  = ui128(1);
            tkn_delete(tkn);
            push_expr(c_ast_expr_create_iconst(c_ast_expr_iconst_create(pos, prim, val)));

        } else if (is_keyw(0, C_KEYW_false)) { // Reduce false.
            token_t  tkn  = pop_token();
            pos_t    pos  = tkn.pos;
            c_prim_t prim = C_PRIM_BOOL;
            i128_t   val  = UI128_ZERO;
            tkn_delete(tkn);
            push_expr(c_ast_expr_create_iconst(c_ast_expr_iconst_create(pos, prim, val)));

        } else if (is_token(0, TOKENTYPE_IDENT)) { // Reduce ident to expr.
            token_t tkn = pop_token();
            pos_t   pos = tkn.pos;
            char   *val = lilycc_strdup(tkn.strval);
            tkn_delete(tkn);
            push_expr(c_ast_expr_create_ident(c_ast_ident_create(pos, val)));

        } else if (is_token(1, TOKENTYPE_SCONST) && is_token(0, TOKENTYPE_SCONST)) { // Reduce sconst pasting.
            token_t rhs    = pop_token();
            token_t lhs    = pop_token();
            char   *strval = lilycc_malloc(lhs.strval_len + rhs.strval_len + 1);
            memcpy(strval, lhs.strval, lhs.strval_len);
            memcpy(strval + lhs.strval_len, rhs.strval, rhs.strval_len);
            strval[lhs.strval_len + rhs.strval_len] = 0;
            token_t combined                        = {
                .pos        = pos_including(lhs.pos, rhs.pos),
                .type       = TOKENTYPE_SCONST,
                .strval     = strval,
                .strval_len = lhs.strval_len + rhs.strval_len,
            };
            tkn_delete(lhs);
            tkn_delete(rhs);
            push_token(combined);

        } else if (is_token(0, TOKENTYPE_SCONST) && peek.type != TOKENTYPE_SCONST) { // Reduce sconst to expr.
            token_t    tkn = pop_token();
            pos_t      pos = tkn.pos;
            vec_char_t val = {0};
            // The `strval`, being a C-string too is always NUL-terminated.
            // We simply copy in its NUL terminator too here.
            vec_reserve_exact(&val, tkn.strval_len + 1);
            val.len = tkn.strval_len + 1;
            memcpy(val.arr, tkn.strval, val.len);
            tkn_delete(tkn);
            push_expr(c_ast_expr_create_sconst(c_ast_expr_sconst_create(pos, val)));

        } else if (can_push) { // Push next token.
            token_t next = tkn_next(ctx->tkn_ctx);
            if (next.type == TOKENTYPE_OTHER && next.subtype == C_TKN_QUESTION) {
                tern_count++;
            } else if (next.type == TOKENTYPE_OTHER && next.subtype == C_TKN_COLON) {
                tern_count--; // Can't underflow because of a check before the if statement cascade.
            }
            push_token(next);

        } else { // Can't reduce anything.
            break;
        }
    }

#undef push
#undef push_token
#undef push_expr
#undef push_type
#undef pop
#undef pop_token
#undef pop_expr
#undef pop_type
#undef is_keyw
#undef is_punct
#undef is_token
#undef is_expr
#undef is_type

    if (stack.len > 1) {
        // Invalid expression.
        pos_t pos;
        switch (stack.arr[1].tag) {
            case LR_ENTRY_TOKEN: pos = stack.arr[1].token.pos; break;
            case LR_ENTRY_EXPR: pos = stack.arr[1].expr->pos; break;
            case LR_ENTRY_TYPE: pos = stack.arr[1].type->pos; break;
        }
        cctx_diagnostic(ctx->tkn_ctx->cctx, pos, DIAG_ERR, "Expected end of expression or operator");
        goto err;
    } else if (stack.arr[0].tag != LR_ENTRY_EXPR) {
        // Not an expression
        pos_t pos;
        switch (stack.arr[0].tag) {
            case LR_ENTRY_TOKEN: pos = stack.arr[0].token.pos; break;
            case LR_ENTRY_EXPR: pos = stack.arr[0].expr->pos; break;
            case LR_ENTRY_TYPE: pos = stack.arr[0].type->pos; break;
        }
        cctx_diagnostic(ctx->tkn_ctx->cctx, pos, DIAG_ERR, "Expected expression");
        goto err;
    } else {
        // Valid expression.
        c_ast_expr_t *expr = stack.arr[0].expr;
        vec_clear(&stack);
        return expr;
    }

err:;
    pos_t pos0;
    switch (stack.arr[0].tag) {
        case LR_ENTRY_TOKEN: pos0 = stack.arr[0].token.pos; break;
        case LR_ENTRY_EXPR: pos0 = stack.arr[0].expr->pos; break;
        case LR_ENTRY_TYPE: pos0 = stack.arr[0].type->pos; break;
    }
    if (stack.len > 1) {
        pos_t pos1;
        switch (stack.arr[1].tag) {
            case LR_ENTRY_TOKEN: pos1 = stack.arr[1].token.pos; break;
            case LR_ENTRY_EXPR: pos1 = stack.arr[1].expr->pos; break;
            case LR_ENTRY_TYPE: pos1 = stack.arr[1].type->pos; break;
        }
        pos0 = pos_including(pos0, pos1);
    }

    for (size_t i = 0; i < stack.len; i++) {
        lr_entry_delete(stack.arr[i]);
    }
    vec_clear(&stack);

    return c_ast_expr_create_garbage(c_ast_garbage_create(pos0));
}

// Parse a type name.
c_ast_type_name_t *c_parse2_type_name(c_parser_t *ctx) {
    bool                    is_typedef;
    c_ast_spec_qual_list_t *spec_qual = c_parse2_spec_qual_list(ctx, &is_typedef);
    if (is_typedef) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, spec_qual->pos, DIAG_ERR, "`typedef` not allowed here");
    }
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if ((peek.type == TOKENTYPE_OTHER
         && (peek.subtype == C_TKN_MUL || peek.subtype == C_TKN_LBRAC || peek.subtype == C_TKN_LPAR))
        || peek.subtype == TOKENTYPE_IDENT) {
        c_ast_decl_t *decl = c_parse2_decl(ctx, false, false);
        return c_ast_type_name_create(pos_including(spec_qual->pos, decl->pos), spec_qual, decl);
    } else {
        return c_ast_type_name_create(spec_qual->pos, spec_qual, NULL);
    }
}


// Parse a direct (abstract) declaration.
static c_ast_decl_t *c_parse2_ddecl(c_parser_t *ctx, bool allows_name, bool is_typedef) {
    token_t       peek  = tkn_peek(ctx->tkn_ctx);
    token_t       peek1 = tkn_peek_n(ctx->tkn_ctx, 1);
    c_ast_decl_t *inner;

    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LPAR && peek1.type == TOKENTYPE_OTHER
        && (peek1.subtype == C_TKN_MUL || peek1.subtype == C_TKN_LPAR || peek1.subtype == C_TKN_LBRAC)) {
        // Parenthesized declarator.
        token_t lpar = tkn_next(ctx->tkn_ctx);

        inner = c_parse2_decl(ctx, allows_name, is_typedef);
        if (inner->tag == C_AST_TAG_DECL_GARBAGE) {
            tkn_delete(lpar);
            return inner;
        }
        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
            pos_t pos = pos_including(lpar.pos, inner->pos);
            tkn_delete(lpar);
            return c_ast_decl_create_garbage(c_ast_garbage_create(pos));
        }
        token_t rpar = tkn_next(ctx->tkn_ctx);
        inner->pos   = pos_including(lpar.pos, rpar.pos);
        tkn_delete(lpar);
        tkn_delete(rpar);

    } else if (peek.type == TOKENTYPE_IDENT && allows_name) {
        // Identifier.
        token_t tkn = tkn_next(ctx->tkn_ctx);
        inner       = c_ast_decl_create_ident(c_ast_ident_create(tkn.pos, lilycc_strdup(tkn.strval)));
        if (is_typedef) {
            if (ctx->func_body) {
                set_add(&ctx->local_type_names, tkn.strval);
            } else {
                set_add(&ctx->type_names, tkn.strval);
            }
        }
        tkn_delete(tkn);

    } else if (peek.type != TOKENTYPE_OTHER || (peek.subtype != C_TKN_LBRAC && peek.subtype != C_TKN_LPAR)) {
        // Garbaj.
        pos_t pos = peek.pos;
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ( or [");
        tkn_delete(tkn_next(ctx->tkn_ctx));
        pos.len = 0;
        return c_ast_decl_create_garbage(c_ast_garbage_create(pos));
    } else {
        inner = NULL;
    }

    peek = tkn_peek(ctx->tkn_ctx);
    while (peek.type == TOKENTYPE_OTHER && (peek.subtype == C_TKN_LBRAC || peek.subtype == C_TKN_LPAR)) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
        if (peek.subtype == C_TKN_LBRAC) {
            // Array type.
            peek  = tkn_peek(ctx->tkn_ctx);
            peek1 = tkn_peek_n(ctx->tkn_ctx, 1);

            if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_MUL && peek1.type == TOKENTYPE_OTHER
                && peek1.subtype == C_TKN_RBRAC) {
                // [*] style undimensioned array.
                peek = peek1;
                tkn_delete(tkn_next(ctx->tkn_ctx));
            }

            if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RBRAC) {
                // Undimensioned array.
                pos_t pos = inner ? pos_including(inner->pos, peek.pos) : peek.pos;
                inner     = c_ast_decl_create_array(c_ast_decl_array_create(pos, inner, NULL));
                tkn_delete(tkn_next(ctx->tkn_ctx));
            } else {
                // Dimensioned array.
                pos_t         pos  = inner ? pos_including(inner->pos, peek.pos) : peek.pos;
                c_ast_expr_t *expr = c_ast_expr_create_exprs(c_parse2_exprs(ctx));
                peek               = tkn_peek(ctx->tkn_ctx);
                if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RBRAC) {
                    tkn_delete(tkn_next(ctx->tkn_ctx));
                } else {
                    cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ]");
                }
                inner = c_ast_decl_create_array(c_ast_decl_array_create(pos, inner, expr));
            }

        } else {
            // Function type.
            peek      = tkn_peek(ctx->tkn_ctx);
            pos_t pos = peek.pos;
            pos.len   = 0;

            vec_c_ast_arg_def_t params = {0};
            if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
                // Parse function args.
                while (1) {
                    bool                    is_typedef;
                    c_ast_spec_qual_list_t *param_qual = c_parse2_spec_qual_list(ctx, &is_typedef);
                    if (is_typedef) {
                        cctx_diagnostic(ctx->tkn_ctx->cctx, param_qual->pos, DIAG_ERR, "`typedef` not allowed here");
                    }
                    peek = tkn_peek(ctx->tkn_ctx);
                    if (peek.type == TOKENTYPE_OTHER && (peek.subtype == C_TKN_RPAR || peek.subtype == C_TKN_COMMA)) {
                        vec_push(&params, c_ast_arg_def_create(param_qual->pos, param_qual, NULL));
                    } else {
                        c_ast_decl_t *param_decl = c_parse2_decl(ctx, true, false);
                        vec_push(&params, c_ast_arg_def_create(param_qual->pos, param_qual, param_decl));
                        peek = tkn_peek(ctx->tkn_ctx);
                    }
                    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_COMMA) {
                        tkn_delete(tkn_next(ctx->tkn_ctx));
                    } else if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
                        pos = pos_including(pos, peek.pos);
                        break;
                    }
                }
            }

            if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
                tkn_delete(tkn_next(ctx->tkn_ctx));
            } else {
                pos = pos_between(pos, peek.pos);
                cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
            }
            inner = c_ast_decl_create_func(
                c_ast_decl_func_create(pos, inner, c_ast_arg_def_list_create(pos, params), false)
            );
        }

        peek = tkn_peek(ctx->tkn_ctx);
    }

    return inner;
}

// Parse an (abstract) declaration.
static c_ast_decl_t *c_parse2_decl(c_parser_t *ctx, bool allows_name, bool is_typedef) {
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_MUL) {
        // If no `*`, there is required to be a direct decl.
        return c_parse2_ddecl(ctx, allows_name, is_typedef);
    }

    // Parse the pointer before the ddecl.
    token_t ptr     = tkn_next(ctx->tkn_ctx);
    pos_t   ptr_pos = ptr.pos;
    tkn_delete(ptr);
    c_ast_spec_qual_list_t *list = c_parse2_type_qual_list(ctx);
    peek                         = tkn_peek(ctx->tkn_ctx);
    bool empty = peek.type != TOKENTYPE_OTHER
                 || (peek.subtype != C_TKN_MUL && peek.subtype != C_TKN_LPAR && peek.subtype != C_TKN_LBRAC);
    if (allows_name && peek.type == TOKENTYPE_IDENT) {
        // Non-abstract decls can have idents here too.
        empty = false;
    }
    c_ast_decl_t *inner = NULL;
    pos_t         pos   = ptr_pos;
    if (!empty) {
        inner = c_parse2_decl(ctx, allows_name, is_typedef);
        pos   = pos_including(ptr_pos, inner->pos);
    }
    return c_ast_decl_create_ptr(c_ast_decl_ptr_create(pos, ptr_pos, list, inner));
}

// Parse a type qualifier list.
static c_ast_spec_qual_list_t *c_parse2_type_qual_list(c_parser_t *ctx) {
    vec_c_ast_spec_qual_t args = {0};

    token_t peek = tkn_peek(ctx->tkn_ctx);
    pos_t   pos  = peek.pos;
    pos.len      = 0;
    while (is_type_qualifier(peek)) {
        pos         = pos_including(pos, pos);
        token_t tkn = tkn_next(ctx->tkn_ctx);
        vec_push(&args, c_ast_spec_qual_create_keyw(tkn.pos, tkn.subtype));
        tkn_delete(tkn);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    return c_ast_spec_qual_list_create(pos, args);
}

// Parse one or more C expressions separated by commas or a type.
// The return type is either `c_ast_type_name_t *` or `c_ast_expr_list_t *`.
static void *c_parse2_exprs_or_type(c_parser_t *ctx, bool *is_type_out) {
    token_t tkn  = tkn_peek(ctx->tkn_ctx);
    *is_type_out = is_spec_qual_list_tkn(ctx, tkn);
    if (*is_type_out) {
        return c_parse2_type_name(ctx);
    } else {
        return c_parse2_exprs(ctx);
    }
}

// Parse a compound initializer or expression.
static c_ast_initval_t *c_parse2_compinit_or_expr(c_parser_t *ctx) {
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LCURL) {
        return c_ast_initval_create_compound(c_parse2_comp_init(ctx));
    } else {
        return c_ast_initval_create_expr(c_parse2_expr(ctx));
    }
}

// Parse a type specifier/qualifier list.
c_ast_spec_qual_list_t *c_parse2_spec_qual_list(c_parser_t *ctx, bool *is_typedef_out) {
    vec_c_ast_spec_qual_t args = {0};
    *is_typedef_out            = false;
    bool seen_type_spec        = false;

    token_t peek = tkn_peek(ctx->tkn_ctx);
    pos_t   pos  = peek.pos;
    pos.len      = 0;
    while (1) {
        if (is_type_specifier(peek) || is_type_qualifier(peek)) {
            if (peek.type == TOKENTYPE_KEYWORD && peek.subtype == C_KEYW_typedef && is_typedef_out) {
                *is_typedef_out = true;
            }

            seen_type_spec |= is_type_specifier(peek);

            // Token added verbatim.
            token_t tkn = tkn_next(ctx->tkn_ctx);
            vec_push(&args, c_ast_spec_qual_create_keyw(tkn.pos, tkn.subtype));
            tkn_delete(tkn);

        } else if (
            peek.type == TOKENTYPE_IDENT
            && (set_contains(&ctx->type_names, peek.strval)
                || (ctx->func_body && set_contains(&ctx->local_type_names, peek.strval)))
        ) {
            // Identifier added verbatim.
            if (seen_type_spec) {
                // This check guards against a parsing error that might happen if you `typedef int foo` and proceed to
                // use `foo` as the name of another decl.
                break;
            }
            token_t tkn = tkn_next(ctx->tkn_ctx);
            vec_push(&args, c_ast_spec_qual_create_typedef(c_ast_ident_create(tkn.pos, lilycc_strdup(tkn.strval))));
            tkn_delete(tkn);
            seen_type_spec = true;

        } else if (peek.type == TOKENTYPE_KEYWORD && (peek.subtype == C_KEYW_struct || peek.subtype == C_KEYW_union)) {
            // Parse a struct/union specifier.
            vec_push(&args, c_ast_spec_qual_create_struct(c_parse2_struct_spec(ctx)));
            seen_type_spec = true;

        } else if (peek.type == TOKENTYPE_KEYWORD && (peek.subtype == C_KEYW_enum)) {
            // Parse an enum specifier.
            vec_push(&args, c_ast_spec_qual_create_enum(c_parse2_enum_spec(ctx)));
            seen_type_spec = true;

        } else {
            // Not valid in a specifier/qualifier list.
            break;
        }
        pos  = pos_including(pos, peek.pos);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    return c_ast_spec_qual_list_create(pos, args);
}

// Parse a `_Static_assert(cond);` or `_Static_assert(cond, message);` declaration.
// Caller has confirmed the next token is C_KEYW__Static_assert.
static c_ast_def_t *c_parse2_static_assert(c_parser_t *ctx) {
    token_t keyw = tkn_next(ctx->tkn_ctx);
    pos_t   pos  = keyw.pos;
    tkn_delete(keyw);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected (");
        c_eat_delim(ctx->tkn_ctx, false);
        return c_ast_def_create_garbage(c_ast_garbage_create(pos));
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    c_ast_expr_t *cond    = c_parse2_expr(ctx);
    c_ast_expr_t *message = NULL;

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_COMMA) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
        message = c_parse2_expr(ctx);
        peek    = tkn_peek(ctx->tkn_ctx);
    }

    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
    } else {
        pos = pos_including(pos, peek.pos);
        tkn_delete(tkn_next(ctx->tkn_ctx));
    }

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
        c_eat_delim(ctx->tkn_ctx, false);
    } else {
        pos = pos_including(pos, peek.pos);
        tkn_delete(tkn_next(ctx->tkn_ctx));
    }

    return c_ast_def_create_static_assert(c_ast_def_static_assert_create(pos, cond, message));
}

// Parse a variable/function declarations/definition.
c_ast_def_t *c_parse2_def(c_parser_t *ctx, bool allow_func_body) {
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_KEYWORD && peek.subtype == C_KEYW__Static_assert) {
        return c_parse2_static_assert(ctx);
    }

    vec_c_ast_init_decl_t   decls = {0};
    bool                    is_typedef;
    c_ast_spec_qual_list_t *spec_qual = c_parse2_spec_qual_list(ctx, &is_typedef);


    // Decls are actually allowed to be empty.
    peek            = tkn_peek(ctx->tkn_ctx);
    pos_t decls_pos = peek.pos;
    decls_pos.len   = 0;
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_SEMIC) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
        goto exit;
    }

    do {
        if (decls.len) {
            // There's already a decl; function body no longer allowed.
            tkn_delete(tkn_next(ctx->tkn_ctx));
            allow_func_body = false;
        }
        c_ast_decl_t *decl    = c_parse2_decl(ctx, true, is_typedef);
        peek                  = tkn_peek(ctx->tkn_ctx);
        c_ast_initval_t *init = NULL;
        if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_ASSIGN) {
            tkn_delete(tkn_next(ctx->tkn_ctx));
            init = c_parse2_compinit_or_expr(ctx);
            peek = tkn_peek(ctx->tkn_ctx);
        }

        pos_t decl_pos = init ? pos_including(decl->pos, init->pos) : decl->pos;
        decls_pos      = pos_including(decls_pos, decl_pos);
        vec_push(&decls, c_ast_init_decl_create(decl_pos, decl, init));
    } while (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_COMMA);

    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LCURL) {
        // Parse function body.
        if (!allow_func_body) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
            goto garbage;
        }

        tkn_delete(tkn_next(ctx->tkn_ctx));
        c_ast_stmt_list_t *body = c_parse2_stmts(ctx);
        decls_pos               = pos_including(decls_pos, body->pos);

        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected }");
        } else {
            decls_pos = pos_including(decls_pos, peek.pos);
            tkn_delete(tkn_next(ctx->tkn_ctx));
        }

        if (!allow_func_body) {
            goto garbage;
        }

        c_ast_decl_t *decl = decls.arr[0]->decl;
        decls.arr[0]->decl = NULL;
        c_ast_init_decl_delete(decls.arr[0]);
        vec_clear(&decls);
        return c_ast_def_create_func(
            c_ast_def_func_create(pos_including(spec_qual->pos, decls_pos), spec_qual, decl, body)
        );

    } else if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
        // Should have been a semicolon here.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
        c_eat_delim(ctx->tkn_ctx, false);
    } else {
        decls_pos = pos_including(decls_pos, peek.pos);
        tkn_delete(tkn_next(ctx->tkn_ctx));
    }

exit:
    return c_ast_def_create_defs(c_ast_defs_create(
        pos_including(spec_qual->pos, decls_pos),
        spec_qual,
        c_ast_init_decl_list_create(decls_pos, decls)
    ));

garbage:
    for (size_t i = 0; i < decls.len; i++) {
        c_ast_init_decl_delete(decls.arr[i]);
    }
    vec_clear(&decls);
    return c_ast_def_create_garbage(c_ast_garbage_create(pos_including(spec_qual->pos, decls_pos)));
}

// Parse a struct or union specifier/definition.
c_ast_struct_spec_t *c_parse2_struct_spec(c_parser_t *ctx) {
    c_ast_def_list_t *body = NULL;
    c_ast_ident_t    *name = NULL;

    token_t keyw     = tkn_next(ctx->tkn_ctx);
    pos_t   keyw_pos = keyw.pos;
    bool    is_union = keyw.subtype == C_KEYW_union;
    tkn_delete(keyw);
    pos_t   pos  = keyw_pos;
    token_t peek = tkn_peek(ctx->tkn_ctx);

    if (peek.type == TOKENTYPE_IDENT) {
        token_t tkn = tkn_next(ctx->tkn_ctx);
        name        = c_ast_ident_create(tkn.pos, lilycc_strdup(tkn.strval));
        tkn_delete(tkn);
        pos  = pos_including(pos, name->pos);
        peek = tkn_peek(ctx->tkn_ctx);

        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LCURL) {
            // Just a enum/struct/union name; don't parse args.
            goto exit;
        }
    }

    if (!name && (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LCURL)) {
        // There should be a decl here since it's anonymous.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected {");
        return c_ast_struct_spec_create(pos, is_union, keyw_pos, NULL, NULL);
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    vec_c_ast_def_t args = {0};
    peek                 = tkn_peek(ctx->tkn_ctx);
    pos_t body_pos       = peek.pos;
    body_pos.len         = 0;
    while (peek.type != TOKENTYPE_EOF && (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL)) {
        c_ast_def_t *def = c_parse2_def(ctx, false);
        body_pos         = pos_including(body_pos, def->pos);
        vec_push(&args, def);
        peek = tkn_peek(ctx->tkn_ctx);
    }
    body = c_ast_def_list_create(body_pos, args);
    pos  = pos_including(pos, body_pos);

    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL) {
        // There should be a decl here since it's anonymous.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected }");
    } else {
        pos = pos_including(pos, peek.pos);
        tkn_delete(tkn_next(ctx->tkn_ctx));
    }

exit:
    return c_ast_struct_spec_create(pos, is_union, keyw_pos, name, body);
}

// Parse an enum specifier/definition.
c_ast_enum_spec_t *c_parse2_enum_spec(c_parser_t *ctx) {
    c_ast_enumvar_list_t *body = NULL;
    c_ast_ident_t        *name = NULL;

    token_t keyw     = tkn_next(ctx->tkn_ctx);
    pos_t   keyw_pos = keyw.pos;
    tkn_delete(keyw);
    pos_t   pos  = keyw_pos;
    token_t peek = tkn_peek(ctx->tkn_ctx);

    if (peek.type == TOKENTYPE_IDENT) {
        token_t tkn = tkn_next(ctx->tkn_ctx);
        name        = c_ast_ident_create(tkn.pos, lilycc_strdup(tkn.strval));
        tkn_delete(tkn);
        pos  = pos_including(pos, name->pos);
        peek = tkn_peek(ctx->tkn_ctx);

        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LCURL) {
            // Just a enum/struct/union name; don't parse args.
            goto exit;
        }
    }

    if (!name && (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LCURL)) {
        // There should be a decl here since it's anonymous.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected {");
        return c_ast_enum_spec_create(pos, keyw_pos, NULL, NULL);
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    vec_c_ast_enumvar_t args = {0};
    peek                     = tkn_peek(ctx->tkn_ctx);
    pos_t body_pos           = peek.pos;
    body_pos.len             = 0;
    while (1) {
        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type == TOKENTYPE_EOF || (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RCURL)) {
            break;
        } else if (peek.type != TOKENTYPE_IDENT) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected identifier or }");
            break;
        } else {
            token_t        ident     = tkn_next(ctx->tkn_ctx);
            c_ast_ident_t *ident_ast = c_ast_ident_create(ident.pos, lilycc_strdup(ident.strval));
            tkn_delete(ident);
            peek = tkn_peek(ctx->tkn_ctx);

            if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_ASSIGN) {
                // Enum variant with specific index.
                tkn_delete(tkn_next(ctx->tkn_ctx));
                c_ast_expr_t *expr = c_parse2_expr(ctx);
                vec_push(&args, c_ast_enumvar_create(pos_including(ident_ast->pos, expr->pos), ident_ast, expr));

                peek = tkn_peek(ctx->tkn_ctx);
            } else {
                // Enum variant with implicit index.
                vec_push(&args, c_ast_enumvar_create(ident_ast->pos, ident_ast, NULL));
            }
            body_pos = pos_including(body_pos, args.arr[args.len - 1]->pos);
            if (peek.type != TOKENTYPE_OTHER || (peek.subtype != C_TKN_COMMA && peek.subtype != C_TKN_RCURL)) {
                cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ,");
                c_eat_delim(ctx->tkn_ctx, true);
            } else if (peek.subtype == C_TKN_COMMA) {
                tkn_delete(tkn_next(ctx->tkn_ctx));
            }
        }
    }
    body = c_ast_enumvar_list_create(body_pos, args);
    pos  = pos_including(pos, body_pos);

    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL) {
        // There should be a decl here since it's anonymous.
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected }");
    } else {
        pos = pos_including(pos, peek.pos);
        tkn_delete(tkn_next(ctx->tkn_ctx));
    }

exit:
    return c_ast_enum_spec_create(pos, keyw_pos, name, body);
}

// Parse a switch statement.
static c_ast_stmt_t *c_parse2_switch(c_parser_t *ctx) {
    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && keyw.subtype == C_KEYW_switch);
    pos_t keyw_pos = keyw.pos;
    tkn_delete(keyw);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected (");
        return c_ast_stmt_create_garbage(c_ast_garbage_create(keyw_pos));
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    c_ast_expr_list_t *cond = c_parse2_exprs(ctx);

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
        pos_t pos = pos_including(keyw_pos, cond->pos);
        c_ast_expr_list_delete(cond);
        return c_ast_stmt_create_garbage(c_ast_garbage_create(pos));
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    c_ast_stmt_t *body = c_parse2_stmt(ctx);
    return c_ast_stmt_create_switch(
        c_ast_stmt_switch_create(pos_including(keyw_pos, body->pos), c_ast_expr_create_exprs(cond), body)
    );
}

// Parse a case statement.
static c_ast_stmt_t *c_parse2_case(c_parser_t *ctx) {
    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && keyw.subtype == C_KEYW_case);
    pos_t keyw_pos = keyw.pos;
    tkn_delete(keyw);

    c_ast_expr_t *lo = c_parse2_expr(ctx);
    c_ast_expr_t *hi = NULL;

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_ELIPSIS) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
        hi   = c_parse2_expr(ctx);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_COLON) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
    } else {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected :");
    }

    c_ast_stmt_t *inner = c_parse2_stmt(ctx);
    c_ast_stmt_t *res
        = c_ast_stmt_create_case(c_ast_stmt_case_create(pos_including(keyw_pos, inner->pos), lo, hi, inner));
    return res;
}

// Parse a do...while statement.
static c_ast_stmt_t *c_parse2_do_while(c_parser_t *ctx) {
    pos_t err_pos;

    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && keyw.subtype == C_KEYW_do);
    pos_t keyw_pos = keyw.pos;
    tkn_delete(keyw);

    c_ast_stmt_t *body = c_parse2_stmt(ctx);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_KEYWORD || peek.subtype != C_KEYW_while) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected while");
        err_pos = pos_including(keyw_pos, body->pos);
        goto err1;
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected (");
        err_pos = pos_including(keyw_pos, body->pos);
        goto err1;
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    c_ast_expr_list_t *cond = c_parse2_exprs(ctx);

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
        err_pos = pos_including(keyw_pos, cond->pos);
        goto err2;
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
        err_pos = pos_including(keyw_pos, cond->pos);
        goto err2;
    }
    pos_t end_pos = peek.pos;
    tkn_delete(tkn_next(ctx->tkn_ctx));

    return c_ast_stmt_create_while(
        c_ast_stmt_while_create(pos_including(keyw_pos, end_pos), c_ast_expr_create_exprs(cond), body, true)
    );

err2:
    c_ast_expr_list_delete(cond);
err1:
    c_ast_stmt_delete(body);
    return c_ast_stmt_create_garbage(c_ast_garbage_create(err_pos));
}

// Parse a while statement.
static c_ast_stmt_t *c_parse2_while(c_parser_t *ctx) {
    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && keyw.subtype == C_KEYW_while);
    pos_t keyw_pos = keyw.pos;
    tkn_delete(keyw);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected (");
        return c_ast_stmt_create_garbage(c_ast_garbage_create(keyw_pos));
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    c_ast_expr_list_t *cond = c_parse2_exprs(ctx);

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
        pos_t pos = pos_including(keyw_pos, cond->pos);
        c_ast_expr_list_delete(cond);
        return c_ast_stmt_create_garbage(c_ast_garbage_create(pos));
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    c_ast_stmt_t *body = c_parse2_stmt(ctx);
    return c_ast_stmt_create_while(
        c_ast_stmt_while_create(pos_including(keyw_pos, body->pos), c_ast_expr_create_exprs(cond), body, false)
    );
}

// Parse a for statement.
static c_ast_stmt_t *c_parse2_for(c_parser_t *ctx) {
    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && keyw.subtype == C_KEYW_for);
    pos_t keyw_pos = keyw.pos;
    tkn_delete(keyw);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected (");
        return c_ast_stmt_create_garbage(c_ast_garbage_create(keyw_pos));
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    peek = tkn_peek(ctx->tkn_ctx);
    c_ast_stmt_t *init;
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_SEMIC) {
        // No initializer.
        tkn_delete(tkn_next(ctx->tkn_ctx));
        init = NULL;
    } else if (is_spec_qual_list_tkn(ctx, peek)) {
        // Declaration as initializer.
        init = c_ast_stmt_create_def(c_parse2_def(ctx, false));
    } else {
        // Expression as initializer.
        init = c_ast_stmt_create_expr(c_parse2_exprs(ctx));

        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
            pos_t pos = pos_including(keyw_pos, init->pos);
            c_ast_stmt_delete(init);
            return c_ast_stmt_create_garbage(c_ast_garbage_create(pos));
        }
        tkn_delete(tkn_next(ctx->tkn_ctx));
    }

    peek = tkn_peek(ctx->tkn_ctx);
    c_ast_expr_list_t *cond;
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_SEMIC) {
        // No condition.
        tkn_delete(tkn_next(ctx->tkn_ctx));
        cond = NULL;
    } else {
        // Expression as condition.
        cond = c_parse2_exprs(ctx);

        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
            pos_t pos = pos_including(keyw_pos, cond->pos);
            if (init) {
                c_ast_stmt_delete(init);
            }
            c_ast_expr_list_delete(cond);
            return c_ast_stmt_create_garbage(c_ast_garbage_create(pos));
        }
        tkn_delete(tkn_next(ctx->tkn_ctx));
    }

    peek = tkn_peek(ctx->tkn_ctx);
    c_ast_expr_list_t *inc;
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_RPAR) {
        // No increment.
        inc = NULL;
    } else {
        // Expression as increment.
        inc  = c_parse2_exprs(ctx);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
        pos_t pos = pos_between(keyw_pos, peek.pos);
        if (init) {
            c_ast_stmt_delete(init);
        }
        if (cond) {
            c_ast_expr_list_delete(cond);
        }
        if (inc) {
            c_ast_expr_list_delete(inc);
        }
        return c_ast_stmt_create_garbage(c_ast_garbage_create(pos));
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    c_ast_stmt_t *body = c_parse2_stmt(ctx);
    return c_ast_stmt_create_for(c_ast_stmt_for_create(pos_including(keyw_pos, body->pos), init, cond, inc, body));
}

// Parse a if statement.
static c_ast_stmt_t *c_parse2_if(c_parser_t *ctx) {
    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && keyw.subtype == C_KEYW_if);
    pos_t keyw_pos = keyw.pos;
    tkn_delete(keyw);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_LPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected (");
        return c_ast_stmt_create_garbage(c_ast_garbage_create(keyw_pos));
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    c_ast_expr_list_t *cond = c_parse2_exprs(ctx);

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RPAR) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected )");
        pos_t pos = pos_including(keyw_pos, cond->pos);
        c_ast_expr_list_delete(cond);
        return c_ast_stmt_create_garbage(c_ast_garbage_create(pos));
    }
    tkn_delete(tkn_next(ctx->tkn_ctx));

    c_ast_stmt_t *body = c_parse2_stmt(ctx);

    c_ast_stmt_t *else_body = NULL;
    pos_t         end_pos   = body->pos;
    peek                    = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_KEYWORD && peek.subtype == C_KEYW_else) {
        // An if...else statement.
        tkn_delete(tkn_next(ctx->tkn_ctx));
        else_body = c_parse2_stmt(ctx);
        end_pos   = else_body->pos;
    }

    return c_ast_stmt_create_if(
        c_ast_stmt_if_create(pos_including(keyw_pos, end_pos), c_ast_expr_create_exprs(cond), body, else_body)
    );
}

// Parse a goto statement.
static c_ast_stmt_t *c_parse2_goto(c_parser_t *ctx) {
    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && keyw.subtype == C_KEYW_goto);
    pos_t keyw_pos = keyw.pos;
    tkn_delete(keyw);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_IDENT) {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected identifier");
        return c_ast_stmt_create_garbage(c_ast_garbage_create(keyw_pos));
    }

    token_t            ident  = tkn_next(ctx->tkn_ctx);
    c_ast_stmt_goto_t *s_goto = c_ast_stmt_goto_create(
        pos_including(keyw_pos, ident.pos),
        c_ast_ident_create(ident.pos, lilycc_strdup(ident.strval))
    );
    tkn_delete(ident);

    peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_SEMIC) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
    } else {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
    }

    return c_ast_stmt_create_goto(s_goto);
}

// Parse a return statement.
static c_ast_stmt_t *c_parse2_return(c_parser_t *ctx) {
    c_ast_expr_list_t *expr = NULL;

    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && keyw.subtype == C_KEYW_return);
    pos_t keyw_pos = keyw.pos;
    pos_t pos      = keyw_pos;
    tkn_delete(keyw);
    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
        expr = c_parse2_exprs(ctx);
        peek = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
            pos = pos_between(pos, peek.pos);
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
            c_eat_delim(ctx->tkn_ctx, false);
        } else {
            pos = pos_including(pos, peek.pos);
            tkn_delete(tkn_next(ctx->tkn_ctx));
        }
    } else {
        tkn_delete(tkn_next(ctx->tkn_ctx));
    }

    return c_ast_stmt_create_return(c_ast_stmt_return_create(pos, expr));
}

// Parse a default-labelled statement.
static c_ast_stmt_t *c_parse2_default(c_parser_t *ctx) {
    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && keyw.subtype == C_KEYW_default);
    pos_t keyw_pos = keyw.pos;
    tkn_delete(keyw);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_COLON) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
    } else {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected :");
    }

    c_ast_stmt_t *inner = c_parse2_stmt(ctx);
    c_ast_stmt_t *res
        = c_ast_stmt_create_case(c_ast_stmt_case_create(pos_including(keyw_pos, inner->pos), NULL, NULL, inner));

    return res;
}

// Parse a labelled statement.
static c_ast_stmt_t *c_parse2_label(c_parser_t *ctx) {
    token_t ident = tkn_next(ctx->tkn_ctx);
    assert(ident.type == TOKENTYPE_IDENT);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_COLON) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
    } else {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected :");
    }

    c_ast_stmt_t *inner = c_parse2_stmt(ctx);
    c_ast_stmt_t *res   = c_ast_stmt_create_label(c_ast_stmt_label_create(
        pos_including(ident.pos, inner->pos),
        c_ast_ident_create(ident.pos, lilycc_strdup(ident.strval)),
        inner
    ));

    tkn_delete(ident);
    return res;
}

// Parse a break/continue statement.
static c_ast_stmt_t *c_parse2_break(c_parser_t *ctx) {
    token_t keyw = tkn_next(ctx->tkn_ctx);
    assert(keyw.type == TOKENTYPE_KEYWORD && (keyw.subtype == C_KEYW_break || keyw.subtype == C_KEYW_continue));
    c_ast_stmt_t *res = c_ast_stmt_create_break(c_ast_stmt_break_create(keyw.pos, keyw.subtype == C_KEYW_continue));
    tkn_delete(keyw);

    token_t peek = tkn_peek(ctx->tkn_ctx);
    if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_SEMIC) {
        tkn_delete(tkn_next(ctx->tkn_ctx));
    } else {
        cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
    }

    return res;
}

// Parse a statment.
c_ast_stmt_t *c_parse2_stmt(c_parser_t *ctx) {
    token_t peek  = tkn_peek(ctx->tkn_ctx);
    token_t peek2 = tkn_peek_n(ctx->tkn_ctx, 1);

    if (peek.type == TOKENTYPE_IDENT && peek2.type == TOKENTYPE_OTHER && peek2.subtype == C_TKN_COLON) {
        return c_parse2_label(ctx);
    } else if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_SEMIC) {
        // No-op statement.
        tkn_delete(tkn_next(ctx->tkn_ctx));
        return c_ast_stmt_create_nop(c_ast_stmt_nop_create(peek.pos));
    } else if (peek.type == TOKENTYPE_OTHER && peek.subtype == C_TKN_LCURL) {
        // Multi-statement parser will always continue until RCRUL token.
        tkn_delete(tkn_next(ctx->tkn_ctx));
        c_ast_stmt_list_t *stmts = c_parse2_stmts(ctx);
        tkn_delete(tkn_next(ctx->tkn_ctx));
        return c_ast_stmt_create_stmts(stmts);
    } else if (is_spec_qual_list_tkn(ctx, peek)) {
        return c_ast_stmt_create_def(c_parse2_def(ctx, false));
    } else if (peek.type == TOKENTYPE_KEYWORD) {
        switch (peek.subtype) {
            case C_KEYW_break:
            case C_KEYW_continue: return c_parse2_break(ctx);
            case C_KEYW_switch: return c_parse2_switch(ctx);
            case C_KEYW_case: return c_parse2_case(ctx);
            case C_KEYW_default: return c_parse2_default(ctx);
            case C_KEYW_do: return c_parse2_do_while(ctx);
            case C_KEYW_while: return c_parse2_while(ctx);
            case C_KEYW_for: return c_parse2_for(ctx);
            case C_KEYW_if: return c_parse2_if(ctx);
            case C_KEYW_goto: return c_parse2_goto(ctx);
            case C_KEYW_return: return c_parse2_return(ctx);
            default: {
                cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected a statement");
                c_eat_delim(ctx->tkn_ctx, false);
                pos_t pos = peek.pos;
                pos.len   = 0;
                return c_ast_stmt_create_garbage(c_ast_garbage_create(pos));
            }
        }
    } else {
        c_ast_expr_list_t *expr = c_parse2_exprs(ctx);
        peek                    = tkn_peek(ctx->tkn_ctx);
        if (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_SEMIC) {
            cctx_diagnostic(ctx->tkn_ctx->cctx, peek.pos, DIAG_ERR, "Expected ;");
            c_eat_delim(ctx->tkn_ctx, false);
        } else {
            tkn_delete(tkn_next(ctx->tkn_ctx));
        }
        return c_ast_stmt_create_expr(expr);
    }
}

// Parse multiple statments.
c_ast_stmt_list_t *c_parse2_stmts(c_parser_t *ctx) {
    vec_c_ast_stmt_t args = {0};

    token_t peek = tkn_peek(ctx->tkn_ctx);
    pos_t   pos  = peek.pos;
    pos.len      = 0;
    while (peek.type != TOKENTYPE_EOF && (peek.type != TOKENTYPE_OTHER || peek.subtype != C_TKN_RCURL)) {
        vec_push(&args, c_parse2_stmt(ctx));
        pos  = pos_including(pos, args.arr[args.len - 1]->pos);
        peek = tkn_peek(ctx->tkn_ctx);
    }

    return c_ast_stmt_list_create(pos, args);
}



// Eat tokens up to an including the next delimiter,
// or stop before next curly bracket.
static void c_eat_delim(tokenizer_t *tkn_ctx, bool include_comma) {
    token_t peek    = tkn_peek(tkn_ctx);
    bool    tooketh = false;
    while (1) {
        if (peek.type == TOKENTYPE_EOF) {
            return;
        } else if (peek.type == TOKENTYPE_OTHER && (peek.subtype == C_TKN_RCURL || peek.subtype == C_TKN_LCURL)) {
            if (!tooketh) {
                tkn_delete(tkn_next(tkn_ctx));
            }
            return;
        } else if (
            peek.type == TOKENTYPE_OTHER
            && (peek.subtype == C_TKN_SEMIC || (include_comma && peek.subtype == C_TKN_COMMA))
        ) {
            tkn_delete(tkn_next(tkn_ctx));
            return;
        } else {
            tkn_delete(tkn_next(tkn_ctx));
            peek = tkn_peek(tkn_ctx);
        }
        tooketh = true;
    }
}
