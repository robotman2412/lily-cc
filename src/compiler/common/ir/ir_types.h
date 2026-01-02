
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "map.h"
#include "unreachable.h"

#include <endian.h>
#include <stdlib.h>

// Defines the prototype for a machine-specific assembly menemonic.
typedef struct insn_proto insn_proto_t;

// IR primitive types.
// Must be before <stdbool.h> because it contains the identifier `bool`.
typedef enum __attribute__((packed)) {
#define IR_PRIM_DEF(name) IR_PRIM_##name,
#include "defs/ir_primitives.inc"
    IR_N_PRIM,
} ir_prim_t;

// Whether a primitive is of integer type.
static inline bool ir_prim_is_integer(ir_prim_t prim) {
    switch (prim) {
        case IR_PRIM_s8:
        case IR_PRIM_s16:
        case IR_PRIM_s32:
        case IR_PRIM_s64:
        case IR_PRIM_s128:
        case IR_PRIM_u8:
        case IR_PRIM_u16:
        case IR_PRIM_u32:
        case IR_PRIM_u64:
        case IR_PRIM_u128: return true;
        default: return false;
    }
}

// Whether a primitive is signed.
static inline bool ir_prim_is_signed(ir_prim_t prim) {
    switch (prim) {
        case IR_PRIM_s8:
        case IR_PRIM_s16:
        case IR_PRIM_s32:
        case IR_PRIM_s64:
        case IR_PRIM_s128: return true;
        default: return false;
    }
}

// Get unsigned counterpart of primitive.
static inline ir_prim_t ir_prim_as_unsigned(ir_prim_t prim) {
    switch (prim) {
        case IR_PRIM_s8: return IR_PRIM_u8;
        case IR_PRIM_s16: return IR_PRIM_u16;
        case IR_PRIM_s32: return IR_PRIM_u32;
        case IR_PRIM_s64: return IR_PRIM_u64;
        case IR_PRIM_s128: return IR_PRIM_u128;
        default: return prim;
    }
}

// Get unsigned counterpart of primitive.
static inline ir_prim_t ir_prim_as_signed(ir_prim_t prim) {
    switch (prim) {
        case IR_PRIM_u8: return IR_PRIM_s8;
        case IR_PRIM_u16: return IR_PRIM_s16;
        case IR_PRIM_u32: return IR_PRIM_s32;
        case IR_PRIM_u64: return IR_PRIM_s64;
        case IR_PRIM_u128: return IR_PRIM_s128;
        default: return prim;
    }
}

#include "arith128.h"
#include "list.h"
#include "set.h"

// Binary IR operators.
typedef enum __attribute__((packed)) {
#define IR_OP2_DEF(name) IR_OP2_##name,
#include "defs/ir_op2.inc"
    IR_N_OP2,
} ir_op2_type_t;

// Unary IR operators.
typedef enum __attribute__((packed)) {
#define IR_OP1_DEF(name) IR_OP1_##name,
#include "defs/ir_op1.inc"
    IR_N_OP1,
} ir_op1_type_t;

// Type of IR instruction.
typedef enum __attribute__((packed)) {
    // 2-operand computation instructions (e.g. `mul` or `slt`).
    IR_INSN_EXPR2,
    // 1-operand computation instructions (e.g. `mov` or `bnot`).
    IR_INSN_EXPR1,
    // Unconditional jump.
    // Operands: target.
    IR_INSN_JUMP,
    // Conditional branch.
    // Operands: target, condition.
    IR_INSN_BRANCH,
    // Load effective address.
    // Operands: memory location.
    IR_INSN_LEA,
    // Load memory.
    // Operands: memory location.
    IR_INSN_LOAD,
    // Store to memory.
    // Operands: memory location, value to store.
    IR_INSN_STORE,
    // SSA function combinator / PHY node.
    IR_INSN_COMBINATOR,
    // Function call.
    IR_INSN_CALL,
    // Return from function the specified operands as per ABI.
    IR_INSN_RETURN,
    // The memory copying intrinsic.
    // Operands: dest. memory location, src. memory location, size in bytes.
    IR_INSN_MEMCPY,
    // The memory filling intrinsic.
    // Operands: dest. memory location, value (must be u8 or s8), size in bytes.
    IR_INSN_MEMSET,
    // Machine instructions; target architecture-dependent.
    IR_INSN_MACHINE,
} ir_insn_type_t;

