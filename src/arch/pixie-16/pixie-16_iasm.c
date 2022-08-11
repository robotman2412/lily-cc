
#include <asm.h>
#include <gen.h>
#include <pixie-16_gen.h>
#include <tokeniser.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include "ctxalloc_warn.h"

#include <pixie-16_iasm.h>
#include "pixie-16_internal.h"

// Defined in pixie-16_gen.c
void px_write_insn(asm_ctx_t *ctx, px_insn_t insn, asm_label_t label0, address_t offs0, asm_label_t label1, address_t offs1);

// All keywords that occur.
char *px_iasm_keyw[] = {
	// Math: two args.
	"ADD",  "SUB",  "CMP",   "AND",  "OR",   "XOR",  "",     "",
	// Mathc: two args.
	"ADDC", "SUBC", "CMPC",  "ANDC", "ORC",  "XORC", "",     "",
	// Math: one arg.
	"INC",  "DEC",  "CMP1",  "",     "",     "",     "SHL",  "SHR",
	// Mathc: one arg.
	"INCC", "DECC", "CMP1C", "",     "",     "",     "SHLC", "SHRC",
	// Move instructions.
	"MOV.ULT", "MOV.UGT", "MOV.SLT", "MOV.SGT", "MOV.EQ",  "MOV.CS",  "MOV",     "",
	"MOV.UGE", "MOV.ULE", "MOV.SGE", "MOV.SLE", "MOV.NE",  "MOV.CC",  "MOV.JSR", "MOV.CX",
	// Load effective address instructions.
	"LEA.ULT", "LEA.UGT", "LEA.SLT", "LEA.SGT", "LEA.EQ",  "LEA.CS",  "LEA",     "",
	"LEA.UGE", "LEA.ULE", "LEA.SGE", "LEA.SLE", "LEA.NE",  "LEA.CC",  "LEA.JSR", "",
	// Registers.
	"R0", "R1", "R2", "R3",
	"ST", "PF", "PC", "IMM",
};

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
	if (!c) return (px_token_t) {.type = PX_TKN_END, .ident=NULL, .ival=0};
	
	// Check for end of line.
	if (c == '\r') {
		if (tokeniser_nextchar(ctx) == '\n') {
			tokeniser_readchar(ctx);
		}
	}
	if (c == '\r' || c == '\n') {
		DEBUG_TKN("eol\n");
		return (px_token_t) {.type = PX_TKN_END, .ident=NULL, .ival=0};
	}
	
	char next = tokeniser_nextchar(ctx);
	size_t index0 = ctx->index - 1;
	int x0 = ctx->x, y0 = ctx->y;
	
	// Check for comments and single-character tokens.
	switch (c) {
		case (','):
			DEBUG_TKN("token ','\n");
			return (px_token_t) {.type = PX_TKN_COMMA, .ident=NULL, .ival=0};
		case ('['):
			DEBUG_TKN("token '['\n");
			return (px_token_t) {.type = PX_TKN_LBRAC, .ident=NULL, .ival=0};
		case (']'):
			DEBUG_TKN("token ']'\n");
			return (px_token_t) {.type = PX_TKN_RBRAC, .ident=NULL, .ival=0};
		case ('('):
			DEBUG_TKN("token '('\n");
			return (px_token_t) {.type = PX_TKN_LPAR, .ident=NULL, .ival=0};
		case (')'):
			DEBUG_TKN("token ')'\n");
			return (px_token_t) {.type = PX_TKN_RPAR, .ident=NULL, .ival=0};
		case ('+'):
			DEBUG_TKN("token '+'\n");
			return (px_token_t) {.type = PX_TKN_PLUS, .ident=NULL, .ival=0};
		case ('~'):
			DEBUG_TKN("token '~'\n");
			return (px_token_t) {.type = PX_TKN_TILDE, .ident=NULL, .ival=0};
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
				return (px_token_t) {.type = PX_TKN_END, .ident=NULL, .ival=0};
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
		char *strval = (char *) xalloc(ctx->allocator, sizeof(char) * offs);
		strval[offs-1] = 0;
		for (int i = 0; i < offs-1; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Turn it into a number, hexadecimal.
		unsigned long long ival = strtoull(strval, NULL, 16);
		DEBUG_TKN("ival  %lld (0x%s)\n", ival, strval);
		xfree(ctx->allocator, strval);
		return (px_token_t) {
			.type  = PX_TKN_IVAL,
			.ival  = ival,
			.ident = NULL
		};
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
		unsigned long long ival = strtoull(strval, NULL, c == '0' ? 8 : 10);
		DEBUG_TKN("ival  %lld (%s)\n", ival, strval);
		xfree(ctx->allocator, strval);
		return (px_token_t) {
			.type  = PX_TKN_IVAL,
			.ival  = ival,
			.ident = NULL
		};
	}
	
	// Or an ident or keyword.
	if (px_is_label_char(c)) {
		// Check how many of these we get.
		int offs = 0;
		while (px_is_label_char(tokeniser_nextchar_no(ctx, offs))) offs++;
		offs ++;
		// Now, grab it.
		char *strval = (char *) xalloc(ctx->allocator, sizeof(char) * (offs + 1));
		*strval = c;
		strval[offs] = 0;
		for (int i = 1; i < offs; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Next, check for keywords.
		for (size_t i = 0; i < PX_NUM_KEYW; i++) {
			if (!strcasecmp(strval, px_iasm_keyw[i])) {
				DEBUG_TKN("keyw  '%s'\n", strval);
				xfree(ctx->allocator, strval);
				return (px_token_t) {
					.type  = (px_iasm_token_id_t) i,
					.ident = NULL,
					.ival  = 0
				};
			}
		}
		DEBUG_TKN("ident '%s'\n", strval);
		// Return the appropriate alternative.
		return (px_token_t) {
			.type  = PX_TKN_IDENT,
			.ident = strval,
			.ival  = 0
		};
	}
	
	DEBUG_TKN("other '%c'\n", c);
	return (px_token_t) {
		.type  = PX_TKN_OTHER,
		.other = c,
		.ident = NULL,
		.ival  = 0
	};
}

#define PX_ERROR_L(lex_ctx, pos, error) report_error(lex_ctx, E_ERROR, pos_merge(pos, pos_empty(lex_ctx)), error)
#define PX_ERROR_P(lex_ctx, pos, error) report_error(lex_ctx, E_ERROR, pos, error)
#define PX_ERROR(lex_ctx, error) PX_ERROR_P(lex_ctx, pos_empty(lex_ctx), error)
#define TKN_EXPECT(lex_ctx, ex_type, error) { if (px_iasm_lex(lex_ctx).type != ex_type) { PX_ERROR(lex_ctx, "Expected '%s'.\n", error); goto nope; } }
#define TKN_EXPECT_EOL(lex_ctx) {\
	tkn = px_iasm_lex(lex_ctx);\
	if (tkn.type != PX_TKN_COMMA && tkn.type != PX_TKN_END) {\
		PX_ERROR(lex_ctx, "Expected ','.\n"); goto nope;\
	}\
}

