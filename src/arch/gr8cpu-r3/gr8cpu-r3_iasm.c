
#include <asm.h>
#include <gen.h>
#include <gr8cpu-r3_gen.h>
#include <tokeniser.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include <gr8cpu-r3_iasm.h>

#define A_IMM       0x00

#define A_REG_A     0x01
#define A_REG_X     0x02
#define A_REG_Y     0x04
#define A_REG_Y     0x04

#define A_MEM       0x10
#define A_PTR       0x20

#define A_MEM_X     0x12
#define A_MEM_Y     0x14
#define A_PTR_X     0x22
#define A_PTR_Y     0x24
#define A_PTR_XY    0x26

#define A_REG_F     0xf1
#define A_REG_STL   0xf2
#define A_REG_STH   0xf3

// All keywords that occur.
char *r3_iasm_keyw[] = {
	"bki",  "brk",  "call", "ret", 
	"psh",  "pul",  "pop",  "jmp", 
	"beq",  "bne",  "bgt",  "ble", 
	"blt",  "bge",  "bcs",  "bcc", 
	"mov",  "add",  "sub",  "cmp", 
	"inc",  "dec",  "shl",  "shr", 
	"addc", "subc", "cmpc", "incc", 
	"decc", "shlc", "shrc", "rol", 
	"ror",  "and",  "or",   "xor", 
	"rti",  "jmpt", "calt", "sirq", 
	"cirq", "gptr", "virq", "vnmi", 
	"vst",  "hlt",  "a",    "x", 
	"y",    "stl",  "sth",  "f", 
};

