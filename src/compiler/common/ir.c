
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir.h"

#include "strong_malloc.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

// Create a new IR function.
// Function argument types are IR_PRIM_S32 by default.
ir_func_t *ir_func_create(char const *name, char const *entry_name, size_t args_len, char const *const *args_name) {
    ir_func_t *func = strong_malloc(sizeof(ir_func_t));
    func->name      = strong_strdup(name);
    func->args      = strong_malloc(sizeof(ir_var_t *) * args_len);
    func->args_len  = args_len;
    for (size_t i = 0; i < args_len; i++) {
        func->args[i]
            = ir_var_create(func, IR_PRIM_S32, args_name && args_name[i] ? strong_strdup(args_name[i]) : NULL);
    }
    func->entry = ir_code_create(func, entry_name);
    return func;
}

// Delete an IR function.
void ir_func_destroy(ir_func_t *func) {
    free(func->name);

    ir_var_t *var = (ir_var_t *)func->vars_list.head;
    while (var) {
        free(var->name);
        void *tmp = var;
        var       = (ir_var_t *)var->node.next;
        free(tmp);
    }

    ir_code_t *code = (ir_code_t *)func->code_list.head;
    while (code) {
        free(code->name);
        void *tmp = code;

        ir_insn_t *insn = (ir_insn_t *)code->insns.head;
        while (insn) {
            void *tmp2 = insn;
            if (insn->is_expr) {
                ir_expr_t *expr = (ir_expr_t *)insn;
                if (expr->type == IR_EXPR_COMBINATOR) {
                    free(expr->e_combinator.from);
                }
            } else {
                ir_flow_t *flow = (ir_flow_t *)insn;
                if (flow->type == IR_FLOW_CALL_DIRECT || flow->type == IR_FLOW_CALL_PTR) {
                    free(flow->f_call_direct.args);
                }
            }
            insn = (ir_insn_t *)insn->node.next;
            free(tmp2);
        }

        code = (ir_code_t *)code->node.next;
        free(tmp);
    }
    free(func);
}

// Serialize an IR operand.
static void ir_operand_serialize(ir_operand_t operand, FILE *to) {
    if (operand.is_const) {
        if (operand.prim_type == IR_PRIM_BOOL) {
            fputs(operand.iconst ? "true" : "false", to);
        } else {
            fputs(ir_prim_names[operand.prim_type], to);
            fputs("'0x", to);
            uint8_t size = ir_prim_sizes[operand.prim_type];
            if (size == 16) {
                fprintf(to, "%016" PRIX64 "%016" PRIX64, operand.iconsth, operand.iconst);
            } else {
                fprintf(to, "%0*" PRIX64, (int)size * 2, operand.iconst);
            }
            if (operand.prim_type == IR_PRIM_F32) {
                float fval;
                memcpy(&fval, &operand.iconst, sizeof(float));
                fprintf(to, " /* %f */", fval);
            } else if (operand.prim_type == IR_PRIM_F64) {
                double dval;
                memcpy(&dval, &operand.iconst, sizeof(double));
                fprintf(to, " /* %lf */", dval);
            }
        }
    } else {
        fputc('%', to);
        fputs(operand.var->name, to);
    }
}

