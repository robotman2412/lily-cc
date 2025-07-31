
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

// List of keywords.
// Must be before <stdbool.h> because it contains the identifier `bool`.
#include "arith128.h"
#include "compiler.h"
char const *const ir_keywords[] = {
#define IR_KEYW_DEF(keyw) #keyw,
#include "defs/ir_keywords.inc"
};

#include "ir_tokenizer.h"
#include "ir_types.h"
#include "strong_malloc.h"

#include <arrays.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>



// Create an IR text tokenizer.
tokenizer_t *ir_tkn_create(srcfile_t *srcfile) {
    tokenizer_t *tkn_ctx = strong_calloc(sizeof(tokenizer_t), 1);
    tkn_ctx->cctx        = srcfile->ctx;
    tkn_ctx->pos.srcfile = srcfile;
    tkn_ctx->file        = srcfile;
    tkn_ctx->next        = ir_tkn_next;
    return tkn_ctx;
}


// Helper function to create tokens for better readability.
static token_t other_tkn(ir_tokentype_t type, pos_t from, pos_t to) {
    return ((token_t){
        .pos        = pos_between(from, to),
        .type       = TOKENTYPE_OTHER,
        .subtype    = type,
        .strval     = NULL,
        .strval_len = 0,
        .params_len = 0,
        .params     = NULL,
    });
}

// Test whether a character is legal in an IR identifier.
static inline bool ir_is_sym_char(int c) {
    if (c == '_') {
        return true;
    } else if (c >= '0' && c <= '9') {
        return true;
    }
    c |= 0x20;
    return c >= 'a' && c <= 'z';
}


