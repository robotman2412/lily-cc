
#ifndef TOKENISER_H
#define TOKENISER_H

struct tokeniser_ctx;

typedef struct tokeniser_ctx tokeniser_ctx_t;

#include <parser.h>
#include <stdbool.h>

struct tokeniser_ctx {
	char *source;
	size_t source_len;
	size_t index;
	int x, y;
};

void tokeniser_init(tokeniser_ctx_t *ctx, char *file);
bool is_space(char c);
bool is_numeric(char c);
bool is_alphanumeric(char c);
char tokeniser_readchar(tokeniser_ctx_t *ctx);
char tokeniser_nextchar(tokeniser_ctx_t *ctx);
char tokeniser_nextchar_no(tokeniser_ctx_t *ctx, int no);
int tokenise(tokeniser_ctx_t *ctx);

#endif // TOKENISER_H
