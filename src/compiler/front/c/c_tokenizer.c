
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_tokenizer.h"

#include "arith128.h"
#include "c_std.h"
#include "c_types.h"
#include "compiler.h"
#include "strong_malloc.h"
#include "tokenizer.h"
#include "utf8.h"

#include <arrays.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Enum names of `c_keyw_t` values.
char const *const c_keyw_name[] = {
#define C_KEYW_DEF(since, deprecated, name) "C_KEYW_" #name,
#include "c_keywords.inc"
};

// Enum names of `c_tokentype_t` values.
char const *const c_token_id[] = {
#define C_TOKEN_DEF(id, name) "C_TKN_" #id,
#include "c_tokens.inc"
};

// List of keywords.
char const *const c_keywords[] = {
#define C_KEYW_DEF(since, deprecated, name) #name,
#include "c_keywords.inc"
};

// List of tokens.
char const *const c_token_name[] = {
#define C_TOKEN_DEF(id, name) name,
#include "c_tokens.inc"
};

// Token introduction dates.
static long c_keyw_since[] = {
#define C_KEYW_DEF(since, deprecated, name) since,
#include "c_keywords.inc"
};


// Create a new C tokenizer.
c_tokenizer_t *c_tkn_create(srcfile_t *srcfile, int c_std) {
    c_tokenizer_t *tkn_ctx    = strong_calloc(sizeof(c_tokenizer_t), 1);
    tkn_ctx->base.cctx        = srcfile->ctx;
    tkn_ctx->base.pos.srcfile = srcfile;
    tkn_ctx->base.file        = srcfile;
    tkn_ctx->base.next        = c_tkn_next;
    tkn_ctx->c_std            = c_std;
    return tkn_ctx;
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


// Preprocessing number token.
static token_t c_tkn_pre_number(tokenizer_t *ctx) {
    size_t len = 0;
    size_t cap = 8;
    char  *buf = strong_malloc(cap);

    pos_t pos0 = ctx->pos;
    int   c    = c_srcfile_getc(ctx->file, &ctx->pos);
    array_lencap_insert_strong(&buf, 1, &len, &cap, (char[]){(char)c}, len);
    if (c == '.') {
        // `.` followed by digit (otherwise, just a digit).
        c = c_srcfile_getc(ctx->file, &ctx->pos);
        array_lencap_insert_strong(&buf, 1, &len, &cap, (char[]){(char)c}, len);
    }

    while (1) {
        pos_t pos1 = ctx->pos;
        c          = c_srcfile_getc(ctx->file, &pos1);
        if (c == '.') {
            array_lencap_insert_strong(&buf, 1, &len, &cap, ".", len);
            ctx->pos = pos1;
        } else if (c == 'e' || c == 'E' || c == 'p' || c == 'P') {
            pos_t pos2 = pos1;
            int   c2   = c_srcfile_getc(ctx->file, &pos2);
            if (c2 == '+' || c2 == '-') {
                char enc[] = {(char)c, (char)c2};
                array_lencap_insert_n_strong(&buf, 1, &len, &cap, enc, len, 2);
                ctx->pos = pos1;
            } else {
                array_lencap_insert_strong(&buf, 1, &len, &cap, (char[]){(char)c}, len);
                ctx->pos = pos1;
            }
        } else if (c_is_sym_char(c)) {
            char enc[4];
            int  enc_len = utf8_encode(enc, 4, c);
            array_lencap_insert_n_strong(&buf, 1, &len, &cap, enc, len, enc_len);
            ctx->pos = pos1;
        } else if (c == '\'') {
            int c2 = c_srcfile_getc(ctx->file, &pos1);
            if (c_is_sym_char(c2)) {
                char enc[5]  = {'\''};
                int  enc_len = utf8_encode(enc + 1, 4, c2);
                array_lencap_insert_n_strong(&buf, 1, &len, &cap, enc, len, enc_len + 1);
                ctx->pos = pos1;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    array_lencap_insert_strong(&buf, 1, &len, &cap, "", len);
    return (token_t){
        .pos        = pos_including(pos0, ctx->pos),
        .type       = TOKENTYPE_IDENT,
        .subtype    = C_PPNUMBER,
        .strval     = buf,
        .strval_len = len - 1,
    };
}

// Convert preprocessing number token to C number token.
token_t c_tkn_conv_number(cctx_t *cctx, int c_std, token_t const *pre_tkn) {
    i128_t val      = int128(0, 0);
    bool   hasdat   = false;
    bool   toolarge = false;
    bool   invalid  = false;

    tknoff_t     off0 = {0};
    tknoff_t     off1 = off0;
    unsigned int base;
    int          c = tkn_getc(pre_tkn, &off1);
    if (c == '0') {
        c = tkn_getc(pre_tkn, &off1);
        if ((c | 0x20) == 'x') {
            base = 16;
            off0 = off1;
        } else if ((c | 0x20) == 'b') {
            base = 2;
            off0 = off1;
        } else {
            base = 8;
        }
    } else {
        base = 10;
    }

    while (1) {
        unsigned int digit;
        off1  = off0;
        int c = tkn_getc(pre_tkn, &off1);
        if (c >= '0' && c <= '9') {
            // Valid digit 0-9.
            digit = c - '0';
        } else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f') {
            // Valid digit a-f / A-F.
            digit = (c | 0x20) - 'a' + 10;
        } else if (hasdat && c == '\'' && c_std >= C_STD_C23) {
            // A separator.
            hasdat = false;
            off0   = off1;
            continue;
        } else {
            // End of constant.
            break;
        }
        if (digit >= base) {
            invalid = true;
        }
        i128_t next = add128(mul128(val, int128(0, base)), int128(0, digit));
        if (cmp128u(next, val) < 0) {
            toolarge = true;
        }
        val    = next;
        hasdat = true;
        off0   = off1;
    }
    off1 = off0;

    // Check for literal suffixes.
    tknoff_t lit_end     = off0;
    c_prim_t c_prim      = C_PRIM_SINT;
    bool     bad_suffix  = false;
    bool     u_suffix    = false;
    bool     l_suffix    = false;
    bool     ll_suffix   = false;
    bool     i128_suffix = false;
    c                    = tkn_getc(pre_tkn, &off1);

    // Promote the primitive to be bigger if necessary.
    // TODO: Tokenizer is currently not aware of the C options.
    i128_t const i32_max = int128(0, INT32_MAX);
    i128_t const u32_max = int128(0, UINT32_MAX);
    i128_t const i64_max = int128(0, INT64_MAX);
    i128_t const u64_max = int128(0, UINT64_MAX);
    // Note: No automatic promotion to 128-bit without explicit suffix;
    // 128-bit literals are a Lily-C (not even GCC/clang) extension.
    if (cmp128u(val, i32_max) < 0) {
        c_prim = C_PRIM_SINT;
    } else if (cmp128u(val, u32_max) < 0) {
        c_prim = C_PRIM_UINT;
    } else if (cmp128u(val, i64_max) < 0) {
        c_prim = C_PRIM_SLONG;
    } else if (cmp128u(val, u64_max) < 0) {
        c_prim = C_PRIM_ULONG;
    } else if (cmp128u(val, i64_max) < 0) {
        c_prim = C_PRIM_SLLONG;
    } else {
        c_prim = C_PRIM_ULLONG;
    }

    // Unsigned (before).
    if (c == 'u' || c == 'U') {
        u_suffix = true;
        off0     = off1;
        c        = tkn_getc(pre_tkn, &off1);
        if (c != 'l' && c != 'L' && ((c | 0x20) >= 'a' && (c | 0x20) <= 'z')) {
            bad_suffix = true;
        }
    }
    // Long / long long / _x128.
    if (!bad_suffix && (c == 'l' || c == 'L')) {
        off0   = off1;
        int c2 = c;
        c      = tkn_getc(pre_tkn, &off1);
        if (c == c2) {
            off0      = off1;
            c         = tkn_getc(pre_tkn, &off1);
            ll_suffix = true;
        } else if (c != 'u' && ((c | 0x20) >= 'a' && (c | 0x20) <= 'z')) {
            bad_suffix = true;
        } else {
            l_suffix = true;
        }
    } else if (!bad_suffix && c == '_') {
        tknoff_t pos2 = off1;
        int      c_x  = tkn_getc(pre_tkn, &pos2);
        int      c_1  = tkn_getc(pre_tkn, &pos2);
        int      c_2  = tkn_getc(pre_tkn, &pos2);
        int      c_8  = tkn_getc(pre_tkn, &pos2);
        if ((c_x == 'x' || c_x == 'X') && c_1 == '1' && c_2 == '2' && c_8 == '8') {
            off0 = off1 = pos2;
            c           = tkn_getc(pre_tkn, &off1);
            i128_suffix = true;
        } else {
            bad_suffix = true;
        }
    } else if (((c | 0x20) >= 'a' && (c | 0x20) <= 'z')) {
        bad_suffix = true;
    }
    // Unsigned (after).
    if (!bad_suffix && !u_suffix && c_prim != C_PRIM_SINT && (c == 'u' || c == 'U')) {
        off0     = off1;
        c        = tkn_getc(pre_tkn, &off1);
        u_suffix = true;
    } else if (c == '_' || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z')) {
        bad_suffix = true;
    }
    // Assert the suffix to end now.
    if (!bad_suffix && c != -1) {
        bad_suffix = true;
    }

    // Change primitive type according to literal suffix.
    if (i128_suffix && c_prim < C_PRIM_S128) {
        c_prim = C_PRIM_S128;
    } else if (ll_suffix && c_prim < C_PRIM_SLLONG) {
        c_prim = C_PRIM_SLLONG;
    } else if (l_suffix && c_prim < C_PRIM_SLONG) {
        c_prim = C_PRIM_SLONG;
    }
    if (hi64(val) != 0 && !i128_suffix) {
        val      = int128(0, lo64(val));
        toolarge = true;
    }
    if (u_suffix) {
        c_prim |= 1; // Unsigned primitives have uneven encoding in the enum.
    }

    if (bad_suffix && hasdat) {
        pos_t pos  = pre_tkn->pos;
        pos.off   += (off_t)lit_end.offset;
        pos.len   -= (off_t)lit_end.offset;
        pos.col   += lit_end.col_offset;
        pos.line  += lit_end.line_offset;
        cctx_diagnostic(cctx, pos, DIAG_ERR, "Invalid literal suffix");
    }

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
        cctx_diagnostic(cctx, pre_tkn->pos, DIAG_ERR, "Invalid %s constant", ctype);
        return (token_t){
            .type       = TOKENTYPE_ICONST,
            .pos        = pre_tkn->pos,
            .ival       = 0,
            .subtype    = 0,
            .strval     = NULL,
            .strval_len = 0,
            .params_len = 0,
            .params     = NULL,
        };
    } else if (toolarge) {
        char dec[40];
        itoa128(val, 1, dec);
        char hex[33];
        if (hi64(val) == 0) {
            snprintf(hex, sizeof(hex), "%" PRIx64, lo64(val));
        } else {
            snprintf(hex, sizeof(hex), "%" PRIx64 "%016" PRIx64, hi64(val), lo64(val));
        }
        cctx_diagnostic(
            cctx,
            pre_tkn->pos,
            DIAG_WARN,
            "Constant is too large and was truncated to %s (0x%s)",
            dec,
            hex
        );
    }
    return (token_t){
        .type       = TOKENTYPE_ICONST,
        .pos        = pre_tkn->pos,
        .ival       = lo64(val),
        .ivalh      = hi64(val),
        .subtype    = c_prim,
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
        int c = c_srcfile_getc(ctx->file, &pos1);
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
    c_keyw_t keyw = c_keyw_get(c_ctx->c_std, ptr);
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
        .subtype    = C_IDENT,
        .params_len = 0,
        .params     = NULL,
    };
}

// Preprocessing string token.
static token_t c_tkn_pre_str(tokenizer_t *ctx, pos_t start_pos, c_strtype_t subtype) {
    size_t cap     = 32;
    size_t len     = 0;
    char  *buf     = strong_malloc(cap);
    pos_t  end_pos = start_pos;
    bool   do_esc;
    char   start;
    char   end;
    switch (subtype) {
        default: abort();
        case C_STR_RAW_DQUOT:
            start = end = '\"';
            do_esc      = true;
            break;
        case C_STR_RAW_SQUOT:
            start = end = '\'';
            do_esc      = true;
            break;
        case C_STR_ANGLEBRAC:
            start  = '<';
            end    = '>';
            do_esc = false;
            break;
    }

    // Skip start char.
    c_srcfile_getc(ctx->file, &end_pos);
    pos_t open_pos = pos_between(start_pos, end_pos);
    bool  esc      = false;
    while (1) {
        pos_t pos1 = end_pos;
        int   c    = c_srcfile_getc(ctx->file, &end_pos);
        if (c == -1 || c == '\n') {
            cctx_diagnostic(ctx->cctx, pos1, DIAG_ERR, "Expected %c", end);
            cctx_diagnostic(ctx->cctx, open_pos, DIAG_HINT, "To match this %c", start);
            break;
        } else if (c == end && !esc) {
            break;
        } else {
            esc = c == '\\' && do_esc && !esc;
            if (c >= 0x80) {
                uint8_t utf8_len = utf8_encode(NULL, 0, c);
                array_lencap_resize_strong(&buf, 1, &len, &cap, len + utf8_len);
                utf8_encode(buf + len - utf8_len, utf8_len, c);
            } else {
                uint8_t tmp = c;
                array_lencap_insert_strong(&buf, 1, &len, &cap, &tmp, len);
            }
        }
    }

    array_lencap_insert_strong(&buf, 1, &len, &cap, "", len);
    ctx->pos = end_pos;
    return (token_t){
        .pos        = pos_including(start_pos, end_pos),
        .type       = TOKENTYPE_SCONST,
        .subtype    = subtype,
        .strval     = buf,
        .strval_len = len - 1, // -1 excludes the NUL terminator
    };
}

// Hex parsing helper for strings.
static int c_str_conv_hex(cctx_t *cctx, token_t const *pre_tkn, tknoff_t *off, int min_w, int max_w) {
    int      value = 0;
    tknoff_t off0  = *off;
    tknoff_t off1;
    for (int i = 0; i < max_w; i++) {
        off1  = off0;
        int c = tkn_getc(pre_tkn, &off1);
        if (c >= '0' && c <= '9') {
            value <<= 4;
            value  |= c - '0';
        } else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f') {
            value <<= 4;
            value  |= (c | 0x20) - 'a' + 0xa;
        } else {
            if (i < min_w) {
                pos_t pos  = pre_tkn->pos;
                pos.off   += (off_t)off->offset;
                pos.col   += off->col_offset;
                pos.line  += off->line_offset;
                pos.len    = (off_t)(off0.offset - off->offset);
                cctx_diagnostic(cctx, pos, DIAG_ERR, "Invalid hexadecimal escape sequence");
            }
            break;
        }
        off0 = off1;
    }
    *off = off0;
    return value;
}

// Octal parsing helper for strings.
static int c_str_conv_octal(cctx_t *cctx, token_t const *pre_tkn, tknoff_t *off, int first, int max_w) {
    (void)cctx;
    int      value = first - '0';
    tknoff_t off0  = *off;
    tknoff_t off1;
    for (int i = 1; i < max_w; i++) {
        off1  = off0;
        int c = tkn_getc(pre_tkn, &off1);
        if (c < '0' || c > '7') {
            break;
        } else {
            value <<= 3;
            value  |= c - '0';
        }
        off0 = off1;
    }
    *off = off0;
    return value;
}

// Convert preprocessing string token to C string token.
token_t c_tkn_conv_str(cctx_t *cctx, int c_std, token_t const *pre_tkn) {
    (void)c_std;
    size_t cap     = 32;
    size_t len     = 0;
    char  *ptr     = strong_malloc(cap);
    bool   is_char = pre_tkn->subtype == C_STR_RAW_SQUOT;

    tknoff_t off = {0};
    while (1) {
        tknoff_t off0    = off;
        int      c       = tkn_getc(pre_tkn, &off);
        bool     as_utf8 = false;
        if (c == -1) {
            // End of string.
            break;
        } else if (c == '\\') {
            // Escape sequence.
            c = tkn_getc(pre_tkn, &off);

            if (c == 'U') {
                // 8-hexit unicode point.
                c = c_str_conv_hex(cctx, pre_tkn, &off, 8, 8);
            } else if (c == 'u') {
                // 4-hexit unicode point.
                as_utf8 = true;
                c       = c_str_conv_hex(cctx, pre_tkn, &off, 4, 4);
            } else if (c == 'x') {
                // Hexadecimal (of any length (because of course that's logical (it isn't))).
                c = c_str_conv_hex(cctx, pre_tkn, &off, 1, 32767);
            } else if (c >= '0' && c <= '3') {
                // 1- to 3-digit octal.
                c = c_str_conv_octal(cctx, pre_tkn, &off, c, 3);
            } else if (c >= '4' && c <= '7') {
                // 1- or 2-digit octal.
                c = c_str_conv_octal(cctx, pre_tkn, &off, c, 2);
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
                    case '\n':
                    case 'n': c = '\n'; break;
                    case 'r': c = '\r'; break;
                    case 't': c = '\t'; break;
                    case 'v': c = '\v'; break;
                    default: {
                        pos_t pos  = pre_tkn->pos;
                        pos.off   += (off_t)off0.offset;
                        pos.col   += off0.col_offset;
                        pos.line  += off0.line_offset;
                        pos.len    = (off_t)(off.offset - off0.offset);
                        cctx_diagnostic(cctx, pos, DIAG_ERR, "Invalid escape sequence");
                    } break;
                }
            }
        } else if (c >= 0x80) {
            as_utf8 = true;
        }
        if (as_utf8) {
            uint8_t utf8_len = utf8_encode(NULL, 0, c);
            array_lencap_resize_strong(&ptr, 1, &len, &cap, len + utf8_len);
            utf8_encode(ptr + len - utf8_len, utf8_len, c);
        } else {
            uint8_t tmp = c;
            array_lencap_insert_strong(&ptr, 1, &len, &cap, &tmp, len);
        }
    }

    if (is_char) {
        uint64_t val = 0;
        for (size_t i = 0; i < len; i++) {
            val <<= 8;
            val  |= ptr[i];
        }
        if (len == 0) {
            cctx_diagnostic(cctx, pre_tkn->pos, DIAG_ERR, "Empty character constant");
        } else if (len > 1) {
            cctx_diagnostic(cctx, pre_tkn->pos, DIAG_WARN, "Multi-character character constant");
        }
        free(ptr);
        return (token_t){
            .pos        = pre_tkn->pos,
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
            .pos        = pre_tkn->pos,
            .type       = TOKENTYPE_SCONST,
            .strval     = ptr,
            .strval_len = len - 1,
            .ival       = 0,
            .params_len = 0,
            .params     = NULL,
        };
    }
}

