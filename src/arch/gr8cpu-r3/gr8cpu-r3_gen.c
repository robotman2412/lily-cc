
#include "gr8cpu-r3_gen.h"
#include "gen_util.h"
#include "definitions.h"
#include "asm.h"
#include "malloc.h"
#include "string.h"

/* ======= Gen-specific helper definitions ======= */

#define OFFS_BRANCH		0x0F
#define OFFS_B_EQ       0x00
#define OFFS_B_GT       0x02
#define OFFS_B_LT       0x04
#define OFFS_B_CS       0x06
#define OFFS_B_INV      0x01

#define INSN_JMP		0x0E
#define INSN_BEQ        0x0F
#define INSN_BNE        0x10
#define INSN_BGT        0x11
#define INSN_BLE        0x12
#define INSN_BLT        0x13
#define INSN_BGE        0x14
#define INSN_BCS        0x15
#define INSN_BCC        0x16

#define OFFS_PIE		0x80

// Simple math: A, X & Y
#define OFFS_ADD		0x32
#define OFFS_SUB		0x34
#define OFFS_CMP		0x36

#define OFFS_CALC_AX	0x00
#define OFFS_CALC_AY	0x01
#define OFFS_CALC_AV	0x06
#define OFFS_CALC_AM	0x07
#define OFFS_CALC_XV	0x2A
#define OFFS_CALC_XM	0x2B
#define OFFS_CALC_YV	0x32
#define OFFS_CALC_YM	0x33
#define OFFS_CALC_CC	0x0C

// Incrementation: A, X, Y & mem
#define INSN_INC_A		0x3E
#define INSN_DEC_A		0x40
#define INSN_INC_M		0x3F
#define INSN_DEC_M		0x41
#define INSN_INC_X		0x62
#define INSN_DEC_X		0x63
#define INSN_INC_Y		0x6A
#define INSN_DEC_Y		0x6B

#define INSN_INCC_A		0x4A
#define INSN_DECC_A		0x4C
#define INSN_INCC_M		0x3B
#define INSN_DECC_M		0x4D

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
#define INSN_MOV_AX		0x17
#define INSN_MOV_AY		0x18
#define INSN_MOV_XA		0x19
#define INSN_MOV_XY		0x1A
#define INSN_MOV_YA		0x1B
#define INSN_MOV_YX		0x1C
#define INSN_MOV_AI		0x1D
#define INSN_MOV_XI		0x1E
#define INSN_MOV_YI		0x1F

#define OFFS_MOV_RI		0x1D

// Memory moves
#define OFFS_MOVLD		0x20
#define OFFS_MOVST		0x29
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
#define OFFS_PUSHR		0x04
#define OFFS_PULLR		0x09
#define INSN_PUSHI		0x07
#define INSN_PUSHM		0x08
#define INSN_POP		0x0C

// Subroutines
#define INSN_BRK		0x01
#define INSN_BKI		0x00
#define INSN_CALL		0x02
#define INSN_RET		0x03
#define INSN_RTI		0x74

// Macro for determining PIE.
#define DET_PIE(ctx) 1
// Macro for determining PIE and applying OFFS_PIE based on context.
#define PIE(ctx)     (DET_PIE(ctx) ? OFFS_PIE : 0)
// Macro for determining PIE and applying relative addressing based on context.
#define OFFS(ctx)    (DET_PIE(ctx) ? ASM_LABEL_REF_OFFS_PTR : ASM_LABEL_REF_ABS_PTR)
// Macro for inverting the branch condition of a branch instruction.
#define INV_BR(insn) (((insn - OFFS_BRANCH) ^ 1) + OFFS_BRANCH)

#ifdef ENABLE_DEBUG_LOGS
char *b_insn_names[] = {"BEQ", "BNE", "BGT", "BLE", "BLT", "BGE", "BCS", "BCC"};
#endif

/* ======== Gen-specific helper functions ======== */

