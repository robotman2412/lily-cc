
#ifndef  GR8CPU_R3_GEN_H
#define  GR8CPU_R3_GEN_H


/* ======= Gen-specific helper definitions ======= */

#define OFFS_BRANCH     0x0F
#define OFFS_B_EQ       0x00
#define OFFS_B_GT       0x02
#define OFFS_B_LT       0x04
#define OFFS_B_CS       0x06
#define OFFS_B_INV      0x01

#define INSN_JMP        0x0E
#define INSN_BEQ        0x0F
#define INSN_BNE        0x10
#define INSN_BGT        0x11
#define INSN_BLE        0x12
#define INSN_BLT        0x13
#define INSN_BGE        0x14
#define INSN_BCS        0x15
#define INSN_BCC        0x16

#define OFFS_PIE        0x80

// Simple math: A, X & Y
#define OFFS_ADD        0x32
#define OFFS_SUB        0x34
#define OFFS_CMP        0x36

#define OFFS_CALC_AX    0x00
#define OFFS_CALC_AY    0x01
#define OFFS_CALC_AV    0x06
#define OFFS_CALC_AM    0x07
#define OFFS_CALC_XV    0x2A
#define OFFS_CALC_XM    0x2B
#define OFFS_CALC_YV    0x32
#define OFFS_CALC_YM    0x33
#define OFFS_CALC_CC    0x0C

// Incrementation: A, X, Y & mem
#define INSN_INC_A      0x3E
#define INSN_DEC_A      0x40
#define INSN_INC_M      0x3F
#define INSN_DEC_M      0x41
#define INSN_INC_X      0x62
#define INSN_DEC_X      0x63
#define INSN_INC_Y      0x6A
#define INSN_DEC_Y      0x6B

#define INSN_INCC_A     0x4A
#define INSN_DECC_A     0x4C
#define INSN_INCC_M     0x3B
#define INSN_DECC_M     0x4D

// Bitwise operations: AND, OR & XOR
#define OFFS_BIT_AV     0x00
#define OFFS_BIT_AM     0x01
#define OFFS_AND        0x52
#define OFFS_OR         0x54
#define OFFS_XOR        0x56

// Bitwise operations: SHL, SHR
#define OFFS_SHM        0x42
#define OFFS_SHM_L      0x00
#define OFFS_SHM_R      0x01
#define OFFS_SHM_CC     0x0C

#define OFFS_SHA        0x58
#define OFFS_SHA_L      0x00
#define OFFS_SHA_R      0x01
#define OFFS_ROLA       0x02

// Register moves
#define INSN_MOV_AX     0x17
#define INSN_MOV_AY     0x18
#define INSN_MOV_XA     0x19
#define INSN_MOV_XY     0x1A
#define INSN_MOV_YA     0x1B
#define INSN_MOV_YX     0x1C
#define INSN_MOV_AI     0x1D
#define INSN_MOV_XI     0x1E
#define INSN_MOV_YI     0x1F

#define OFFS_MOV_RI     0x1D

// Memory moves
#define OFFS_MOVLD      0x20
#define OFFS_MOVST      0x29
#define OFFS_MOVM_RM	0x00
#define OFFS_MOVM_AM	0x00
#define OFFS_MOVM_XM	0x01
#define OFFS_MOVM_YM	0x02
#define OFFS_MOVM_AMX	0x03
#define OFFS_MOVM_AMY	0x04
#define OFFS_MOVM_AP	0x05
#define OFFS_MOVM_APXY	0x06
#define OFFS_MOVM_APX	0x07
#define OFFS_MOVM_APY	0x08

// Stack operations
#define OFFS_PUSHR      0x04
#define OFFS_PULLR      0x09
#define INSN_PUSHI      0x07
#define INSN_PUSHM      0x08
#define INSN_POP        0x0C

// Subroutines
#define INSN_BRK        0x01
#define INSN_BKI        0x00
#define INSN_CALL       0x02
#define INSN_RET        0x03
#define INSN_RTI        0x74

// Miscellaneous
#define INSN_GPTR       0x7B

// Macro for determining PIE.
#define DET_PIE(ctx)  1
// Macro for determining PIE and applying OFFS_PIE based on context.
#define PIE(ctx)       (DET_PIE(ctx) ? OFFS_PIE : 0)
// Macro for determining PIE and applying relative addressing based on context (pointer).
#define OFFS(ctx)      (DET_PIE(ctx) ? ASM_LABEL_REF_OFFS_PTR : ASM_LABEL_REF_ABS_PTR)
// Macro for determining PIE and applying relative addressing based on context (int).
#define OFFS_W_LO(ctx) (DET_PIE(ctx) ? ASM_LABEL_REF_OFFS_WORD : ASM_LABEL_REF_ABS_WORD)
#define OFFS_W_HI(ctx) (DET_PIE(ctx) ? ASM_LABEL_REF_OFFS_WORD : ASM_LABEL_REF_ABS_WORD_HIGH)
// Macro for inverting the branch condition of a branch instruction.
#define INV_BR(insn)   ((((insn) - OFFS_BRANCH) ^ 1) + OFFS_BRANCH)

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