// Tokenize numeric constant.
static token_t ir_tkn_numeric(tokenizer_t *ctx, ir_prim_t prim, pos_t start_pos, unsigned int base, bool is_negative) {
    i128_t val      = int128(0, 0);
    bool   hasdat   = false;
    bool   toolarge = false;
    bool   invalid  = false;

    if (prim == IR_PRIM_f32 || prim == IR_PRIM_f64) {
        cctx_diagnostic(ctx->cctx, start_pos, DIAG_ERR, "[TODO] Floating-point constants not supported");
        return (token_t){
            .type       = TOKENTYPE_GARBAGE,
            .pos        = start_pos,
            .ival       = 0,
            .subtype    = 0,
            .strval     = NULL,
            .strval_len = 0,
            .params_len = 0,
            .params     = NULL,
        };
    }

    pos_t pos0 = ctx->pos;
    pos_t pos1;
    while (1) {
        unsigned int digit;
        pos1  = pos0;
        int c = srcfile_getc(ctx->file, &pos1);
        if (c >= '0' && c <= '9') {
            // Valid digit 0-9.
            digit = c - '0';
        } else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'z') {
            // Valid digit a-z / A-Z.
            digit = (c | 0x20) - 'a' + 10;
        } else if (c == '_') {
            // Spacer.
            continue;
        } else {
            // End of constant.
            break;
        }
        if (digit >= base) {
            invalid = true;
        }
        i128_t new_val = add128(mul128(val, int128(0, base)), int128(0, digit));
        if (cmp128u(new_val, val) < 0) {
            toolarge = true;
        }
        val    = new_val;
        hasdat = true;
        pos0   = pos1;
    }
    ctx->pos = pos0;

    pos_t pos = pos_between(start_pos, pos0);
    if (invalid || !hasdat) {
        // Report error (invalid constant).
        char const *ctype;
        switch (base) {
            case 2: ctype = "binary"; break;
            case 8: ctype = "octal"; break;
            case 10: ctype = "decimal"; break;
            case 16: ctype = "hexadecimal"; break;
            default: __builtin_unreachable();
        }
        cctx_diagnostic(ctx->cctx, pos, DIAG_ERR, "Invalid %s constant", ctype);
        return (token_t){
            .type       = TOKENTYPE_GARBAGE,
            .pos        = pos,
            .ival       = 0,
            .subtype    = 0,
            .strval     = NULL,
            .strval_len = 0,
            .params_len = 0,
            .params     = NULL,
        };
    }

    bool const is_signed = prim <= IR_PRIM_s128 && !(prim & 1);
    int const  bitcount  = prim == IR_PRIM_bool ? 1 : ir_prim_sizes[prim] * 8;

    // Negate and check for overflow.
    if (is_signed) {
        // Check for overflow (signed edition).
        i128_t const sign_mask = shl128(int128(0, 1), bitcount - 1);
        i128_t const max_val   = add128(sign_mask, neg128(int128(0, 1)));
        if (is_negative) {
            toolarge |= cmp128u(val, sign_mask) > 0;
        } else {
            toolarge |= cmp128u(val, max_val) > 0;
        }

        if (is_negative) {
            val = neg128(val);
        }

        // Truncate signed number.
        if (cmp128u(and128(val, sign_mask), int128(0, 0))) {
            // Sign-extend.
            val = or128(val, neg128(sign_mask));
        } else {
            // Truncate bits after sign.
            val = and128(val, max_val);
        }
    } else {
        // Check for overflow (unsigned edition).
        i128_t const max_val = add128(shl128(int128(0, 1), bitcount), neg128(int128(0, 1)));
        if (is_negative) {
            toolarge = true;
        } else if (cmp128u(val, max_val)) {
            toolarge = true;
        }

        if (is_negative) {
            val = neg128(val);
        }

        // Truncate unsigned number.
        val = and128(val, max_val);
    }

    // If the value is out of range, add a warning showing the actual value used.
    if (toolarge) {
        bool   val_u_negative = false;
        i128_t val_u;
        if (is_signed && hi64(val) >> 63) {
            val_u = neg128(val);
        } else {
            val_u = val;
        }
        char as_dec_str[40];
        itoa128(val_u, 0, as_dec_str);
        cctx_diagnostic(
            ctx->cctx,
            pos,
            DIAG_WARN,
            "Constant is out of range and was truncated to %s%s (0x%.0" PRIx64 "%0*" PRIx64 ")",
            val_u_negative ? "-" : "",
            as_dec_str,
            hi64(val),
            hi64(val) != 0 ? 16 : 1,
            lo64(val)
        );
    }
    if (prim == IR_PRIM_bool) {
        cctx_diagnostic(ctx->cctx, pos, DIAG_WARN, "Consider using `%s` instead", lo64(val) ? "true" : "false");
    }

    return (token_t){
        .type       = TOKENTYPE_ICONST,
        .pos        = pos,
        .ival       = lo64(val),
        .ivalh      = hi64(val),
        .subtype    = prim,
        .strval     = NULL,
        .strval_len = 0,
        .params_len = 0,
        .params     = NULL,
    };
}

// Tokenize identifier.
static token_t ir_tkn_ident(tokenizer_t *ctx, pos_t start_pos, char first, bool as_keyw) {
    size_t cap = 32;
    size_t len = 1;
    char  *ptr = strong_malloc(cap);
    ptr[0]     = first;

    pos_t pos0 = ctx->pos;
    pos_t pos1;
    while (1) {
        pos1  = pos0;
        int c = srcfile_getc(ctx->file, &pos1);
        if (!ir_is_sym_char(c)) {
            // End of identifier.
            break;
        }
        if (len == cap - 1) {
            // Even longer name, allocate more memory.
            cap *= 2;
            ptr  = strong_realloc(ptr, cap);
        }
        ptr[len++] = c;
        pos0       = pos1;
    }
    ptr[len] = 0;
    ctx->pos = pos0;

    // Test for keywords.
    if (as_keyw) {
        ir_keyw_t keyw = ir_keyw_get(ptr);
        if (keyw < IR_N_KEYWS) {
            free(ptr);
            // Return keyword token with main spelling.
            return (token_t){
                .pos        = pos_between(start_pos, pos0),
                .type       = TOKENTYPE_KEYWORD,
                .subtype    = keyw,
                .strval     = NULL,
                .strval_len = 0,
                .params_len = 0,
                .params     = NULL,
            };
        }
    }

    return (token_t){
        .pos        = pos_between(start_pos, pos0),
        .type       = TOKENTYPE_IDENT,
        .strval     = ptr,
        .strval_len = len,
        .subtype    = IR_IDENT_BARE,
        .params_len = 0,
        .params     = NULL,
    };
}