// Addressing modes belonging to instructions.
r3_iasm_modes_t r3_insn_lut[46] = {
	{ // bki
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=0, .opcode=0x00, .n_words=0},
		}
	},
	{ // brk
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=0, .opcode=0x01, .n_words=0},
		}
	},
	{ // call
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x02, .n_words=2, .arg_modes={A_IMM}},
			{ .n_args=1, .opcode=0x6C, .n_words=2, .arg_modes={A_PTR}},
		}
	},
	{ // ret
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=0, .opcode=0x03, .n_words=0},
		}
	},
	{ // psh
		.num = 5, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x04, .n_words=0, .arg_modes={A_REG_A}},
			{ .n_args=1, .opcode=0x05, .n_words=0, .arg_modes={A_REG_X}},
			{ .n_args=1, .opcode=0x06, .n_words=0, .arg_modes={A_REG_Y}},
			{ .n_args=1, .opcode=0x07, .n_words=1, .arg_modes={A_IMM}},
			{ .n_args=1, .opcode=0x08, .n_words=2, .arg_modes={A_MEM}},
		}
	},
	{ // pul
		.num = 4, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x09, .n_words=0, .arg_modes={A_REG_A}},
			{ .n_args=1, .opcode=0x0A, .n_words=0, .arg_modes={A_REG_X}},
			{ .n_args=1, .opcode=0x0B, .n_words=0, .arg_modes={A_REG_Y}},
			{ .n_args=1, .opcode=0x0D, .n_words=2, .arg_modes={A_MEM}},
		}
	},
	{ // pop
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=0, .opcode=0x0C, .n_words=0},
		}
	},
	{ // jmp
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x0E, .n_words=2, .arg_modes={A_IMM}},
			{ .n_args=1, .opcode=0x6D, .n_words=2, .arg_modes={A_PTR}},
		}
	},
	{ // beq
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x0F, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // bne
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x10, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // bgt
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x11, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // ble
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x12, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // blt
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x13, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // bge
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x14, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // bcs
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x15, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // bcc
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x16, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // mov
		.num = 33, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x17, .n_words=0, .arg_modes={A_REG_A, A_REG_X}},
			{ .n_args=2, .opcode=0x18, .n_words=0, .arg_modes={A_REG_A, A_REG_Y}},
			{ .n_args=2, .opcode=0x19, .n_words=0, .arg_modes={A_REG_X, A_REG_A}},
			{ .n_args=2, .opcode=0x1A, .n_words=0, .arg_modes={A_REG_X, A_REG_Y}},
			{ .n_args=2, .opcode=0x1B, .n_words=0, .arg_modes={A_REG_Y, A_REG_A}},
			{ .n_args=2, .opcode=0x1C, .n_words=0, .arg_modes={A_REG_Y, A_REG_X}},
			{ .n_args=2, .opcode=0x1D, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x1E, .n_words=1, .arg_modes={A_REG_X, A_IMM}},
			{ .n_args=2, .opcode=0x1F, .n_words=1, .arg_modes={A_REG_Y, A_IMM}},
			{ .n_args=2, .opcode=0x20, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
			{ .n_args=2, .opcode=0x21, .n_words=2, .arg_modes={A_REG_X, A_MEM}},
			{ .n_args=2, .opcode=0x22, .n_words=2, .arg_modes={A_REG_Y, A_MEM}},
			{ .n_args=2, .opcode=0x23, .n_words=2, .arg_modes={A_REG_A, A_MEM_X}},
			{ .n_args=2, .opcode=0x24, .n_words=2, .arg_modes={A_REG_A, A_MEM_Y}},
			{ .n_args=2, .opcode=0x25, .n_words=2, .arg_modes={A_REG_A, A_PTR}},
			{ .n_args=2, .opcode=0x26, .n_words=2, .arg_modes={A_REG_A, A_PTR_XY}},
			{ .n_args=2, .opcode=0x27, .n_words=2, .arg_modes={A_REG_A, A_PTR_X}},
			{ .n_args=2, .opcode=0x28, .n_words=2, .arg_modes={A_REG_A, A_PTR_Y}},
			{ .n_args=2, .opcode=0x29, .n_words=2, .arg_modes={A_MEM, A_REG_A}},
			{ .n_args=2, .opcode=0x2A, .n_words=2, .arg_modes={A_MEM, A_REG_X}},
			{ .n_args=2, .opcode=0x2B, .n_words=2, .arg_modes={A_MEM, A_REG_Y}},
			{ .n_args=2, .opcode=0x2C, .n_words=2, .arg_modes={A_MEM_X, A_REG_A}},
			{ .n_args=2, .opcode=0x2D, .n_words=2, .arg_modes={A_MEM_Y, A_REG_A}},
			{ .n_args=2, .opcode=0x2E, .n_words=2, .arg_modes={A_PTR, A_REG_A}},
			{ .n_args=2, .opcode=0x2F, .n_words=2, .arg_modes={A_PTR_XY, A_REG_A}},
			{ .n_args=2, .opcode=0x30, .n_words=2, .arg_modes={A_PTR_X, A_REG_A}},
			{ .n_args=2, .opcode=0x31, .n_words=2, .arg_modes={A_PTR_Y, A_REG_A}},
			{ .n_args=2, .opcode=0x6E, .n_words=0, .arg_modes={A_REG_A, A_REG_STL}},
			{ .n_args=2, .opcode=0x6F, .n_words=0, .arg_modes={A_REG_A, A_REG_STH}},
			{ .n_args=2, .opcode=0x70, .n_words=0, .arg_modes={A_REG_STL, A_REG_A}},
			{ .n_args=2, .opcode=0x71, .n_words=0, .arg_modes={A_REG_STH, A_REG_A}},
			{ .n_args=2, .opcode=0x72, .n_words=0, .arg_modes={A_REG_F, A_REG_A}},
			{ .n_args=2, .opcode=0x73, .n_words=0, .arg_modes={A_REG_A, A_REG_F}},
		}
	},
	{ // add
		.num = 8, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x32, .n_words=0, .arg_modes={A_REG_A, A_REG_X}},
			{ .n_args=2, .opcode=0x33, .n_words=0, .arg_modes={A_REG_A, A_REG_Y}},
			{ .n_args=2, .opcode=0x38, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x39, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
			{ .n_args=2, .opcode=0x5C, .n_words=1, .arg_modes={A_REG_X, A_IMM}},
			{ .n_args=2, .opcode=0x5D, .n_words=2, .arg_modes={A_REG_X, A_MEM}},
			{ .n_args=2, .opcode=0x64, .n_words=1, .arg_modes={A_REG_Y, A_IMM}},
			{ .n_args=2, .opcode=0x65, .n_words=2, .arg_modes={A_REG_Y, A_MEM}},
		}
	},
	{ // sub
		.num = 8, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x34, .n_words=0, .arg_modes={A_REG_A, A_REG_X}},
			{ .n_args=2, .opcode=0x35, .n_words=0, .arg_modes={A_REG_A, A_REG_Y}},
			{ .n_args=2, .opcode=0x3A, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x3B, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
			{ .n_args=2, .opcode=0x5E, .n_words=1, .arg_modes={A_REG_X, A_IMM}},
			{ .n_args=2, .opcode=0x5F, .n_words=2, .arg_modes={A_REG_X, A_MEM}},
			{ .n_args=2, .opcode=0x66, .n_words=1, .arg_modes={A_REG_Y, A_IMM}},
			{ .n_args=2, .opcode=0x67, .n_words=2, .arg_modes={A_REG_Y, A_MEM}},
		}
	},
	{ // cmp
		.num = 8, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x36, .n_words=0, .arg_modes={A_REG_A, A_REG_X}},
			{ .n_args=2, .opcode=0x37, .n_words=0, .arg_modes={A_REG_A, A_REG_Y}},
			{ .n_args=2, .opcode=0x3C, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x3D, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
			{ .n_args=2, .opcode=0x60, .n_words=1, .arg_modes={A_REG_X, A_IMM}},
			{ .n_args=2, .opcode=0x61, .n_words=2, .arg_modes={A_REG_X, A_MEM}},
			{ .n_args=2, .opcode=0x68, .n_words=1, .arg_modes={A_REG_Y, A_IMM}},
			{ .n_args=2, .opcode=0x69, .n_words=2, .arg_modes={A_REG_Y, A_MEM}},
		}
	},
	{ // inc
		.num = 4, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x3E, .n_words=0, .arg_modes={A_REG_A}},
			{ .n_args=1, .opcode=0x3F, .n_words=2, .arg_modes={A_MEM}},
			{ .n_args=1, .opcode=0x62, .n_words=0, .arg_modes={A_REG_X}},
			{ .n_args=1, .opcode=0x6A, .n_words=0, .arg_modes={A_REG_Y}},
		}
	},
	{ // dec
		.num = 4, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x40, .n_words=0, .arg_modes={A_REG_A}},
			{ .n_args=1, .opcode=0x41, .n_words=2, .arg_modes={A_MEM}},
			{ .n_args=1, .opcode=0x63, .n_words=0, .arg_modes={A_REG_X}},
			{ .n_args=1, .opcode=0x6B, .n_words=0, .arg_modes={A_REG_Y}},
		}
	},
	{ // shl
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x42, .n_words=2, .arg_modes={A_MEM}},
			{ .n_args=1, .opcode=0x58, .n_words=0, .arg_modes={A_REG_A}},
		}
	},
	{ // shr
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x43, .n_words=2, .arg_modes={A_MEM}},
			{ .n_args=1, .opcode=0x59, .n_words=0, .arg_modes={A_REG_A}},
		}
	},
	{ // addc
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x44, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x45, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
		}
	},
	{ // subc
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x46, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x47, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
		}
	},
	{ // cmpc
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x48, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x49, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
		}
	},
	{ // incc
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x4A, .n_words=0, .arg_modes={A_REG_A}},
			{ .n_args=1, .opcode=0x4B, .n_words=2, .arg_modes={A_MEM}},
		}
	},
	{ // decc
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x4C, .n_words=0, .arg_modes={A_REG_A}},
			{ .n_args=1, .opcode=0x4D, .n_words=2, .arg_modes={A_MEM}},
		}
	},
	{ // shlc
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x4E, .n_words=2, .arg_modes={A_MEM}},
		}
	},
	{ // shrc
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x4F, .n_words=2, .arg_modes={A_MEM}},
		}
	},
	{ // rol
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x50, .n_words=2, .arg_modes={A_MEM}},
			{ .n_args=1, .opcode=0x5A, .n_words=0, .arg_modes={A_REG_A}},
		}
	},
	{ // ror
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x51, .n_words=2, .arg_modes={A_MEM}},
			{ .n_args=1, .opcode=0x5B, .n_words=0, .arg_modes={A_REG_A}},
		}
	},
	{ // and
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x52, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x53, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
		}
	},
	{ // or
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x54, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x55, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
		}
	},
	{ // xor
		.num = 2, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=2, .opcode=0x56, .n_words=1, .arg_modes={A_REG_A, A_IMM}},
			{ .n_args=2, .opcode=0x57, .n_words=2, .arg_modes={A_REG_A, A_MEM}},
		}
	},
	{ // rti
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=0, .opcode=0x74, .n_words=0},
		}
	},
	{ // jmpt
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x75, .n_words=2, .arg_modes={A_MEM_X}},
		}
	},
	{ // calt
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x76, .n_words=2, .arg_modes={A_MEM_X}},
		}
	},
	{ // sirq
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=0, .opcode=0x79, .n_words=0},
		}
	},
	{ // cirq
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=0, .opcode=0x7A, .n_words=0},
		}
	},
	{ // gptr
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x7B, .n_words=2, .arg_modes={A_MEM}},
		}
	},
	{ // virq
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x7C, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // vnmi
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x7D, .n_words=2, .arg_modes={A_IMM}},
		}
	},
	{ // vst
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=1, .opcode=0x7E, .n_words=1, .arg_modes={A_IMM}},
		}
	},
	{ // hlt
		.num = 1, .modes = (r3_iasm_mode_t[]) {
			{ .n_args=0, .opcode=0x7F, .n_words=0},
		}
	},
};