inline void r3_branch_to_var(asm_ctx_t *ctx, uint8_t b_insn, gen_var_t *output) {
    uint8_t regno   = REG_A;
    uint8_t n_words = 2;
    // Helper var.
    gen_var_t helper = {
        .type = VAR_TYPE_CONST,
        .iconst = 0
    };
    // Write coditional assignment.
    char *l_true = asm_get_label(ctx);
    char *l_skip = asm_get_label(ctx);
    DEBUG_GEN("  %s %s\n", b_insn_names[b_insn - OFFS_BRANCH - PIE(ctx)], l_true);
    asm_write_memword(ctx, b_insn);
    asm_write_label_ref(ctx, l_true, 0, OFFS(ctx));
    // Code for false.
    r3_load_part(ctx, &helper, regno, 0);
    DEBUG_GEN("  JMP %s\n", l_skip);
    asm_write_memword(ctx, INSN_JMP);
    asm_write_label_ref(ctx, l_skip, 0, OFFS(ctx));
    // Code for true.
    helper.iconst = 1;
    asm_write_label(ctx, l_true);
    r3_load_part(ctx, &helper, regno, 0);
    // Skip label.
    asm_write_label(ctx, l_skip);
    // Storage code.
    r3_store_part(ctx, output, regno, 0);
    r3_load_part(ctx, &helper, regno, 1);
    for (uint8_t i = 1; i < n_words; i++)
        r3_store_part(ctx, output, regno, i);
}

// Creates a set of branch instructions for the given conditions.
void r3_branch(asm_ctx_t *ctx, gen_var_t *cond, char *l_true, char *l_false) {
    uint8_t b_insn;
    if (cond->type == VAR_TYPE_LABEL) {
        uint8_t n_words = 2;
        char *label = cond->label;
        // Load first byte.
        DEBUG_GEN("  MOV A, [%s+0]\n", label);
        asm_write_memword(ctx, OFFS_MOVLD + OFFS_MOVM_AM + PIE(ctx));
        asm_write_label_ref(ctx, label, 0, OFFS(ctx));
        // OR the following bytes.
        for (int i = 1; i < n_words; i++) {
            DEBUG_GEN("  OR A, [%s+%d]\n", label, i);
            asm_write_memword(ctx, OFFS_BIT_AM + OFFS_OR + PIE(ctx));
            asm_write_label_ref(ctx, label, i, OFFS(ctx));
        }
        // The branch instruction is now BNE.
        b_insn = INSN_BNE;
    } else if (cond->type == VAR_TYPE_COND) {
        // Take the branch instruction from the condition.
        b_insn = cond->cond;
    }
    
    if (l_true) {
        // Non-inverted branch.
        DEBUG_GEN("  %s %s\n", b_insn_names[(b_insn & 0x7f) - OFFS_BRANCH], l_true);
        asm_write_memword(ctx, b_insn | PIE(ctx));
        asm_write_label_ref(ctx, l_true, 0, OFFS(ctx));
        if (l_false) {
            // With else label.
            DEBUG_GEN("  JMP %s\n", l_false);
            asm_write_memword(ctx, INSN_JMP + PIE(ctx));
            asm_write_label_ref(ctx, l_false, 0, OFFS(ctx));
        }
    } else if (l_false) {
        // Inverted branch.
        b_insn = INV_BR(b_insn);
        DEBUG_GEN("  %s %s\n", b_insn_names[(b_insn & 0x7f) - OFFS_BRANCH], l_false);
        asm_write_memword(ctx, b_insn | PIE(ctx));
        asm_write_label_ref(ctx, l_false, 0, OFFS(ctx));
    }
}

// Moves a byte of the variable into the given register.
void r3_load_part(asm_ctx_t *ctx, gen_var_t *var, uint8_t regno, uint8_t offs) {
    switch (var->type) {
        case VAR_TYPE_CONST:
            DEBUG_GEN("  MOV %s, 0x%02x\n", reg_names[regno], var->iconst >> (offs * 8));
            // Correct "MOV reg, val" instruction.
            asm_write_memword(ctx, OFFS_MOV_RI + regno);
            // Selected byte of value.
            asm_write_memword(ctx, var->iconst >> (offs * 8));
            break;
        case VAR_TYPE_LABEL:
            DEBUG_GEN("  MOV %s, [%s+%d]\n", reg_names[regno], var->label, offs);
            // Correct "MOV reg, [label+offs]" instruction.
            asm_write_memword(ctx, OFFS_MOVLD + OFFS_MOVM_RM + regno + PIE(ctx));
            // Label reference.
            asm_write_label_ref(ctx, var->label, offs, OFFS(ctx));
            break;
    }
}

