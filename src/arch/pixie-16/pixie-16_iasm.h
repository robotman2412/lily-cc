
#ifndef PIXIE_16_IASM_H
#define PIXIE_16_IASM_H

typedef enum px_iasm_token_id {
	// Math: two args.
	PX_KEYW_ADD, PX_KEYW_SUB, PX_KEYW_CMP,  PX_KEYW_AND, PX_KEYW_OR,  PX_KEYW_XOR, PX_dummy0,   PX_dummy1,
	// Math: one arg.
	PX_KEYW_INC, PX_KEYW_DEC, PX_KEYW_CMP1, PX_dummy2,   PX_dummy3,   PX_dummy4,   PX_KEYW_SHL, PX_KEYW_SHR,
	// Mathc: two args.
	PX_KEYW_ADDC, PX_KEYW_SUBC, PX_KEYW_CMPC,  PX_KEYW_ANDC, PX_KEYW_ORC,  PX_KEYW_XORC, PX_dummy0C,   PX_dummy1C,
	// Mathc: one arg.
	PX_KEYW_INCC, PX_KEYW_DECC, PX_KEYW_CMP1C, PX_dummy2C,   PX_dummy3C,   PX_dummy4C,   PX_KEYW_SHLC, PX_KEYW_SHRC,
	// Move instructions.
	PX_KEYW_MOV_ULT, PX_KEYW_MOV_UGT, PX_KEYW_MOV_SLT, PX_KEYW_MOV_SGT, PX_KEYW_MOV_EQ,  PX_KEYW_MOV_CS,  PX_KEYW_MOV,     PX_KEYW_MOV_BRK,
	PX_KEYW_MOV_UGE, PX_KEYW_MOV_ULE, PX_KEYW_MOV_SGE, PX_KEYW_MOV_SLE, PX_KEYW_MOV_NE,  PX_KEYW_MOV_CC,  PX_KEYW_MOV_JSR, PX_KEYW_MOV_RTI,
	// Load effective address instructions_
	PX_KEYW_LEA_ULT, PX_KEYW_LEA_UGT, PX_KEYW_LEA_SLT, PX_KEYW_LEA_SGT, PX_KEYW_LEA_EQ,  PX_KEYW_LEA_CS,  PX_KEYW_LEA,     PX_KEYW_LEA_BRK,
	PX_KEYW_LEA_UGE, PX_KEYW_LEA_ULE, PX_KEYW_LEA_SGE, PX_KEYW_LEA_SLE, PX_KEYW_LEA_NE,  PX_KEYW_LEA_CC,  PX_KEYW_LEA_JSR, PX_KEYW_LEA_RTI,
	// Registers.
	PX_TKN_R0, PX_TKN_R1, PX_TKN_R2, PX_TKN_R3,
	PX_TKN_ST, PX_TKN_PF, PX_TKN_PC, PX_TKN_IMM,
	// Other tokens.
	PX_TKN_COMMA,
	PX_TKN_LPAR,
	PX_TKN_RPAR,
	PX_TKN_LBRAC,
	PX_TKN_RBRAC,
	PX_TKN_PLUS,
	PX_TKN_TILDE,
	PX_TKN_IDENT,
	PX_TKN_IVAL,
	PX_TKN_MEM,
	PX_TKN_OTHER,
	PX_TKN_END
} px_iasm_token_id_t;

#define PX_TKN_INSN_KEYWORDS 64
#define PX_NUM_KEYW 72

struct px_iasm_token;
struct px_iasm_modes;
struct px_iasm_mode;

typedef struct px_iasm_token px_token_t;
typedef struct px_iasm_modes px_iasm_modes_t;
typedef struct px_iasm_mode px_iasm_mode_t;

#include <stdint.h>
#include <stddef.h>

struct px_iasm_token {
	px_iasm_token_id_t type;
	bool               cont;
	uint8_t            regno;
	uint8_t            addr_mode;
	char               other;
	char              *ident;
	long long          ival;
};

struct px_iasm_modes {
	size_t          num;
	px_iasm_mode_t *modes;
};

struct px_iasm_mode {
	size_t  n_args;
	uint8_t opcode;
	uint8_t n_words;
	uint8_t arg_modes[2];
};

#endif //PIXIE_16_IASM_H
