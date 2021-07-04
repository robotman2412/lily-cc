
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
void print_pos(pos_t pos);

#include <parser.h>

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

void tokeniser_init_cstr(tokeniser_ctx_t *ctx, char *raw);
void tokeniser_init_file(tokeniser_ctx_t *ctx, FILE *file);
bool is_space(char c);
bool is_numeric(char c);
bool is_alphanumeric(char c);
char tokeniser_readchar(tokeniser_ctx_t *ctx);
char tokeniser_nextchar(tokeniser_ctx_t *ctx);
char tokeniser_nextchar_no(tokeniser_ctx_t *ctx, int no);
int tokenise(tokeniser_ctx_t *ctx);

void syntax_error(parser_ctx_t *parser_ctx, pos_t pos, char *message);

#endif // TOKENISER_H
