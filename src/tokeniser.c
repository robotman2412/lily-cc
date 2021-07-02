
#include "tokeniser.h"
#include <stdlib.h>
#include <config.h>

pos_t pos_merge(pos_t one, pos_t two) {
	if (one.index0 > two.index0) {
		pos_t temp = one;
		one = two;
		two = temp;
	}
	pos_t out = {
		.filename = one.filename,
		.x0 = one.x0,
		.y0 = one.y0,
		.x1 = two.x1,
		.y1 = two.y1,
		.index0 = one.index0,
		.index1 = two.index1
	};
	return out;
}

pos_t pos_empty(tokeniser_ctx_t *ctx) {
	return (pos_t) {
		.filename = ctx->filename,
		.x0 = ctx->x,
		.y0 = ctx->y,
		.x1 = ctx->x,
		.y1 = ctx->y,
		.index0 = ctx->index,
		.index1 = ctx->index
	};
}

void tokeniser_init_cstr(tokeniser_ctx_t *ctx, char *raw) {
	*ctx = (tokeniser_ctx_t) {
		.filename = "<anonymous>",
		.source = raw,
		.source_len = strlen(raw),
		.fd = NULL,
		.use_fd = false,
		.index = 0,
		.x = 0,
		.y = 0
	};
}

void tokeniser_init_file(tokeniser_ctx_t *ctx, FILE *file) {
	*ctx = (tokeniser_ctx_t) {
		.filename = "<anonymous>",
		.source = NULL,
		.source_len = 0,
		.fd = file,
		.use_fd = true,
		.index = 0,
		.x = 0,
		.y = 0
	};
}

