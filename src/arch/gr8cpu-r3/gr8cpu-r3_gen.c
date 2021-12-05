
#include "gr8cpu-r3_gen.h"
#include "gen_util.h"
#include "definitions.h"
#include "asm.h"
#include "malloc.h"
#include "string.h"

/* ======= Gen-specific helper definitions ======= */

#define INSN_JMP		0x0E
#define OFFS_BRANCH		0x0F
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

/* ======== Gen-specific helper functions ======== */

// Moves a byte of the variable into the given register.
void r3_load_part(asm_ctx_t *ctx, gen_var_t *var, uint8_t regno, uint8_t offs) {
    switch (var->type) {
        case VAR_TYPE_CONST:
            DEBUG_GEN("MOV %s, 0x%02x\n", reg_names[regno], var->iconst >> (offs * 8));
            // Correct "MOV reg, val" instruction.
            asm_write_memword(ctx, OFFS_MOV_RI + regno);
            // Selected byte of value.
            asm_write_memword(ctx, var->iconst >> (offs * 8));
            break;
        case VAR_TYPE_LABEL:
            DEBUG_GEN("MOV %s, [%s+%d]\n", reg_names[regno], var->label, offs);
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
        DEBUG_GEN("MOV %s, A\n", reg_names[offs + REG_X]);
        asm_write_memword(ctx, offs ? INSN_MOV_YA : INSN_MOV_XA);
    }
    if (var->type == VAR_TYPE_LABEL) {
        // Correct "MOV [label+offs], reg" instruction.
        DEBUG_GEN("MOV [%s+%d], %s\n", var->label, offs, reg_names[regno]);
        asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_RM + regno + PIE(ctx));
        // Label reference.
        asm_write_label_ref(ctx, var->label, offs, OFFS(ctx));
    }
}

// Moves a long into the registers X and Y.
// Used before function entry to functions which take exactly one two-byte integer.
gen_var_t *r3_movl_to_mem(asm_ctx_t *ctx, char *label) {
    // Move X to memory.
    DEBUG_GEN("MOV [%s+0], X\n", label);
    asm_write_memword  (ctx, OFFS_MOVST + OFFS_MOVM_XM + PIE(ctx));
    asm_write_label_ref(ctx, label, 0, OFFS(ctx));
    // Move Y to memory.
    DEBUG_GEN("MOV [%s+0], Y\n", label);
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
        DEBUG_GEN("MOV X, [%s+0]\n", label);
        asm_write_memword  (ctx, OFFS_MOVLD + OFFS_MOVM_XM + PIE(ctx));
        asm_write_label_ref(ctx, label, 0, OFFS(ctx));
        // Move memory to Y.
        DEBUG_GEN("MOV Y, [%s+0]\n", label);
        asm_write_memword  (ctx, OFFS_MOVLD + OFFS_MOVM_YM + PIE(ctx));
        asm_write_label_ref(ctx, label, 1, OFFS(ctx));
    } else if (var->type == VAR_TYPE_CONST) {
        // The constant.
        address_t iconst = var->iconst;
        // Move memory to X.
        DEBUG_GEN("MOV X, %02x\n", iconst & 255);
        asm_write_memword  (ctx, OFFS_MOV_RI + REG_X);
        asm_write_memword  (ctx, iconst & 255);
        // Move memory to Y.
        DEBUG_GEN("MOV Y, %02x\n", iconst >> 8);
        asm_write_memword  (ctx, OFFS_MOV_RI + REG_Y);
        asm_write_memword  (ctx, iconst >> 8);
    }
}

// Performs a binary math operation for numbers of one byte.
gen_var_t *r3_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b);

// Performs a binary math operation for numbers two bytes or larger.
gen_var_t *r3_math2_l(asm_ctx_t *ctx, oper_t oper, gen_var_t *output, gen_var_t *a, gen_var_t *b) {
    // Number of words to operate on.
    uint8_t n_words  = 2;
    // Instruction for the occasion.
    uint8_t insn     = 0;
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
        case OP_BIT_AND:
            insn = OFFS_AND + (is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM  + PIE(ctx));
#ifdef DEBUG_GENERATOR
            insn_name = "AND";
#endif
            break;
        case OP_BIT_OR:
            insn = OFFS_AND + (is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM  + PIE(ctx));
#ifdef DEBUG_GENERATOR
            insn_name = "OR";
#endif
            break;
        case OP_BIT_XOR:
            insn = OFFS_AND + (is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM  + PIE(ctx));
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
            DEBUG_GEN("%s%s %s, 0x%02x\n", insn_name, (i && does_cc ? "C" : ""), reg_names[regno], b->iconst >> (i * 8));
            asm_write_memword(ctx, insn + (i && does_cc ? OFFS_CALC_CC : 0));
            asm_write_memword(ctx, b->iconst >> (i * 8));
        } else {
            // Label reference.
            DEBUG_GEN("%s%s %s, [%s+%d]\n", insn_name, (i && does_cc ? "C" : ""), reg_names[regno], b->label, i);
            asm_write_memword(ctx, insn + (i && does_cc ? OFFS_CALC_CC : 0));
            asm_write_label_ref(ctx, b->label, i, OFFS(ctx));
        }
        // Store the result.
        r3_store_part(ctx, output, regno, i);
    }
    return output;
}

// Performs a unary math operation for numbers of one byte.
gen_var_t *r3_math1      (asm_ctx_t *ctx, oper_t     oper, gen_var_t *out_hint, gen_var_t *a);
// Performs a unary math operation for numbers two bytes or larger.
gen_var_t *r3_math1_l    (asm_ctx_t *ctx, oper_t     oper, gen_var_t *out_hint, gen_var_t *a);

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
        
        // Go back to the original section the function was in.
        asm_use_sect(ctx, sect_id, ASM_NOT_ALIGNED);
        // Add the entry label.
        asm_write_label(ctx, funcdef->ident.strval);
        
        // Move over the value.
        DEBUG_GEN("MOV [%s+0], X\n", label);
        asm_write_memword(ctx, OFFS_MOVST + OFFS_MOVM_XM + PIE(ctx));
        asm_write_label_ref(ctx, label, 0, OFFS(ctx));
        DEBUG_GEN("MOV [%s+1], Y\n", label);
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
        DEBUG_GEN("RTI\n");
        asm_write_memword(ctx, INSN_RTI);
    } else {
        DEBUG_GEN("RET\n");
        asm_write_memword(ctx, INSN_RET);
    }
}

/* ================= Expressions ================= */

// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *callee, size_t n_args, gen_var_t **args) {
    
}

// Expression: Binary math operation.
gen_var_t *gen_expr_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a, gen_var_t *b) {
    r3_math2_l(ctx, oper, out_hint, a, b);
}

// Expression: Unary math operation.
gen_var_t *gen_expr_math1(asm_ctx_t *ctx, oper_t oper, gen_var_t *out_hint, gen_var_t *a) {
    gen_var_t one = {
        .iconst = 1,
        .type   = VAR_TYPE_CONST
    };
    gen_expr_math2(ctx, oper, out_hint, a, &one);
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

// Variables: Create a variable based on other value.
// Other value is null if not initialised.
void gen_var_dup(asm_ctx_t *ctx, funcdef_t *funcdef, ident_t *ident, gen_var_t *other) {
    // TODO: Account for other value.
    char *fn_label = funcdef->ident.strval;
    char *label = malloc(strlen(fn_label) + 8);
    sprintf(label, "%s.LV%04lx", fn_label, ctx->current_scope->local_num);
    gen_define_var(ctx, label, ident->strval);
}