// Read in a span of whitespace as a token.
static token_t c_tkn_whitespace(tokenizer_t *ctx, pos_t start_pos, c_whitespace_t subtype) {
    c_tokenizer_t *c_ctx = (c_tokenizer_t *)ctx;
    int            prev  = 0;

    size_t len = 0;
    size_t cap = 32;
    char  *buf = strong_malloc(cap);

    pos_t pos;
    while (1) {
        pos   = ctx->pos;
        int c = c_srcfile_getc(ctx->file, &pos);
        if (subtype == C_WHITESPACE) {
            if (c > 0x20 || c == -1) {
                break;
            }
        } else if (subtype == C_LINE_COMMENT) {
            if (c == '\n' || c == -1) {
                break;
            }
        } else {
            assert(subtype == C_BLOCK_COMMENT);
            if (c == -1) {
                cctx_diagnostic(ctx->cctx, pos, DIAG_ERR, "Unterminated block comment");
                break;
            } else if (c == '/' && prev == '*') {
                len--; // Exclude the `*` from the `*/`.
                break;
            }
            prev = c;
        }
        if (c_ctx->keep_comments || subtype == C_WHITESPACE) {
            char    enc[4];
            uint8_t enc_len = utf8_encode(enc, sizeof(enc), c);
            array_lencap_insert_n_strong(&buf, 1, &len, &cap, enc, len, enc_len);
        }
        ctx->pos = pos;
    }

    if (c_ctx->keep_comments || subtype == C_WHITESPACE) {
        array_lencap_insert_strong(&buf, 1, &len, &cap, "", len);
    } else {
        buf[0]  = ' ';
        buf[1]  = 0;
        len     = 2;
        subtype = C_WHITESPACE;
    }
    return (token_t){
        .pos        = pos_between(start_pos, pos),
        .type       = TOKENTYPE_WHITESPACE,
        .subtype    = subtype,
        .strval     = buf,
        .strval_len = len - 1,
    };
}

