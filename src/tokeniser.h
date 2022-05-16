
#ifndef TOKENISER_H
#define TOKENISER_H

struct tokeniser_ctx;
struct pos;

typedef struct tokeniser_ctx tokeniser_ctx_t;
typedef struct pos pos_t;

typedef enum {
	E_ERROR,
	E_SYNTAX,
	E_WARN,
	E_NOTE,
} error_type_t;

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ctxalloc.h"

// Contins position information for tokens.
// Both 'filename' and 'y' can be changed by #line statements.
struct pos {
	char *filename;
	int x0, y0;
	int x1, y1;
	size_t index0, index1;
};

// Contains info required to tokenise a source file.
struct tokeniser_ctx {
	// Filename.
	char      *filename;
	// For raw string inputs.
	char       *source;
	size_t      source_len;
	// For file descriptor inputs.
	FILE       *fd;
	bool        use_fd;
	// Current position.
	size_t      index;
	int         x, y;
	// Allocation context to use for e.g. strings.
	alloc_ctx_t allocator;
};

#include <parser-util.h>

pos_t pos_merge(pos_t one, pos_t two);
pos_t pos_empty(tokeniser_ctx_t *ctx);
// void  print_pos(tokeniser_ctx_t *ctx, pos_t pos);
void  report_error(tokeniser_ctx_t *ctx, error_type_t type, pos_t pos, char *message);

#define report_errorf(ctx, type, pos, ...) do{ \
		size_t len = snprintf(NULL, 0, __VA_ARGS__); \
		char  *buf = xalloc(global_alloc, len); \
		sprintf(buf, __VA_ARGS__); \
		report_error(ctx, type, pos, buf); \
		xfree(global_alloc, buf); \
	} while(0)

// Initialise a context, given c-string.
void tokeniser_init_cstr(tokeniser_ctx_t *ctx, char *raw);
// Initialise a context, given a file descriptor.
void tokeniser_init_file(tokeniser_ctx_t *ctx, FILE *file);
// Clean up a tokeniser context.
void tokeniser_destroy(tokeniser_ctx_t *ctx);

// Is c a space character?
bool is_space(char c);
// Is c a numeric character?
bool is_numeric(char c);
// Is c an alphanumberic character?
bool is_alphanumeric(char c);
// Is c an hexadecimal character?
bool is_hexadecimal(char c);

// Read a single character.
char tokeniser_readchar(tokeniser_ctx_t *ctx);
// Next character.
// Identical to tokeniser_nextchar_no(0).
char tokeniser_nextchar(tokeniser_ctx_t *ctx);
// Next character + offset.
char tokeniser_nextchar_no(tokeniser_ctx_t *ctx, int no);

// Unescape an escaped c-string.
char *tokeniser_getstr(tokeniser_ctx_t *ctx, char term);

// Grab next non-space token.
int tokenise(tokeniser_ctx_t *ctx);

#endif // TOKENISER_H