// Parse the instruction address specifier.
bool px_iasm_parse_addr(asm_ctx_t *ctx, tokeniser_ctx_t *lex_ctx, px_token_t *out, bool *has_next) {
	px_token_t tkn = px_iasm_lex(lex_ctx);
	px_token_t addressed;
	*has_next = false;
	
	if (tkn.type == PX_TKN_IVAL || tkn.type == PX_TKN_IDENT) {
		// imm
		addressed = tkn;
		addressed.regno = REG_IMM;
		addressed.addr_mode = ADDR_IMM;
		TKN_EXPECT_EOL(lex_ctx);
		goto check_addressed;
	} else if (tkn.type >= PX_TKN_R0 && tkn.type <= PX_TKN_PC) {
		// reg
		addressed = tkn;
		addressed.regno = tkn.type - PX_TKN_R0;
		addressed.addr_mode = ADDR_IMM;
		TKN_EXPECT_EOL(lex_ctx);
		goto check_addressed;
	} else if (tkn.type == PX_TKN_LBRAC) {
		// [ ... ]
		bool has_reg       = false;
		bool has_addressed = false;
		bool has_ident     = false;
		mem_start:
		tkn = px_iasm_lex(lex_ctx);
		// Expect a term.
		if (tkn.type >= PX_TKN_R0 && tkn.type <= PX_TKN_PC) {
			// Registers.
			if (tkn.type == PX_TKN_PF) {
				// PF is not an addressable register.
				PX_ERROR(lex_ctx, "Register 'PF' is not allowed in memory parameter.\n");
				return false;
			} else if (has_addressed && addressed.addr_mode != ADDR_MEM) {
				// Two registers plus imm is not possible.
				PX_ERROR(lex_ctx, "Argument too complex, consider removing a regiser.\n");
				return false;
			} else if (has_addressed) {
				// Add the register.
				addressed.addr_mode = tkn.type - PX_TKN_R0;
			} else {
				// Set the register.
				addressed           = tkn;
				addressed.regno     = tkn.type - PX_TKN_R0;
				addressed.addr_mode = ADDR_MEM;
				has_reg             = true;
				has_addressed       = true;
			}
		} else if (tkn.type == PX_TKN_IDENT) {
			// Ident.
			if (has_addressed && addressed.ident) {
				// Can't handle two idents at once.
				PX_ERROR(lex_ctx, "Cannot handle more than one ident.");
				return false;
			} else if (has_addressed && !addressed.ival && addressed.addr_mode != ADDR_MEM) {
				// Two registers plus imm is not possible.
				PX_ERROR(lex_ctx, "Argument too complex, consider removing a regiser.");
			} else if (has_addressed) {
				// Add the ident.
				if (has_reg && !addressed.ival) {
					addressed.addr_mode = addressed.regno;
					addressed.regno     = REG_IMM;
				}
				addressed.ident     = tkn.ident;
				has_ident           = true;
			} else {
				// Set the ident.
				addressed           = tkn;
				addressed.regno     = REG_IMM;
				addressed.addr_mode = ADDR_MEM;
				has_ident           = true;
				has_addressed       = true;
			}
		} else if (tkn.type == PX_TKN_IVAL) {
			// Ival.
			if (has_reg && !has_ident && addressed.addr_mode != ADDR_MEM) {
				// Two registers plus imm is not possible.
				PX_ERROR(lex_ctx, "Argument too complex, consider removing a regiser.");
				return false;
			} else if (has_addressed) {
				// Add the FUNNY.
				if (has_reg && !has_ident) {
					addressed.addr_mode = addressed.regno;
					addressed.regno     = REG_IMM;
				}
				addressed.ival += tkn.ival;
			} else {
				// Set the FUNNY.
				addressed           = tkn;
				addressed.regno     = REG_IMM;
				addressed.addr_mode = ADDR_MEM;
				has_addressed       = true;
			}
		} else {
			// Garbage.
			PX_ERROR(lex_ctx, "Expected REGISTER, IDENT or IVAL.\n");
			return false;
		}
		// Expect plus or end.
		tkn = px_iasm_lex(lex_ctx);
		if (tkn.type == PX_TKN_PLUS || tkn.type == PX_TKN_TILDE) {
			// Next term.
			goto mem_start;
		} else if (tkn.type == PX_TKN_RBRAC) {
			// Final term.
			TKN_EXPECT_EOL(lex_ctx);
			goto check_addressed;
		} else {
			// Garbage.
			PX_ERROR(lex_ctx, "Expected '+', '~' or ']'.\n");
			return false;
		}
	} else if (tkn.type != PX_TKN_END) {
		// Garbage.
		PX_ERROR(lex_ctx, "Expected REGISTER, IDENT, IVAL or '['.");
	}
	return false;
	
	nope:
	return false;
	
	check_addressed:
	*out = addressed;
	*has_next = tkn.type == PX_TKN_COMMA;
	if (addressed.type >= PX_TKN_R0 && addressed.type <= PX_TKN_PC) {
		return true;
	} else if (addressed.type == PX_TKN_IDENT || addressed.type == PX_TKN_IVAL) {
		return true;
	} else if (addressed.type < PX_NUM_KEYW) {
		char *insert = px_iasm_keyw[(size_t) addressed.type];
		char *buf = xalloc(ctx->allocator, strlen(insert) + 25);
		sprintf(buf, "Expected LABEL, IVAL or '[', got '%s'.", insert);
		PX_ERROR(lex_ctx, buf);
		xfree(ctx->allocator, buf);
		return false;
	} else {
		PX_ERROR(lex_ctx, "Expected LABEL, IVAL or '['.");
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
		bool got = px_iasm_parse_addr(ctx, lex_ctx, &next, &has_next);
		if (got) {
			len ++;
			list = (px_token_t *) xrealloc(ctx->allocator, list, sizeof(px_token_t) * len);
			list[len - 1] = next;
		}
	} while (has_next);
	*args = list;
	return len;
}

