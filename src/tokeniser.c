
#include "tokeniser.h"
#include <stdlib.h>
#include <config.h>

void tokeniser_init(tokeniser_ctx_t *ctx, char *file) {
	*ctx = (tokeniser_ctx_t) {
		.index = 0,
		.source = file,
		.source_len = strlen(file),
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
	if (ctx->index >= ctx->source_len) {
		return 0;
	}
	char c = ctx->source[ctx->index];
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
	if (ctx->index + no >= ctx->source_len) {
		return 0;
	}
	char c = ctx->source[ctx->index + no];
	if (c == '\r') c = '\n';
	return c;
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
				return INC;
			} else {
				DEBUG("ADD\n");
				return ADD;
			}
		case ('-'):
			if (next == '-') {
				tokeniser_readchar(ctx);
				DEBUG("DEC\n");
				return DEC;
			} else {
				DEBUG("SUB\n");
				return SUB;
			}
		case ('('):
			DEBUG("LPAR\n");
			return LPAR;
		case (')'):
			DEBUG("RPAR\n");
			return RPAR;
		case ('{'):
			DEBUG("LBRAC\n");
			return LBRAC;
		case ('}'):
			DEBUG("RBRAC\n");
			return RBRAC;
		case ('*'):
			DEBUG("MUL\n");
			return MUL;
		case ('/'):
			DEBUG("DIV\n");
			return DIV;
		case ('%'):
			DEBUG("REM\n");
			return REM;
		case ('='):
			DEBUG("ASSIGN\n");
			return ASSIGN;
		case (','):
			DEBUG("COMMA\n");
			return COMMA;
		case (';'):
			DEBUG("SEMI\n");
			return SEMI;
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
		yylval.ival = ival;
		DEBUG("NUM(%d)\n", ival);
		return NUM;
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
		if (!strcmp(strval, "var")) {
			DEBUG("VAR\n");
			keyw = VAR;
		} else if (!strcmp(strval, "if")) {
			DEBUG("IF\n");
			keyw = IF;
		} else if (!strcmp(strval, "else")) {
			DEBUG("ELSE\n");
			keyw = ELSE;
		} else if (!strcmp(strval, "while")) {
			DEBUG("WHILE\n");
			keyw = WHILE;
		} else if (!strcmp(strval, "func")) {
			DEBUG("FUNC\n");
			keyw = FUNC;
		} else if (!strcmp(strval, "return")) {
			DEBUG("RETURN\n");
			keyw = RETURN;
		}
		// Return the appropriate alternative.
		if (keyw) {
			free(strval);
			return keyw;
		}
		yylval.ident = strval;
		DEBUG("IDENT(%s)\n", strval);
		return IDENT;
	}
	// Otherwise, this is garbage.
	// TODO: Better solution.
	printf("LOL_IS_NOT_OK_XDE\n");
	return 0;
}
