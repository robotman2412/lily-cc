
#pragma once

#include <gen.h>

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

// Generate a branch to one of two labels.
void px_branch(asm_ctx_t *ctx, expr_t *expr, gen_var_t *cond_var, char *l_true, char *l_false);
// Generate a jump to a label.
void px_jump(asm_ctx_t *ctx, char *label);

// Bump a register to the top of the usage list.
void px_touch_reg(asm_ctx_t *ctx, reg_t regno);
// Pick a register to use.
reg_t px_pick_reg(asm_ctx_t *ctx, bool do_vacate);
// Pick a register to use, but only pick empty registers.
bool px_pick_empty_reg(asm_ctx_t *ctx, reg_t *regno, address_t size);

// Gets or adds a temp var.
// Each temp label represents one word, so some variables will use multiple.
gen_var_t *px_get_tmp(asm_ctx_t *ctx, size_t size, bool allow_reg);
// Gets the constant required for a stack indexing memory access.
address_t px_get_depth(asm_ctx_t *ctx, gen_var_t *var, address_t var_offs) __attribute__((pure));
// Grab an addressing mode for a parameter.
reg_t px_addr_var(asm_ctx_t *ctx, gen_var_t *var, address_t part, px_addr_t *addrmode, asm_label_t *label, address_t *offs, reg_t dest);