// Do a blob of assembly.
void gen_asm(asm_ctx_t *ctx, tokeniser_ctx_t *lex_ctx) {
	pos_t start_pos = pos_empty(lex_ctx);
	
	// Instruction.
	px_token_t tkn = px_iasm_lex(lex_ctx);
	if (tkn.type == PX_TKN_IDENT) {
		// This is not an instruction.
		char *buf = xalloc(ctx->allocator, strlen(tkn.ident) + 31);
		sprintf(buf, "No instruction with name '%s'.\n", tkn.ident);
		PX_ERROR(lex_ctx, buf);
		xfree(ctx->allocator, buf);
		xfree(lex_ctx->allocator, tkn.ident);
	} else if (tkn.type < PX_TKN_INSN_KEYWORDS) {
		// This is an instruction keyword.
		px_token_t *args;
		// Parse the funny parameters.
		size_t n_args = px_iasm_parse_addrs(ctx, lex_ctx, &args);
		// The number of args expected.
		size_t expect_args = 2;
		
		if (tkn.type >= PX_KEYW_INC && tkn.type <= PX_KEYW_SHRC) {
			expect_args = 1;
		}
		
		if (n_args != expect_args) {
			// No matching parameters for instruction.
			char buf[60];
			sprintf(buf, "Instruction '%s' has %zd argument%s (%zd given).\n", px_iasm_keyw[(size_t) tkn.type], expect_args, expect_args == 1 ? "" : "s", n_args);
			PX_ERROR_L(lex_ctx, start_pos, buf);
		} else if (args[0].addr_mode == ADDR_IMM && args[0].regno == REG_IMM) {
			// Can't have A be imm.
			PX_ERROR_L(lex_ctx, start_pos, "First parameter must be a register or memory reference.\n");
		} else if (n_args == 2 && args[0].addr_mode == ADDR_MEM && args[1].addr_mode == ADDR_MEM) {
			// Can't have both be memory.
			PX_ERROR_L(lex_ctx, start_pos, "No more than one memory reference is allowed.\n");
		} else {
			// Determine instruction value.
			px_insn_t insn = {
				.y = n_args == 2 && args[1].addr_mode != ADDR_IMM,
				.b = n_args == 2 ? args[1].regno : 0,
				.a = args[0].regno,
				.o = tkn.type
			};
			// Determine addressing mode.
			px_token_t tkn_x = insn.y ? args[1] : args[0];
			insn.x = tkn_x.addr_mode;
			
			// Write it out.
			px_write_insn(
				ctx, insn,
				args[0].ident,
				args[0].ival,
				n_args == 2 ? args[1].ident : NULL,
				n_args == 2 ? args[1].ival  : 0
			);
		}
		
		// Clean up the args list.
		for (size_t i = 0; i < n_args; i++) {
			if (args[i].ident) {
				xfree(lex_ctx->allocator, args[i].ident);
			}
		}
		xfree(ctx->allocator, args);
	} else if (tkn.type < PX_NUM_KEYW) {
		// This is a keyword, but not an instruction.
		char *buf = xalloc(ctx->allocator, strlen(px_iasm_keyw[(size_t) tkn.type]) + 31);
		sprintf(buf, "No instruction with name '%s'.\n", px_iasm_keyw[(size_t) tkn.type]);
		PX_ERROR_L(lex_ctx, start_pos, buf);
		xfree(ctx->allocator, buf);
		
	} else if (tkn.type == PX_TKN_END) {
		// End of line.
		return;
		
	} else {
		// Anything else is not allowed.
		PX_ERROR(lex_ctx, "Expected INSTRUCTION or IDENT.\n");
	}
}