// Moves the given register into a byte of the variable.
void r3_store_part(asm_ctx_t *ctx, gen_var_t *var, uint8_t regno, uint8_t offs) {
    if (var->type == VAR_TYPE_RETVAL) {
        // TODO: replace it if not possible.
        // Correct MOV reg, A instruction.
        DEBUG_GEN("  MOV %s, A\n", reg_names[offs + REG_X]);
        asm_write_memword(ctx, offs ? INSN_MOV_YA : INSN_MOV_XA);
    }
    if (var->type == VAR_TYPE_LABEL) {
        // Correct "MOV [label+offs], reg" instruction.
        DEBUG_GEN("  MOV [%s+%d], %s\n", var->label, offs, reg_names[regno]);
        asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_RM + regno + PIE(ctx));
        // Label reference.
        asm_write_label_ref(ctx, var->label, offs, OFFS(ctx));
    }
}

// Moves a long into the registers X and Y.
// Used before function entry to functions which take exactly one two-byte integer.
gen_var_t *r3_movl_to_mem(asm_ctx_t *ctx, char *label) {
    // Move X to memory.
    DEBUG_GEN("  MOV [%s+0], X\n", label);
    asm_write_memword  (ctx, OFFS_MOVST + OFFS_MOVM_XM + PIE(ctx));
    asm_write_label_ref(ctx, label, 0, OFFS(ctx));
    // Move Y to memory.
    DEBUG_GEN("  MOV [%s+0], Y\n", label);
    asm_write_memword  (ctx, OFFS_MOVST + OFFS_MOVM_YM + PIE(ctx));
    asm_write_label_ref(ctx, label, 1, OFFS(ctx));
}

// Moves a long into memory.
// Used before function return from functions which return exactly one two-byte integer.
void r3_movl_to_reg(asm_ctx_t *ctx, gen_var_t *var) {
    if (var->type == VAR_TYPE_LABEL) {
        // Label-ish.
        char *label = var->label;
        // Move memory to X.
        DEBUG_GEN("  MOV X, [%s+0]\n", label);
        asm_write_memword  (ctx, OFFS_MOVLD + OFFS_MOVM_XM + PIE(ctx));
        asm_write_label_ref(ctx, label, 0, OFFS(ctx));
        // Move memory to Y.
        DEBUG_GEN("  MOV Y, [%s+1]\n", label);
        asm_write_memword  (ctx, OFFS_MOVLD + OFFS_MOVM_YM + PIE(ctx));
        asm_write_label_ref(ctx, label, 1, OFFS(ctx));
    } else if (var->type == VAR_TYPE_CONST) {
        // The constant.
        address_t iconst = var->iconst;
        // Move memory to X.
        DEBUG_GEN("  MOV X, %02x\n", iconst & 255);
        asm_write_memword  (ctx, OFFS_MOV_RI + REG_X);
        asm_write_memword  (ctx, iconst & 255);
        // Move memory to Y.
        DEBUG_GEN("  MOV Y, %02x\n", iconst >> 8);
        asm_write_memword  (ctx, OFFS_MOV_RI + REG_Y);
        asm_write_memword  (ctx, iconst >> 8);
    }
}

// Move data from src to dst.
void r3_mov(asm_ctx_t *ctx, gen_var_t *dst, gen_var_t *src) {
    uint8_t regno = REG_A;
    uint8_t n_words = 2;
    if (src->type == VAR_TYPE_COND) {
        // Have it converted.
        r3_branch_to_var(ctx, src->cond, dst);
    } else {
        // Simple copy will do.
        for (uint8_t i = 0; i < n_words; i++) {
            r3_load_part (ctx, src, regno, i);
            r3_store_part(ctx, dst, regno, i);
        }
    }
}

