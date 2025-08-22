
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "compiler.h"

#include "arrays.h"
#include "color.h"
#include "utf8.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>



#ifndef NDEBUG
// Enum names of `tokentype_t` values.
char const *const tokentype_names[] = {
    "TOKENTYPE_KEYWORD",
    "TOKENTYPE_IDENT",
    "TOKENTYPE_ICONST",
    "TOKENTYPE_CCONST",
    "TOKENTYPE_SCONST",
    "TOKENTYPE_OTHER",
    "TOKENTYPE_GARBAGE",
    "TOKENTYPE_EOL",
    "TOKENTYPE_EOF",
    "TOKENTYPE_AST",
};
#endif

// Get position from start to end (exclusive).
pos_t pos_between(pos_t start, pos_t end) {
    start.len = end.off - start.off;
    return start;
}

// Get position from start to end (inclusive).
pos_t pos_including(pos_t start, pos_t end) {
    start.len = end.off - start.off + end.len;
    return start;
}

// Create new compiler context.
cctx_t *cctx_create() {
    return calloc(1, sizeof(cctx_t));
}

// Close a source file.
static void srcfile_close(srcfile_t *file) {
    if (file->is_ram_file) {
        free(file->content);
    } else {
        fclose(file->fd);
    }
    free(file->path);
    free(file);
}

// Delete compiler context (and therefor all compiler resources).
void cctx_delete(cctx_t *ctx) {
    for (size_t i = 0; i < ctx->srcs_len; i++) {
        srcfile_close(ctx->srcs[i]);
    }
    free(ctx->srcs);

    while (ctx->diagnostics.len) {
        diagnostic_t *diag = (diagnostic_t *)dlist_pop_front(&ctx->diagnostics);
        free(diag->msg);
        free(diag);
    }

    free(ctx);
}

// Create a formatted diagnostic message.
// Returns diagnostic created, or NULL on failure.
diagnostic_t *cctx_diagnostic(cctx_t *ctx, pos_t pos, diag_lvl_t lvl, char const *fmt, ...) {
    // Measure how much space is needed for the message.
    va_list va;
    va_start(va, fmt);
    size_t len = vsnprintf(NULL, 0, fmt, va);
    va_end(va);

    // Format the message.
    size_t cap = len + 1;
    char  *buf = calloc(cap, 1);
    if (!buf)
        return NULL;
    va_start(va, fmt);
    vsnprintf(buf, cap, fmt, va);
    va_end(va);

    // Allocate a diagnostic and add it to the list.
    diagnostic_t *diag = malloc(sizeof(diagnostic_t));
    if (!diag) {
        free(buf);
        return NULL;
    }
    diag->node = DLIST_NODE_EMPTY;
    diag->pos  = pos;
    diag->lvl  = lvl;
    diag->msg  = buf;
    dlist_append(&ctx->diagnostics, &diag->node);
    return diag;
}

// Helper function that determines what

// Print a diagnostic.
void print_diagnostic(diagnostic_t const *diag, FILE *to) {
    char const *const color[] = {
        [DIAG_ERR]  = ANSI_RED_FG,
        [DIAG_WARN] = ANSI_YELLOW_FG,
        [DIAG_INFO] = ANSI_BLUE_FG,
        [DIAG_HINT] = ANSI_MAGENTA_FG,
    };
    char const *const prefix[] = {
        [DIAG_ERR]  = "error",
        [DIAG_WARN] = "warning",
        [DIAG_INFO] = "info",
        [DIAG_HINT] = "hint",
    };
    color_fputs(ANSI_BLUE_FG ANSI_BOLD, to);
    if (!diag->pos.srcfile) {
        fprintf(to, "???:?:?: ");
    } else {
        fprintf(to, "%s:%d:%d: ", diag->pos.srcfile->path, diag->pos.line + 1, diag->pos.col + 1);
    }
    color_fputs(color[diag->lvl], to);
    fprintf(to, "%s: ", prefix[diag->lvl]);
    color_fputs(ANSI_DEFAULT, to);
    fprintf(to, "%s\n", diag->msg);

    if (!diag->pos.srcfile) {
        // Unknown position; can only report message.
        return;
    }

    // Find line starting point (or at most 80 characters before `diag->pos`).
    pos_t pos = diag->pos;
    for (int i = 0; i < 80; i++) {
        if (!srcfile_seekprev(diag->pos.srcfile, &pos)) {
            break;
        }
    }

    // Print all the characters, splitting at each line ending and drawing a squiggly line underneath.
    // Extend until the next end-of-line (or at most 80 characters past `diag->pos`).
    int  min_chars  = diag->pos.col - pos.col + diag->pos.len;
    int  max_chars  = min_chars + 80;
    int  off        = pos.off;
    int  line       = pos.line + 1;
    int  line_start = off;
    bool draw       = false;
    fprintf(to, "%5d | ", line);
    for (int x = 0; x < max_chars; x++) {
        // The bit that prints the line.
        if (off == diag->pos.off + diag->pos.len) {
            color_fputs(ANSI_DEFAULT, to);
        } else if (off == diag->pos.off) {
            color_fputs(ANSI_BOLD ANSI_RED_FG, to);
        }
        int c = srcfile_getc_raw(pos.srcfile, &off);
        if (c < 0) {
            break;
        }
        char buf[4];
        fwrite(buf, 1, utf8_encode(buf, 4, c), to);

        // The bit that prints the arrow.
        if (c == '\n' || x >= max_chars) {
            if (draw) {
                color_fputs(ANSI_DEFAULT, to);
            }
            fputs("      | ", to);
            if (draw) {
                color_fputs(ANSI_BOLD ANSI_RED_FG, to);
            }
            for (int y = line_start; y < off; y++) {
                if (y == diag->pos.off + diag->pos.len) {
                    color_fputs(ANSI_DEFAULT, to);
                    fputc(' ', to);
                    draw = false;
                } else if (y == diag->pos.off) {
                    color_fputs(ANSI_BOLD ANSI_RED_FG, to);
                    fputc('^', to);
                    draw = true;
                } else {
                    fputc(draw ? '~' : ' ', to);
                }
            }
            if (draw) {
                color_fputs(ANSI_DEFAULT, to);
            }
            fputc('\n', to);

            line_start = off;
            if (x >= min_chars) {
                break;
            }

            line++;
            fprintf(to, "%5d | ", line);
            if (draw) {
                color_fputs(ANSI_BOLD ANSI_RED_FG, to);
            }
        }
    }
}