bool is_space(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

bool is_numeric(char c) {
	return c >= '0' && c <= '9';
}

bool is_alphanumeric(char c) {
	switch (c) {
		case '0' ... '9':
		case 'a' ... 'z':
		case 'A' ... 'Z':
		case '_':
			return true;
		default:
			return false;
	}
}

char tokeniser_readchar(tokeniser_ctx_t *ctx) {
	char c;
	if (ctx->use_fd) {
		if (feof(ctx->fd)) {
			return 0;
		}
		fread(&c, 1, 1, ctx->fd);
	} else {
		if (ctx->index >= ctx->source_len) {
			return 0;
		}
		c = ctx->source[ctx->index];
	}
	ctx->index ++;
	ctx->x ++;
	if (c == '\r') {
		c = '\n';
		ctx->index += tokeniser_nextchar(ctx) == '\n';
	}
	if (c == '\n') {
		ctx->y ++;
		ctx->x = 0;
	}
	int x, y;
	return c;
}

// Identical to tokeniser_nextchar_no(0).
char tokeniser_nextchar(tokeniser_ctx_t *ctx) {
	return tokeniser_nextchar_no(ctx, 0);
}

// Next character + offset.
char tokeniser_nextchar_no(tokeniser_ctx_t *ctx, int no) {
	if (ctx->use_fd) {
		size_t pos = ftell(ctx->fd);
		int success = fseek(ctx->fd, pos + no, SEEK_SET);
		if (!success) {
			fseek(ctx->fd, pos, SEEK_SET);
			return 0;
		}
		char c;
		fread(&c, 1, 1, ctx->fd);
		fseek(ctx->fd, pos, SEEK_SET);
		return c;
	} else {
		if (ctx->index + no >= ctx->source_len) {
			return 0;
		}
		char c = ctx->source[ctx->index + no];
		if (c == '\r') c = '\n';
		return c;
	}
}

int tokenise(tokeniser_ctx_t *ctx) {
	// Get the first non-space character.
	char c;
	do {
		c = tokeniser_readchar(ctx);
	} while(is_space(c));
	char next = tokeniser_nextchar(ctx);
	// Do stuff based on the character.
	switch (c) {
		case ('+'):
			if (next == '+') {
				tokeniser_readchar(ctx);
				DEBUG("INC\n");
				return TKN_INC;
			} else {
				DEBUG("ADD\n");
				return TKN_ADD;
			}
		case ('-'):
			if (next == '-') {
				tokeniser_readchar(ctx);
				DEBUG("DEC\n");
				return TKN_DEC;
			} else {
				DEBUG("SUB\n");
				return TKN_SUB;
			}
		case ('('):
			DEBUG("LPAR\n");
			return TKN_LPAR;
		case (')'):
			DEBUG("RPAR\n");
			return TKN_RPAR;
		case ('{'):
			DEBUG("LBRAC\n");
			return TKN_LBRAC;
		case ('}'):
			DEBUG("RBRAC\n");
			return TKN_RBRAC;
		case ('*'):
			DEBUG("MUL\n");
			return TKN_MUL;
		case ('/'):
			DEBUG("DIV\n");
			return TKN_DIV;
		case ('%'):
			DEBUG("REM\n");
			return TKN_REM;
		case ('='):
			DEBUG("ASSIGN\n");
			return TKN_ASSIGN;
		case (','):
			DEBUG("COMMA\n");
			return TKN_COMMA;
		case (';'):
			DEBUG("SEMI\n");
			return TKN_SEMI;
	}
	// This could be a number.
	if (is_numeric(c)) {
		// Check how many of these we get.
		int offs = 0;
		while (is_alphanumeric(tokeniser_nextchar_no(ctx, offs))) offs++;
		offs ++;
		// Now, grab it.
		char *strval = (char *) malloc(sizeof(char) * (offs + 1));
		*strval = c;
		strval[offs] = 0;
		for (int i = 1; i < offs; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Turn it into a number.
		// TODO: Verify that the constant is in range.
		int ival = atoi(strval);
		free(strval);
		yylval.ival = (ival_t) {
			//.pos = pos,
			.ival = ival
		};
		DEBUG("NUM(%d)\n", ival);
		return TKN_NUM;
	}
	// Or an ident or keyword.
	if (is_alphanumeric(c)) {
		// Check how many of these we get.
		int offs = 0;
		while (is_alphanumeric(tokeniser_nextchar_no(ctx, offs))) offs++;
		offs ++;
		// Now, grab it.
		char *strval = (char *) malloc(sizeof(char) * (offs + 1));
		*strval = c;
		strval[offs] = 0;
		for (int i = 1; i < offs; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Next, check for keywords.
		int keyw = 0;
		if (!strcmp(strval, "if")) {
			DEBUG("IF\n");
			keyw = TKN_IF;
		} else if (!strcmp(strval, "else")) {
			DEBUG("ELSE\n");
			keyw = TKN_ELSE;
		} else if (!strcmp(strval, "while")) {
			DEBUG("WHILE\n");
			keyw = TKN_WHILE;
		} else if (!strcmp(strval, "for")) {
			DEBUG("FOR\n");
			keyw = TKN_FOR;
		} else if (!strcmp(strval, "return")) {
			DEBUG("RETURN\n");
			keyw = TKN_RETURN;
		} else if (!strcmp(strval, "char")) {
			DEBUG("CHAR\n");
			keyw = TKN_CHAR;
		} else if (!strcmp(strval, "int")) {
			DEBUG("INT\n");
			keyw = TKN_INT;
		} else if (!strcmp(strval, "short")) {
			DEBUG("SHORT\n");
			keyw = TKN_SHORT;
		} else if (!strcmp(strval, "long")) {
			DEBUG("LONG\n");
			keyw = TKN_LONG;
		} else if (!strcmp(strval, "float")) {
			DEBUG("FLOAT\n");
			keyw = TKN_FLOAT;
		} else if (!strcmp(strval, "double")) {
			DEBUG("LONG\n");
			keyw = TKN_LONG;
		} else if (!strcmp(strval, "void")) {
			DEBUG("VOID\n");
			keyw = TKN_VOID;
		}
		// Return the appropriate alternative.
		if (keyw) {
			free(strval);
			return keyw;
		}
		yylval.ident = (ident_t) {
			//.pos = pos,
			.ident = strval
		};
		DEBUG("IDENT(%s)\n", strval);
		return TKN_IDENT;
	}
	// Otherwise, this is garbage.
	// TODO: Better solution.
	printf("LOL_IS_NOT_OK_XDE\n");
	return 0;
}
