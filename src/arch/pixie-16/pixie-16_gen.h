
#ifndef  PIXIE_16_GEN_H
#define  PIXIE_16_GEN_H

#include <gen.h>

/* ======= Gen-specific helper definitions ======= */

// Bump a register to the top of the usage list.
void px_touch_reg(asm_ctx_t *ctx, reg_t regno);

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
reg_t px_addr_var(asm_ctx_t *ctx, gen_var_t *var, address_t part, px_addr_t *addrmode, asm_label_t *label, address_t *offs, reg_t dest);

// Determine the calling conventions to use.
void px_update_cc(asm_ctx_t *ctx, funcdef_t *funcdef);

// Creates a branch condition from a variable.
cond_t px_var_to_cond(asm_ctx_t *ctx, expr_t *expr, gen_var_t *var);
// Generate a branch to one of two labels.
void px_branch(asm_ctx_t *ctx, expr_t *expr, gen_var_t *cond_var, char *l_true, char *l_false);
// Generate a jump to a label.
void px_jump(asm_ctx_t *ctx, char *label);

// Pick a register to use.
reg_t px_pick_reg(asm_ctx_t *ctx, bool do_vacate);
// Pick a register to use, but only pick empty registers.
bool px_pick_empty_reg(asm_ctx_t *ctx, reg_t *regno, address_t size);
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

// Generate some conditional MOV statements.
void px_gen_cond_mov_stmt(asm_ctx_t *ctx, stmt_t *stmt, cond_t cond);
// Check whether a statement can be reduced to some MOV.
bool px_is_mov_stmt(asm_ctx_t *ctx, stmt_t *stmt);
// Check whether an if statement can be reduced to conditional MOV.
bool px_cond_mov_applicable(asm_ctx_t *ctx, gen_var_t *cond, stmt_t *s_if, stmt_t *s_else);

/* ========= Common instruction patterns ========= */

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