// Serialize an IR function.
void ir_func_serialize(ir_func_t *func, FILE *to) {
    if (func->enforce_ssa) {
        fputs("ssa ", to);
    }
    fprintf(to, "function %%%s\n", func->name);

    ir_var_t *var = (ir_var_t *)func->vars_list.head;
    while (var) {
        fprintf(to, "    var %s %%%s\n", ir_prim_names[var->prim_type], var->name);
        var = (ir_var_t *)var->node.next;
    }

    for (size_t i = 0; i < func->args_len; i++) {
        fprintf(to, "    arg %%%s\n", func->args[i]->name);
    }

    ir_code_t *code = (ir_code_t *)func->code_list.head;
    while (code) {
        fprintf(to, "code <%s>\n", code->name);

        ir_insn_t *insn = (ir_insn_t *)code->insns.head;
        while (insn) {
            fputs("    ", to);
            if (insn->is_expr) {
                ir_expr_t *expr = (ir_expr_t *)insn;
                switch (expr->type) {
                    case IR_EXPR_COMBINATOR: {
                        fprintf(to, "combinator %%%s", expr->dest->name);
                        for (size_t i = 0; i < expr->e_combinator.from_len; i++) {
                            fprintf(
                                to,
                                ", <%s> %%%s",
                                expr->e_combinator.from[i].prev->name,
                                expr->e_combinator.from[i].bind->name
                            );
                        }
                    } break;
                    case IR_EXPR_UNARY: {
                        fprintf(to, "%s %%%s, ", ir_op1_names[expr->e_unary.oper], expr->dest->name);
                        ir_operand_serialize(expr->e_unary.value, to);
                        fputc('\n', to);
                    } break;
                    case IR_EXPR_BINARY: {
                        fprintf(to, "%s %%%s, ", ir_op2_names[expr->e_binary.oper], expr->dest->name);
                        ir_operand_serialize(expr->e_binary.lhs, to);
                        fputs(", ", to);
                        ir_operand_serialize(expr->e_binary.rhs, to);
                        fputc('\n', to);
                    } break;
                    case IR_EXPR_UNDEFINED: {
                        fprintf(to, "undef %%%s\n", expr->dest->name);
                    } break;
                }
            } else {
                ir_flow_t *flow = (ir_flow_t *)insn;
                fputs(ir_flow_names[flow->type], to);
                switch (flow->type) {
                    case IR_FLOW_JUMP: {
                        fprintf(to, " <%s>\n", flow->f_jump.target->name);
                    } break;
                    case IR_FLOW_BRANCH: {
                        fprintf(to, " %%%s, <%s>\n", flow->f_branch.cond->name, flow->f_branch.target->name);
                    } break;
                    case IR_FLOW_CALL_DIRECT: {
                        fprintf(to, " <%s>", flow->f_call_direct.label);
                        for (size_t i = 0; i < flow->f_call_direct.args_len; i++) {
                            fputs(", ", to);
                            ir_operand_serialize(flow->f_call_direct.args[i], to);
                        }
                        fputc('\n', to);
                    } break;
                    case IR_FLOW_CALL_PTR: {
                        fprintf(to, " %%%s", flow->f_call_ptr.addr->name);
                        for (size_t i = 0; i < flow->f_call_ptr.args_len; i++) {
                            fputs(", ", to);
                            ir_operand_serialize(flow->f_call_ptr.args[i], to);
                        }
                        fputc('\n', to);
                    } break;
                    case IR_FLOW_RETURN: {
                        if (flow->f_return.has_value) {
                            fputc(' ', to);
                            ir_operand_serialize(flow->f_return.value, to);
                        }
                        fputc('\n', to);
                    } break;
                }
            }
            insn = (ir_insn_t *)insn->node.next;
        }

        code = (ir_code_t *)code->node.next;
    }
}


// Create a new variable.
// If `name` is `NULL`, its name will be a decimal number.
// For this reason, avoid explicitly passing names that are just a decimal number.
ir_var_t *ir_var_create(ir_func_t *func, ir_prim_t type, char const *name) {
    ir_var_t *var = calloc(1, sizeof(ir_var_t));
    if (name) {
        var->name = strong_strdup(name);
    } else {
        char const *fmt = "%zu";
        size_t      len = snprintf(NULL, 0, fmt, func->vars_list.len);
        var->name       = calloc(1, len + 1);
        snprintf(var->name, len + 1, fmt, func->vars_list.len);
    }
    var->prim_type = type;
    var->func      = func;
    dlist_append(&func->vars_list, &var->node);
    return var;
}


// Create a new IR code block.
// If `name` is `NULL`, its name will be a decimal number.
// For this reason, avoid explicitly passing names that are just a decimal number.
ir_code_t *ir_code_create(ir_func_t *func, char const *name) {
    ir_code_t *code = calloc(1, sizeof(ir_code_t));
    code->func      = func;
    if (name) {
        code->name = strong_strdup(name);
    } else {
        char const *fmt = "%zu";
        size_t      len = snprintf(NULL, 0, fmt, func->code_list.len);
        code->name      = calloc(1, len + 1);
        snprintf(code->name, len + 1, fmt, func->code_list.len);
    }
    dlist_append(&func->code_list, &code->node);
    return code;
}

