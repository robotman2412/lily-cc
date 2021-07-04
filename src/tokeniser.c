
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
	print_pos(one);
	printf(", ");
	print_pos(two);
	printf(" => ");
	print_pos(out);
	printf("\n");
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

void print_pos(pos_t pos) {
	printf("%s:%d:%d -> %d:%d", pos.filename, pos.y0, pos.x0, pos.y1, pos.x1);
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
		.y = 1
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
		.y = 1
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
		if (tokeniser_nextchar(ctx)) tokeniser_nextchar(ctx);
	}
	if (c == '\n') {
		ctx->y ++;
		ctx->x = 0;
	}
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
	if (!c) return c;
	char next = tokeniser_nextchar(ctx);
	size_t index0 = ctx->index - 1;
	int x0 = ctx->x, y0 = ctx->y;
	// Do stuff based on the character.
	enum yytokentype ret = 0;
	switch (c) {
		case ('+'):
			if (next == '+') {
				tokeniser_readchar(ctx);
				DEBUG("INC @ ");
				ret = TKN_INC;
			} else {
				DEBUG("ADD @ ");
				ret = TKN_ADD;
			}
			break;
		case ('-'):
			if (next == '-') {
				tokeniser_readchar(ctx);
				DEBUG("DEC @ ");
				ret = TKN_DEC;
			} else {
				DEBUG("SUB @ ");
				ret = TKN_SUB;
			}
			break;
		case ('('):
			DEBUG("LPAR @ ");
			ret = TKN_LPAR;
			break;
		case (')'):
			DEBUG("RPAR @ ");
			ret = TKN_RPAR;
			break;
		case ('{'):
			DEBUG("LBRAC @ ");
			ret = TKN_LBRAC;
			break;
		case ('}'):
			DEBUG("RBRAC @ ");
			ret = TKN_RBRAC;
			break;
		case ('*'):
			DEBUG("MUL @ ");
			ret = TKN_MUL;
			break;
		case ('/'):
			DEBUG("DIV @ ");
			ret = TKN_DIV;
			break;
		case ('%'):
			DEBUG("REM @ ");
			ret = TKN_REM;
			break;
		case ('='):
			DEBUG("ASSIGN @ ");
			ret = TKN_ASSIGN;
			break;
		case (','):
			DEBUG("COMMA @ ");
			ret = TKN_COMMA;
			break;
		case (';'):
			DEBUG("SEMI @ ");
			ret = TKN_SEMI;
			break;
	}
	if (ret) {
		yylval.pos = (pos_t) {
			.filename = ctx->filename,
			.x0 = x0,
			.y0 = y0,
			.x1 = ctx->x,
			.y1 = ctx->y,
			.index0 = index0,
			.index1 = ctx->index - 1
		};
#ifdef ENABLE_DEBUG_LOGS
		print_pos(yylval.pos);
		printf("\n");
#endif
		return ret;
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
			.pos = {
				.filename = ctx->filename,
				.x0 = x0,
				.y0 = y0,
				.x1 = ctx->x,
				.y1 = ctx->y,
				.index0 = index0,
				.index1 = ctx->index - 1
			},
			.ival = ival
		};
#ifdef ENABLE_DEBUG_LOGS
		DEBUG("NUM(%d) @ ", ival);
		print_pos(yylval.ival.pos);
		printf("\n");
