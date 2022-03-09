
#ifndef TOKENISER_H
#define TOKENISER_H

struct tokeniser_ctx;
struct pos;

typedef struct tokeniser_ctx tokeniser_ctx_t;
typedef struct pos pos_t;

#include <stdio.h>
#include <stdbool.h>

// Contins position information for tokens.
// Both 'filename' and 'y' can be changed by #line statements.
struct pos {
	char *filename;
	int x0, y0;
	int x1, y1;
	size_t index0, index1;
};

pos_t pos_merge(pos_t one, pos_t two);
pos_t pos_empty(tokeniser_ctx_t *ctx);
void  print_pos(pos_t pos);

// Contains info required to tokenise a source file.
struct tokeniser_ctx {
	// Filename.
	char *filename;
	// For raw string inputs.
	char *source;
	size_t source_len;
	// For file descriptor inputs.
	FILE *fd;
	bool use_fd;
	// Current position.
	size_t index;
	int x, y;
};

#include <parser-util.h>

// Initialise a context, given c-string.
void tokeniser_init_cstr(tokeniser_ctx_t *ctx, char *raw);
// Initialise a context, given a file descriptor.
void tokeniser_init_file(tokeniser_ctx_t *ctx, FILE *file);

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

void report_error(parser_ctx_t *parser_ctx, char *type, pos_t pos, char *message);

#endif // TOKENISER_H
