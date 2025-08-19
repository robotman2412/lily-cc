
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir_parser.h"

#include "arrays.h"
#include "compiler.h"
#include "ir_tokenizer.h"
#include "strong_malloc.h"
#include "tokenizer.h"

#include <stdlib.h>



// Helper function that skips past any EOL tokens.
static void ir_skip_eol(tokenizer_t *from) {
    while (tkn_peek(from).type == TOKENTYPE_EOL) {
        tkn_delete(tkn_next(from));
    }
}

// Helper function that eats tokens until EOL or EOF.
static void ir_eat_eol(tokenizer_t *from) {
    while (1) {
        token_t tkn = tkn_next(from);
        tkn_delete(tkn);
        if (tkn.type == TOKENTYPE_EOF || tkn.type == TOKENTYPE_EOL) {
            return;
        }
    }
}

// Helper function that eats tokens until EOL, EOF or RPAR.
static void ir_eat_rpar(tokenizer_t *from) {
    while (1) {
        token_t tkn = tkn_next(from);
        tkn_delete(tkn);
        if (tkn.type == TOKENTYPE_EOF || tkn.type == TOKENTYPE_EOL
            || (tkn.type == TOKENTYPE_OTHER && tkn.subtype == IR_TKN_RPAR)) {
            return;
        }
    }
}

// Helper function that eats tokens until EOL, EOF, COMMA or ASSIGN (but do not eat that token).
static void ir_until_list_next(tokenizer_t *from) {
    while (1) {
        token_t tkn = tkn_peek(from);
        if (tkn.type == TOKENTYPE_EOF || tkn.type == TOKENTYPE_EOL
            || (tkn.type == TOKENTYPE_OTHER && (tkn.subtype == IR_TKN_COMMA || tkn.subtype == IR_TKN_ASSIGN))) {
            return;
        }
        tkn_next(from);
        tkn_delete(tkn);
    }
}

// Helper function that expects RPAR.
static bool ir_expect_rpar(tokenizer_t *from) {
    bool    ok  = true;
    token_t tkn = tkn_next(from);
    if (tkn.type != TOKENTYPE_OTHER || tkn.subtype != IR_TKN_RPAR) {
        cctx_diagnostic(from->cctx, tkn.pos, DIAG_ERR, "Expected )");
        ir_eat_rpar(from);
        ok = false;
    }
    tkn_delete(tkn);
    return ok;
}

// Helper function that expects EOL or EOF.
static bool ir_expect_eol(tokenizer_t *from) {
    bool    ok  = true;
    token_t tkn = tkn_next(from);
    if (tkn.type != TOKENTYPE_EOF && tkn.type != TOKENTYPE_EOL) {
        cctx_diagnostic(from->cctx, tkn.pos, DIAG_ERR, "Expected end-of-line");
        ir_eat_eol(from);
        ok = false;
    }
    tkn_delete(tkn);
    return ok;
}

// Helper function that expects COMMA.
static bool ir_expect_comma(tokenizer_t *from) {
    bool    ok  = true;
    token_t tkn = tkn_next(from);
    if (tkn.type != TOKENTYPE_OTHER || tkn.subtype != IR_TKN_COMMA) {
        cctx_diagnostic(from->cctx, tkn.pos, DIAG_ERR, "Expected )");
        ir_eat_rpar(from);
        ok = false;
    }
    tkn_delete(tkn);
    return ok;
}



// Parse a memory operand.
token_t ir_parse_memoperand(tokenizer_t *from) {
    token_t const lpar = tkn_next(from);
    if (lpar.type != TOKENTYPE_OTHER || lpar.subtype != IR_TKN_LPAR) {
        cctx_diagnostic(from->cctx, lpar.pos, DIAG_ERR, "Expected (");
        return ast_from_va(IR_AST_GARBAGE, 1, lpar);
    }

    token_t       res;
    token_t const base = tkn_next(from);
    if (base.type == TOKENTYPE_ICONST) {
        res = ast_from_va(IR_AST_MEMOPERAND, 1, base);
    } else if (base.type == TOKENTYPE_IDENT && (base.subtype == IR_IDENT_LOCAL || base.subtype == IR_IDENT_GLOBAL)) {
        token_t const peek = tkn_peek(from);
        if (peek.type == TOKENTYPE_OTHER && peek.subtype == IR_TKN_RPAR) {
            res = base;
        } else if (peek.type == TOKENTYPE_OTHER && (peek.subtype == IR_TKN_ADD || peek.subtype == IR_TKN_SUB)) {
            abort(); // TODO.
        } else {
            res = ast_from_va(IR_AST_GARBAGE, 1, base);
        }
    } else {
        cctx_diagnostic(from->cctx, base.pos, DIAG_ERR, "Expected %%ident, <ident> or number");
        res = ast_from_va(IR_AST_GARBAGE, 1, base);
    }

    token_t rpar = tkn_next(from);
    if (rpar.type != TOKENTYPE_OTHER || rpar.subtype != IR_TKN_RPAR) {
        cctx_diagnostic(from->cctx, rpar.pos, DIAG_ERR, "Expected )");
    }

    return tkn_with_pos(res, pos_including(lpar.pos, rpar.pos));
}

