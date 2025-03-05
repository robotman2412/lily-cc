
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

// IR primitive types.
// Must be before <stdbool.h> because it contains the identifier `bool`.
typedef enum __attribute__((packed)) {
#define IR_PRIM_DEF(name) IR_PRIM_##name,
#include "defs/ir_primitives.inc"
    IR_N_PRIM,
} ir_prim_t;

#include "arith128.h"
#include "list.h"
#include "set.h"

// IR expression types.
typedef enum __attribute__((packed)) {
    // Combinator expression.
    IR_EXPR_COMBINATOR,
    // Unary expression.
    IR_EXPR_UNARY,
    // Binary expression.
    IR_EXPR_BINARY,
    // Set to undefined.
    IR_EXPR_UNDEFINED,
} ir_expr_type_t;

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

// IR control flow types.
typedef enum __attribute__((packed)) {
    // Unconditional jump
    IR_FLOW_JUMP,
    // Conditional branch.
    IR_FLOW_BRANCH,
    // Function call (direct).
    IR_FLOW_CALL_DIRECT,
    // Function call (via pointer).
    IR_FLOW_CALL_PTR,
    // Return from function.
    IR_FLOW_RETURN,
} ir_flow_type_t;



// IR stack frame.
typedef struct ir_frame      ir_frame_t;
// IR variable.
typedef struct ir_var        ir_var_t;
// IR constant.
typedef struct ir_const      ir_const_t;
// IR expression operand.
typedef struct ir_operand    ir_operand_t;
// IR combinator code block -> variable map.
typedef struct ir_combinator ir_combinator_t;
// IR instruction.
typedef struct ir_insn       ir_insn_t;
// IR expression.
typedef struct ir_expr       ir_expr_t;
// IR control flow.
typedef struct ir_flow       ir_flow_t;
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
    // Frame size.
    uint64_t     size;
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
    // Expressions that assign this variable.
    dlist_t      assigned_at;
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
            // Low 64 bits of constant.
            uint64_t constl;
            // High 64 bits of constant.
            uint64_t consth;
        };
        // 128-bit constant.
        i128_t const128;
    };
};

// IR expression operand.
struct ir_operand {
    // Is this a constant?
    bool is_const;
    union {
        // Constant.
        ir_const_t iconst;
        // Variable.
        ir_var_t  *var;
    };
};

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
    dlist_node_t node;
    // Parent code block.
    ir_code_t   *parent;
    // If true, this is an `ir_expr_t`; otherwise, an `ir_flow_t`.
    bool         is_expr;
};

// IR expression.
struct ir_expr {
    ir_insn_t      base;
    // Expression type.
    ir_expr_type_t type;
    // Linked list node for variable assignments.
    dlist_node_t   dest_node;
    // Destination variable.
    ir_var_t      *dest;
    union {
        struct {
            // Number of combination points.
            size_t           from_len;
            // Combination point map.
            ir_combinator_t *from;
        } e_combinator;
        struct {
            // Operator type.
            ir_op1_type_t oper;
            // Expression operand.
            ir_operand_t  value;
        } e_unary;
        struct {
            // Operator type.
            ir_op2_type_t oper;
            // First operand.
            ir_operand_t  lhs;
            // Second operand.
            ir_operand_t  rhs;
        } e_binary;
    };
};

// IR control flow.
struct ir_flow {
    ir_insn_t      base;
    // Control flow type.
    ir_flow_type_t type;
    union {
        struct {
            // Jump target.
            ir_code_t *target;
        } f_jump;
        struct {
            // Branch target if condition is satisfied.
            ir_code_t   *target;
            // Branch condition.
            ir_operand_t cond;
        } f_branch;
        struct {
            // Number of arguments.
            size_t        args_len;
            // Function call arguments.
            ir_operand_t *args;
            // Destination label.
            char         *label;
        } f_call_direct;
        struct {
            // Number of arguments.
            size_t        args_len;
            // Function call arguments.
            ir_operand_t *args;
            // Destination address.
            ir_operand_t  addr;
        } f_call_ptr;
        struct {
            // Has a return value.
            bool         has_value;
            // Return value, if any.
            ir_operand_t value;
        } f_return;
    };
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
    ir_var_t **args;
    // Function entrypoint.
    ir_code_t *entry;
    // Unordered list of code blocks.
    dlist_t    code_list;
    // Unordered list of variables.
    dlist_t    vars_list;
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
