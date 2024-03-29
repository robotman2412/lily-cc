
#include "tokeniser.h"
#include "parser.h"
#include <stdlib.h>
#include <stdint.h>
#include "ctxalloc_warn.h"

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
	// print_pos(one);
	// printf(", ");
	// print_pos(two);
	// printf(" => ");
	// print_pos(out);
	// printf("\n");
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

void print_pos(tokeniser_ctx_t *ctx, pos_t pos) {
	// Print just the pos.
	printf("%s:%d:%d -> %d:%d\n", pos.filename, pos.y0, pos.x0, pos.y1, pos.x1);
}

// Initialise a context, given c-string.
void tokeniser_init_cstr(tokeniser_ctx_t *ctx, char *raw) {
	*ctx = (tokeniser_ctx_t) {
		.filename = "<anonymous>",
		.source_len = strlen(raw),
		.fd = NULL,
		.use_fd = false,
		.index = 0,
		.x = 0,
		.y = 1,
		.allocator = alloc_create(ALLOC_NO_PARENT),
	};
	ctx->source = xalloc(ctx->allocator, strlen(raw));
	strcpy(ctx->source, raw);
}

// Initialise a context, given a file descriptor.
void tokeniser_init_file(tokeniser_ctx_t *ctx, FILE *file) {
	*ctx = (tokeniser_ctx_t) {
		.filename = "<anonymous>",
		.source = NULL,
		.source_len = 0,
		.fd = file,
		.use_fd = true,
		.index = 0,
		.x = 0,
		.y = 1,
		.allocator = alloc_create(ALLOC_NO_PARENT),
	};
}

// Clean up a tokeniser context.
void tokeniser_destroy(tokeniser_ctx_t *ctx) {
	alloc_destroy(ctx->allocator);
}