// Parse an instruction.
token_t ir_parse_insn(tokenizer_t *from) {
    (void)from;
    abort();
}

// Parse a variable definition.
token_t ir_parse_var(tokenizer_t *from) {
    (void)from;
    abort();
}

// Parse a code block definition.
token_t ir_parse_code(tokenizer_t *from) {
    (void)from;
    abort();
}

// Parse an argument definition.
token_t ir_parse_arg(tokenizer_t *from) {
    (void)from;
    abort();
}

// Parse an entrypoint definition.
token_t ir_parse_entry(tokenizer_t *from) {
    (void)from;
    abort();
}

// Parse a stack frame definition.
token_t ir_parse_frame(tokenizer_t *from) {
    (void)from;
    abort();
}

// Parse a combinator instruction binding.
token_t ir_parse_bind(tokenizer_t *from) {
    token_t peek = tkn_peek(from);
    if (peek.type != TOKENTYPE_IDENT || peek.subtype != IR_IDENT_LOCAL) {
        cctx_diagnostic(from->cctx, peek.pos, DIAG_ERR, "Expected a local identifier");
        return ast_from_va(IR_AST_GARBAGE, 1, tkn_next(from));
    }
    token_t peek2 = tkn_peek_n(from, 1);
    if (peek2.type != TOKENTYPE_IDENT || peek2.subtype != IR_IDENT_LOCAL) {
        cctx_diagnostic(from->cctx, peek2.pos, DIAG_ERR, "Expected a local identifier");
        return ast_from_va(IR_AST_GARBAGE, 2, tkn_next(from), tkn_next(from));
    }
    return ast_from_va(IR_AST_BIND, 2, tkn_next(from), tkn_next(from));
}

// Helper for parsing comma-separated lists.
static token_t ir_parse_list(tokenizer_t *from, token_t (*item_parser)(tokenizer_t *)) {
    size_t   arr_len = 1;
    size_t   arr_cap = 2;
    token_t *arr     = strong_calloc(arr_cap, sizeof(token_t));
    bool     garbage = false;
    arr[0]           = item_parser(from);

    while (true) {
        if (arr[arr_len - 1].type == TOKENTYPE_AST && arr[arr_len - 1].subtype == TOKENTYPE_GARBAGE) {
            ir_until_list_next(from);
            garbage = true;
        }
        token_t peek = tkn_peek(from);
        if (peek.type == TOKENTYPE_EOL || peek.type == TOKENTYPE_EOF
            || (peek.type == TOKENTYPE_OTHER && peek.subtype == IR_TKN_ASSIGN)) {
            return ast_from(garbage ? IR_AST_GARBAGE : IR_AST_LIST, arr_len, arr);
        }
        ir_expect_comma(from);
        token_t next = item_parser(from);
        array_lencap_insert_strong(&arr, sizeof(token_t), &arr_len, &arr_cap, &next, arr_len);
    }
}

// Parse a returns list.
token_t ir_parse_ret_list(tokenizer_t *from) {
    return ir_parse_list(from, tkn_next);
}

// Parse an operands list.
token_t ir_parse_operand_list(tokenizer_t *from) {
    return ir_parse_list(from, ir_parse_operand);
}

// Parse a combinator bindings list.
token_t ir_parse_bind_list(tokenizer_t *from) {
    return ir_parse_list(from, ir_parse_bind);
}

// Parse an IR operand.
token_t ir_parse_operand(tokenizer_t *from) {
    token_t const peek = tkn_peek(from);
    if (
        (peek.type == TOKENTYPE_IDENT && peek.subtype == IR_IDENT_LOCAL)     // Variable.
        || peek.type == TOKENTYPE_ICONST                                     // Numeric constant.
        || (peek.type == TOKENTYPE_KEYWORD && peek.subtype == IR_KEYW_true)  // True constant.
        || (peek.type == TOKENTYPE_KEYWORD && peek.subtype == IR_KEYW_false) // False constant.
        || (peek.type == TOKENTYPE_OTHER && peek.subtype == IR_TKN_UNDEF)    // Undefined operand.
    ) {
        return tkn_next(from);

    } else if (peek.type == TOKENTYPE_OTHER && peek.subtype == IR_TKN_LPAR) {
        // Memory operand.
        return ir_parse_memoperand(from);

    } else {
        cctx_diagnostic(from->cctx, peek.pos, DIAG_ERR, "Expected an operand");
        return ast_from_va(IR_AST_GARBAGE, 1, tkn_next(from));
    }
}

// Parse an IR statement.
token_t ir_parse_stmt(tokenizer_t *from) {
    (void)from;
    abort();
}

// Parse an IR function.
token_t ir_parse_func(tokenizer_t *from) {
    (void)from;
    abort();
}