#define R3_TKN_INSN_KEYWORDS 46

#define R3_NUM_KEYW 52

bool r3_is_label_char(char c) {
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
r3_token_t r3_iasm_lex(tokeniser_ctx_t *ctx) {
	// Get the first non-space character.
	char c;
	retry:
	do {
		c = tokeniser_readchar(ctx);
	} while(is_space(c));
	if (!c) return (r3_token_t) {.type = R3_TKN_END};
	
	char next = tokeniser_nextchar(ctx);
	size_t index0 = ctx->index - 1;
	int x0 = ctx->x, y0 = ctx->y;
	
	// Check for comments and single-character tokens.
	switch (c) {
		case (','):
			return (r3_token_t) {.type = R3_TKN_COMMA};
		case ('['):
			return (r3_token_t) {.type = R3_TKN_LBRAC};
		case (']'):
			return (r3_token_t) {.type = R3_TKN_RBRAC};
		case ('('):
			return (r3_token_t) {.type = R3_TKN_LPAR};
		case (')'):
			return (r3_token_t) {.type = R3_TKN_RPAR};
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
				return (r3_token_t) {.type = R3_TKN_END};
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
		DEBUG_TKN("ival  %d (0x%s)\n", ival, strval);
		free(strval);
		return (r3_token_t) {
			.type = R3_TKN_IVAL,
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
		DEBUG_TKN("ival  %d (%s)\n", ival, strval);
		free(strval);
		return (r3_token_t) {
			.type = R3_TKN_IVAL,
			.ival = ival
		};
	}
	
	// Or an ident or keyword.
	if (r3_is_label_char(c)) {
		// Check how many of these we get.
		int offs = 0;
		while (r3_is_label_char(tokeniser_nextchar_no(ctx, offs))) offs++;
		offs ++;
		// Now, grab it.
		char *strval = (char *) malloc(sizeof(char) * (offs + 1));
		*strval = c;
		strval[offs] = 0;
		for (int i = 1; i < offs; i++) {
			strval[i] = tokeniser_readchar(ctx);
		}
		// Next, check for keywords.
		for (size_t i = 0; i < R3_NUM_KEYW; i++) {
			if (!strcasecmp(strval, r3_iasm_keyw[i])) {
				free(strval);
				return (r3_token_t) {
					.type  = (r3_iasm_token_id_t) i,
					.ident = r3_iasm_keyw[i]
				};
			}
		}
		// Return the appropriate alternative.
		return (r3_token_t) {
			.type  = R3_TKN_IDENT,
			.ident = strval
		};
	}
	
	return (r3_token_t) {
		.type  = R3_TKN_OTHER,
		.other = c
	};
}

#define TKN_EXPECT(lex_ctx, ex_type, error) { if (r3_iasm_lex(lex_ctx).type != ex_type) { printf("Expected '%s'.\n", error); goto nope; } }
#define TKN_EXPECT_EOL(lex_ctx) {\
	r3_token_t tkn = r3_iasm_lex(lex_ctx);\
	if (tkn.type != R3_TKN_COMMA && tkn.type != R3_TKN_END) {\
		printf("Expected ','.\n"); goto nope;\
	}\
}

// Parse the instruction address specifier.
bool r3_iasm_parse_addr(asm_ctx_t *ctx, tokeniser_ctx_t *lex_ctx, r3_token_t *out) {
	r3_token_t tkn = r3_iasm_lex(lex_ctx);
	r3_token_t addressed;
	if (tkn.type == R3_KEYW_X) {
		// X[%] or X(%) or X(%)Y
		tkn = r3_iasm_lex(lex_ctx);
		if (tkn.type == R3_TKN_LPAR) {
			addressed = r3_iasm_lex(lex_ctx);
			TKN_EXPECT(lex_ctx, R3_TKN_RPAR, ")");
			tkn = r3_iasm_lex(lex_ctx);
			if (tkn.type == R3_TKN_COMMA || tkn.type == R3_TKN_END) {
				// X(%)
				addressed.addr_mode = A_PTR_X;
			} else if (tkn.type == R3_KEYW_Y) {
				// X(%)Y
				addressed.addr_mode = A_PTR_XY;
				TKN_EXPECT_EOL(lex_ctx);
			} else {
				printf("Expected 'Y' or ','.\n");
				goto nope;
			}
			goto check_addressed;
		} else if (tkn.type == R3_TKN_LBRAC) {
			// X[%]
			addressed = r3_iasm_lex(lex_ctx);
			TKN_EXPECT(lex_ctx, R3_TKN_RBRAC, "]");
			TKN_EXPECT_EOL(lex_ctx);
			addressed.addr_mode = A_MEM_X;
			goto check_addressed;
		}
	} else if (tkn.type == R3_KEYW_Y) {
		// Y[%]
		TKN_EXPECT(lex_ctx, R3_TKN_RBRAC, "[");
		addressed = r3_iasm_lex(lex_ctx);
		TKN_EXPECT(lex_ctx, R3_TKN_RBRAC, "]");
		TKN_EXPECT_EOL(lex_ctx);
		addressed.addr_mode = A_MEM_Y;
		goto check_addressed;
	} else if (tkn.type == R3_TKN_LBRAC) {
		// [%]
		addressed = r3_iasm_lex(lex_ctx);
		TKN_EXPECT(lex_ctx, R3_TKN_RBRAC, "]");
		TKN_EXPECT_EOL(lex_ctx);
		addressed.addr_mode = A_MEM;
		goto check_addressed;
	} else if (tkn.type == R3_TKN_LPAR) {
		// (%) or (%)Y
		addressed = r3_iasm_lex(lex_ctx);
		TKN_EXPECT(lex_ctx, R3_TKN_RPAR, ")");tkn = r3_iasm_lex(lex_ctx);
		if (tkn.type == R3_TKN_COMMA || tkn.type == R3_TKN_END) {
			// (%)
			addressed.addr_mode = A_PTR;
			TKN_EXPECT_EOL(lex_ctx);
		} else if (tkn.type == R3_KEYW_Y) {
			// (%)Y
			addressed.addr_mode = A_PTR_Y;
			TKN_EXPECT_EOL(lex_ctx);
		} else {
			printf("Expected 'Y' or ','.\n");
			goto nope;
		}
		goto check_addressed;
	} else if (tkn.type == R3_KEYW_A) {
		tkn.addr_mode = A_REG_A;*out = tkn;
	} else if (tkn.type == R3_KEYW_X) {
		tkn.addr_mode = A_REG_X; *out = tkn;
	} else if (tkn.type == R3_KEYW_Y) {
		tkn.addr_mode = A_REG_Y; *out = tkn;
	} else if (tkn.type == R3_KEYW_F) {
		tkn.addr_mode = A_REG_F; *out = tkn;
	} else if (tkn.type == R3_KEYW_STL) {
		tkn.addr_mode = A_REG_STL; *out = tkn;
	} else if (tkn.type == R3_KEYW_STH) {
		tkn.addr_mode = A_REG_STH; *out = tkn;
	} else if (tkn.type == R3_TKN_IDENT || tkn.type == R3_TKN_IVAL) {
		tkn.addr_mode = A_IMM; *out = tkn;
		return true;
	} else if (tkn.type == R3_TKN_END) {
		return false;
	} else if (tkn.type < R3_NUM_KEYW) {
		printf("Expected one of: 'A', 'X', 'Y', 'F', 'STL', 'STH', LABEL, got '%s'.\n", r3_iasm_keyw[(size_t) tkn.type]);
		return false;
	} else {
		printf("Expected one of: 'A', 'X', 'Y', 'F', 'STL', 'STH', LABEL.\n");
		return false;
	}
	TKN_EXPECT_EOL(lex_ctx);
	return true;
	
	nope:
	return false;
	
	check_addressed:
	*out = addressed;
	if (addressed.type == R3_TKN_IDENT || addressed.type == R3_TKN_IVAL) {
		return true;
	} else if (addressed.type < R3_NUM_KEYW) {
		printf("Expected LABEL, got '%s'.\n", r3_iasm_keyw[(size_t) addressed.type]);
		return false;
	} else {
		printf("Expected LABEL.\n");
		return false;
	}
}

// Parse the instruction address list.
size_t r3_iasm_parse_addrs(asm_ctx_t *ctx, tokeniser_ctx_t *lex_ctx, r3_token_t **args) {
	r3_token_t *list = NULL;
	size_t len = 0;
	r3_token_t next;
	bool has_next;
	do {
		has_next = r3_iasm_parse_addr(ctx, lex_ctx, &next);
		if (has_next) {
			len ++;
			list = (r3_token_t *) realloc(list, sizeof(r3_token_t) * len);
			list[len - 1] = next;
		}
	} while (has_next);
	*args = list;
	return len;
}

// Do a blob of assembly.
void gen_asm(asm_ctx_t *ctx, char *text) {
	DEBUG_GEN("// inline assembly\n");
	
	// Make a little tokeniser context.
	tokeniser_ctx_t lex_ctx;
	tokeniser_init_cstr(&lex_ctx, text);
	
	r3_token_t ooer;
	
	// do {
	//     ooer = r3_iasm_lex(&lex_ctx);
	//     if (ooer.type < R3_NUM_KEYW) DEBUG_GEN("keyw %s\n", r3_iasm_keyw[(size_t) ooer.type]);
	//     else if (ooer.type == R3_TKN_IDENT) DEBUG_GEN("ident %s\n", ooer.ident);
	//     else if (ooer.type == R3_TKN_IVAL) DEBUG_GEN("ival %lld\n", ooer.ival);
	//     else if (ooer.type == R3_TKN_END) DEBUG_GEN("end\n");
	//     else DEBUG_GEN("type %d\n", ooer.type);
	// } while (ooer.type != R3_TKN_END);
	
	// bool succ = r3_iasm_parse_addr(ctx, &lex_ctx, &ooer);
	// if (!succ) return;
	// if (ooer.type < R3_NUM_KEYW) DEBUG_GEN("keyw %s mode %d\n", r3_iasm_keyw[(size_t) ooer.type], ooer.addr_mode);
	// else if (ooer.type == R3_TKN_IDENT) DEBUG_GEN("ident %s mode %d\n", ooer.ident, ooer.addr_mode);
	// else DEBUG_GEN("type %d mode %d\n", ooer.type, ooer.addr_mode);
	
	// r3_token_t *list;
	// size_t len = r3_iasm_parse_addrs(ctx, &lex_ctx, &list);
	// DEBUG_GEN("There's %ld%c\n", len, len > 0 ? ':' : '\0');
	// for (size_t i = 0; i < len; i++) {
	//     r3_token_t ooer = list[i];
	//     if (ooer.type < R3_NUM_KEYW) DEBUG_GEN("keyw %s mode %d\n", r3_iasm_keyw[(size_t) ooer.type], ooer.addr_mode);
	//     else if (ooer.type == R3_TKN_IDENT) DEBUG_GEN("ident %s mode %d\n", ooer.ident, ooer.addr_mode);
	//     else if (ooer.type == R3_TKN_IVAL) DEBUG_GEN("ival %lld mode %d\n", ooer.ival, ooer.addr_mode);
	//     else DEBUG_GEN("type %d mode %d\n", ooer.type, ooer.addr_mode);
	// }
	
	// return;
	
	// Instruction.
	r3_token_t tkn = r3_iasm_lex(&lex_ctx);
	if (tkn.type == R3_TKN_IDENT) {
		// This is some garbage instruction.
		printf("No instruction with name '%s'.\n", tkn.ident);
	} else if (tkn.type < R3_TKN_INSN_KEYWORDS) {
		// This is an instruction keyword.
		r3_token_t *args;
		size_t n_args = r3_iasm_parse_addrs(ctx, &lex_ctx, &args);
		// Check for matching address patterns.
		bool found_len = false;
		r3_iasm_modes_t *arr = &r3_insn_lut[(size_t) tkn.type];
		for (size_t i = 0; i < arr->num; i++) {
			// Check length.
			r3_iasm_mode_t *mode = &arr->modes[i];
			if (mode->n_args != n_args) continue;
			found_len = true;
			// Check all the bits.
			for (size_t x = 0; x < n_args; x++) {
				if (mode->arg_modes[x] != args[x].addr_mode) goto outer_cont;
			}
			// If we get here, we got good a match.
			asm_write_memword(ctx, mode->opcode + PIE(ctx));
			for (size_t x = 0; x < n_args; x++) {
				if (args[x].type == R3_TKN_IDENT) {
					if (mode->n_words == 1)
						asm_write_label_ref(ctx, args[x].ident, 0, OFFS_W_LO(ctx));
					else
						asm_write_label_ref(ctx, args[x].ident, 0, OFFS(ctx));
				} else if (args[x].type == R3_TKN_IVAL) {
					asm_write_num(ctx, args[x].ival, mode->n_words);
				}
			}
			return;
			// What.
			outer_cont:;
		}
		// If we got here, there's no match.
		if (found_len) {
			printf("No '%s' found for given arguments.\n", r3_iasm_keyw[(size_t) tkn.type]);
		} else {
			printf("No '%s' found for %ld arguments.\n", r3_iasm_keyw[(size_t) tkn.type], n_args);
		}
	} else if (tkn.type < R3_NUM_KEYW) {
		// This is a keyword.
		printf("No instruction with name '%s'.\n", r3_iasm_keyw[(size_t) tkn.type]);
	} else if (tkn.type == R3_TKN_END) {
		// End of the assembly.
		return;
	} else {
		// This is not allowed.
		printf("Expected INSTRUCTION or IDENT.\n");
		do {
			tkn = r3_iasm_lex(&lex_ctx);
		} while (tkn.type != R3_TKN_END);
	}
}