// Performs a bit shifting operation
gen_var_t *r3_shift(asm_ctx_t *ctx, bool left, gen_var_t *output, gen_var_t *a, int16_t amount) {
    // Correct direction.
    if (amount < 0) {
        amount = -amount;
        left  ^= 1;
    }
    uint8_t n_words = 2;
    // If it exceeds n_words, don't bother.
    if (amount > n_words * 8) {
        gen_var_t zero = {
            .type   = VAR_TYPE_CONST,
            .iconst = 0
        };
        r3_mov(ctx, output, &zero);
        return output;
    }
    
    // If there's no output hint, create one ourselves.
    if (!output) {
        output = (gen_var_t *) malloc(sizeof(gen_var_t));
        *output = (gen_var_t) {
            .label = "o",
            .type  = VAR_TYPE_LABEL
        };
    }
    
    if (output->type == VAR_TYPE_COND) {
        // TODO: Use a temporary variable.
    } else if (output != a) {
        // If output != input, copy to output first.
        r3_mov(ctx, output, a);
    }
    
    /*if (amount >= 8) {
        // A byte or more shifted.
        uint8_t bytes = amount >> 3;
        uint8_t bits  = amount &  7;
        uint8_t insn  = left ? OFFS_SHM + OFFS_SHM_L : OFFS_SHM + OFFS_SHM_R;
    } else */{
        // Less than a byte shifted.
        oper_t oper = left ? OP_SHIFT_L : OP_SHIFT_R;
        for (int16_t i = 0; i < amount; i++)
            r3_math1_l(ctx, oper, output, output);
    }
    return output;
}

// Performs a binary math operation for numbers of one byte.
gen_var_t *r3_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b, bool is_comp);

// Performs a binary math operation for numbers two bytes or larger.
gen_var_t *r3_math2_l(asm_ctx_t *ctx, oper_t oper, gen_var_t *output, gen_var_t *a, gen_var_t *b, bool is_comp) {
    // Number of words to operate on.
    uint8_t n_words  = 2;
    // Instruction for the occasion.
    uint8_t insn     = 0;
    // Branch instruction for comparisons.
    uint8_t b_insn   = 0;
    // Used to determine patterns used.
    bool is_b_const  = b->type == VAR_TYPE_CONST;
    // False for AND, OR, XOR and alike.
    bool does_cc     = 0;
    // If there's no output hint, create one ourselves.
    if (!output) {
        output = (gen_var_t *) malloc(sizeof(gen_var_t));
        *output = (gen_var_t) {
            .label = "o",
            .type  = VAR_TYPE_LABEL
        };
    }
#ifdef DEBUG_GENERATOR
    char *insn_name = "???";
#endif
    switch (oper) {
        case OP_ADD:
            insn = OFFS_ADD + (is_b_const ? OFFS_CALC_AV : OFFS_CALC_AM + PIE(ctx));
            does_cc     = 1;
#ifdef DEBUG_GENERATOR
            insn_name = "ADD";
#endif
            break;
        case OP_SUB:
            insn = OFFS_SUB + (is_b_const ? OFFS_CALC_AV : OFFS_CALC_AM + PIE(ctx));
            does_cc     = 1;
#ifdef DEBUG_GENERATOR
            insn_name = "SUB";
#endif
            break;
        case OP_EQ: b_insn = INSN_BEQ; goto cmp;
        case OP_NE: b_insn = INSN_BNE; goto cmp;
        case OP_LT: b_insn = INSN_BLT; goto cmp;
        case OP_GE: b_insn = INSN_BGE; goto cmp;
        case OP_GT: b_insn = INSN_BGT; goto cmp;
        case OP_LE: b_insn = INSN_BLE; goto cmp;
            cmp:
            is_comp = true;
            b_insn += PIE(ctx);
            insn = OFFS_CMP + (is_b_const ? OFFS_CALC_AV : OFFS_CALC_AM + PIE(ctx));
            does_cc     = 1;
#ifdef DEBUG_GENERATOR
            insn_name = "CMP";
#endif
            break;
        case OP_BIT_AND:
            insn = OFFS_AND + (is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM  + PIE(ctx));
#ifdef DEBUG_GENERATOR
            insn_name = "AND";
#endif
            break;
        case OP_BIT_OR:
            insn = OFFS_OR + (is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM  + PIE(ctx));
#ifdef DEBUG_GENERATOR
            insn_name = "OR";
#endif
            break;
        case OP_BIT_XOR:
            insn = OFFS_XOR + (is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM  + PIE(ctx));
#ifdef DEBUG_GENERATOR
            insn_name = "XOR";
#endif
            break;
    }
    uint8_t regno = REG_A;
    for (uint8_t i = 0; i < n_words; i++) {
        // Load part of the thing.
        r3_load_part(ctx, a, regno, i);
        // Add the instruction.
        if (is_b_const) {
            // Constant.
            DEBUG_GEN("  %s%s %s, 0x%02x\n", insn_name, (i && does_cc ? "C" : ""), reg_names[regno], b->iconst >> (i * 8));
            asm_write_memword(ctx, insn + (i && does_cc ? OFFS_CALC_CC : 0));
            asm_write_memword(ctx, b->iconst >> (i * 8));
        } else {
            // Label reference.
            DEBUG_GEN("  %s%s %s, [%s+%d]\n", insn_name, (i && does_cc ? "C" : ""), reg_names[regno], b->label, i);
            asm_write_memword(ctx, insn + (i && does_cc ? OFFS_CALC_CC : 0));
            asm_write_label_ref(ctx, b->label, i, OFFS(ctx));
        }
        if (!is_comp) {
            // Store the result.
            r3_store_part(ctx, output, regno, i);
        }
    }
    if (is_comp) {
        if (output->type == VAR_TYPE_COND) {
            // Condition.
            output->cond = b_insn;
        } else {
            // Have it converted.
            r3_branch_to_var(ctx, b_insn, output);
        }
    }
    return output;
}