// Add a combinator function to a code block.
// Takes ownership of the `from` array.
void ir_add_combinator(ir_code_t *code, ir_var_t *dest, size_t from_len, ir_combinator_t *from) {
    ir_expr_t *expr             = calloc(1, sizeof(ir_expr_t));
    expr->base.is_expr          = true;
    expr->base.parent           = code;
    expr->type                  = IR_EXPR_COMBINATOR;
    expr->e_combinator.from_len = from_len;
    expr->e_combinator.from     = from;
    expr->dest                  = dest;
    if (dest->is_assigned && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    dest->assigned_at = expr;
    dest->is_assigned = true;
    dlist_append(&code->insns, &expr->base.node);
}

// Add an expression to a code block.
void ir_add_expr1(ir_code_t *code, ir_var_t *dest, ir_op1_type_t oper, ir_operand_t operand) {
    ir_expr_t *expr     = calloc(1, sizeof(ir_expr_t));
    expr->base.is_expr  = true;
    expr->base.parent   = code;
    expr->type          = IR_EXPR_UNARY;
    expr->e_unary.oper  = oper;
    expr->e_unary.value = operand;
    expr->dest          = dest;
    if (dest->is_assigned && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    dest->assigned_at = expr;
    dest->is_assigned = true;
    dlist_append(&code->insns, &expr->base.node);
}

// Add an expression to a code block.
void ir_add_expr2(ir_code_t *code, ir_var_t *dest, ir_op2_type_t oper, ir_operand_t lhs, ir_operand_t rhs) {
    ir_expr_t *expr     = calloc(1, sizeof(ir_expr_t));
    expr->base.is_expr  = true;
    expr->base.parent   = code;
    expr->type          = IR_EXPR_BINARY;
    expr->e_binary.oper = oper;
    expr->e_binary.lhs  = lhs;
    expr->e_binary.rhs  = rhs;
    expr->dest          = dest;
    if (dest->is_assigned && code->func->enforce_ssa) {
        fprintf(stderr, "[BUG] IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    dest->assigned_at = expr;
    dest->is_assigned = true;
    dlist_append(&code->insns, &expr->base.node);
}

// Add an undefined variable.
void ir_add_undefined(ir_code_t *code, ir_var_t *dest) {
    ir_expr_t *expr    = calloc(1, sizeof(ir_expr_t));
    expr->base.is_expr = true;
    expr->base.parent  = code;
    expr->type         = IR_EXPR_UNDEFINED;
    expr->dest         = dest;
    if (dest->is_assigned) {
        fprintf(stderr, "[BUG] IR variable %%%s assigned twice\n", dest->name);
        abort();
    }
    dest->assigned_at = expr;
    dest->is_assigned = true;
    dlist_append(&code->insns, &expr->base.node);
}

// Add a direct (by label) function call.
// Takes ownership of `params`.
void ir_add_call_direct(ir_code_t *from, char const *label, size_t params_len, ir_operand_t *params) {
    ir_flow_t *flow              = calloc(1, sizeof(ir_flow_t));
    flow->base.parent            = from;
    flow->type                   = IR_FLOW_CALL_DIRECT;
    flow->f_call_direct.label    = strong_strdup(label);
    flow->f_call_direct.args_len = params_len;
    flow->f_call_direct.args     = params;
    dlist_append(&from->insns, &flow->base.node);
}

// Add an indirect (by pointer) function call.
// Takes ownership of `params`.
void ir_add_call_ptr(ir_code_t *from, ir_var_t *funcptr, size_t params_len, ir_operand_t *params) {
    ir_flow_t *flow           = calloc(1, sizeof(ir_flow_t));
    flow->base.parent         = from;
    flow->type                = IR_FLOW_CALL_PTR;
    flow->f_call_ptr.addr     = funcptr;
    flow->f_call_ptr.args_len = params_len;
    flow->f_call_ptr.args     = params;
    dlist_append(&from->insns, &flow->base.node);
}

// Add an unconditional jump.
void ir_add_jump(ir_code_t *from, ir_code_t *to) {
    ir_flow_t *flow     = calloc(1, sizeof(ir_flow_t));
    flow->base.parent   = from;
    flow->type          = IR_FLOW_JUMP;
    flow->f_jump.target = to;
    dlist_append(&from->insns, &flow->base.node);
}

// Add a conditional branch.
void ir_add_branch(ir_code_t *from, ir_var_t *cond, ir_code_t *to) {
    ir_flow_t *flow       = calloc(1, sizeof(ir_flow_t));
    flow->base.parent     = from;
    flow->type            = IR_FLOW_BRANCH;
    flow->f_branch.cond   = cond;
    flow->f_branch.target = to;
    dlist_append(&from->insns, &flow->base.node);
}

// Add a return without value.
void ir_add_return0(ir_code_t *from) {
    ir_flow_t *flow   = calloc(1, sizeof(ir_flow_t));
    flow->base.parent = from;
    flow->type        = IR_FLOW_RETURN;
    dlist_append(&from->insns, &flow->base.node);
}

// Add a return with value.
void ir_add_return1(ir_code_t *from, ir_operand_t value) {
    ir_flow_t *flow          = calloc(1, sizeof(ir_flow_t));
    flow->base.parent        = from;
    flow->type               = IR_FLOW_RETURN;
    flow->f_return.has_value = true;
    flow->f_return.value     = value;
    dlist_append(&from->insns, &flow->base.node);
}