// Hex parsing helper for strings.
static int ir_str_hex(tokenizer_t *ctx, pos_t start_pos, int min_w, int max_w) {
    int   value = 0;
    pos_t pos0  = ctx->pos;
    pos_t pos1;
    for (int i = 0; i < max_w; i++) {
        pos1  = pos0;
        int c = srcfile_getc(ctx->file, &pos1);
        if (c >= '0' && c <= '9') {
            value <<= 4;
            value  |= c - '0';
        } else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f') {
            value <<= 4;
            value  |= (c | 0x20) - 'a' + 0xa;
        } else {
            if (i < min_w) {
                cctx_diagnostic(
                    ctx->cctx,
                    pos_between(start_pos, pos0),
                    DIAG_ERR,
                    "Invalid hexadecimal escape sequence"
                );
            }
            break;
        }
        pos0 = pos1;
    }
    ctx->pos = pos0;
    return value;
}

// Octal parsing helper for strings.
static int ir_str_octal(tokenizer_t *ctx, pos_t start_pos, int first, int max_w) {
    (void)start_pos;
    int   value = first - '0';
    pos_t pos0  = ctx->pos;
    pos_t pos1;
    for (int i = 1; i < max_w; i++) {
        pos1  = pos0;
        int c = srcfile_getc(ctx->file, &pos1);
        if (c < '0' || c > '7') {
            break;
        } else {
            value <<= 3;
            value  |= c - '0';
        }
        pos0 = pos1;
    }
    ctx->pos = pos0;
    return value;
}

// Tokenize string or character constant.
static token_t ir_tkn_str(tokenizer_t *ctx, pos_t start_pos, bool is_char) {
    size_t cap = 32;
    size_t len = 0;
    char  *ptr = strong_malloc(cap);

    while (1) {
        pos_t pos0 = ctx->pos;
        int   c    = srcfile_getc(ctx->file, &ctx->pos);
        if (c == -1 || c == '\n') {
            cctx_diagnostic(
                ctx->cctx,
                pos_between(start_pos, pos0),
                DIAG_ERR,
                "%s constant spans end of %s",
                is_char ? "Character" : "String",
                c == '\n' ? "line" : "file"
            );
            break;
        } else if (c == (is_char ? '\'' : '\"')) {
            // End of string.
            break;
        } else if (c == '\\') {
            // Escape sequence.
            c = srcfile_getc(ctx->file, &ctx->pos);

            if (c == 'U') {
                // 8-hexit unicode point.
                c = ir_str_hex(ctx, start_pos, 8, 8);
            } else if (c == 'u') {
                // 4-hexit unicode point.
                c = ir_str_hex(ctx, start_pos, 4, 4);
            } else if (c == 'x') {
                // Hexadecimal (of any length (because of course that's logical (it isn't))).
                c = ir_str_hex(ctx, start_pos, 1, 32767);
            } else if (c >= '0' && c <= '3') {
                // 1- to 3-digit octal.
                c = ir_str_octal(ctx, start_pos, c, 3);
            } else if (c >= '4' && c <= '7') {
                // 1- or 2-digit octal.
                c = ir_str_octal(ctx, start_pos, c, 2);
            } else {
                // Single-character escape sequences.
                switch (c) {
                    case '?': c = '?'; break;
                    case '\\': c = '\\'; break;
                    case '\'': c = '\''; break;
                    case '\"': c = '\"'; break;
                    case 'a': c = '\a'; break;
                    case 'b': c = '\b'; break;
                    case 'f': c = '\f'; break;
                    case 'n': c = '\n'; break;
                    case 'r': c = '\r'; break;
                    case 't': c = '\t'; break;
                    case 'v': c = '\v'; break;
                    default:
                        cctx_diagnostic(ctx->cctx, pos_between(pos0, ctx->pos), DIAG_ERR, "Invalid escape sequence");
                        break;
                }
            }
        }
        array_lencap_insert_strong(&ptr, 1, &len, &cap, &c, len);
    }

    if (is_char) {
        uint64_t val = 0;
        for (size_t i = 0; i < len; i++) {
            val <<= 8;
            val  |= ptr[i];
        }
        free(ptr);
        return (token_t){
            .pos        = pos_between(start_pos, ctx->pos),
            .type       = TOKENTYPE_CCONST,
            .ival       = val,
            .strval     = NULL,
            .strval_len = 0,
            .params_len = 0,
            .params     = NULL,
        };
    } else {
        array_lencap_insert_strong(&ptr, 1, &len, &cap, "", len);
        return (token_t){
            .pos        = pos_between(start_pos, ctx->pos),
            .type       = TOKENTYPE_SCONST,
            .strval     = ptr,
            .strval_len = len - 1,
            .ival       = 0,
            .params_len = 0,
            .params     = NULL,
        };
    }
}

