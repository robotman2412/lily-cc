
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir/ir_serialization.h"

#include <inttypes.h>



// Serialize an IR operand.
static void ir_operand_serialize(ir_operand_t operand, FILE *to) {
    if (operand.is_const) {
        if (operand.iconst.prim_type == IR_PRIM_bool) {
            fputs(operand.iconst.constl ? "true" : "false", to);
        } else {
            fputs(ir_prim_names[operand.iconst.prim_type], to);
            fputs("'0x", to);
            uint8_t size = ir_prim_sizes[operand.iconst.prim_type];
            if (size == 16) {
                fprintf(to, "%016" PRIX64 "%016" PRIX64, operand.iconst.consth, operand.iconst.constl);
            } else {
                fprintf(to, "%0*" PRIX64, (int)size * 2, operand.iconst.constl);
            }
            if (operand.iconst.prim_type == IR_PRIM_f32) {
                float fval;
                memcpy(&fval, &operand.iconst, sizeof(float));
                fprintf(to, " /* %f */", fval);
            } else if (operand.iconst.prim_type == IR_PRIM_f64) {
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
        fputs("ssa_", to);
    }
    fprintf(to, "function <%s>\n", func->name);

    ir_var_t *var = (ir_var_t *)func->vars_list.head;
    while (var) {
        fprintf(to, "    var %%%s %s\n", var->name, ir_prim_names[var->prim_type]);
        var = (ir_var_t *)var->node.next;
    }

    for (size_t i = 0; i < func->args_len; i++) {
        fprintf(to, "    arg %%%s\n", func->args[i]->name);
    }

    dlist_foreach_node(ir_frame_t, frame, &func->frames_list) {
        fprintf(to, "    frame %%%s u64'%" PRId64 " u64'%" PRId64 "\n", frame->name, frame->size, frame->align);
    }

    ir_code_t *code = (ir_code_t *)func->code_list.head;
    while (code) {
        fprintf(to, "code %%%s\n", code->name);

        ir_insn_t *insn = (ir_insn_t *)code->insns.head;
        while (insn) {
            fputs("    ", to);
            if (insn->type == IR_INSN_EXPR) {
                ir_expr_t *expr = (ir_expr_t *)insn;
                switch (expr->type) {
                    case IR_EXPR_COMBINATOR: {
                        fprintf(to, "phi %%%s", expr->dest->name);
                        for (size_t i = 0; i < expr->e_combinator.from_len; i++) {
                            fprintf(to, ", %%%s ", expr->e_combinator.from[i].prev->name);
                            ir_operand_serialize(expr->e_combinator.from[i].bind, to);
                        }
                        fputc('\n', to);
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
            } else if (insn->type == IR_INSN_MEM) {
                ir_mem_t *mem = (ir_mem_t *)insn;
                if (mem->type == IR_MEM_LEA_STACK) {
                    fprintf(
                        to,
                        "lea %%%s, %%%s+%" PRId64 "\n",
                        mem->m_lea_stack.dest->name,
                        mem->m_lea_stack.frame->name,
                        mem->m_lea_stack.offset
                    );
                } else if (mem->type == IR_MEM_LEA_SYMBOL) {
                    fprintf(
                        to,
                        "lea %%%s, <%s>+%" PRId64 "\n",
                        mem->m_lea_symbol.dest->name,
                        mem->m_lea_symbol.symbol,
                        mem->m_lea_symbol.offset
                    );
                } else if (mem->type == IR_MEM_LOAD) {
                    fprintf(to, "load %%%s, ", mem->m_load.dest->name);
                    ir_operand_serialize(mem->m_load.addr, to);
                    fputc('\n', to);
                } else if (mem->type == IR_MEM_STORE) {
                    fputs("store ", to);
                    ir_operand_serialize(mem->m_store.src, to);
                    fputs(", ", to);
                    ir_operand_serialize(mem->m_store.addr, to);
                    fputc('\n', to);
                }
            } else {
                ir_flow_t *flow = (ir_flow_t *)insn;
                fputs(ir_flow_names[flow->type], to);
                switch (flow->type) {
                    case IR_FLOW_JUMP: {
                        fprintf(to, " %%%s\n", flow->f_jump.target->name);
                    } break;
                    case IR_FLOW_BRANCH: {
                        fputc(' ', to);
                        ir_operand_serialize(flow->f_branch.cond, to);
                        fprintf(to, ", %%%s\n", flow->f_branch.target->name);
                    } break;
                    case IR_FLOW_CALL_DIRECT: {
                        fprintf(to, " %%%s", flow->f_call_direct.label);
                        for (size_t i = 0; i < flow->f_call_direct.args_len; i++) {
                            fputs(", ", to);
                            ir_operand_serialize(flow->f_call_direct.args[i], to);
                        }
                        fputc('\n', to);
                    } break;
                    case IR_FLOW_CALL_PTR: {
                        fputc(' ', to);
                        ir_operand_serialize(flow->f_call_ptr.addr, to);
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



// Deserialize a single IR function from the file.
// Call multiple times on a single file if you want all the functions.
ir_func_t *ir_func_deserialize(tokenizer_t *from) {
    (void)from;
    abort();
}
