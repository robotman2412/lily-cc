
#include "token/c_tokenizer.h"

#include <arrays.h>
#include <stdlib.h>
#include <string.h>



// List of keywords.
char const *c_keywords[] = {
#define C_KEYW_DEF(name) #name,
#include "c_keywords.inc"
};


// Comparator function for searching for keywords.
static int keyw_comp(void const *_a, void const *_b) {
    char const *a = *(char const *const *)_a;
    char const *b = _b;
    return strcmp(a, b);
}

// Helper function to create tokens for better readability.
token_t other_tkn(c_tokentype_t type, pos_t from, pos_t to) {
    return ((token_t){
        .pos     = pos_between(from, to),
        .type    = TOKENTYPE_OTHER,
        .subtype = type,
    });
}

// Test whether a character is legal as the first in a C identifier.
bool c_is_first_sym_char(int c) {
    if (c == '_' || c == '$') {
        return true;
    }
    c |= 0x20;
    return c >= 'a' && c <= 'z';
}

// Test whether a character is legal in a C identifier.
bool c_is_sym_char(int c) {
    if (c == '_' || c == '$') {
        return true;
    } else if (c >= '0' && c <= '9') {
        return true;
    }
    c |= 0x20;
    return c >= 'a' && c <= 'z';
}


// Tokenize numeric constant.
static token_t c_tkn_numeric(tokenizer_t *ctx, pos_t start_pos, int base) {
    uint64_t val      = 0;
    bool     hasdat   = false;
    bool     toolarge = false;
    bool     invalid  = false;

    pos_t pos0 = ctx->pos;
    pos_t pos1;
    while (1) {
        int digit;
        pos1  = pos0;
        int c = srcfile_getc(ctx->file, &pos1);
        if (c >= '0' && c <= '9') {
            // Valid digit 0-9.
            digit = c - '0';
        } else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'z') {
            // Valid digit a-z / A-Z.
            digit = (c | 0x20) - 'a' + 10;
        } else if (c == '_') {
            // Invalid character.
            invalid = true;
        } else {
            // End of constant.
            break;
        }
        if (digit >= base) {
            invalid = true;
        }
        if (val * base < val) {
            toolarge = true;
        }
        val    *= base;
        val    += digit;
        hasdat  = true;
        pos0    = pos1;
    }
    ctx->pos = pos0;

    if (invalid || !hasdat) {
        // TODO: Report error (invalid constant).
        return (token_t){
            .type = TOKENTYPE_GARBAGE,
            .pos  = pos_between(start_pos, pos0),
        };
    } else if (toolarge) {
        // TODO: Report warning (constant too large).
    }
    return (token_t){
        .type = TOKENTYPE_ICONST,
        .pos  = pos_between(start_pos, pos0),
        .ival = val,
    };
}

// Tokenize identifier.
static token_t c_tkn_ident(tokenizer_t *ctx, pos_t start_pos, char first) {
    size_t cap = 32;
    size_t len = 1;
    char  *ptr = malloc(cap);
    ptr[0]     = first;
    // TODO: Assert on ptr.

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
            ptr  = realloc(ptr, cap);
            // TODO: Assert on ptr.
        }
        ptr[len++] = c;
        pos0       = pos1;
    }
    ptr[len] = 0;
    ctx->pos = pos0;

    // Test for keywords.
    array_binsearch_t res = array_binsearch(c_keywords, sizeof(char *), C_N_KEYWS, ptr, keyw_comp);
    if (res.found) {
        return (token_t){
            .pos     = pos_between(start_pos, pos0),
            .type    = TOKENTYPE_KEYWORD,
            .subtype = res.index,
        };
    }

    return (token_t){
        .pos    = pos_between(start_pos, pos0),
        .type   = TOKENTYPE_IDENT,
        .strval = rc_new_strong(ptr, free),
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
                // TODO: Report error.
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
    int   value = first - '0';
    pos_t pos0  = ctx->pos;
    pos_t pos1;
    for (int i = 0; i < max_w; i++) {
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
    char  *ptr = malloc(cap);
    // TODO: Assert on ptr.

    while (1) {
        pos_t start_pos = ctx->pos;
        int   c         = srcfile_getc(ctx->file, &ctx->pos);
        if (c == -1) {
            // TODO: Report EOF error.
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
                c = c_str_hex(ctx, start_pos, 4, 4);
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
                    default: /* TODO: report error */ break;
                }
            }
        }
        if (!array_lencap_insert(&ptr, 1, &len, &cap, &c, len)) {
            // TODO: Out of memory, abort.
        }
    }

    if (is_char) {
        uint64_t val = 0;
        for (size_t i = 0; i < len; i++) {
            val <<= 8;
            val  |= ptr[i];
        }
        free(ptr);
        return (token_t){
            .pos  = pos_between(start_pos, ctx->pos),
            .type = TOKENTYPE_CCONST,
            .ival = val,
        };
    } else {
        return (token_t){
            .pos    = pos_between(start_pos, ctx->pos),
            .type   = TOKENTYPE_SCONST,
            .strval = rc_new_strong(ptr, free),
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
retry:
    pos_t pos0 = ctx->pos;
    int   c    = srcfile_getc(ctx->file, &ctx->pos);
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
            return c_tkn_numeric(ctx, pos0, 16);
        } else if (c2 == 'b' || c2 == 'B') {
            // GNU extension: Binary.
            ctx->pos = pos2;
            return c_tkn_numeric(ctx, pos0, 2);
        } else if (c2 >= '0' && c2 <= '9') {
            // Octal.
            return c_tkn_numeric(ctx, pos0, 8);
        } else {
            // Just a zero.
            return (token_t){
                .type = TOKENTYPE_ICONST,
                .pos  = pos_between(pos0, pos1),
                .ival = 0,
            };
        }
    }

    // Decimal constants.
    if (c >= '1' && c <= '9') {
        ctx->pos = pos0;
        return c_tkn_numeric(ctx, pos0, 10);
    }

    // Identifiers.
    if (c_is_sym_char(c)) {
        return c_tkn_ident(ctx, pos0, c);
    }

    // Single-character tokens.
    if (c == '(') {
        return other_tkn(C_TKN_LPAR, pos0, pos1);
    } else if (c == ')') {
        return other_tkn(C_TKN_RPAR, pos0, pos1);
    } else if (c == '.') {
        return other_tkn(C_TKN_DOT, pos0, pos1);
    } else if (c == ':') {
        return other_tkn(C_TKN_COLON, pos0, pos1);
    } else if (c == ';') {
        return other_tkn(C_TKN_SEMIC, pos0, pos1);
    } else if (c == '?') {
        return other_tkn(C_TKN_QUESTION, pos0, pos1);
    } else if (c == '[') {
        return other_tkn(C_TKN_LBRAC, pos0, pos1);
    } else if (c == ']') {
        return other_tkn(C_TKN_RBRAC, pos0, pos1);
    } else if (c == '{') {
        return other_tkn(C_TKN_LCURL, pos0, pos1);
    } else if (c == '}') {
        return other_tkn(C_TKN_RCURL, pos0, pos1);
    } else if (c == '~') {
        return other_tkn(C_TKN_NOT, pos0, pos1);
    }

    // Possibly multi-character tokens.
    pos_t pos2 = ctx->pos;
    int   c2   = srcfile_getc(ctx->file, &pos2);
    if (c == '!') {
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
        .pos  = pos2,
        .type = TOKENTYPE_GARBAGE,
    };
#undef pos1
}
