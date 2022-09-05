
#ifndef  PIXIE_16_GEN_H
#define  PIXIE_16_GEN_H

#include <gen.h>

/* ======= Gen-specific helper definitions ======= */

// Representation of an instruction.
typedef struct {
	bool      y;
	reg_t     x;
	reg_t     b;
	reg_t     a;
	reg_t     o;
} px_insn_t;

// Bump a register to the top of the usage list.
void px_touch_reg(asm_ctx_t *ctx, reg_t regno);
// Gets the least used register in the list.
reg_t px_least_used_reg(asm_ctx_t *ctx);

// Macro for determining PIE.
#define DET_PIE(ctx)  1
// Macro for determining PIE and applying relative addressing based on context (pointer).
#define OFFS(ctx)      (DET_PIE(ctx) ? ASM_LABEL_REF_OFFS_PTR : ASM_LABEL_REF_ABS_PTR)
// Macro for inverting the branch condition of a branch instruction.
#define INV_BR(insn)   ((insn) ^ 0x0008)

// Function for packing an instruction.
memword_t px_pack_insn(px_insn_t insn) __attribute__((pure));
// Function for unpacking an instruction.
px_insn_t px_unpack_insn(memword_t packed) __attribute__((pure));
// Gets or adds a temp var.
// Each temp label represents one word, so some variables will use multiple.
gen_var_t *px_get_tmp(asm_ctx_t *ctx, size_t size, bool allow_reg);

// Grab an addressing mode for a parameter.
reg_t px_addr_var(asm_ctx_t *ctx, gen_var_t *var, address_t part, reg_t *addrmode, asm_label_t *label, address_t *offs, reg_t dest);

// Determine the calling conventions to use.
void px_update_cc(asm_ctx_t *ctx, funcdef_t *funcdef);

// Creates a branch condition from a variable.
cond_t px_var_to_cond(asm_ctx_t *ctx, gen_var_t *var);
// Generate a branch to one of two labels.
void px_branch(asm_ctx_t *ctx, gen_var_t *cond_var, char *l_true, char *l_false);
// Generate a jump to a label.
void px_jump(asm_ctx_t *ctx, char *label);

// Pick a register to use.
reg_t px_pick_reg(asm_ctx_t *ctx, bool do_vacate);
// Pick a register to use, but only pick empty registers.
bool px_pick_empty_reg(asm_ctx_t *ctx, reg_t *regno);
// Move part of a value to a register.
void px_part_to_reg(asm_ctx_t *ctx, gen_var_t *val, reg_t dest, address_t index);
// Move a value to a register.
void px_mov_to_reg(asm_ctx_t *ctx, gen_var_t *val, reg_t dest);

// Creates MATH1 instructions.
gen_var_t *px_math1(asm_ctx_t *ctx, memword_t opcode, gen_var_t *out_hint, gen_var_t *a);
// Creates MATH2 instructions.
gen_var_t *px_math2(asm_ctx_t *ctx, memword_t opcode, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b);

// Called before a memory clobbering instruction is to be written.
void px_memclobber(asm_ctx_t *ctx, bool clobbers_stack);

// Variables: Move variable to another location.
void px_mov_n(asm_ctx_t *ctx, gen_var_t *dst, gen_var_t *src, address_t n_words);
// Variables: Move given variable into a register.
void px_var_to_reg(asm_ctx_t *ctx, gen_var_t *var, bool allow_const);
// Variables: Move stored variablue out of the given register.
void px_vacate_reg(asm_ctx_t *ctx, reg_t regno);

/* ========= Common instruction patterns ========= */

// Unsigned less than.
#define COND_ULT  000
// Unsigned greater than.
#define COND_UGT  001
// Signed less than.
#define COND_SLT  002
// Signed greater than.
#define COND_SGT  003
// Equal.
#define COND_EQ   004
// Unsigned carry set.
#define COND_CS   005
// Always true.
#define COND_TRUE 006
// Unsigned greater than or equal.
#define COND_UGE  010
// Unsigned less than or equal.
#define COND_ULE  011
// Signed greater than or equal.
#define COND_SGE  012
// Signed less than or equal.
#define COND_SLE  013
// Not equal.
#define COND_NE   014
// Unsigned carry not set.
#define COND_CC   015
// Reserved for JSR instruction.
#define COND_JSR  016
// Reserved for carry extend instruction.
#define COND_CX   017

// Address by memory.
#define ADDR_MEM 5
// Address by register or imm.
#define ADDR_IMM 7
// Address by register offset.
#define ADDR_REG(regno) (regno)
#define ADDR_R0 ADDR_REG(REG_R0)
#define ADDR_R1 ADDR_REG(REG_R1)
#define ADDR_R2 ADDR_REG(REG_R2)
#define ADDR_R3 ADDR_REG(REG_R3)
#define ADDR_ST ADDR_REG(REG_ST)
#define ADDR_PC ADDR_REG(REG_PC)

// ADD instructions.
#define PX_OP_ADD 000
// SUB instruction.
#define PX_OP_SUB 001
// CMP instructions.
#define PX_OP_CMP 002
// AND instructions.
#define PX_OP_AND 003
// OR instructions.
#define PX_OP_OR  004
// XOR instructions.
#define PX_OP_XOR 005

// INC instructions.
#define PX_OP_INC 020
// DEC instruction.
#define PX_OP_DEC 021
// CMP1 instructions.
#define PX_OP_CMP1 022
// SHL instructions.
#define PX_OP_SHL 026
// SHR instructions.
#define PX_OP_SHR 027

// Unconditional MOV instructions.
#define PX_OP_MOV 046
// Unconditional LEA instructions.
#define PX_OP_LEA 066

// Offset for MOV instructions.
#define PX_OFFS_MOV 040
// Offset for LEA instructions.
#define PX_OFFS_LEA 060

// Offset for carry continue.
#define PX_OFFS_CC 010

// Return: MOV PC, [ST]
#define INSN_RET     0xd9a6
// Jump: MOV PC, imm
#define INSN_JMP     0x7fa6
// Jump (PIE): LEA PC, [PC+imm]
#define INSN_JMP_PIE 0xefb6
// SUB ST, imm
#define INSN_SUB_ST  0x7f01
// ADD ST, imm
#define INSN_ADD_ST  0x7f00

#endif // PIXIE_16_GEN_H
