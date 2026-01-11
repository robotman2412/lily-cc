
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir_parser.h"

#include "arrays.h"
#include "compiler.h"
#include "ir_tokenizer.h"
#include "ir_types.h"
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

    bool          has_type = false;
    token_t const type     = tkn_peek(from);
    if (type.type == TOKENTYPE_KEYWORD && type.subtype >= IR_KEYW_s8 && type.subtype <= IR_KEYW_f64) {
        tkn_next(from);
        has_type = true;
    }

    token_t       res;
    token_t const base = tkn_next(from);
    if (base.type == TOKENTYPE_ICONST) {
        res = ast_from_va(IR_AST_MEMOPERAND, 1, base);
    } else if (base.type == TOKENTYPE_IDENT && (base.subtype == IR_IDENT_LOCAL || base.subtype == IR_IDENT_GLOBAL)) {
        token_t const peek = tkn_peek(from);
        if (peek.type == TOKENTYPE_OTHER && peek.subtype == IR_TKN_RPAR) {
            res = ast_from_va(IR_AST_MEMOPERAND, 1, base);
        } else if (peek.type == TOKENTYPE_OTHER && (peek.subtype == IR_TKN_ADD || peek.subtype == IR_TKN_SUB)) {
            tkn_next(from);
            token_t off = tkn_next(from);
            if (off.type != TOKENTYPE_ICONST) {
                cctx_diagnostic(from->cctx, off.pos, DIAG_ERR, "Expected number");
                res = ast_from_va(IR_AST_GARBAGE, 3, off, peek, base);
            } else {
                res = ast_from_va(IR_AST_MEMOPERAND, 3, off, peek, base);
            }
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

    if (has_type) {
        array_len_insert_strong(&res.params, sizeof(token_t), &res.params_len, &type, 0);
    }

    return tkn_with_pos(res, pos_including(lpar.pos, rpar.pos));
}

// Parse an instruction.
token_t ir_parse_insn(tokenizer_t *from) {
    pos_t pos0, pos1;
    bool  garbage = false;

    token_t peek = tkn_peek(from);
    pos0         = peek.pos;
    token_t returns;
    if (peek.type != TOKENTYPE_KEYWORD) {
        returns        = ir_parse_ret_list(from);
        token_t assign = tkn_next(from);
        if (assign.type != TOKENTYPE_OTHER || assign.subtype != IR_TKN_ASSIGN) {
            cctx_diagnostic(from->cctx, assign.pos, DIAG_ERR, "Expected =");
            garbage = true;
        }
        if (returns.subtype == IR_AST_GARBAGE) {
            garbage = true;
        }
    } else {
        returns = ast_from_va(IR_AST_LIST, 0);
    }

    token_t insn = tkn_next(from);
    if (insn.type == TOKENTYPE_IDENT && insn.subtype == IR_IDENT_BARE) {
        cctx_diagnostic(from->cctx, insn.pos, DIAG_ERR, "Unrecognised instruction");
        garbage = true;
    } else if (insn.type != TOKENTYPE_KEYWORD) {
        cctx_diagnostic(from->cctx, insn.pos, DIAG_ERR, "Expected instruction mnemonic");
        garbage = true;
    } else {
        switch (insn.subtype) {
            default:
                garbage = true;
                cctx_diagnostic(from->cctx, insn.pos, DIAG_ERR, "Unrecognised instruction");
                break;
            case IR_KEYW_comb:
#define IR_OP2_DEF(x) case IR_KEYW_##x:
#include "defs/ir_op2.inc"
#define IR_OP1_DEF(x) case IR_KEYW_##x:
#include "defs/ir_op1.inc"
            case IR_KEYW_jump:
            case IR_KEYW_branch:
            case IR_KEYW_load:
            case IR_KEYW_store:
            case IR_KEYW_lea:
            case IR_KEYW_call:
            case IR_KEYW_return:
            case IR_KEYW_memcpy:
            case IR_KEYW_memset: break; // Valid instruction mnemonic.
        }
    }
    pos1 = insn.pos;

    peek = tkn_peek(from);
    token_t params;
    if (peek.type != TOKENTYPE_EOL && peek.type != TOKENTYPE_EOF) {
        if (insn.subtype == IR_KEYW_comb) {
            params = ir_parse_bind_list(from);
        } else {
            params = ir_parse_operand_list(from);
        }
        if (params.subtype == IR_AST_GARBAGE) {
            garbage = true;
        }
        pos1 = params.pos;
    } else {
        params = ast_from_va(IR_AST_LIST, 0);
    }

    return tkn_with_pos(
        ast_from_va(garbage ? IR_AST_GARBAGE : IR_AST_INSN, 3, returns, insn, params),
        pos_including(pos0, pos1)
    );
}

// Parse a variable definition.
token_t ir_parse_var(tokenizer_t *from) {
    token_t const keyw = tkn_next(from);
    if (keyw.type != TOKENTYPE_KEYWORD || keyw.subtype != IR_KEYW_var) {
        cctx_diagnostic(from->cctx, keyw.pos, DIAG_ERR, "Expected `var`");
        return ast_from_va(IR_AST_GARBAGE, 1, keyw);
    }
    tkn_delete(keyw);
    token_t ident = tkn_next(from);
    if (ident.type != TOKENTYPE_IDENT || ident.subtype != IR_IDENT_LOCAL) {
        cctx_diagnostic(from->cctx, ident.pos, DIAG_ERR, "Expected %%identifier");
        return ast_from_va(IR_AST_GARBAGE, 1, ident);
    }
    token_t type = tkn_next(from);
    if (type.type != TOKENTYPE_KEYWORD || type.subtype < IR_KEYW_s8 || type.subtype > IR_KEYW_f64) {
        cctx_diagnostic(from->cctx, type.pos, DIAG_ERR, "Expected type");
        return ast_from_va(IR_AST_GARBAGE, 2, ident, type);
    }
    return ast_from_va(IR_AST_VAR, 2, ident, type);
}

// Parse a code block definition.
token_t ir_parse_code(tokenizer_t *from) {
    token_t const keyw = tkn_next(from);
    if (keyw.type != TOKENTYPE_KEYWORD || keyw.subtype != IR_KEYW_code) {
        cctx_diagnostic(from->cctx, keyw.pos, DIAG_ERR, "Expected `code`");
        return ast_from_va(IR_AST_GARBAGE, 1, keyw);
    }
    tkn_delete(keyw);
    token_t ident = tkn_next(from);
    if (ident.type != TOKENTYPE_IDENT || ident.subtype != IR_IDENT_LOCAL) {
        cctx_diagnostic(from->cctx, ident.pos, DIAG_ERR, "Expected %%identifier");
        return ast_from_va(IR_AST_GARBAGE, 1, ident);
    }
    return ast_from_va(IR_AST_CODE, 1, ident);
}

// Parse an argument definition.
token_t ir_parse_arg(tokenizer_t *from) {
    token_t keyw = tkn_next(from);
    if (keyw.type != TOKENTYPE_KEYWORD || keyw.subtype != IR_KEYW_arg) {
        cctx_diagnostic(from->cctx, keyw.pos, DIAG_ERR, "Expected `arg`");
        return ast_from_va(IR_AST_GARBAGE, 1, keyw);
    }
    tkn_delete(keyw);
    token_t ident_struct = tkn_next(from);
    if (ident_struct.type == TOKENTYPE_KEYWORD && ident_struct.subtype == IR_KEYW_struct) {
        token_t ident = tkn_next(from);
        if (ident.type != TOKENTYPE_IDENT || ident.subtype != IR_IDENT_LOCAL) {
            cctx_diagnostic(from->cctx, keyw.pos, DIAG_ERR, "Expected %%identifier");
            return ast_from_va(IR_AST_GARBAGE, 2, ident_struct, ident);
        }
        return ast_from_va(IR_AST_STRUCTARG, 1, ident);

    } else {
        if (ident_struct.type != TOKENTYPE_IDENT || ident_struct.subtype != IR_IDENT_LOCAL) {
            cctx_diagnostic(from->cctx, ident_struct.pos, DIAG_ERR, "Expected `struct` or integer constant");
            return ast_from_va(IR_AST_GARBAGE, 1, ident_struct);
        }
        return ast_from_va(IR_AST_ARG, 1, ident_struct);
    }
}

// Parse an entrypoint definition.
token_t ir_parse_entry(tokenizer_t *from) {
    token_t const keyw = tkn_next(from);
    if (keyw.type != TOKENTYPE_KEYWORD || keyw.subtype != IR_KEYW_entry) {
        cctx_diagnostic(from->cctx, keyw.pos, DIAG_ERR, "Expected `entry`");
        return ast_from_va(IR_AST_GARBAGE, 1, keyw);
    }
    tkn_delete(keyw);
    token_t ident = tkn_next(from);
    if (ident.type != TOKENTYPE_IDENT || ident.subtype != IR_IDENT_LOCAL) {
        cctx_diagnostic(from->cctx, ident.pos, DIAG_ERR, "Expected %%identifier");
        return ast_from_va(IR_AST_GARBAGE, 1, ident);
    }
    return ast_from_va(IR_AST_ENTRY, 1, ident);
}

// Parse a stack frame definition.
token_t ir_parse_frame(tokenizer_t *from) {
    token_t const keyw = tkn_next(from);
    if (keyw.type != TOKENTYPE_KEYWORD || keyw.subtype != IR_KEYW_var) {
        cctx_diagnostic(from->cctx, keyw.pos, DIAG_ERR, "Expected `frame`");
        return ast_from_va(IR_AST_GARBAGE, 1, keyw);
    }
    tkn_delete(keyw);
    token_t ident = tkn_next(from);
    if (ident.type != TOKENTYPE_IDENT || ident.subtype != IR_IDENT_LOCAL) {
        cctx_diagnostic(from->cctx, ident.pos, DIAG_ERR, "Expected %%identifier");
        return ast_from_va(IR_AST_GARBAGE, 1, ident);
    }
    token_t size = tkn_next(from);
    if (ident.type != TOKENTYPE_ICONST) {
        cctx_diagnostic(from->cctx, ident.pos, DIAG_ERR, "Expected number");
        return ast_from_va(IR_AST_GARBAGE, 2, ident, size);
    }
    token_t align = tkn_next(from);
    if (ident.type != TOKENTYPE_ICONST) {
        cctx_diagnostic(from->cctx, ident.pos, DIAG_ERR, "Expected number");
        return ast_from_va(IR_AST_GARBAGE, 3, ident, size, align);
    }
    return ast_from_va(IR_AST_FRAME, 3, ident, size, align);
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

// Whether a token denotes the end of a list.
static bool ir_is_list_end_token(token_t tkn) {
    switch (tkn.type) {
        case TOKENTYPE_EOL:
        case TOKENTYPE_EOF: return true;
        case TOKENTYPE_OTHER: return tkn.subtype == IR_TKN_ASSIGN;
        default: return false;
    }
}

// Helper for parsing comma-separated lists.
static token_t ir_parse_list(tokenizer_t *from, token_t (*item_parser)(tokenizer_t *)) {
    size_t   arr_len = 1;
    size_t   arr_cap = 2;
    token_t *arr     = strong_calloc(arr_cap, sizeof(token_t));
    arr[0]           = item_parser(from);
    bool garbage     = arr[0].type == TOKENTYPE_AST && arr[0].subtype == IR_AST_GARBAGE;

    while (true) {
        if (arr[arr_len - 1].type == TOKENTYPE_AST && arr[arr_len - 1].subtype == TOKENTYPE_GARBAGE) {
            ir_until_list_next(from);
            garbage = true;
        }
        token_t peek = tkn_peek(from);
        if (ir_is_list_end_token(peek)) {
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

    } else if (peek.type == TOKENTYPE_KEYWORD && peek.subtype == IR_KEYW_struct) {
        tkn_delete(tkn_next(from));
        token_t tmp = tkn_next(from);
        if (tmp.type != TOKENTYPE_IDENT || tmp.subtype != IR_IDENT_LOCAL) {
            cctx_diagnostic(from->cctx, tmp.pos, DIAG_ERR, "Expected a local identifier");
            return ast_from_va(IR_AST_GARBAGE, 1, tmp);
        }
        return ast_from_va(IR_AST_STRUCTOPERAND, 1, tmp);

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
    ir_skip_eol(from);
    token_t peek = tkn_peek(from);
    token_t res;
    if (peek.type == TOKENTYPE_KEYWORD) {
        switch (peek.subtype) {
            case IR_KEYW_var: res = ir_parse_var(from); break;
            case IR_KEYW_code: res = ir_parse_code(from); break;
            case IR_KEYW_arg: res = ir_parse_arg(from); break;
            case IR_KEYW_entry: res = ir_parse_entry(from); break;
            case IR_KEYW_frame: res = ir_parse_frame(from); break;
            default: res = ir_parse_insn(from); break;
        }
    } else {
        res = ir_parse_insn(from);
    }
    ir_expect_eol(from);
    return res;
}

// Parse an IR function.
token_t ir_parse_func(tokenizer_t *from) {
    ir_skip_eol(from);

    token_t keyw = tkn_next(from);
    if (keyw.type != TOKENTYPE_KEYWORD || (keyw.subtype != IR_KEYW_ssa_function && keyw.subtype != IR_KEYW_function)) {
        cctx_diagnostic(from->cctx, keyw.pos, DIAG_ERR, "Expected `ssa_function` or `function`");
        return ast_from_va(IR_AST_GARBAGE, 1, keyw);
    }

    token_t ident = tkn_next(from);
    if (ident.type != TOKENTYPE_IDENT || ident.subtype != IR_IDENT_GLOBAL) {
        cctx_diagnostic(from->cctx, ident.pos, DIAG_ERR, "Expected global identifier");
        return ast_from_va(IR_AST_GARBAGE, 1, ident);
    }

    size_t   arr_len = 0;
    size_t   arr_cap = 16;
    token_t *arr     = strong_calloc(arr_cap, sizeof(token_t));
    bool     garbage = false;

    while (1) {
        token_t peek = tkn_peek(from);
        if (peek.type == TOKENTYPE_EOF) {
            break;
        }
        if (peek.type == TOKENTYPE_KEYWORD
            && (peek.subtype == IR_KEYW_ssa_function || peek.subtype == IR_KEYW_function)) {
            break;
        }
        token_t stmt = ir_parse_stmt(from);
        if (stmt.subtype == IR_AST_GARBAGE) {
            garbage = true;
        }
        array_lencap_insert_strong(&arr, sizeof(token_t), &arr_len, &arr_cap, &stmt, arr_len);
        ir_skip_eol(from);
    }

    return ast_from_va(garbage ? IR_AST_GARBAGE : IR_AST_FUNCTION, 3, keyw, ident, ast_from(IR_AST_LIST, arr_len, arr));
}
