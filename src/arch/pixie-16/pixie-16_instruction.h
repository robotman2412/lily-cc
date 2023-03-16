
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
// #define OFFS(ctx)      (DET_PIE(ctx) ? ASM_LABEL_REF_OFFS_PTR : ASM_LABEL_REF_ABS_PTR)
// Macro for inverting the branch condition of a branch instruction.
#define INV_BR(insn)   ((insn) ^ 0x0008)

// Function for packing an instruction.
memword_t px_pack_insn(px_insn_t insn) __attribute__((pure));
// Function for unpacking an instruction.
px_insn_t px_unpack_insn(memword_t packed) __attribute__((pure));
