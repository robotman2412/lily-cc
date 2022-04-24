
#ifndef  PIXIE_16_GEN_H
#define  PIXIE_16_GEN_H

#include <gen.h>

/* ======= Gen-specific helper definitions ======= */

// Representation of an instruction.
typedef struct {
	bool     y;
	uint8_t  x;
	uint8_t  b;
	uint8_t  a;
	uint8_t  o;
} px_insn_t;

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
// Reserved for BRK instruction.
#define COND_BRK  007
// Unsigned greater than or equal.
#define COND_UGE  010
// Unsigned less than or equal.
#define COND_ULE  010
// Signed greater than or equal.
#define COND_SGE  010
// Signed less than or equal.
#define COND_SLE  010
// Not equal.
#define COND_NE   010
// Unsigned carry not set.
#define COND_CC   010
// Reserved for JSR instruction.
#define COND_JSR  010
// Reserved for RTI instruction.
#define COND_RTI  010

// Address by memory.
#define ADDR_MEM 5
// Address by register or imm.
#define ADDR_IMM 7
// Address by register offset.
#define ADDR_REG(regno) (regno)
#define ADDR_R0 ADDR_REG(REG_R0)
#define ADDR_R1 ADDR_REG(REG_R0)
#define ADDR_R2 ADDR_REG(REG_R0)
#define ADDR_R3 ADDR_REG(REG_R0)
#define ADDR_ST ADDR_REG(REG_R0)
#define ADDR_PC ADDR_REG(REG_R0)

// Unconditional MOV instructions.
#define PX_OP_MOV 046
// Unconditional LEA instructions.
#define PX_OP_LEA 066

// Return: MOV PC, [ST]
#define INSN_RET 0xd9a6
// Jump: MOV PC, imm
#define INSN_JMP 0x7fa6
// Jump (PIE): LEA PC, [PC+imm]
#define INSN_JMP_PIE 0xefb6

// Generates .bss labels for variables and temporary variables in a function.
void       px_gen_var    (asm_ctx_t *ctx, funcdef_t *func);
// Gets or adds a temp var.
char      *px_get_tmp    (asm_ctx_t *ctx, size_t     size);

#endif // PIXIE_16_GEN_H