// Open or get a source file from compiler context.
srcfile_t *srcfile_open(cctx_t *ctx, char const *path) {
    srcfile_t *file = calloc(1, sizeof(srcfile_t));
    if (!file)
        goto err0;
    file->ctx         = ctx;
    file->is_ram_file = false;

    file->path = strdup(path);
    if (!file->path)
        goto err1;

    file->fd = fopen(path, "rb");
    if (!file->fd)
        goto err2;

    if (!array_lencap_insert(&ctx->srcs, sizeof(void *), &ctx->srcs_len, &ctx->srcs_cap, &file, ctx->srcs_len))
        goto err3;

    return file;

err3:
    fclose(file->fd);
err2:
    free(file->path);
err1:
    free(file);
err0:
    return NULL;
}

// Create a source file from binary data.
srcfile_t *srcfile_create(cctx_t *ctx, char const *virt_path, void const *data, size_t len) {
    if (len > __INT_MAX__)
        return NULL;
    srcfile_t *file = calloc(1, sizeof(srcfile_t));
    if (!file)
        goto err0;
    file->ctx         = ctx;
    file->is_ram_file = true;

    file->path = strdup(virt_path);
    if (!file->path)
        goto err1;

    file->is_ram_file = true;
    file->content     = strong_malloc(len);
    file->content_len = len;
    file->ctx         = ctx;
    if (!file->content)
        goto err2;
    memcpy(file->content, data, len);

    if (!array_lencap_insert(&ctx->srcs, sizeof(void *), &ctx->srcs_len, &ctx->srcs_cap, &file, ctx->srcs_len))
        goto err3;

    return file;

err3:
    free(file->content);
err2:
    free(file->path);
err1:
    free(file);
err0:
    return NULL;
}

// Read a raw byte from a source file.
// Returns -1 on EOF.
static int srcfile_read_raw_byte(srcfile_t *file, int off) {
    if (file->is_ram_file) {
        if (off >= file->content_len) {
            return -1;
        }
        return file->content[off];
    } else {
        if (file->fd_off != off) {
            fseek(file->fd, off, SEEK_SET);
            file->fd_off = off;
        }
        int val = fgetc(file->fd);
        if (val != -1) {
            file->fd_off = off + 1;
        }
        return val;
    }
}

// Read a character from a source file and update offset.
int srcfile_getc_raw(srcfile_t *file, int *off) {
    // Read first UTF-8 byte.
    int val  = 0;
    int head = srcfile_read_raw_byte(file, *off);
    if (head < 0) {
        return -1;
    }
    ++*off;

    // Determine number of UTF-8 bytes after this one to read.
    size_t size;
    if ((head & 0xf4) == 0xf0) {
        size = 3;
        val  = head & 0x07;
    } else if ((head & 0xf0) == 0xe0) {
        size = 2;
        val  = head & 0x0f;
    } else if ((head & 0xe0) == 0xc0) {
        size = 1;
        val  = head & 0x1f;
    } else if ((head & 0x80) == 0x00) {
        size = 0;
        val  = head;
    } else {
        size = 0;
        val  = 0xfffd;
    }

    // Try to read this amount of remaining bytes.
    for (; size; --size) {
        int data = srcfile_read_raw_byte(file, *off);
        if (data < 0 || (data & 0xc0) != 0x80) {
            val = 0xfffd;
            break;
        }
        ++*off;
        val = (val << 6) | (data & 0x3f);
    }

    return val;
}

