
#include <asm.h>
#include <gen.h>
#include <pixie-16_gen.h>
#include <tokeniser.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include <pixie-16_iasm.h>

// All keywords that occur.
char *px_iasm_keyw[] = {
	// Instructions.
	"ADD", "SUB", "CMP", "AND",
	"OR",  "XOR", "SHL", "SHR",
	"MOV", "LEA",
	// Registers.
	"R0", "R1", "R2", "R3",
	"ST", "PF", "PC", "IMM",
	// Conditions.
	"ULT", "UGT", "SLT", "SGT",
	"EQ",  "NE",  "JSR"
};

#define PX_NUM_KEYW 34

bool px_is_label_char(char c) {
	switch (c) {
		case '0' ... '9':
		case 'a' ... 'z':
		case 'A' ... 'Z':
		case '_':
		case '.':
			return true;
		default:
			return false;
	}
}

// Tokenise for GR8CPU Rev3 assembly.
px_token_t px_iasm_lex(tokeniser_ctx_t *ctx) {
	// Get the first non-space character.
	char c;
	retry:
	do {
		c = tokeniser_readchar(ctx);
	} while(c == ' ' || c == '\t');
	if (!c) return (px_token_t) {.type = PX_TKN_END};
	
	// Check for end of line.
	if (c == '\r') {
		if (tokeniser_nextchar(ctx) == '\n') {
			tokeniser_readchar(ctx);
		}
	}
	if (c == '\r' || c == '\n') {
		DEBUG_TKN("eol\n");
		return (px_token_t) {.type = PX_TKN_END};
	}
	
	char next = tokeniser_nextchar(ctx);
	size_t index0 = ctx->index - 1;
	int x0 = ctx->x, y0 = ctx->y;
	
	// Check for comments and single-character tokens.
	switch (c) {
		case (','):
			DEBUG_TKN("token ','\n");
			return (px_token_t) {.type = PX_TKN_COMMA};
		case ('['):
			DEBUG_TKN("token '['\n");
			return (px_token_t) {.type = PX_TKN_LBRAC};
		case (']'):
			DEBUG_TKN("token ']'\n");
			return (px_token_t) {.type = PX_TKN_RBRAC};
		case ('('):
			DEBUG_TKN("token '('\n");
			return (px_token_t) {.type = PX_TKN_LPAR};
		case (')'):
			DEBUG_TKN("token ')'\n");
			return (px_token_t) {.type = PX_TKN_RPAR};
		case (';'):
			goto linecomment;
		case ('/'):
			if (next == '/') {
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
				return (px_token_t) {.type = PX_TKN_END};
			}
			break;
	}
	
	// This could be hexadecimal.
	if (c == '0' && (next == 'x' || next == 'X')) {
		int offs = 1;
		while (is_hexadecimal(tokeniser_nextchar_no(ctx, offs))) offs++;
		// Skip the x.
		tokeniser_readchar(ctx);
		// Now, grab it.
		char *strval = (char *) malloc(sizeof(char) * offs);
		strval[offs] = 0;
		for (int i = 0; i < offs-1; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Turn it into a number, hexadecimal.
		unsigned long long ival = strtoull(strval, NULL, 16);
		DEBUG_TKN("ival  %lld (0x%s)\n", ival, strval);
		free(strval);
		return (px_token_t) {
			.type = PX_TKN_IVAL,
			.ival = ival
		};
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
		// Turn it into a number, respecting octal.
		unsigned long long ival = strtoull(strval, NULL, c == '0' ? 8 : 10);
		DEBUG_TKN("ival  %lld (%s)\n", ival, strval);
		free(strval);
		return (px_token_t) {
			.type = PX_TKN_IVAL,
			.ival = ival
		};
	}
	
	// Or an ident or keyword.
	if (px_is_label_char(c)) {
		// Check how many of these we get.
		int offs = 0;
		while (px_is_label_char(tokeniser_nextchar_no(ctx, offs))) offs++;
		offs ++;
		// Now, grab it.
		char *strval = (char *) malloc(sizeof(char) * (offs + 1));
		*strval = c;
		strval[offs] = 0;
		for (int i = 1; i < offs; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Next, check for keywords.
		for (size_t i = 0; i < PX_NUM_KEYW; i++) {
			if (!strcasecmp(strval, px_iasm_keyw[i])) {
				DEBUG_TKN("keyw  '%s'\n", strval);
				free(strval);
				return (px_token_t) {
					.type  = (px_iasm_token_id_t) i,
					.ident = px_iasm_keyw[i]
				};
			}
		}
		DEBUG_TKN("ident '%s'\n", strval);
		// Return the appropriate alternative.
		return (px_token_t) {
			.type  = PX_TKN_IDENT,
			.ident = strval
		};
	}
	
	DEBUG_TKN("other '%c'\n", c);
	return (px_token_t) {
		.type  = PX_TKN_OTHER,
		.other = c
	};
}

#define TKN_EXPECT(lex_ctx, ex_type, error) { if (px_iasm_lex(lex_ctx).type != ex_type) { printf("Expected '%s'.\n", error); goto nope; } }
#define TKN_EXPECT_EOL(lex_ctx) {\
	px_token_t tkn = px_iasm_lex(lex_ctx);\
	if (tkn.type != PX_TKN_COMMA && tkn.type != PX_TKN_END) {\
		printf("Expected ','.\n"); goto nope;\
	}\
}

// Parse the instruction address specifier.
bool px_iasm_parse_addr(asm_ctx_t *ctx, tokeniser_ctx_t *lex_ctx, px_token_t *out) {
	px_token_t tkn = px_iasm_lex(lex_ctx);
	px_token_t addressed;
	
	
	TKN_EXPECT_EOL(lex_ctx);
	return true;
	
	nope:
	return false;
	
	check_addressed:
	*out = addressed;
	if (addressed.type == PX_TKN_IDENT || addressed.type == PX_TKN_IVAL) {
		return true;
	} else if (addressed.type < PX_NUM_KEYW) {
		printf("Expected LABEL, got '%s'.\n", px_iasm_keyw[(size_t) addressed.type]);
		return false;
	} else {
		printf("Expected LABEL.\n");
		return false;
	}
}

// Parse the instruction address list.
size_t px_iasm_parse_addrs(asm_ctx_t *ctx, tokeniser_ctx_t *lex_ctx, px_token_t **args) {
	px_token_t *list = NULL;
	size_t len = 0;
	px_token_t next;
	bool has_next;
	do {
		has_next = px_iasm_parse_addr(ctx, lex_ctx, &next);
		if (has_next) {
			len ++;
			list = (px_token_t *) realloc(list, sizeof(px_token_t) * len);
			list[len - 1] = next;
		}
	} while (has_next);
	*args = list;
	return len;
}

// Do a blob of assembly.
void gen_asm(asm_ctx_t *ctx, tokeniser_ctx_t *lex_ctx) {
	DEBUG_GEN("// inline assembly\n");
	
	// Instruction.
	px_token_t tkn = px_iasm_lex(lex_ctx);
	if (tkn.type == PX_TKN_IDENT) {
		// This is not an instruction.
		printf("No instruction with name '%s'.\n", tkn.ident);
		
	} else if (tkn.type < PX_TKN_INSN_KEYWORDS) {
		// This is an instruction keyword.
		px_token_t *args;
		// Parse the funny parameters.
		size_t n_args = px_iasm_parse_addrs(ctx, lex_ctx, &args);
		
		bool found_len = false;
		
		
		// No matching parameters for instruction.
		if (found_len) {
			printf("No '%s' found for given arguments (%zd).\n", px_iasm_keyw[(size_t) tkn.type], n_args);
		} else {
			printf("No '%s' found for %zd argument%s.\n", px_iasm_keyw[(size_t) tkn.type], n_args, n_args == 1 ? "" : "s");
		}
	} else if (tkn.type < PX_NUM_KEYW) {
		// This is a keyword, but not an instruction.
		printf("No instruction with name '%s'.\n", px_iasm_keyw[(size_t) tkn.type]);
		
	} else if (tkn.type == PX_TKN_END) {
		// End of line.
		return;
		
	} else {
		// Anything else is not allowed.
		printf("Expected INSTRUCTION or IDENT.\n");
		// TODO: Finalise the tokeniser context.
	}
}