// Type of IR operand.
typedef enum __attribute__((packed)) {
    // Constant / immediate value.
    IR_OPERAND_TYPE_CONST,
    // Undefined variable.
    IR_OPERAND_TYPE_UNDEF,
    // Compiler-managed register.
    IR_OPERAND_TYPE_VAR,
    // Memory location.
    IR_OPERAND_TYPE_MEM,
    // A specific register.
    IR_OPERAND_TYPE_REG,
} ir_operand_type_t;

// Things for the offset of a memory operand to be relative to.
typedef enum __attribute__((packed)) {
    // Absolute (relative to 0).
    IR_MEMBASE_ABS,
    // Relative to a symbol.
    IR_MEMBASE_SYM,
    // Relative to a stack frame.
    IR_MEMBASE_FRAME,
    // Relative to a code block.
    IR_MEMBASE_CODE,
    // Relative to an IR variable's value.
    IR_MEMBASE_VAR,
    // Relative to a register's value.
    IR_MEMBASE_REG,
} ir_membase_t;


// IR stack frame.
typedef struct ir_frame      ir_frame_t;
// IR function argument.
typedef struct ir_arg        ir_arg_t;
// IR variable.
typedef struct ir_var        ir_var_t;
// IR constant.
typedef struct ir_const      ir_const_t;
// IR memory reference.
typedef struct ir_memref     ir_memref_t;
// IR instruction operand.
typedef struct ir_operand    ir_operand_t;
// IR combinator code block -> variable map.
typedef struct ir_combinator ir_combinator_t;
// IR instruction.
typedef struct ir_insn       ir_insn_t;
// IR code block.
typedef struct ir_code       ir_code_t;
// IR function.
typedef struct ir_func       ir_func_t;



// IR stack frame.
struct ir_frame {
    // Function's frames list node.
    dlist_node_t node;
    // Frame name.
    char        *name;
    // Parent function.
    ir_func_t   *func;
    // Frame alignment.
    uint64_t     align;
    // Frame size.
    uint64_t     size;
};

// IR function argument.
struct ir_arg {
    // Whether this argument has a variable.
    // If it doesn't, it's still counted by the ABI but not used.
    bool has_var;
    union {
        // Type for variable-less args.
        ir_prim_t type;
        // Variable args.
        ir_var_t *var;
    };
};

// IR variable.
struct ir_var {
    // Function's variables list node.
    dlist_node_t node;
    // Variable name.
    char        *name;
    // Parent function.
    ir_func_t   *func;
    // Variable type.
    ir_prim_t    prim_type;
    // Is one of this function's args and if so, which.
    ptrdiff_t    arg_index;
    // Expressions that assign this variable.
    set_t        assigned_at;
    // instructions that read this variable.
    set_t        used_at;
};

// IR constant.
struct ir_const {
    // Constant type.
    ir_prim_t prim_type;
    union {
        // 32-bit float constant.
        float  constf32;
        // 64-bit float constant.
        double constf64;
        struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
            // Low 64 bits of constant.
            uint64_t constl;
            // High 64 bits of constant.
            uint64_t consth;
#elif __BYTE_ORDER == __BIG_ENDIAN
            // High 64 bits of constant.
            uint64_t consth;
            // Low 64 bits of constant.
            uint64_t constl;
#else
#error                                                                                                                 \
    "Invalid endianness; Lily-CC requires a machine that is big-endian or little-endian, but the endianness either different or not detected"
#endif
        };
        // 128-bit constant.
        i128_t const128;
    };
};

#define IR_CONST_BOOL(value) ((ir_const_t){.prim_type = IR_PRIM_bool, .constl = (bool)(value)})