#endif
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
			DEBUG("IF @ ");
			keyw = TKN_IF;
		} else if (!strcmp(strval, "else")) {
			DEBUG("ELSE @ ");
			keyw = TKN_ELSE;
		} else if (!strcmp(strval, "while")) {
			DEBUG("WHILE @ ");
			keyw = TKN_WHILE;
		} else if (!strcmp(strval, "for")) {
			DEBUG("FOR @ ");
			keyw = TKN_FOR;
		} else if (!strcmp(strval, "return")) {
			DEBUG("RETURN @ ");
			keyw = TKN_RETURN;
		} else if (!strcmp(strval, "char")) {
			DEBUG("CHAR @ ");
			keyw = TKN_CHAR;
		} else if (!strcmp(strval, "int")) {
			DEBUG("INT @ ");
			keyw = TKN_INT;
		} else if (!strcmp(strval, "short")) {
			DEBUG("SHORT @ ");
			keyw = TKN_SHORT;
		} else if (!strcmp(strval, "long")) {
			DEBUG("LONG @ ");
			keyw = TKN_LONG;
		} else if (!strcmp(strval, "float")) {
			DEBUG("FLOAT @ ");
			keyw = TKN_FLOAT;
		} else if (!strcmp(strval, "double")) {
			DEBUG("LONG @ ");
			keyw = TKN_LONG;
		} else if (!strcmp(strval, "void")) {
			DEBUG("VOID @ ");
			keyw = TKN_VOID;
		}
		// Return the appropriate alternative.
		if (keyw) {
			yylval.pos = (pos_t) {
				.filename = ctx->filename,
				.x0 = x0,
				.y0 = y0,
				.x1 = ctx->x,
				.y1 = ctx->y,
				.index0 = index0,
				.index1 = ctx->index - 1
			};
#ifdef ENABLE_DEBUG_LOGS
			print_pos(yylval.pos);
			printf("\n");
#endif
			free(strval);
			return keyw;
		}
		yylval.ident = (ident_t) {
			.pos = {
				.filename = ctx->filename,
				.x0 = x0,
				.y0 = y0,
				.x1 = ctx->x,
				.y1 = ctx->y,
				.index0 = index0,
				.index1 = ctx->index - 1
			},
			.ident = strval
		};
#ifdef ENABLE_DEBUG_LOGS
		DEBUG("IDENT(%s) @ ", strval);
		print_pos(yylval.ident.pos);
		printf("\n");
#endif
		return TKN_IDENT;
	}
	// Otherwise, this is garbage.
	// TODO: Better solution.
	char *garbagestr = malloc(2);
	garbagestr[0] = c;
	garbagestr[1] = 0;
	yylval.garbage = (ident_t) {
		.pos = {
			.filename = ctx->filename,
			.x0 = ctx->x,
			.y0 = ctx->y,
			.x1 = ctx->x,
			.y1 = ctx->y,
			.index0 = index0,
			.index1 = index0
		},
		.ident = garbagestr
	};
#ifdef ENABLE_DEBUG_LOGS
	DEBUG("GARBAGE(%c) @ ", c);
	print_pos(yylval.ident.pos);
	printf("\n");
#endif
	return TKN_GARBAGE;
}

static void print_src(tokeniser_ctx_t *ctx, int line) {
	if (ctx->use_fd) {
		printf("TODO\n");
	} else {
		char *index = ctx->source;
		line --;
		for (int i = 0; i < ctx->source_len && line; i++) {
			char *ptr = &ctx->source[i];
			if (*ptr == '\r') {
				if (ptr[1] == '\n') index = ptr + 2;
				else index = ptr + 1;
				line --;
			} else if (*ptr == '\n') {
				index = ptr + 1;
				line --;
			}
		}
		char *a = strchr(index, '\r');
		char *b = strchr(index, '\n');
		if (b < a || !a && b) {
			a = b;
		} else if (!a) {
			a = index + strlen(index);
		}
		fwrite(index, 1, a - index, stdout);
		fputc('\n', stdout);
	}
}

static void print_pos_range(int x0, int x1) {
	for (int i = 1; i < x0; i++) {
		fputc(' ', stdout);
	}
	fputc('^', stdout);
	for (int i = x0 + 1; i < x1; i++) {
		fputc('~', stdout);
	}
	fputc('\n', stdout);
}

void syntax_error(parser_ctx_t *parser_ctx, pos_t pos, char *message) {
	printf("Syntax error in %s:%d:%d -> %d:%d: %s\n", pos.filename, pos.y0, pos.x0, pos.y1, pos.x1, message);
	printf("%5d | ", pos.y0);
	print_src(parser_ctx->tokeniser_ctx, pos.y0);
	printf("      | ");
	print_pos_range(pos.x0, pos.x1);
}