// Read a character from a source file and update position.
int srcfile_getc(srcfile_t *file, pos_t *pos) {
    int c = srcfile_getc_raw(file, &pos->off);
    if (c == '\r') {
        int tmp = pos->off;
        c       = srcfile_getc_raw(file, &tmp);
        if (c == '\n') {
            pos->off = tmp;
        }
        pos->line++;
        pos->col = 0;
        return '\n';
    } else if (c == '\n') {
        pos->line++;
        pos->col = 0;
        return '\n';
    } else {
        pos->col++;
        return c;
    }
}

// Try to seek to the previous character in a source file.
static bool srcfile_seekprev_raw(srcfile_t *file, int *off) {
    if (file->is_ram_file) {
        if (*off == 0) {
            return false;
        }
        (*off)--;
        while (*off > 0 && ((unsigned char)file->content[*off] & 0xC0) == 0x80) {
            (*off)--;
        }
        return true;
    } else {
        if (*off == 0) {
            return false;
        }
        (*off)--;
        int c;
        do {
            fseek(file->fd, *off, SEEK_SET);
            c = fgetc(file->fd);
            if (*off == 0 || ((unsigned char)c & 0xC0) != 0x80) {
                break;
            }
            (*off)--;
        } while (*off > 0);
        return true;
    }
}

// Try to seek to the previous character in a source file.
// Will stop at the beginnings of lines.
bool srcfile_seekprev(srcfile_t *file, pos_t *pos) {
    int tmp_off = pos->off;
    if (!srcfile_seekprev_raw(file, &tmp_off)) {
        return false;
    }

    int tmp_off_2 = tmp_off;
    int c         = srcfile_getc_raw(file, &tmp_off_2);
    if (c == '\n') {
        return false;
    }

    pos->col--;
    pos->off = tmp_off;
    return true;
}


// Create an empty AST token with a position.
token_t ast_empty(int subtype, pos_t pos) {
    return (token_t){
        .pos        = pos,
        .type       = TOKENTYPE_AST,
        .subtype    = subtype,
        .strval     = NULL,
        .ival       = 0,
        .params_len = 0,
        .params     = NULL,
    };
}

// Create an AST token with a fixed number of param tokens.
token_t ast_from_va(int subtype, size_t n_param, ...) {
    token_t *params = strong_malloc(sizeof(token_t) * n_param);
    va_list  va;
    va_start(va, n_param);
    for (size_t i = 0; i < n_param; i++) {
        params[i] = va_arg(va, token_t);
    }
    va_end(va);
    return ast_from(subtype, n_param, params);
}

// Create an AST token with a fixed number of param tokens.
token_t ast_from(int subtype, size_t n_param, token_t *params) {
    pos_t min_pos = {0};
    pos_t max_pos = {0};
    for (size_t i = 0; i < n_param; i++) {
        if (!min_pos.srcfile || params[i].pos.off < min_pos.off) {
            min_pos = params[i].pos;
        }
        if (!max_pos.srcfile || params[i].pos.off + params[i].pos.len > max_pos.off + max_pos.len) {
            max_pos = params[i].pos;
        }
    }
    return (token_t){
        .pos        = pos_including(min_pos, max_pos),
        .type       = TOKENTYPE_AST,
        .subtype    = subtype,
        .strval     = NULL,
        .ival       = 0,
        .params_len = n_param,
        .params     = params,
    };
}


// Append one parameter to an AST node.
void ast_append_param(token_t *ast, token_t param) {
    array_len_insert_strong(&ast->params, sizeof(token_t), &ast->params_len, &param, ast->params_len);
}

// Append one or more parameters to an AST node.
void ast_append_params_va(token_t *ast, size_t n_param, ...) {
    size_t offset = ast->params_len;
    array_len_resize_strong(&ast->params, sizeof(token_t), &ast->params_len, ast->params_len + n_param);
    va_list va;
    va_start(va, n_param);
    for (size_t i = 0; i < n_param; i++) {
        ast->params[offset + i] = va_arg(va, token_t);
    }
    va_end(va);
}

// Append one or more parameters to an AST node.
void ast_append_params(token_t *ast, size_t n_param, token_t *params) {
    array_len_insert_n_strong(&ast->params, sizeof(token_t), &ast->params_len, params, ast->params_len, n_param);
}