#define IR_CONST_U8(value)   ((ir_const_t){.prim_type = IR_PRIM_u8, .constl = (uint8_t)(value)})
#define IR_CONST_U16(value)  ((ir_const_t){.prim_type = IR_PRIM_u16, .constl = (uint16_t)(value)})
#define IR_CONST_U32(value)  ((ir_const_t){.prim_type = IR_PRIM_u32, .constl = (uint32_t)(value)})
#define IR_CONST_U64(value)  ((ir_const_t){.prim_type = IR_PRIM_u64, .constl = (uint64_t)(value)})
#define IR_CONST_U128(value) ((ir_const_t){.prim_type = IR_PRIM_u128, .const128 = (value)})

#define IR_CONST_S8(value)   ((ir_const_t){.prim_type = IR_PRIM_s8, .constl = (int8_t)(value)})
#define IR_CONST_S16(value)  ((ir_const_t){.prim_type = IR_PRIM_s16, .constl = (int16_t)(value)})
#define IR_CONST_S32(value)  ((ir_const_t){.prim_type = IR_PRIM_s32, .constl = (int32_t)(value)})
#define IR_CONST_S64(value)  ((ir_const_t){.prim_type = IR_PRIM_s64, .constl = (int64_t)(value)})
#define IR_CONST_S128(value) ((ir_const_t){.prim_type = IR_PRIM_s128, .const128 = (value)})

#define IR_CONST_F32(value) ((ir_const_t){.prim_type = IR_PRIM_f32, .constf32 = (float)(value)})
#define IR_CONST_F64(value) ((ir_const_t){.prim_type = IR_PRIM_f64, .constf64 = (double)(value)})

// IR memory reference.
struct ir_memref {
    // What the offset is relative to.
    ir_membase_t base_type;
    // What type of data to load.
    ir_prim_t    data_type;
    union {
        // Base address symbol.
        char       *base_sym;
        // Base address of stack frame.
        ir_frame_t *base_frame;
        // Base address of code block.
        ir_code_t  *base_code;
        // Base address IR variable.
        ir_var_t   *base_var;
        // Base address register.
        size_t      base_regno;
    };
    // Offset from base.
    int64_t offset;
};

// Absolute base address for `ir_memref_t` compound initializer.
#define IR_BADDR_ABS()        .base_type = IR_MEMBASE_ABS
// Symbol base address for `ir_memref_t` compound initializer.
#define IR_BADDR_SYM(sym)     .base_type = IR_MEMBASE_SYM, .base_sym = (sym)
// Stack frame base address for `ir_memref_t` compound initializer.
#define IR_BADDR_FRAME(frame) .base_type = IR_MEMBASE_FRAME, .base_frame = (frame)
// Code block base address for `ir_memref_t` compound initializer.
#define IR_BADDR_CODE(code)   .base_type = IR_MEMBASE_CODE, .base_code = (code)
// Variable base address for `ir_memref_t` compound initializer.
#define IR_BADDR_VAR(var)     .base_type = IR_MEMBASE_VAR, .base_var = (var)

// Create an `ir_memref_t` without offset.
#define IR_MEMREF(data_type_, ...) ((ir_memref_t){.data_type = (data_type_), __VA_ARGS__})


// IR instruction operand.
struct ir_operand {
    // What kind of operand this is.
    ir_operand_type_t type;
    union {
        // Constant / immediate value.
        ir_const_t  iconst;
        // Data type of this undefined.
        ir_prim_t   undef_type;
        // Variable / register.
        ir_var_t   *var;
        // Memory location.
        ir_memref_t mem;
        // Register index.
        size_t      regno;
    };
};

// Get the data type of an IR operand.
static inline ir_prim_t ir_operand_prim(ir_operand_t oper) {
    switch (oper.type) {
        case IR_OPERAND_TYPE_CONST: return oper.iconst.prim_type;
        case IR_OPERAND_TYPE_UNDEF: return oper.undef_type;
        case IR_OPERAND_TYPE_VAR: return oper.var->prim_type;
        case IR_OPERAND_TYPE_MEM: return oper.mem.data_type;
        case IR_OPERAND_TYPE_REG: break;
    }
    UNREACHABLE();
}

