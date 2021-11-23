
#include "gr8cpu-r3_gen.h"
#include "definitions.h"
#include "asm.h"

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
#define OFFS_CALC_AM	0x09
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

// Moves a long into the registers X and Y.
// Used before function entry to functions which take exactly one two-byte integer.
gen_var_t *r3_movl_to_mem(asm_ctx_t *ctx, char *label) {
    // Move X to memory.
    asm_write_memword  (ctx, OFFS_MOVST + OFFS_MOVM_XM + PIE(ctx));
    asm_write_label_ref(ctx, label, 0, OFFS(ctx));
    // Move Y to memory.
    asm_write_memword  (ctx, OFFS_MOVST + OFFS_MOVM_YM + PIE(ctx));
    asm_write_label_ref(ctx, label, 1, OFFS(ctx));
}

// Moves a long into memory.
// Used before function return from functions which return exactly one two-byte integer.
void r3_movl_to_reg(asm_ctx_t *ctx, gen_var_t *var) {
    
}

// Performs a binary math operation for numbers of one byte.
gen_var_t *r3_math2(asm_ctx_t *ctx, oper_t oper, gen_var_t *a, gen_var_t *b);

// Performs a binary math operation for numbers two bytes or larger.
gen_var_t *r3_math2_l(asm_ctx_t *ctx, oper_t oper, gen_var_t *a, gen_var_t *b) {
    uint8_t n_words  = 2;
    uint8_t insn     = 0;
    bool is_b_const  = b->type == VAR_TYPE_CONST;
    bool exchangable = 1;
    switch (oper) {
        case OP_ADD:
            insn = OFFS_ADD + is_b_const ? OFFS_CALC_AV : OFFS_CALC_AM;
            break;
        case OP_SUB:
            insn = OFFS_SUB + is_b_const ? OFFS_CALC_AV : OFFS_CALC_AM;
            exchangable = 0;
            break;
        case OP_BIT_AND:
            insn = OFFS_AND + is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM;
            break;
        case OP_BIT_OR:
            insn = OFFS_AND + is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM;
            break;
        case OP_BIT_XOR:
            insn = OFFS_AND + is_b_const ? OFFS_BIT_AV  : OFFS_BIT_AM;
            break;
    }
    for (uint8_t i = 0; i < n_words; i++) {
        
    }
}

// Performs a unary math operation for numbers of one byte.
gen_var_t *r3_math1      (asm_ctx_t *ctx, oper_t     oper, gen_var_t *a);
// Performs a unary math operation for numbers two bytes or larger.
gen_var_t *r3_math1_l    (asm_ctx_t *ctx, oper_t     oper, gen_var_t *a);

/* ================== Functions ================== */

// Function entry for non-inlined functions. 
void gen_function_entry(asm_ctx_t *ctx, funcdef_t *funcdef) {
    if (funcdef->args.num == 1 /*&& funcdef->args.arr[0].size == 2*/) {
        // Exactly one long for an argument.
        gen_var_arg(ctx, funcdef, 0);
    } else {
        // Arguments on the stack.
        for (size_t i = 0; i < funcdef->args.num; i++) {
            gen_var_arg(ctx, funcdef, i);
        }
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
    if (/*func is not noreturn?*/1) {
        if (/*func is interrupt handler?*/0) {
            asm_write_memword(ctx, INSN_RTI);
        } else {
            asm_write_memword(ctx, INSN_RET);
        }
    }
}

/* ================= Expressions ================= */

// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *callee, size_t n_args, gen_var_t **args) {
    
}

// Expression: Binary math operation.
gen_var_t *gen_expr_math2(asm_ctx_t *ctx, oper_t *expr, gen_var_t *a, gen_var_t *b) {
}

// Expression: Unary math operation.
gen_var_t *gen_expr_math1(asm_ctx_t *ctx, oper_t *expr, gen_var_t *a) {
}

// Miscellaneous: Move variable to another location.
void gen_mov(asm_ctx_t *ctx, gen_var_t *dest, gen_var_t *src) {
}

// Variables: Create a variable based on parameter.
void gen_var_arg(asm_ctx_t *ctx, funcdef_t *funcdef, size_t argno) {
}

// Variables: Create a variable based on other value.
// Other value is null if not initialised.
void gen_var_dup(asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *other) {
}

