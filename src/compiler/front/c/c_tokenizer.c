
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_tokenizer.h"

#include "c_compiler.h"
#include "compiler.h"
#include "strong_malloc.h"
#include "utf8.h"

#include <arrays.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>



#ifndef NDEBUG
// Enum names of `c_keyw_t` values.
char const *const c_keyw_name[] = {
#define C_KEYW_DEF(since, deprecated, name) "C_KEYW_" #name,
#include "c_keywords.inc"
};

// Enum names of `c_tokentype_t` values.
char const *const c_tokentype_name[] = {
#define C_TOKEN_DEF(id, name) "C_TKN_" #id,
#include "c_tokens.inc"
};
#endif

// List of keywords.
char const *const c_keywords[] = {
#define C_KEYW_DEF(since, deprecated, name) #name,
#include "c_keywords.inc"
};

// Token introduction dates.
static long c_keyw_since[] = {
#define C_KEYW_DEF(since, deprecated, name) since,
#include "c_keywords.inc"
};


// Create a new C tokenizer.
tokenizer_t *c_tkn_create(srcfile_t *srcfile, int c_std) {
    c_tokenizer_t *tkn_ctx    = strong_calloc(sizeof(c_tokenizer_t), 1);
    tkn_ctx->base.cctx        = srcfile->ctx;
    tkn_ctx->base.pos.srcfile = srcfile;
    tkn_ctx->base.file        = srcfile;
    tkn_ctx->base.next        = c_tkn_next;
    tkn_ctx->c_std            = c_std;
    return &tkn_ctx->base;
}


// Comparator function for searching for keywords.
static int keyw_comp(void const *a0, void const *b0) {
    char const *a = *(char const *const *)a0;
    char const *b = b0;
    return strcmp(a, b);
}