// A constant operand.
#define IR_OPERAND_CONST(iconst_)     ((ir_operand_t){.type = IR_OPERAND_TYPE_CONST, .iconst = (iconst_)})
// An undefined operand.
#define IR_OPERAND_UNDEF(undef_type_) ((ir_operand_t){.type = IR_OPERAND_TYPE_UNDEF, .undef_type = (undef_type_)})
// A variable operand.
#define IR_OPERAND_VAR(var_)          ((ir_operand_t){.type = IR_OPERAND_TYPE_VAR, .var = (var_)})
// A memory location operand.
#define IR_OPERAND_MEM(mem_)          ((ir_operand_t){.type = IR_OPERAND_TYPE_MEM, .mem = (mem_)})
// A register operand.
#define IR_OPERAND_REG(regno_)        ((ir_operand_t){.type = IR_OPERAND_TYPE_REG, .regno = (regno_)})

// IR combinator code block -> variable map.
struct ir_combinator {
    // Previous code block.
    ir_code_t   *prev;
    // Variable or constant to bind.
    ir_operand_t bind;
};

// IR instruction.
struct ir_insn {
    // Code block's instruction list node.
    dlist_node_t   node;
    // Parent code block.
    ir_code_t     *code;
    // Distinguishes between the types of instruction.
    ir_insn_type_t type;
    union {
        // Instruction-specific flags.
        uint32_t      flags;
        // Computation operator.
        ir_op1_type_t op1;
        // Computation operator.
        ir_op2_type_t op2;
    };
    // Number of return values.
    size_t     returns_len;
    // Return values.
    ir_var_t **returns;
    union {
        struct {
            // Number of operands.
            size_t        operands_len;
            // Operands.
            ir_operand_t *operands;
        };
        struct {
            // Number of combinator sources.
            size_t           combinators_len;
            // Combinator sources.
            ir_combinator_t *combinators;
        };
    };
    // Machine instruction prototye.
    insn_proto_t const *prototype;
};

// IR code block.
struct ir_code {
    // Function's code list node.
    dlist_node_t node;
    // Set of successors.
    set_t        succ;
    // Set of predecessors.
    set_t        pred;
    // Code block name.
    char        *name;
    // Parent function.
    ir_func_t   *func;
    // Instructions in program order.
    dlist_t      insns;
    // Whether this node was visited by the depth-first search.
    // Used while converting into SSA form.
    bool         visited;
    // Node index in depth-first search tree.
    // Used while converting into SSA form.
    size_t       dfs_index;
};

// IR function.
struct ir_func {
    // Function name.
    char      *name;
    // Number of arguments.
    size_t     args_len;
    // Function arguments.
    ir_arg_t  *args;
    // Function entrypoint.
    ir_code_t *entry;
    // Unordered list of code blocks.
    dlist_t    code_list;
    // Unordered list of variables.
    dlist_t    vars_list;
    // Unordered list of stack frames.
    dlist_t    frames_list;
    // Map from name to code blocks.
    map_t      code_by_name;
    // Map from name to variables.
    map_t      var_by_name;
    // Map from name to frames.
    map_t      frame_by_name;
    // Enforce the SSA form.
    bool       enforce_ssa;
};

// Byte size per primitive type.
extern uint8_t const     ir_prim_sizes[];
// Names used in the serialized representation for `ir_prim_t`.
extern char const *const ir_prim_names[];
// Names used in the serialized representation for `ir_op2_type_t`.
extern char const *const ir_op2_names[];
// Names used in the serialized representation for `ir_op1_type_t`.
extern char const *const ir_op1_names[];
// Names used in the serialized representation for `ir_flow_type_t`.
extern char const *const ir_flow_names[];

// Convert an IR constant into bytes.
// Assumes the caller allocates enough space.
void       ir_const_to_blob(ir_const_t iconst, uint8_t *blob, bool big_endian);
// Convert an IR constant from bytes.
// Assumes the caller guarantees the correctness of `blob`.
ir_const_t ir_const_from_blob(ir_prim_t prim, uint8_t const *blob, bool big_endian);