// Performs a unary math operation for numbers of one byte.
gen_var_t *r3_math1(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a);

// Performs a unary math operation for numbers two bytes or larger.
gen_var_t *r3_math1_l(asm_ctx_t *ctx, oper_t oper, gen_var_t *output, gen_var_t *a) {
    bool use_mem     = a == output || oper == OP_SHIFT_L || oper == OP_SHIFT_R;
    // Number of words to operate on.
    uint8_t n_words  = 2;
    // Instruction for the occasion.
    uint8_t insn     = 0;
    uint8_t insn_cc  = 0;
    // If there's no output hint, create one ourselves.
    if (!output) {
        output = (gen_var_t *) malloc(sizeof(gen_var_t));
        *output = (gen_var_t) {
            .label = "o",
            .type  = VAR_TYPE_LABEL
        };
    }
#ifdef DEBUG_GENERATOR
    char *insn_name = "???";
    char *insn_cc_name = "???";
#endif
    // Find the correct instruction.
    switch (oper) {
        case OP_ADD:
            insn      = use_mem ? INSN_INC_M  + PIE(ctx) : INSN_INC_A;
            insn_cc   = use_mem ? INSN_INCC_M + PIE(ctx) : INSN_INCC_A;
#ifdef DEBUG_GENERATOR
            insn_name    = "INC";
            insn_cc_name = "INCC";
#endif
            break;
        case OP_SUB:
            insn      = use_mem ? INSN_INC_M  + PIE(ctx) : INSN_INC_A;
            insn_cc   = use_mem ? INSN_INCC_M + PIE(ctx) : INSN_INCC_A;
#ifdef DEBUG_GENERATOR
            insn_name    = "INC";
            insn_cc_name = "INCC";
#endif
            break;
        case OP_SHIFT_L:
            insn      = OFFS_SHM + OFFS_SHM_L + PIE(ctx);
            insn_cc   = OFFS_SHM + OFFS_SHM_L + OFFS_SHM_CC + PIE(ctx);
#ifdef DEBUG_GENERATOR
            insn_name    = "SHL";
            insn_cc_name = "SHLC";
#endif
            break;
        case OP_SHIFT_R:
            insn      = OFFS_SHM + OFFS_SHM_R + PIE(ctx);
            insn_cc   = OFFS_SHM + OFFS_SHM_R + OFFS_SHM_CC + PIE(ctx);
#ifdef DEBUG_GENERATOR
            insn_name    = "SHR";
            insn_cc_name = "SHRC";
#endif
            break;
    }
    if (use_mem) {
        // Perform on memory.
    } else {
        // Perform on registers.
    }
}

