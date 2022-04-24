
#ifndef PIXIE_16_IASM_H
#define PIXIE_16_IASM_H

typedef enum px_iasm_token_id {
	// Instructions.
	PX_KEYW_ADD, PX_KEYW_SUB, PX_KEYW_CMP, PX_KEYW_AND,
	PX_KEYW_OR,  PX_KEYW_XOR, PX_KEYW_SHL, PX_KEYW_SHR,
	PX_KEYW_MOV, PX_KEYW_LEA,
	// Registers.
	PX_REG_R0, PX_REG_R1, PX_REG_R2, PX_REG_R3,
	PX_REG_ST, PX_REG_PF, PX_REG_PC, PX_REG_IMM,
	// Conditions.
	PX_KEYW_ULT, PX_KEYW_UGT, PX_KEYW_SLT, PX_KEYW_SGT,
	PX_KEYW_EQ,  PX_KEYW_NE,  PX_KEYW_JSR,
	// Other tokens.
	PX_TKN_COMMA,
	PX_TKN_LPAR,
	PX_TKN_RPAR,
	PX_TKN_LBRAC,
	PX_TKN_RBRAC,
	PX_TKN_IDENT,
	PX_TKN_IVAL,
	PX_TKN_OTHER,
	PX_TKN_END
} px_iasm_token_id_t;

#define PX_TKN_INSN_KEYWORDS 10

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
	union {
		char           other;
		char          *ident;
		long long      ival;
	};
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
