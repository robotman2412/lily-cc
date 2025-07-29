
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir/ir_serialization.h"

#include "insn_proto.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "list.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>



// Serialize an IR constant.
void ir_const_serialize(ir_const_t iconst, FILE *to) {
    if (iconst.prim_type == IR_PRIM_bool) {
        fputs(iconst.constl ? "true" : "false", to);
    } else {
        fputs(ir_prim_names[iconst.prim_type], to);
        fputs("'0x", to);
        uint8_t size = ir_prim_sizes[iconst.prim_type];
        if (size == 16) {
            fprintf(to, "%016" PRIX64 "%016" PRIX64, iconst.consth, iconst.constl);
        } else {
            uint64_t shamt = 64 - 8 * size;
            fprintf(to, "%0*" PRIX64, (int)size * 2, iconst.constl << shamt >> shamt);
        }
        if (iconst.prim_type == IR_PRIM_f32) {
            float fval;
            memcpy(&fval, &iconst, sizeof(float));
            fprintf(to, " /* %f */", fval);
        } else if (iconst.prim_type == IR_PRIM_f64) {
            double dval;
            memcpy(&dval, &iconst, sizeof(double));
            fprintf(to, " /* %lf */", dval);
        }
    }
}

// Serialize an IR operand.
void ir_operand_serialize(ir_operand_t operand, FILE *to) {
    if (operand.type == IR_OPERAND_TYPE_CONST) {
        ir_const_serialize(operand.iconst, to);
    } else if (operand.type == IR_OPERAND_TYPE_MEM) {
        fputc('(', to);
        switch (operand.mem.rel_type) {
            case IR_MEMREL_ABS: break;
            case IR_MEMREL_SYM: fprintf(to, "<%s>", operand.mem.base_sym); break;
            case IR_MEMREL_FRAME: fprintf(to, "%%%s", operand.mem.base_frame->name); break;
            case IR_MEMREL_CODE: fprintf(to, "%%%s", operand.mem.base_code->name); break;
            case IR_MEMREL_VAR: fprintf(to, "%%%s", operand.mem.base_var->name); break;
        }
        uint64_t offset = operand.mem.offset;
        if (operand.mem.rel_type != IR_MEMREL_ABS && operand.mem.offset) {
            if (operand.mem.offset < 0) {
                offset = -operand.mem.offset;
                fputc('-', to);
            } else {
                fputc('+', to);
            }
        }
        if (operand.mem.rel_type == IR_MEMREL_ABS || operand.mem.offset) {
            fprintf(to, "0x%" PRIx64, offset);
        }
        fputc(')', to);
    } else if (operand.type == IR_OPERAND_TYPE_UNDEF) {
        fputs(ir_prim_names[operand.undef_type], to);
        fputs("'?", to);
    } else {
        fputc('%', to);
        fputs(operand.var->name, to);
    }
}

// Serialize an IR instruction.
void ir_insn_serialize(ir_insn_t *insn, FILE *to) {
    for (size_t i = 0; i < insn->returns_len; i++) {
        if (i) {
            fputs(", ", to);
        }
        fprintf(to, "%%%s", insn->returns[i]->name);
    }

    if (insn->returns_len) {
        fputs(" = ", to);
    }

    switch (insn->type) {
        case IR_INSN_EXPR2: fputs(ir_op2_names[insn->op2], to); break;
        case IR_INSN_EXPR1: fputs(ir_op1_names[insn->op1], to); break;
        case IR_INSN_JUMP: fputs("jump", to); break;
        case IR_INSN_BRANCH: fputs("branch", to); break;
        case IR_INSN_LEA: fputs("lea", to); break;
        case IR_INSN_LOAD: fputs("load", to); break;
        case IR_INSN_STORE: fputs("store", to); break;
        case IR_INSN_COMBINATOR: fputs("comb", to); break;
        case IR_INSN_CALL: fputs("call", to); break;
        case IR_INSN_RETURN: fputs("return", to); break;
        case IR_INSN_MACHINE: fprintf(to, "mach %s", insn->prototype->name); break;
    }

    if (insn->type == IR_INSN_COMBINATOR) {
        if (insn->combinators_len) {
            fputc(' ', to);
        }

        for (size_t i = 0; i < insn->combinators_len; i++) {
            if (i) {
                fputs(", ", to);
            }
            printf("%%%s ", insn->combinators[i].prev->name);
            ir_operand_serialize(insn->combinators[i].bind, to);
        }
    } else {
        if (insn->operands_len) {
            fputc(' ', to);
        }

        for (size_t i = 0; i < insn->operands_len; i++) {
            if (i) {
                fputs(", ", to);
            }
            ir_operand_serialize(insn->operands[i], to);
        }
    }
}

// Serialize an IR code block.
void ir_code_serialize(ir_code_t *code, FILE *to) {
    fprintf(to, "code %%%s\n", code->name);
    dlist_foreach_node(ir_insn_t, insn, &code->insns) {
        fputs("    ", to);
        ir_insn_serialize(insn, to);
        fputc('\n', to);
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
        if (func->args[i].has_var) {
            fprintf(to, "    arg %%%s\n", func->args[i].var->name);
        } else {
            fprintf(to, "    arg %s\n", ir_prim_names[func->args[i].type]);
        }
    }

    dlist_foreach_node(ir_frame_t, frame, &func->frames_list) {
        fprintf(to, "    frame %%%s u64'%" PRId64 " u64'%" PRId64 "\n", frame->name, frame->size, frame->align);
    }

    dlist_foreach_node(ir_code_t, code, &func->code_list) {
        ir_code_serialize(code, to);
    }
}



// Deserialize a single IR function from the file.
// Call multiple times on a single file if you want all the functions.
ir_func_t *ir_func_deserialize(tokenizer_t *from) {
    (void)from;
    abort();
}