/* ================== Functions ================== */

// Function entry for non-inlined functions. 
void gen_function_entry(asm_ctx_t *ctx, funcdef_t *funcdef) {
    if (funcdef->args.num == 1 /*&& funcdef->args.arr[0].size == 2*/) {
        // Exactly one long for an argument.
        // Define label.
        char *sect_id = ctx->current_section_id;
        asm_use_sect(ctx, ".bss", ASM_NOT_ALIGNED);
        char *label = malloc(strlen(funcdef->ident.strval) + 8);
        sprintf(label, "%s.LA0000", funcdef->ident.strval);
        asm_write_label(ctx, label);
        asm_write_zero (ctx, 2);
        gen_define_var (ctx, strdup(label), funcdef->args.arr[0].strval);
        r3_gen_var     (ctx, funcdef);
        
        // Go back to the original section the function was in.
        asm_use_sect(ctx, sect_id, ASM_NOT_ALIGNED);
        // Add the entry label.
        asm_write_label(ctx, funcdef->ident.strval);
        
        // Move over the value.
        DEBUG_GEN("  MOV [%s+0], X\n", label);
        asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_XM + PIE(ctx));
        asm_write_label_ref(ctx, label, 0, OFFS(ctx));
        DEBUG_GEN("  MOV [%s+1], Y\n", label);
        asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_YM + PIE(ctx));
        asm_write_label_ref(ctx, label, 1, OFFS(ctx));
        
        free(label);
    } else {
        // Arguments in memory.
        // Define labels.
        char *sect_id = ctx->current_section_id;
        asm_use_sect(ctx, ".bss", ASM_NOT_ALIGNED);
        char *label = malloc(strlen(funcdef->ident.strval) + 8);
        for (address_t i = 0; i < funcdef->args.num; i++) {
            sprintf(label, "%s.LA%04x", funcdef->ident.strval, i);
            asm_write_label(ctx, label);
            asm_write_zero (ctx, 2);
            gen_define_var (ctx, strdup(label), funcdef->args.arr[i].strval);
        }
        r3_gen_var(ctx, funcdef);
        
        // Go back to the original section the function was in.
        asm_use_sect(ctx, sect_id, ASM_NOT_ALIGNED);
        // Add the entry label.
        asm_write_label(ctx, funcdef->ident.strval);
        
        free(label);
    }
}

// Return statement for non-inlined functions.
// retval is null for void returns.
void gen_return(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval) {
    if (retval) {
        // Gimme value.
        r3_movl_to_reg(ctx, retval);
    }
    // Append the return.
    if (/*func is interrupt handler?*/0) {
        DEBUG_GEN("  RTI\n");
        asm_write_memword(ctx, INSN_RTI);
    } else {
        DEBUG_GEN("  RET\n");
        asm_write_memword(ctx, INSN_RET);
    }
}

/* ================== Statements ================= */

// If statement implementation.
bool gen_if(asm_ctx_t *ctx, gen_var_t *cond, stmt_t *s_if, stmt_t *s_else) {
    if (s_else) {
        // Write the branch.
        char *l_true = asm_get_label(ctx);
        char *l_skip;
        r3_branch(ctx, cond, l_true, NULL);
        // True:
        bool if_explicit = gen_stmt(ctx, s_if, false);
        if (!if_explicit) {
            // Don't insert a dead jump.
            l_skip = asm_get_label(ctx);
            DEBUG_GEN("  JMP %s\n", l_skip);
            asm_write_memword(ctx, INSN_JMP);
            asm_write_label_ref(ctx, l_skip, 0, OFFS(ctx));
        }
        // False:
        asm_write_label(ctx, l_true);
        bool else_explicit = gen_stmt(ctx, s_else, false);
        // Skip label.
        if (!if_explicit) {
            // Don't add a useless label.
            asm_write_label(ctx, l_skip);
        }
        return if_explicit && else_explicit;
    } else {
        // Write the branch.
        char *l_skip = asm_get_label(ctx);
        r3_branch(ctx, cond, NULL, l_skip);
        // True:
        gen_stmt(ctx, s_if, false);
        // Skip label.
        asm_write_label(ctx, l_skip);
        return false;
    }
}

