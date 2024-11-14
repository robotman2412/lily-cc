
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "frontend.h"

#include "arrays.h"

#include <string.h>



// Get position from start to end (exclusive).
pos_t pos_between(pos_t start, pos_t end) {
    start.len = end.off - start.off;
    return start;
}


// Create new frontend context.
front_ctx_t *front_create() {
    return calloc(1, sizeof(front_ctx_t));
}

// Delete frontend context (and therefor all frondend resources).
void front_delete(front_ctx_t *ctx) {
    // TODO.
}


// Open or get a source file from frontend context.
srcfile_t *srcfile_open(front_ctx_t *ctx, char const *path) {
    srcfile_t *file = calloc(1, sizeof(srcfile_t));
    if (!file)
        goto err0;

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
srcfile_t *srcfile_create(front_ctx_t *ctx, char const *virt_path, void const *data, size_t len) {
    if (len > __INT_MAX__)
        return NULL;
    srcfile_t *file = calloc(1, sizeof(srcfile_t));
    if (!file)
        goto err0;

    file->path = strdup(virt_path);
    if (!file->path)
        goto err1;

    file->is_ram_file = true;
    file->content     = malloc(len);
    file->content_len = len;
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

// Read a character from a source file and update position.
static int srcfile_getc_raw(srcfile_t *file, int *off) {
    if (file->is_ram_file) {
        if (*off >= file->content_len) {
            return -1;
        }
        char val = file->content[*off];
        (*off)++;
        return val;
    } else {
        if (file->fd_off != *off) {
            fseek(file->fd, *off, SEEK_SET);
        }
        int val = fgetc(file->fd);
        if (val != -1)
            (*off)++;
        return val;
    }
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