// A line comment.
static void c_line_comment(tokenizer_t *ctx) {
    while (1) {
        int c = c_srcfile_getc(ctx->file, &ctx->pos);
        if (c == '\\') {
            c_srcfile_getc(ctx->file, &ctx->pos);
        } else if (c == '\n') {
            break;
        }
    }
}

// A block comment.
static void c_block_comment(tokenizer_t *ctx) {
    int prev = 0;
    while (1) {
        int c = c_srcfile_getc(ctx->file, &ctx->pos);
        if (c == '/' && prev == '*') {
            return;
        }
        prev = c;
    }
}

// Wrapper around `srcfile_getc` that handles `\` for newline escapes.
int c_srcfile_getc(srcfile_t *file, pos_t *pos) {
    int c = srcfile_getc(file, pos);

    if (c == '\\') {
        pos_t pos1 = *pos;
        while (1) {
            int c2 = srcfile_getc(file, &pos1);
            if (c2 == '\n') {
                *pos = pos1;
                return srcfile_getc(file, pos);
            } else if (c2 == -1 || c > 0x20) {
                break;
            }
        }
    }

    return c;
}

// Get next token from C tokenizer.
token_t c_tkn_next(tokenizer_t *ctx) {
    c_tokenizer_t *c_ctx = (c_tokenizer_t *)ctx;
    pos_t          pos0;

retry:
    pos0  = ctx->pos;
    int c = c_srcfile_getc(ctx->file, &ctx->pos);
#define pos1 ctx->pos

    if (c == -1) {
        // End of file.
        return (token_t){
            .type = TOKENTYPE_EOF,
            .pos  = pos1,
        };
    } else if (c_ctx->preproc_mode && c == '\n') {
        // A newline is semantically significant to the preprocessor.
        return (token_t){
            .type = TOKENTYPE_EOL,
            .pos  = pos_between(pos0, pos1),
        };
    } else if (c <= 0x20) {
        if (c_ctx->preproc_mode) {
            ctx->pos = pos0;
            return c_tkn_whitespace(ctx, pos0, C_WHITESPACE);
        } else {
            // Only the preprocessor cares about whitespace.
            goto retry;
        }
    }

    // Strings.
    if (c == '\'') {
        token_t tkn = c_tkn_pre_str(ctx, pos0, C_STR_RAW_SQUOT);
        if (c_ctx->preproc_mode) {
            return tkn;
        }
        token_t res = c_tkn_conv_str(ctx->cctx, c_ctx->c_std, &tkn);
        tkn_delete(tkn);
        return res;
    } else if (c == '\"') {
        token_t tkn = c_tkn_pre_str(ctx, pos0, C_STR_RAW_DQUOT);
        if (c_ctx->preproc_mode) {
            return tkn;
        }
        token_t res = c_tkn_conv_str(ctx->cctx, c_ctx->c_std, &tkn);
        tkn_delete(tkn);
        return res;
    } else if (c == '<' && c_ctx->str_anglebrac) {
        return c_tkn_pre_str(ctx, pos0, C_STR_ANGLEBRAC);
    }

    // Numeric constants.
    if (c == '.') {
        // Hex, binary, octal.
        pos_t pos2 = ctx->pos;
        int   c2   = c_srcfile_getc(ctx->file, &pos2);
        if (c2 >= '0' && c2 <= '9') {
            // Numeric (starting with `.` and digit).
            ctx->pos    = pos0;
            token_t tkn = c_tkn_pre_number(ctx);
            if (c_ctx->preproc_mode) {
                return tkn;
            }
            token_t res = c_tkn_conv_number(ctx->cctx, c_ctx->c_std, &tkn);
            tkn_delete(tkn);
            return res;
        }
    }

    // Numeric (starting with a digit).
    if (c >= '0' && c <= '9') {
        ctx->pos    = pos0;
        token_t tkn = c_tkn_pre_number(ctx);
        if (c_ctx->preproc_mode) {
            return tkn;
        }
        token_t res = c_tkn_conv_number(ctx->cctx, c_ctx->c_std, &tkn);
        tkn_delete(tkn);
        return res;
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
    int   c2   = c_srcfile_getc(ctx->file, &pos2);
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
            int   c3   = c_srcfile_getc(ctx->file, &pos3);
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
            if (c_ctx->preproc_mode) {
                return c_tkn_whitespace(ctx, pos0, C_LINE_COMMENT);
            } else {
                c_line_comment(ctx);
                goto retry;
            }
        } else if (c2 == '*') {
            ctx->pos = pos2;
            if (c_ctx->preproc_mode) {
                return c_tkn_whitespace(ctx, pos0, C_BLOCK_COMMENT);
            } else {
                c_block_comment(ctx);
                goto retry;
            }
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
            int   c3   = c_srcfile_getc(ctx->file, &pos3);
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
            int   c3   = c_srcfile_getc(ctx->file, &pos3);
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
    char   *c_str     = malloc(5);
    uint8_t c_str_len = utf8_encode(c_str, 4, c);
    c_str[c_str_len]  = 0;
    return (token_t){
        .pos        = pos2,
        .type       = TOKENTYPE_GARBAGE,
        .strval     = c_str,
        .strval_len = c_str_len,
        .ival       = 0,
        .params_len = 0,
        .params     = NULL,
    };
#undef pos1
}

// Try to find the matching C keyword.
// Returns C_N_KEYWS if not a keyword in the current C standard.
c_keyw_t c_keyw_get(int c_std, char const *name) {
    array_binsearch_t res = array_binsearch(c_keywords, sizeof(char *), C_N_KEYWS, name, keyw_comp);
    if (res.found && c_keyw_since[res.index] <= c_std) {
        return res.index;
    }

    return C_N_KEYWS;
}

// Print the source representation of a token.
void c_tkn_print_src(token_t const *pre_tkn, FILE *to) {
    switch (pre_tkn->type) {
        case TOKENTYPE_KEYWORD: fputs(c_keywords[pre_tkn->subtype], to); break;
        case TOKENTYPE_SCONST:
            switch (pre_tkn->subtype) {
                case C_STR_ANGLEBRAC: fputc('<', to); break;
                case C_STR_RAW_DQUOT: fputc('\"', to); break;
                case C_STR_RAW_SQUOT: fputc('\'', to); break;
                default: abort(); // Not a valid preprocessor token.
            }
            fwrite(pre_tkn->strval, 1, pre_tkn->strval_len, to);
            switch (pre_tkn->subtype) {
                case C_STR_ANGLEBRAC: fputc('>', to); break;
                case C_STR_RAW_DQUOT: fputc('\"', to); break;
                case C_STR_RAW_SQUOT: fputc('\'', to); break;
                default: abort(); // Not a valid preprocessor token.
            }
            break;
        case TOKENTYPE_OTHER: fputs(c_token_name[pre_tkn->subtype], to); break;
        case TOKENTYPE_IDENT:
        case TOKENTYPE_GARBAGE: fwrite(pre_tkn->strval, 1, pre_tkn->strval_len, to); break;
        case TOKENTYPE_WHITESPACE:
            if (pre_tkn->subtype == C_LINE_COMMENT) {
                fputs("//", to);
            } else if (pre_tkn->subtype == C_BLOCK_COMMENT) {
                fputs("/*", to);
            }
            fwrite(pre_tkn->strval, 1, pre_tkn->strval_len, to);
            if (pre_tkn->subtype == C_BLOCK_COMMENT) {
                fputs("*/", to);
            }
            break;
        case TOKENTYPE_EOL: fputc('\n', to); break;
        case TOKENTYPE_EOF: break;
        default: abort(); // Not a valid preprocessor token.
    }
}

// Append the source representation of a token to a heap-allocated string.
// WARNING: Does not NUL-terminate!
void c_tkn_append_src(token_t const *pre_tkn, char **buf_ptr, size_t *len_ptr, size_t *cap_ptr) {
#define append_cstr(what) array_lencap_insert_n_strong(buf_ptr, 1, len_ptr, cap_ptr, (what), *len_ptr, strlen(what))
    switch (pre_tkn->type) {
        case TOKENTYPE_KEYWORD: append_cstr(c_keywords[pre_tkn->subtype]); break;
        case TOKENTYPE_SCONST:
            switch (pre_tkn->subtype) {
                case C_STR_ANGLEBRAC: append_cstr("<"); break;
                case C_STR_RAW_DQUOT: append_cstr("\""); break;
                case C_STR_RAW_SQUOT: append_cstr("\'"); break;
                default: abort(); // Not a valid preprocessor token.
            }
            array_lencap_insert_n_strong(buf_ptr, 1, len_ptr, cap_ptr, pre_tkn->strval, *len_ptr, pre_tkn->strval_len);
            switch (pre_tkn->subtype) {
                case C_STR_ANGLEBRAC: append_cstr(">"); break;
                case C_STR_RAW_DQUOT: append_cstr("\""); break;
                case C_STR_RAW_SQUOT: append_cstr("\'"); break;
                default: abort(); // Not a valid preprocessor token.
            }
            break;
        case TOKENTYPE_OTHER: append_cstr(c_token_name[pre_tkn->subtype]); break;
        case TOKENTYPE_IDENT:
        case TOKENTYPE_GARBAGE:
            array_lencap_insert_n_strong(buf_ptr, 1, len_ptr, cap_ptr, pre_tkn->strval, *len_ptr, pre_tkn->strval_len);
            break;
        case TOKENTYPE_WHITESPACE:
            if (pre_tkn->subtype == C_LINE_COMMENT) {
                append_cstr("//");
            } else if (pre_tkn->subtype == C_BLOCK_COMMENT) {
                append_cstr("/*");
            }
            array_lencap_insert_n_strong(buf_ptr, 1, len_ptr, cap_ptr, pre_tkn->strval, *len_ptr, pre_tkn->strval_len);
            if (pre_tkn->subtype == C_BLOCK_COMMENT) {
                append_cstr("*/");
            }
            break;
        case TOKENTYPE_EOL: append_cstr("\n"); break;
        case TOKENTYPE_EOF: break;
        default: abort(); // Not a valid preprocessor token.
    }
}