// While statement implementation.
bool gen_while(asm_ctx_t *ctx, expr_t *cond, stmt_t *code, bool do_while) {
    do_while = false;
    char *expr_label;
    char *loop_label = asm_get_label(ctx);
    if (!do_while) {
        // Skip first expression checking in do...while loops.
        expr_label = asm_get_label(ctx);
        DEBUG_GEN("  JMP %s\n", expr_label);
        asm_write_memword(ctx, INSN_JMP + PIE(ctx));
        asm_write_label_ref(ctx, expr_label, 0, OFFS(ctx));
    }
    // Write code.
    asm_write_label(ctx, loop_label);
    bool explicit = gen_stmt(ctx, code, false);
    // Loop and condition check.
    if (!do_while) {
        asm_write_label(ctx, expr_label);
    }
    gen_var_t cond_hint = {
        .type = VAR_TYPE_COND
    };
    gen_var_t *cond_res = gen_expression(ctx, cond, &cond_hint);
    // Perform branch.
    r3_branch(ctx, cond_res, loop_label, NULL);
    return (do_while && explicit) || (cond->type == VAR_TYPE_CONST && cond->iconst);
}

/* ================= Expressions ================= */

// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *callee, size_t n_args, gen_var_t **args) {
    
}

// Expression: Binary math operation.
gen_var_t *gen_expr_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b) {
    if (oper == OP_SHIFT_L || oper == OP_SHIFT_R) {
        if (b->type == VAR_TYPE_CONST) {
            r3_shift(ctx, oper == OP_SHIFT_L, out_hint, a, b->iconst);
            // TODO: Have this done by a function.
        }
    } else if (oper == OP_MUL || oper == OP_DIV || oper == OP_MOD) {
        // TODO: Have this done by a function.
    } else {
        r3_math2_l(ctx, oper, out_hint, a, b, out_hint->type == VAR_TYPE_COND);
    }
}

// Expression: Unary math operation.
gen_var_t *gen_expr_math1(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a) {
    return r3_math1_l(ctx, oper, out_hint, a);
}

// Miscellaneous: Move variable to another location.
void gen_mov(asm_ctx_t *ctx, gen_var_t *dest, gen_var_t *src) {
    // Check whether locations are equivalent.
    if (dest->type == VAR_TYPE_LABEL && src->type == VAR_TYPE_LABEL && !strcmp(dest->label, src->label)) {
        return;
    }
    // TODO: MOV, register edition.
    uint8_t n_words = 2;
    uint8_t regno = REG_A;
    for (uint8_t i = 0; i < n_words; i++) {
        r3_load_part(ctx, src, regno, i);
        r3_store_part(ctx, dest, regno, i);
    }
}

// Generates .bss labels for variables and temporary variables in a function.
void r3_gen_var(asm_ctx_t *ctx, funcdef_t *func) {
    // TODO: A per-scope implementation.
    preproc_data_t *data = func->preproc;
    for (size_t i = 0; i < map_size(data->vars); i++) {
        char *label = (char *) data->vars->values[i];
        gen_define_var(ctx, label, data->vars->strings[i]);
        asm_write_label(ctx, label);
        asm_write_zero(ctx, 2);
    }
}

// Variables: Create a variable based on other value.
// Other value is null if not initialised.
void gen_var_dup(asm_ctx_t *ctx, funcdef_t *funcdef, ident_t *ident, gen_var_t *other) {
    // TODO: Account for other value.
}

// Variables: Create a label for the varialbe at preprocessing time.
char *gen_preproc_var(asm_ctx_t *ctx, preproc_data_t *parent, ident_t *ident) {
    char *fn_label = ctx->current_func->ident.strval;
    char *label = malloc(strlen(fn_label) + 8);
    sprintf(label, "%s.LV%04lx", fn_label, ctx->current_scope->local_num);
    return label;
}