// Is c a space character?
bool is_space(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Is c a numeric character?
bool is_numeric(char c) {
	return c >= '0' && c <= '9';
}

// Is c a hexadecimal character?
bool is_hexadecimal(char c) {
	switch (c) {
		case '0' ... '9':
		case 'a' ... 'f':
		case 'A' ... 'F':
			return true;
		default:
			return false;
	}
}

// Is c an alphanumberic character?
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


// Read a single character.
char tokeniser_readchar(tokeniser_ctx_t *ctx) {
	char c;
	if (ctx->use_fd) {
		size_t n = fread(&c, 1, 1, ctx->fd);
		if (!n) return 0;
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
		if (tokeniser_nextchar(ctx) == '\n') tokeniser_readchar(ctx);
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
		long pos = ftell(ctx->fd);
		fseek(ctx->fd, no, SEEK_CUR);
		char c;
		size_t n = fread(&c, 1, 1, ctx->fd);
		fseek(ctx->fd, pos, SEEK_SET);
		if (!n) return 0;
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

// Hexadecimal to binary.
static inline int8_t unhex_char(char c) {
	return	(c >= '0' && c <= '9') ? c - '0' :
			(c >= 'a' && c <= 'f') ? c - 'a' + 0x0A :
			(c >= 'A' && c <= 'F') ? c - 'A' + 0x0A :
			-1;
}

// Unescape an escaped c-string.
char *tokeniser_getstr(tokeniser_ctx_t *ctx, char term) {
	// Make the buf.
	size_t buf_len   = 256;
	size_t buf_index = 0;
	char *buf = xalloc(ctx->allocator, sizeof(char) * buf_len);
	*buf = 0;
	// Grab str.
	while (1) {
		char c = tokeniser_readchar(ctx);
		int  toappend;
		if (c == '\\') {
			char next = tokeniser_readchar(ctx);
			int  seq = -1;
			uint32_t unhex;
			switch (next) {
				// Some variables in this scope.
				int nhex, i;
				char next1, next2;
				
				// Escape the carriage return.
				case ('\r'):
					if (tokeniser_nextchar(ctx) == '\n') {
						// Consume the line feed, if any.
						tokeniser_readchar(ctx);
					}
					break;
				// Escape the line feed.
				case ('\n'): break;
				// Substitutions.
				case ('a'): seq = '\a'; break;
				case ('b'): seq = '\b'; break;
				case ('e'): seq = '\e'; break;
				case ('f'): seq = '\f'; break;
				case ('n'): seq = '\n'; break;
				case ('r'): seq = '\r'; break;
				case ('t'): seq = '\t'; break;
				case ('v'): seq = '\v'; break;
				case ('\\'):seq = '\\'; break;
				case ('\''):seq = '\''; break;
				case ('\"'):seq = '\"'; break;
				case ('?'): seq = '?'; break;
				// Octal.
				case '0'...'3':
					next1 = tokeniser_readchar(ctx);
					if (next1 < '0' || next1 > '7') {
						// TODO: Syntax error: not an octal.
						seq = -2;
					}
					next2 = tokeniser_readchar(ctx);
					if (next2 < '0' || next2 > '7') {
						// TODO: Syntax error: not an octal.
						seq = -2;
					}
					seq = ((next - '0') << 6) | ((next - '0') << 3) | (next - '0');
					break;
				case '4'...'9':
					// TODO: Syntax error: not an octal.
					seq = -2;
					break;
				// Hexadecimal and unicode.
				case ('x'):
					nhex = 2;
					goto unhexing;
				case ('u'):
					nhex = 4;
					goto unhexing;
				case ('U'):
					nhex = 8;
					unhexing:
					unhex = 0;
					for (i = 1; nhex--; i++) {
						next = tokeniser_nextchar_no(ctx, i);
						int8_t bit = unhex_char(next);
						if (bit == -1) {
						// TODO: Syntax error: not a hexadecimal.
							seq = -3;
							break;
						}
						unhex = (unhex << 4) | bit;
					}
					// Consume hex chars.
					while (i--) tokeniser_readchar(ctx);
					// TODO: Map code to characterset.
					seq = -10;
					break;
			}
			if (seq >= 0) {
				toappend = seq;
				goto appendit;
			} else if (seq == -10) {
				toappend = unhex;
				goto appendit;
			} else {
				//TODO: Report error.
			}
		} else if (c == term) {
			break;
		} else {
			toappend = c;
		}
		
		appendit:
		{
			// Append the funy.
			if (buf_index >= buf_len) {
				buf_len *= 2;
				buf = xrealloc(ctx->allocator, buf, buf_len);
			}
			// TODO: Charset encode.
			buf[buf_index] = toappend;
			buf_index ++;
			buf[buf_index] = 0;
		}
	}
	// Shrik to fit.
	buf_len = strlen(buf) + 1;
	buf = xrealloc(ctx->allocator, buf, buf_len);
	return buf;
}

typedef struct {
	int keyw;
	char *str;
} keyw_map_t;

static const keyw_map_t keyw_map[] = {
	// (keyw_map_t) { .keyw=TKN_, .str="" },
	(keyw_map_t) { .keyw=TKN_VOID,     .str="void" },
	(keyw_map_t) { .keyw=TKN_SHORT,    .str="short" },
	(keyw_map_t) { .keyw=TKN_INT,      .str="int" },
	(keyw_map_t) { .keyw=TKN_LONG,     .str="long" },
	(keyw_map_t) { .keyw=TKN_CHAR,     .str="char" },
	(keyw_map_t) { .keyw=TKN_FLOAT,    .str="float" },
	(keyw_map_t) { .keyw=TKN_DOUBLE,   .str="double" },
	(keyw_map_t) { .keyw=TKN_BOOL,     .str="bool" },
	(keyw_map_t) { .keyw=TKN_SIGNED,   .str="signed" },
	(keyw_map_t) { .keyw=TKN_UNSIGNED, .str="unsigned" },
	(keyw_map_t) { .keyw=TKN_IF,       .str="if" },
	(keyw_map_t) { .keyw=TKN_ELSE,     .str="else" },
	(keyw_map_t) { .keyw=TKN_WHILE,    .str="while" },
	(keyw_map_t) { .keyw=TKN_FOR,      .str="for" },
	(keyw_map_t) { .keyw=TKN_RETURN,   .str="return" },
	(keyw_map_t) { .keyw=TKN_ASM,      .str="asm" },
	(keyw_map_t) { .keyw=TKN_GOTO,     .str="goto" },
	(keyw_map_t) { .keyw=TKN_VOLATILE, .str="volatile" },
	(keyw_map_t) { .keyw=TKN_INLINE,   .str="inline" },
};
static const size_t keyw_map_len = sizeof(keyw_map) / sizeof(keyw_map_t);

// The error type returned by tokenise_int, if any.
static error_type_t tkn_int_err_type;
// The error message returned by tokenise_int, if any.
static char        *tkn_int_err_msg;
// Whether or not to free tkn_int_err_msg.
static bool         tkn_int_err_do_free;

// Grab next non-space token (internal method).
static int tokenise_int(tokeniser_ctx_t *ctx, int *i0, int *x0, int *y0) {
	// Get the first non-space character.
	char c;
	retry:
	do {
		c = tokeniser_readchar(ctx);
	} while(is_space(c));
	if (!c) return 0;
	*i0 = ctx->index;
	*x0 = ctx->x;
	*y0 = ctx->y;
    
	char next = tokeniser_nextchar(ctx);
	char next2 = tokeniser_nextchar_no(ctx, 1);
	size_t index0 = ctx->index - 1;
    
	// Do stuff based on the character.
	enum yytokentype ret = 0;
	switch (c) {
		case ('+'):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_ASSIGN_ADD;
			} else if (next == '+') {
				tokeniser_readchar(ctx);
				ret = TKN_INC;
			} else {
				ret = TKN_ADD;
			}
			break;
		case ('-'):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_ASSIGN_SUB;
			} else if (next == '-') {
				tokeniser_readchar(ctx);
				ret = TKN_DEC;
			} else {
				ret = TKN_SUB;
			}
			break;
		case ('('):
			ret = TKN_LPAR;
			break;
		case (')'):
			ret = TKN_RPAR;
			break;
		case (':'):
			ret = TKN_COLON;
			break;
		case ('{'):
			ret = TKN_LBRAC;
			break;
		case ('}'):
			ret = TKN_RBRAC;
			break;
		case ('['):
			ret = TKN_LSBRAC;
			break;
		case (']'):
			ret = TKN_RSBRAC;
			break;
		case ('*'):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_ASSIGN_MUL;
			} else {
				ret = TKN_MUL;
			}
			break;
		case ('&'):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_ASSIGN_AND;
			} else if (next == '&') {
				tokeniser_readchar(ctx);
				ret = TKN_LOGIC_AND;
			} else {
				ret = TKN_AMP;
			}
			break;
		case ('#'):
			goto linecomment;
		case ('/'):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_ASSIGN_DIV;
			} else if (next == '/') {
				// This starts a line commment.
				linecomment:
				for (long i = 1; c != '\r' && c != '\n'; i++) {
					c = tokeniser_readchar(ctx);
					char next = tokeniser_nextchar(ctx);
					if (c == '\\' && (next == '\r' || next == '\n')) {
						// Backslash extends even comments.
						tokeniser_readchar(ctx);
					}
				}
				goto retry;
			} else if (next == '*') {
				// This starts a block commment.
				for (long i = 1; c != 0; i++) {
					char q = tokeniser_readchar(ctx);
					char next = tokeniser_nextchar(ctx);
					if (c == '*' && q == '/') {
						// End the block comment.
						goto retry;
					}
					c = q;
				}
				// TODO: Error: unclosed block comment at end of file.
				ret = 0;
			} else {
				ret = TKN_DIV;
			}
			break;
		case ('%'):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_ASSIGN_REM;
			} else {
				ret = TKN_REM;
			}
			break;
		case ('='):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_EQ;
			} else {
				ret = TKN_ASSIGN;
			}
			break;
		case ('!'):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_NE;
			} else {
				ret = TKN_NOT;
			}
			break;
		case ('<'):
			if (next == '<') {
				tokeniser_readchar(ctx);
				if (next2 == '=') {
					tokeniser_readchar(ctx);
					ret = TKN_ASSIGN_SHL;
				} else {
					ret = TKN_SHL;
				}
			} else if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_LE;
			} else {
				ret = TKN_LT;
			}
			break;
		case ('>'):
			if (next == '>') {
				tokeniser_readchar(ctx);
				if (next2 == '=') {
					tokeniser_readchar(ctx);
					ret = TKN_ASSIGN_SHR;
				} else {
					ret = TKN_SHR;
				}
			} else if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_GE;
			} else {
				ret = TKN_GT;
			}
			break;
		case ('^'):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_ASSIGN_XOR;
			} else {
				ret = TKN_XOR;
			}
			break;
		case ('~'):
			ret = TKN_INV;
			break;
		case ('|'):
			if (next == '=') {
				tokeniser_readchar(ctx);
				ret = TKN_ASSIGN_OR;
			} else if (next == '|') {
				tokeniser_readchar(ctx);
				ret = TKN_LOGIC_OR;
			} else {
				ret = TKN_OR;
			}
			break;
		case (','):
			ret = TKN_COMMA;
			break;
		case (';'):
			ret = TKN_SEMI;
			break;
	}
	if (ret) {
#ifdef DEBUG_TOKENISER
		size_t num = ctx->index - index0;
		if (num == 3) {
			DEBUG_TKN("token '%c%c%c'\n", c, next, next2);
		} else if (num == 2) {
			DEBUG_TKN("token '%c%c'\n", c, next);
		} else {
			DEBUG_TKN("token '%c'\n", c);
		}
#endif
		return ret;
	}
    
	// This could be hexadecimal.
	if (c == '0' && (next == 'x' || next == 'X')) {
		int offs = 1;
		while (is_hexadecimal(tokeniser_nextchar_no(ctx, offs))) offs++;
		// Skip the x.
		tokeniser_readchar(ctx);
		// Now, grab it.
		char *strval = (char *) xalloc(ctx->allocator, sizeof(char) * offs);
		strval[offs-1] = 0;
		for (int i = 0; i < offs-1; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Turn it into a number, hexadecimal.
		int ival = strtoull(strval, NULL, 16);
		DEBUG_TKN("ival  %d (0x%s)\n", ival, strval);
		xfree(ctx->allocator, strval);
		yylval.ival.ival = ival;
		return TKN_IVAL;
	}
	
	// This could be a number.
	if (is_numeric(c)) {
		// Check how many of these we get.
		int offs = 0;
		while (is_alphanumeric(tokeniser_nextchar_no(ctx, offs))) offs++;
		offs ++;
		// Now, grab it.
		char *strval = (char *) xalloc(ctx->allocator, sizeof(char) * (offs + 1));
		*strval = c;
		strval[offs] = 0;
		for (int i = 1; i < offs; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Turn it into a number, respecting octal.
		int ival = strtoull(strval, NULL, c == '0' ? 8 : 10);
		DEBUG_TKN("ival  %d (%s)\n", ival, strval);
		xfree(ctx->allocator, strval);
		yylval.ival.ival = ival;
		return TKN_IVAL;
	}
    
	// Or an ident or keyword.
	if (is_alphanumeric(c)) {
		// Check how many of these we get.
		int offs = 0;
		while (is_alphanumeric(tokeniser_nextchar_no(ctx, offs))) offs++;
		offs ++;
		// Now, grab it.
		char *strval = (char *) xalloc(ctx->allocator, sizeof(char) * (offs + 1));
		*strval = c;
		strval[offs] = 0;
		for (int i = 1; i < offs; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Next, check for keywords.
		int keyw = 0;
		for (int i = 0; i < keyw_map_len; i++) {
			if (!strcmp(strval, keyw_map[i].str)) {
				keyw = keyw_map[i].keyw;
				break;
			}
		}
		// Return the appropriate alternative.
		if (keyw) {
			DEBUG_TKN("token '%s'\n", strval);
			xfree(ctx->allocator, strval);
			return keyw;
		}
		DEBUG_TKN("ident '%s'\n", strval);
		yylval.ident.strval = strval;
		return TKN_IDENT;
	}
	
	// Or a string value.
	if (c == '"') {
		char *strval = tokeniser_getstr(ctx, '"');
		yylval.strval.strval = strval;
		DEBUG_TKN("str   \"%s\"\n", strval);
		return TKN_STRVAL;
	}
	
	// Or a character value.
	if (c == '\'') {
		char *strval = tokeniser_getstr(ctx, '\'');
		int ival = 0;
		
		if (strlen(strval) > 1) {
			// Warn if the constant is too long.
			tkn_int_err_msg     = "Multi-character character constant.";
			tkn_int_err_type    = E_WARN;
			tkn_int_err_do_free = false;
		} else if (!*strval) {
			// Error if the constant is empty.
			tkn_int_err_msg     = "Empty character constant.";
			tkn_int_err_type    = E_ERROR;
			tkn_int_err_do_free = false;
		}
		
		// Turn into an int.
		while (*strval) {
			ival = (ival << 8) | (unsigned char) *strval;
			strval ++;
		}
		yylval.ival.ival = ival;
		DEBUG_TKN("char  '%c'\n", ival);
		return TKN_IVAL;
	}
	
	// Otherwise, this is garbage.
	// TODO: Better solution.
	char *garbagestr = xalloc(ctx->allocator, 2);
	garbagestr[0] = c;
	garbagestr[1] = 0;
	DEBUG_TKN("???   '%c'\n", c);
	tkn_int_err_msg     = "Unrecognised token.";
	tkn_int_err_type    = E_ERROR;
	tkn_int_err_do_free = false;
	return TKN_GARBAGE;
}

// Grab next non-space token.
int tokenise(tokeniser_ctx_t *ctx) {
	// Clear error.
	tkn_int_err_msg = NULL;
	
	// Pre-token position.
	int i0, x0, y0;
	// Get token data.
	int tkn_id = tokenise_int(ctx, &i0, &x0, &y0);
	if (!tkn_id) return 0;
	// Post-token position.
	int i1 = ctx->index;
	int x1 = ctx->x;
	int y1 = ctx->y;
	
	// Return token after setting pos.
	yylval.pos = (pos_t) {
		.filename = ctx->filename,
		.index0   = i0,
		.index1   = i1+1,
		.x0       = x0,
		.y0       = y0,
		.x1       = x1+1,
		.y1       = y1
	};
	if (tkn_int_err_msg) {
		// Report error messages.
		report_error(ctx, tkn_int_err_type, yylval.pos, tkn_int_err_msg);
		// Free memory if required.
		if (tkn_int_err_do_free) free(tkn_int_err_msg);
	}
	return tkn_id;
}

static void print_src(tokeniser_ctx_t *ctx, FILE *outfile, int line, int x0, int x1, char *col, int *outX0, int *outX1) {
	int dummy;
	if (!outX0) outX0 = &dummy;
	if (!outX1) outX1 = &dummy;
	
	int tab_size = 4;
	
	if (ctx->use_fd) {
		// File descriptor source.
		// Save the stream position.
		long pos = ftell(ctx->fd);
		fseek(ctx->fd, 0, SEEK_SET);
		
		// Find the line.
		line --;
		while (line && !feof(ctx->fd)) {
			char c = fgetc(ctx->fd);
			if (c == '\r') {
				char next = fgetc(ctx->fd);
				if (next != '\n') fseek(ctx->fd, -1, SEEK_CUR);
				line --;
			} else if (c == '\n') {
				line --;
			}
		}
		
		// Find the line's length.
		long line_start = ftell(ctx->fd);
		while (!feof(ctx->fd)) {
			char c = fgetc(ctx->fd);
			if (c == '\r' || c == '\n') {
				fseek(ctx->fd, -1, SEEK_CUR);
				break;
			}
		}
		long line_end = ftell(ctx->fd);
		long line_len = line_end - line_start;
		
		// Print the line.
		fseek(ctx->fd, line_start, SEEK_SET);
		int printed_x = 0;
		for (long i = 0; i < line_len; i++) {
			char c = fgetc(ctx->fd);
			if (i == x0 - 1) {
				fputs(col, outfile);
				*outX0 = printed_x + 1;
			}
			if (c == '\t') {
				int error = printed_x % tab_size;
				while (error < tab_size) {
					fputc(' ', outfile);
					error ++;
					printed_x ++;
				}
			} else if (c < 0x20 || c >= 0x7f) {
				fputc(' ', outfile);
				printed_x ++;
			} else {
				fputc(c, outfile);
				printed_x ++;
			}
			if (i == x1 - 2) {
				fputs("\033[0m", outfile);
				*outX1 = printed_x + 1;
			}
		}
		
		// Reset the stream position.
		fseek(ctx->fd, pos, SEEK_SET);
	} else {
		// C-string source.
		// Find the line.
		char *index = ctx->source;
		line --;
		for (size_t i = 0; i < ctx->source_len && line; i++) {
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
		
		// Find line's length.
		char *a = strchr(index, '\r');
		char *b = strchr(index, '\n');
		if (b < a || !a && b) {
			a = b;
		} else if (!a) {
			a = index + strlen(index);
		}
		
		// Print the line.
		int printed_x = 0;
		for (long i = 0; i < a - index; i++) {
			char c = index[i];
			if (i == x0 - 1) {
				fputs(col, outfile);
				*outX0 = printed_x + 1;
			}
			if (c < 0x20 || c >= 0x7f) {
				fputc(' ', outfile);
				printed_x ++;
			} else if (c == '\t') {
				int error = printed_x % tab_size;
				while (error < tab_size) {
					fputc(' ', outfile);
					error ++;
					printed_x ++;
				}
			} else {
				fputc(c, outfile);
				printed_x ++;
			}
			if (i == x1 - 2) {
				fputs("\033[0m", outfile);
				*outX1 = printed_x + 1;
			}
		}
	}
	fputs("\033[0m\n", outfile);
}

static void print_pos_range(int x0, int x1) {
	for (int i = 1; i < x0; i++) {
		fputc(' ', stderr);
	}
	fputc('^', stderr);
	for (int i = x0 + 1; i < x1; i++) {
		fputc('~', stderr);
	}
	fputc('\n', stderr);
}
 
void report_error(tokeniser_ctx_t *tokeniser_ctx, error_type_t e_type, pos_t pos, char *message) {
	char *col  = "\033[91m";
	char *type = "error";
	
	switch (e_type) {
		case E_ERROR:
		default:
			break;
		case E_SYNTAX:
			type = "syntax error";
			break;
		case E_WARN:
			type = "warning";
			col  = "\033[93m";
			break;
		case E_NOTE:
			type = "note";
			col  = "\033[96m";
			break;
	}
	
	fflush(stdout);
	fprintf(stderr, "in %s:%d:%d %s%s:\033[0m %s\n", pos.filename, pos.y0, pos.x0, col, type, message);
	fprintf(stderr, "%5d | ", pos.y0);
	int offset0 = pos.x0, offset1 = pos.x1;
	print_src(tokeniser_ctx, stderr, pos.y0, pos.x0, pos.x1, col, &offset0, &offset1);
	fprintf(stderr, "      | ");
	fputs(col, stderr);
	print_pos_range(offset0, offset1);
	fputs("\033[0m", stderr);
	fflush(stderr);
}

// Prints a numbered line of the source code.
void print_line(tokeniser_ctx_t *ctx, int line) {
	print_src(ctx, stdout, line, 0, 65535, "", NULL, NULL);
}
