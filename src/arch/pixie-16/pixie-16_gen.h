
#ifndef  PIXIE_16_GEN_H
#define  PIXIE_16_GEN_H

#include <gen.h>
#include "pixie-16_instruction.h"

/* ======= Gen-specific helper definitions ======= */

// Creates a branch condition from a variable.
cond_t px_var_to_cond(asm_ctx_t *ctx, expr_t *expr, gen_var_t *var);
// Writes logical AND/OR code for jumping to labels.
// Flow type indicates what condition happens on flow through.
void px_logic(asm_ctx_t *ctx, expr_t *expr, asm_label_t l_true, asm_label_t l_false, bool flow_type);

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

// Determine the calling conventions to use.
void px_update_cc(asm_ctx_t *ctx, funcdef_t *funcdef);

#endif // PIXIE_16_GEN_H
