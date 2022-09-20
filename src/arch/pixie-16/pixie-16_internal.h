
#ifndef PIXIE_16_INTERNAL_H
#define PIXIE_16_INTERNAL_H

#include "pixie-16_gen.h"
#include "gen_util.h"
#include "definitions.h"
#include "asm.h"
#include "malloc.h"
#include "string.h"


#ifdef ENABLE_DEBUG_LOGS
static char *c2_insn_names[] = {
	"ADD", "SUB", "CMP",  "AND", "OR",  "XOR", "???", "???",
};
static char *c1_insn_names[] = {
	"INC", "DEC", "CMP1", "???", "???", "???", "SHL", "SHR",
};
static char *addr_names[] = {
	"R0+", "R1+", "R2+", "R3+",
	"ST+", "",    "PC~", "",
};
static char *b_insn_names[] = {
	".ULT", ".UGT", ".SLT", ".SGT", ".EQ", ".CS", "",     "",
	".UGE", ".ULE", ".SGE", ".SLE", ".NE", ".CC", ".JSR", ".CX",
};
static void PX_DESC_INSN(px_insn_t insn, char *imm0, char *imm1) {
	if (!imm0) imm0 = "???";
	if (!imm1) imm1 = "???";
	if (insn.a < 7) imm0 = reg_names[insn.a];
	if (insn.b < 7) imm1 = reg_names[insn.b];
	
	// Determine instruction name.
	bool  is_math1  = false;
	char *name;
	char *suffix;
	if (insn.o < 020) {
		// MATH2 instructions.
		name     = c2_insn_names[insn.o & 007];
		suffix   = (insn.o & 010) ? "C" : "";
	} else if (insn.o < 040) {
		// MATH1 instructions.
		name     = c1_insn_names[insn.o & 007];
		suffix   = (insn.o & 010) ? "C" : "";
		is_math1 = true;
	} else {
		// MOV and LEA.
		name     = (insn.o & 020) ? "LEA" : "MOV";
		suffix   = b_insn_names[insn.o & 017];
	}
	
	// Find the operand with addressing mode.
	char *tmp;
	char *git;
	if (insn.y) {
		tmp  = xalloc(global_alloc, strlen(imm1) + 6);
		git  = imm1;
		imm1 = tmp;
	} else {
		tmp  = xalloc(global_alloc, strlen(imm0) + 6);
		git  = imm0;
		imm0 = tmp;
	}
	
	// Insert addressing mode.
	if (insn.x == 5) {
		sprintf(tmp, "[%s]", git);
	} else if (insn.x == 7) {
		strcpy(tmp, git);
	} else {
		sprintf(tmp, "[%s%s]", addr_names[insn.x], git);
	}
	
	// Print the final thing.
	if (is_math1) {
		DEBUG_GEN("  %s%s %s\n", name, suffix, imm0);
	} else {
		DEBUG_GEN("  %s%s %s, %s\n", name, suffix, imm0, imm1);
	}
	xfree(global_alloc, tmp);
}
#else
static inline void PX_DESC_INSN(px_insn_t insn, char *imm0, char *imm1) {}
#endif

#endif //PIXIE_16_INTERNAL_H