// Skip past a line comment.
static void ir_line_comment(tokenizer_t *ctx) {
    while (1) {
        int c = srcfile_getc(ctx->file, &ctx->pos);
        if (c == '\\') {
            srcfile_getc(ctx->file, &ctx->pos);
        } else if (c == '\n') {
            return;
        }
    }
}

// Skip past a block comment.
static void ir_block_comment(tokenizer_t *ctx) {
    int prev = 0;
    while (1) {
        int c = srcfile_getc(ctx->file, &ctx->pos);
        if (c == '/' && prev == '*') {
            return;
        }
        prev = c;
    }
}

// Get next token from IR tokenizer.
token_t ir_tkn_next(tokenizer_t *ctx) {
    pos_t pos0;
retry:
    pos0  = ctx->pos;
    int c = srcfile_getc(ctx->file, &ctx->pos);
#define pos1 ctx->pos

    if (c == -1) {
        // End of file.
        return (token_t){
            .type = TOKENTYPE_EOF,
            .pos  = pos1,
        };
    } else if (c == '\n') {
        // End of line.
        return (token_t){
            .type = TOKENTYPE_EOL,
            .pos  = pos1,
        };
    } else if (c <= 0x20) {
        // Whitespace (is ignored).
        goto retry;
    }

    // Comments.
    if (c == '/') {
        pos_t pos2 = pos1;
        int   c1   = srcfile_getc(ctx->file, &pos2);
        if (c1 == '/') {
            ctx->pos = pos2;
            ir_line_comment(ctx);
            goto retry;
        } else if (c1 == '*') {
            ctx->pos = pos2;
            ir_block_comment(ctx);
            goto retry;
        }
    }

    // Strings.
    if (c == '\'') {
        return ir_tkn_str(ctx, pos0, true);
    } else if (c == '\"') {
        return ir_tkn_str(ctx, pos0, false);
    }

    // Identifiers.
    if (c == '%') {
        c             = srcfile_getc(ctx->file, &pos1);
        token_t ident = ir_tkn_ident(ctx, pos0, c, false);
        ident.subtype = IR_IDENT_LOCAL;
        return ident;
    } else if (c == '<') {
        c             = srcfile_getc(ctx->file, &pos1);
        token_t ident = ir_tkn_ident(ctx, pos0, c, false);
        ident.subtype = IR_IDENT_GLOBAL;
        pos_t pos2    = pos1;
        c             = srcfile_getc(ctx->file, &pos2);
        if (c == '>') {
            ctx->pos  = pos2;
            ident.pos = pos_between(pos0, pos2);
        } else {
            cctx_diagnostic(ctx->cctx, pos1, DIAG_ERR, "Expected `>`");
        }
        return ident;
    } else if (ir_is_sym_char(c)) {
        // Without a prefix, it's a keyword.
        token_t kw = ir_tkn_ident(ctx, pos0, c, true);
        if (kw.type != TOKENTYPE_KEYWORD) {
            return kw;
        }

        // If there's a `'`, it's a numeric constant.
        pos_t pos2 = pos1;
        c          = srcfile_getc(ctx->file, &pos2);
        if (c != '\'') {
            // If not, just the keyword.
            return kw;
        }
        ctx->pos = pos2;

        // Numeric constants.
        ir_prim_t prim = kw.subtype - IR_KEYW_s8 + IR_PRIM_s8;
        if (prim < 0 || prim >= IR_N_PRIM) {
            prim = 0;
            cctx_diagnostic(ctx->cctx, kw.pos, DIAG_ERR, "Expected a type for numeric literal");
        }
        c = srcfile_getc(ctx->file, &ctx->pos);
        if (c == '?') {
            // Undefined value.
            token_t tkn = other_tkn(IR_TKN_UNDEF, pos0, ctx->pos);
            tkn.ival    = prim;
            return tkn;
        }
        bool is_negative = false;
        if (c == '-') {
            // Negative numeric constant.
            is_negative = true;
            pos2        = ctx->pos;
            c           = srcfile_getc(ctx->file, &ctx->pos);
        }
        if (c == '0') {
            // Hex, binary, octal.
            pos_t pos3 = ctx->pos;
            int   c2   = srcfile_getc(ctx->file, &pos3);
            if (c2 == 'x' || c2 == 'X') {
                // Hexadecimal.
                ctx->pos = pos3;
                return ir_tkn_numeric(ctx, prim, pos0, 16, is_negative);
            } else if (c2 == 'b' || c2 == 'B') {
                // Binary.
                ctx->pos = pos3;
                return ir_tkn_numeric(ctx, prim, pos0, 2, is_negative);
            } else if (c2 == 'o' || c2 == 'O') {
                // Binary.
                ctx->pos = pos3;
                return ir_tkn_numeric(ctx, prim, pos0, 8, is_negative);
            } else if (c2 >= '0' && c2 <= '9') {
                // Decimal.
                return ir_tkn_numeric(ctx, prim, pos0, 10, is_negative);
            } else {
                // Just a zero.
                return (token_t){
                    .type = TOKENTYPE_ICONST,
                    .pos  = pos_between(pos0, ctx->pos),
                    .ival = 0,
                };
            }

        } else if (c >= '1' && c <= '9') {
            // Decimal.
            ctx->pos = pos2;
            return ir_tkn_numeric(ctx, prim, pos0, 10, is_negative);

        } else {
            // Invalid constant.
            return (token_t){
                .pos        = pos_between(pos0, pos1),
                .type       = TOKENTYPE_GARBAGE,
                .strval     = NULL,
                .ival       = 0,
                .params_len = 0,
                .params     = NULL,
            };
        }
    }

    // Single-character tokens.
    switch (c) {
        case ',': return other_tkn(IR_TKN_COMMA, pos0, pos1);
        case '(': return other_tkn(IR_TKN_LPAR, pos0, pos1);
        case ')': return other_tkn(IR_TKN_RPAR, pos0, pos1);
        case '=': return other_tkn(IR_TKN_ASSIGN, pos0, pos1);
        case '+': return other_tkn(IR_TKN_ADD, pos0, pos1);
        case '-': return other_tkn(IR_TKN_SUB, pos0, pos1);
    }

    // At this point, it's garbage.
    return (token_t){
        .pos        = pos_between(pos0, pos1),
        .type       = TOKENTYPE_GARBAGE,
        .strval     = NULL,
        .ival       = 0,
        .params_len = 0,
        .params     = NULL,
    };
#undef pos1
}


// Get an IR keyword by C-string.
ir_keyw_t ir_keyw_get(char const *keyw) {
    for (ir_keyw_t i = 0; i < IR_N_KEYWS; i++) {
        if (!strcmp(ir_keywords[i], keyw)) {
            return i;
        }
    }
    return IR_N_KEYWS;
}
