
#ifndef  GR8CPU_R3_GEN_H
#define  GR8CPU_R3_GEN_H

#include <gen.h>

// Creates a set of branch instructions for the given conditions.
void       r3_branch     (asm_ctx_t *ctx, gen_var_t *cond, char *l_true, char *l_false);

// Moves a byte of the variable into the given register.
void       r3_load_part  (asm_ctx_t *ctx, gen_var_t *var,  uint8_t regno, uint8_t offs);
// Moves the given register into a byte of the variable.
void       r3_store_part (asm_ctx_t *ctx, gen_var_t *var,  uint8_t regno, uint8_t offs);
// Moves a long into the registers X and Y.
// Used before function entry to functions which take exactly one two-byte integer.
gen_var_t *r3_movl_to_mem(asm_ctx_t *ctx, char      *label);
// Moves a long into memory.
// Used before function return from functions which return exactly one two-byte integer.
void       r3_movl_to_reg(asm_ctx_t *ctx, gen_var_t *var);

// Dereference a pointer.
gen_var_t *r3_deref      (asm_ctx_t *ctx, gen_var_t *out,  gen_var_t *ptr);
// Assign a value to the pointer.
gen_var_t *r3_deref_set  (asm_ctx_t *ctx, gen_var_t *dst,  gen_var_t *src);

// Performs a bit shifting operation
gen_var_t *r3_shift      (asm_ctx_t *ctx, bool       left, gen_var_t *out_hint, gen_var_t *a, int16_t amount);
// Performs a binary math operation for numbers of one byte.
gen_var_t *r3_math2      (asm_ctx_t *ctx, oper_t     oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b, bool is_comp);
// Performs a binary math operation for numbers two bytes or larger.
gen_var_t *r3_math2_l    (asm_ctx_t *ctx, oper_t     oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b, bool is_comp);
// Performs a unary math operation for numbers of one byte.
gen_var_t *r3_math1      (asm_ctx_t *ctx, oper_t     oper, gen_var_t *out_hint, gen_var_t *a);
// Performs a unary math operation for numbers two bytes or larger.
gen_var_t *r3_math1_l    (asm_ctx_t *ctx, oper_t     oper, gen_var_t *out_hint, gen_var_t *a);

// Generates .bss labels for variables and temporary variables in a function.
void       r3_gen_var    (asm_ctx_t *ctx, funcdef_t *func);
// Gets or adds a temp var.
char      *r3_get_tmp    (asm_ctx_t *ctx);

#endif // GR8CPU_R3_GEN_H