// Helper function to create tokens for better readability.
token_t other_tkn(c_tokentype_t type, pos_t from, pos_t to) {
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

// Test whether a character is legal as the first in a C identifier.
bool c_is_first_sym_char(int c) {
    if (c == '_') {
        return true;
    }
    c |= 0x20;
    return c >= 'a' && c <= 'z';
}

// Test whether a character is legal in a C identifier.
bool c_is_sym_char(int c) {
    if (c == '_' || (c >= '0' && c <= '9')) {
        return true;
    }
    c |= 0x20;
    return c >= 'a' && c <= 'z';
}


// Tokenize integer constant.
static token_t c_tkn_integer(tokenizer_t *ctx, pos_t start_pos, unsigned int base) {
    uint64_t val      = 0;
    bool     hasdat   = false;
    bool     toolarge = false;
    bool     invalid  = false;

    pos_t pos0 = ctx->pos;
    pos_t pos1;
    while (1) {
        unsigned int digit;
        pos1  = pos0;
        int c = srcfile_getc(ctx->file, &pos1);
        if (c >= '0' && c <= '9') {
            // Valid digit 0-9.
            digit = c - '0';
        } else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f') {
            // Valid digit a-f / A-F.
            digit = (c | 0x20) - 'a' + 10;
        } else {
            // End of constant.
            break;
        }
        if (digit >= base) {
            invalid = true;
        }
        if (val * base + digit < val) {
            toolarge = true;
        }
        val    = val * base + digit;
        hasdat = true;
        pos0   = pos1;
    }
    pos1 = pos0;

    // Check for literal suffixes.
    pos_t    lit_end    = pos0;
    c_prim_t c_prim     = C_PRIM_SINT;
    bool     bad_suffix = false;
    bool     u_suffix   = false;
    int      c          = srcfile_getc(ctx->file, &pos1);

    // Promote the primitive to be bigger if necessary.
    int opt_int_bits   = 32;
    int opt_long_bits  = 32;
    int opt_llong_bits = 64;
    if (val <= ((1llu << opt_int_bits) - 1)) {
        c_prim = C_PRIM_SINT;
    } else if (val <= ((1llu << opt_long_bits) - 1)) {
        c_prim = C_PRIM_SLONG;
    } else {
        c_prim = C_PRIM_SLLONG;
    }

    // Unsigned (before).
    if (c == 'u' || c == 'U') {
        u_suffix = true;
        pos0     = pos1;
        c        = srcfile_getc(ctx->file, &pos1);
        if (c != 'l' && c != 'L' && (c == '_' || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z'))) {
            bad_suffix = true;
        }
    }
    // Long / long long.
    if (!bad_suffix && (c == 'l' || c == 'L')) {
        pos0   = pos1;
        int c2 = c;
        c      = srcfile_getc(ctx->file, &pos1);
        if (c == c2) {
            pos0   = pos1;
            c      = srcfile_getc(ctx->file, &pos1);
            c_prim = C_PRIM_SLLONG;
        } else if (c != 'u' && (c == '_' || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z'))) {
            bad_suffix = true;
        } else {
            c_prim = C_PRIM_SLONG;
        }
    } else if (c == '_' || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z')) {
        bad_suffix = true;
    }
    // Unsigned (after).
    if (!bad_suffix && !u_suffix && c_prim != C_PRIM_SINT && (c == 'u' || c == 'U')) {
        pos0     = pos1;
        c        = srcfile_getc(ctx->file, &pos1);
        u_suffix = true;
    } else if (c == '_' || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z')) {
        bad_suffix = true;
    }
    // Assert the suffix to end now.
    if (!bad_suffix && (c == '_' || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z'))) {
        bad_suffix = true;
    }

    if (bad_suffix && hasdat) {
        cctx_diagnostic(ctx->cctx, pos_including(lit_end, pos0), DIAG_ERR, "Invalid literal suffix");
    }

    // Make it unsigned if the MSB is set.
    int bits = c_prim == C_PRIM_SINT ? opt_int_bits : c_prim == C_PRIM_SLONG ? opt_long_bits : opt_llong_bits;
    if (val >> (bits - 1)) {
        u_suffix = true;
    }

    ctx->pos  = pos0;
    pos_t pos = pos_including(start_pos, pos0);
    if (invalid || !hasdat) {
        // Report error (invalid constant).
        char const *ctype;
        switch (base) {
            case 2: ctype = "binary"; break;
            case 8: ctype = "octal"; break;
            case 10: ctype = "decimal"; break;
            case 16: ctype = "hexadecimal"; break;
            default: abort();
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
    } else if (toolarge) {
        cctx_diagnostic(ctx->cctx, pos, DIAG_WARN, "Constant is too large and was truncated to %" PRId64, val);
    }
    return (token_t){
        .type       = TOKENTYPE_ICONST,
        .pos        = pos,
        .ival       = val,
        .subtype    = (int)c_prim - u_suffix,
        .strval     = NULL,
        .strval_len = 0,
        .params_len = 0,
        .params     = NULL,
    };
}

// Tokenize identifier.
static token_t c_tkn_ident(tokenizer_t *ctx, pos_t start_pos, char first) {
    c_tokenizer_t *c_ctx = (c_tokenizer_t *)ctx;
    size_t         cap   = 32;
    size_t         len   = 1;
    char          *ptr   = strong_malloc(cap);
    ptr[0]               = first;

    pos_t pos0 = ctx->pos;
    pos_t pos1;
    while (1) {
        pos1  = pos0;
        int c = srcfile_getc(ctx->file, &pos1);
        if (!c_is_sym_char(c)) {
            // End of identifier.
            break;
        }
        if (len == cap - 1) {
            // Even longer name, allocate more memory.
            cap *= 2;
            ptr  = strong_realloc(ptr, cap);
        }
        ptr[len++] = (char)c;
        pos0       = pos1;
    }
    ptr[len] = 0;
    ctx->pos = pos0;

    // Test for keywords.
    c_keyw_t keyw = c_keyw_get(ctx, ptr);
    if (keyw < C_N_KEYWS && !c_ctx->preproc_mode) {
        // Replace alternate spellings with main spellings, even if the main spelling is from a later C standard.
#define C_ALT_KEYW_DEF(main_spelling, alt_spelling)                                                                    \
    if (keyw == C_KEYW_##main_spelling) {                                                                              \
        keyw = C_KEYW_##alt_spelling;                                                                                  \
    }
#include "c_keywords.inc"
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

    return (token_t){
        .pos        = pos_between(start_pos, pos0),
        .type       = TOKENTYPE_IDENT,
        .strval     = ptr,
        .strval_len = len,
        .subtype    = 0,
        .params_len = 0,
        .params     = NULL,
    };
}

// Hex parsing helper for strings.
static int c_str_hex(tokenizer_t *ctx, pos_t start_pos, int min_w, int max_w) {
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
static int c_str_octal(tokenizer_t *ctx, pos_t start_pos, int first, int max_w) {
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
static token_t c_tkn_str(tokenizer_t *ctx, pos_t start_pos, bool is_char) {
    size_t cap = 32;
    size_t len = 0;
    char  *ptr = strong_malloc(cap);

    while (1) {
        pos_t pos0    = ctx->pos;
        int   c       = srcfile_getc(ctx->file, &ctx->pos);
        bool  as_utf8 = false;
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
                c = c_str_hex(ctx, start_pos, 8, 8);
            } else if (c == 'u') {
                // 4-hexit unicode point.
                as_utf8 = true;
                c       = c_str_hex(ctx, start_pos, 4, 4);
            } else if (c == 'x') {
                // Hexadecimal (of any length (because of course that's logical (it isn't))).
                c = c_str_hex(ctx, start_pos, 1, 32767);
            } else if (c >= '0' && c <= '3') {
                // 1- to 3-digit octal.
                c = c_str_octal(ctx, start_pos, c, 3);
            } else if (c >= '4' && c <= '7') {
                // 1- or 2-digit octal.
                c = c_str_octal(ctx, start_pos, c, 2);
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
        if (as_utf8) {
            uint8_t utf8_len = utf8_encode(NULL, 0, c);
            array_lencap_resize_strong(&ptr, 1, &len, &cap, len + utf8_len);
            utf8_encode(ptr - utf8_len, utf8_len, c);
        } else {
            uint8_t tmp = c;
            array_lencap_insert_strong(&ptr, 1, &len, &cap, &tmp, len);
        }
    }

    pos_t pos = pos_between(start_pos, ctx->pos);
    if (is_char) {
        uint64_t val = 0;
        for (size_t i = 0; i < len; i++) {
            val <<= 8;
            val  |= ptr[i];
        }
        if (len == 0) {
            cctx_diagnostic(ctx->cctx, pos, DIAG_ERR, "Empty character constant");
        } else if (len > 1) {
            cctx_diagnostic(ctx->cctx, pos, DIAG_WARN, "Multi-character character constant");
        }
        free(ptr);
        return (token_t){
            .pos        = pos,
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
            .pos        = pos,
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
static void c_line_comment(tokenizer_t *ctx) {
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
static void c_block_comment(tokenizer_t *ctx) {
    int prev = 0;
    while (1) {
        int c = srcfile_getc(ctx->file, &ctx->pos);
        if (c == '/' && prev == '*') {
            return;
        }
        prev = c;
    }
}

// Get next token from C tokenizer.
token_t c_tkn_next(tokenizer_t *ctx) {
    c_tokenizer_t *c_ctx = (c_tokenizer_t *)ctx;
    pos_t          pos0;

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
    } else if (c <= 0x20) {
        // Whitespace (is ignored).
        goto retry;
    }

    // Strings.
    if (c == '\'') {
        return c_tkn_str(ctx, pos0, true);
    } else if (c == '\"') {
        return c_tkn_str(ctx, pos0, false);
    }

    // Numeric constants.
    if (c == '0') {
        // Hex, binary, octal.
        pos_t pos2 = ctx->pos;
        int   c2   = srcfile_getc(ctx->file, &pos2);
        if (c2 == 'x' || c2 == 'X') {
            // Hexadecimal.
            ctx->pos = pos2;
            return c_tkn_integer(ctx, pos0, 16);
        } else if (c2 == 'b' || c2 == 'B') {
            // GNU extension: Binary.
            ctx->pos = pos2;
            return c_tkn_integer(ctx, pos0, 2);
        } else {
            // Octal.
            ctx->pos = pos0;
            return c_tkn_integer(ctx, pos0, 8);
        }
    }

    // Decimal constants.
    if (c >= '1' && c <= '9') {
        ctx->pos = pos0;
        return c_tkn_integer(ctx, pos0, 10);
    }

    // Identifiers.
    if (c_is_sym_char(c)) {
        return c_tkn_ident(ctx, pos0, (char)c);
    }

    // Single-character tokens.
    switch (c) {
        case '(': return other_tkn(C_TKN_LPAR, pos0, pos1);
        case ')': return other_tkn(C_TKN_RPAR, pos0, pos1);
        case ',': return other_tkn(C_TKN_COMMA, pos0, pos1);
        case ':': return other_tkn(C_TKN_COLON, pos0, pos1);
        case ';': return other_tkn(C_TKN_SEMIC, pos0, pos1);
        case '?': return other_tkn(C_TKN_QUESTION, pos0, pos1);
        case '[': return other_tkn(C_TKN_LBRAC, pos0, pos1);
        case ']': return other_tkn(C_TKN_RBRAC, pos0, pos1);
        case '{': return other_tkn(C_TKN_LCURL, pos0, pos1);
        case '}': return other_tkn(C_TKN_RCURL, pos0, pos1);
        case '~': return other_tkn(C_TKN_NOT, pos0, pos1);
        default: break;
    }

    // Possibly multi-character tokens.
    pos_t pos2 = ctx->pos;
    int   c2   = srcfile_getc(ctx->file, &pos2);
    if (c == '#' && c_ctx->preproc_mode) {
        if (c2 == '#') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_PASTE, pos0, pos2);
        } else {
            return other_tkn(C_TKN_HASH, pos0, pos1);
        }
    } else if (c == '.') {
        if (c2 == '.') {
            pos_t pos3 = pos2;
            int   c3   = srcfile_getc(ctx->file, &pos3);
            if (c3 == '.') {
                ctx->pos = pos3;
                return other_tkn(C_TKN_VARARG, pos0, pos3);
            } else {
                return other_tkn(C_TKN_DOT, pos0, pos1);
            }
        } else {
            return other_tkn(C_TKN_DOT, pos0, pos1);
        }
    } else if (c == '!') {
        if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_NE, pos0, pos2);
        } else {
            return other_tkn(C_TKN_LNOT, pos0, pos1);
        }
    } else if (c == '%') {
        if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_MOD_S, pos0, pos2);
        } else {
            return other_tkn(C_TKN_MOD, pos0, pos1);
        }
    } else if (c == '&') {
        if (c2 == '&') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_LAND, pos0, pos2);
        } else if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_AND_S, pos0, pos2);
        } else {
            return other_tkn(C_TKN_AND, pos0, pos1);
        }
    } else if (c == '*') {
        if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_MUL_S, pos0, pos2);
        } else {
            return other_tkn(C_TKN_MUL, pos0, pos1);
        }
    } else if (c == '+') {
        if (c2 == '+') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_INC, pos0, pos2);
        } else if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_ADD_S, pos0, pos2);
        } else {
            return other_tkn(C_TKN_ADD, pos0, pos1);
        }
    } else if (c == '-') {
        if (c2 == '-') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_DEC, pos0, pos2);
        } else if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_SUB_S, pos0, pos2);
        } else if (c2 == '>') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_ARROW, pos0, pos2);
        } else {
            return other_tkn(C_TKN_SUB, pos0, pos1);
        }
    } else if (c == '/') {
        if (c2 == '/') {
            ctx->pos = pos2;
            c_line_comment(ctx);
            goto retry;
        } else if (c2 == '*') {
            ctx->pos = pos2;
            c_block_comment(ctx);
            goto retry;
        } else if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_DIV_S, pos0, pos2);
        } else {
            return other_tkn(C_TKN_DIV, pos0, pos1);
        }
    } else if (c == '<') {
        if (c2 == '<') {
            pos_t pos3 = pos2;
            int   c3   = srcfile_getc(ctx->file, &pos3);
            if (c3 == '=') {
                ctx->pos = pos3;
                return other_tkn(C_TKN_SHL_S, pos0, pos3);
            } else {
                ctx->pos = pos2;
                return other_tkn(C_TKN_SHL, pos0, pos2);
            }
        } else if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_LE, pos0, pos2);
        } else {
            return other_tkn(C_TKN_LT, pos0, pos1);
        }
    } else if (c == '>') {
        if (c2 == '>') {
            pos_t pos3 = pos2;
            int   c3   = srcfile_getc(ctx->file, &pos3);
            if (c3 == '=') {
                ctx->pos = pos3;
                return other_tkn(C_TKN_SHR_S, pos0, pos3);
            } else {
                ctx->pos = pos2;
                return other_tkn(C_TKN_SHR, pos0, pos2);
            }
        } else if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_GE, pos0, pos2);
        } else {
            return other_tkn(C_TKN_GT, pos0, pos1);
        }
    } else if (c == '^') {
        if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_XOR_S, pos0, pos2);
        } else {
            return other_tkn(C_TKN_XOR, pos0, pos1);
        }
    } else if (c == '|') {
        if (c2 == '|') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_LOR, pos0, pos2);
        } else if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_OR_S, pos0, pos2);
        } else {
            return other_tkn(C_TKN_OR, pos0, pos1);
        }
    } else if (c == '=') {
        if (c2 == '=') {
            ctx->pos = pos2;
            return other_tkn(C_TKN_EQ, pos0, pos2);
        } else {
            return other_tkn(C_TKN_ASSIGN, pos0, pos1);
        }
    }

    // At this point, it's garbage.
    return (token_t){
        .pos        = pos2,
        .type       = TOKENTYPE_GARBAGE,
        .strval     = NULL,
        .ival       = 0,
        .params_len = 0,
        .params     = NULL,
    };
#undef pos1
}

// Try to find the matching C keyword.
// Returns C_N_KEYWS if not a keyword in the current C standard.
c_keyw_t c_keyw_get(tokenizer_t const *ctx, char const *name) {
    c_tokenizer_t *c_ctx = (c_tokenizer_t *)ctx;

    array_binsearch_t res = array_binsearch(c_keywords, sizeof(char *), C_N_KEYWS, name, keyw_comp);
    if (res.found && c_keyw_since[res.index] <= c_ctx->c_std) {
        return res.index;
    }

    return C_N_KEYWS;
}
