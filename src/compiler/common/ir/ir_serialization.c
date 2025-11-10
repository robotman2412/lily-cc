
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "ir/ir_serialization.h"

#include "arith128.h"
#include "arrays.h"
#include "compiler.h"
#include "insn_proto.h"
#include "ir.h"
#include "ir_interpreter.h"
#include "ir_parser.h"
#include "ir_tokenizer.h"
#include "ir_types.h"
#include "list.h"
#include "map.h"
#include "set.h"
#include "strong_malloc.h"
#include "tokenizer.h"
#include "unreachable.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Serialize an IR constant.
void ir_const_serialize(ir_const_t iconst, FILE *to) {
    iconst = ir_trim_const(iconst);
    if (iconst.prim_type == IR_PRIM_bool) {
        fputs(iconst.constl ? "true" : "false", to);

    } else if (iconst.prim_type == IR_PRIM_f32) {
        fprintf(stderr, "[TODO] ir_const_serialize for f32\n");
        abort();
    } else if (iconst.prim_type == IR_PRIM_f64) {
        fprintf(stderr, "[TODO] ir_const_serialize for f64\n");
        abort();
    } else {
        fputs(ir_prim_names[iconst.prim_type], to);
        fputc('\'', to);
        uint8_t size = ir_prim_sizes[iconst.prim_type];

        // Print decimal representation.
        i128_t u_val;
        if (!(iconst.prim_type & 1) && iconst.consth >> 63) {
            // Negative number.
            u_val = neg128(iconst.const128);
            fputc('-', to);
        } else {
            // Positive number.
            u_val = iconst.const128;
        }
        bool nontrivial = cmp128u(u_val, int128(0, 9)) > 0;
        if (size == 16) {
            char buf[40];
            itoa128(u_val, 0, buf);
            fputs(buf, to);
        } else {
            fprintf(to, "%" PRIu64, lo64(u_val));
        }

        // Print hexadecimal representation.
        if (nontrivial) {
            fputs(" /* 0x", to);
            if (size == 16) {
                fprintf(to, "%.0" PRIX64 "%0*" PRIX64, iconst.consth, iconst.consth ? 16 : 1, iconst.constl);
            } else {
                uint64_t shamt = 64 - 8 * size;
                fprintf(to, "%" PRIX64, iconst.constl << shamt >> shamt);
            }
            fputs(" */", to);
        }
    }
}

// Serialize an IR operand.
void ir_operand_serialize(ir_operand_t operand, backend_profile_t const *profile_opt, bool show_memop_type, FILE *to) {
    if (operand.type == IR_OPERAND_TYPE_CONST) {
        ir_const_serialize(operand.iconst, to);
    } else if (operand.type == IR_OPERAND_TYPE_MEM) {
        if (show_memop_type && operand.mem.data_type < IR_N_PRIM) {
            fprintf(to, "(%s ", ir_prim_names[operand.mem.data_type]);
        } else {
            fputc('(', to);
        }
        switch (operand.mem.base_type) {
            case IR_MEMBASE_ABS: break;
            case IR_MEMBASE_SYM: fprintf(to, "<%s>", operand.mem.base_sym); break;
            case IR_MEMBASE_FRAME: fprintf(to, "%%%s", operand.mem.base_frame->name); break;
            case IR_MEMBASE_CODE: fprintf(to, "%%%s", operand.mem.base_code->name); break;
            case IR_MEMBASE_VAR: fprintf(to, "%%%s", operand.mem.base_var->name); break;
            case IR_MEMBASE_REG: fputs(profile_opt->gpr_names[operand.mem.base_regno], to); break;
        }

        // TODO: This implicitly assumes IR pointers to be 64-bit, but that information doesn't exist yet.
        if (operand.mem.base_type == IR_MEMBASE_ABS) {
            ir_const_serialize(IR_CONST_U64(operand.mem.offset), to);
        } else if (operand.mem.offset) {
            fputs(" + ", to);
            ir_const_serialize(IR_CONST_S64(operand.mem.offset), to);
        }

        fputc(')', to);
    } else if (operand.type == IR_OPERAND_TYPE_UNDEF) {
        fputs(ir_prim_names[operand.undef_type], to);
        fputs("'?", to);
    } else if (operand.type == IR_OPERAND_TYPE_REG) {
        fputc('$', to);
        fputs(profile_opt->gpr_names[operand.regno], to);
    } else if (operand.type == IR_OPERAND_TYPE_VAR) {
        fputc('%', to);
        fputs(operand.var->name, to);
    } else {
        UNREACHABLE();
    }
}

// Serialize an IR instruction.
void ir_insn_serialize(ir_insn_t *insn, backend_profile_t const *profile_opt, FILE *to) {
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
            ir_operand_serialize(insn->combinators[i].bind, profile_opt, true, to);
        }
    } else {
        if (insn->operands_len) {
            fputc(' ', to);
        }

        bool hide_memop_type = insn->type == IR_INSN_BRANCH || insn->type == IR_INSN_JUMP || insn->type == IR_INSN_CALL
                               || insn->type == IR_INSN_LEA;
        for (size_t i = 0; i < insn->operands_len; i++) {
            if (i) {
                fputs(", ", to);
            }
            ir_operand_serialize(insn->operands[i], profile_opt, !hide_memop_type || i != 0, to);
        }
    }
}

// Serialize an IR code block.
void ir_code_serialize(ir_code_t *code, backend_profile_t const *profile_opt, FILE *to) {
    fprintf(to, "code %%%s\n", code->name);
    dlist_foreach_node(ir_insn_t, insn, &code->insns) {
        fputs("    ", to);
        ir_insn_serialize(insn, profile_opt, to);
        fputc('\n', to);
    }
}

// Serialize an IR function.
void ir_func_serialize(ir_func_t *func, backend_profile_t const *profile_opt, FILE *to) {
    if (!func->entry) {
        fprintf(stderr, "[BUG] IR function <%s> has no entrypoint\n", func->name);
        abort();
    }

    if (func->enforce_ssa) {
        fputs("ssa_", to);
    }
    fprintf(to, "function <%s>\n    entry %%%s\n", func->name, func->entry->name);

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
        ir_code_serialize(code, profile_opt, to);
    }
}



// Deserialize a single IR function from the file.
// Call multiple times on a single file if you want all the functions.
ir_func_t *ir_func_deserialize(tokenizer_t *from) {
    // Parse the function declaration.
    token_t const ast = ir_parse_func(from);
    if (ast.type != TOKENTYPE_AST || ast.subtype != IR_AST_FUNCTION) {
        return NULL;
    }

    // Actually create the function.
    ir_func_t *func   = ir_func_create_empty(ast.params[1].strval);
    func->enforce_ssa = ast.params[0].subtype == IR_KEYW_ssa_function;
    bool has_entry    = false;
    bool has_errors   = false;

    // Pass 1: Definitions.
    set_t taken_names = STR_SET_EMPTY;
#define claim_name(name)                                                                                               \
    if (!set_add(&taken_names, (name).strval)) {                                                                       \
        has_errors = true;                                                                                             \
        cctx_diagnostic(from->cctx, (name).pos, DIAG_ERR, "Redefinition of %%%s", (name).strval);                      \
    } else

    for (size_t i = 0; i < ast.params[2].params_len; i++) {
        token_t const *stmt = &ast.params[2].params[i];
        switch (stmt->subtype) {
            default: UNREACHABLE();
            case IR_AST_VAR: {
                claim_name(stmt->params[0]) {
                    ir_var_create(func, stmt->params[1].subtype - IR_KEYW_s8 + IR_PRIM_s8, stmt->params[0].strval);
                }
            } break;
            case IR_AST_CODE: {
                claim_name(stmt->params[0]) {
                    ir_code_create(func, stmt->params[0].strval);
                }
            } break;
            case IR_AST_FRAME: {
                claim_name(stmt->params[0]) {
                    ir_frame_create(func, stmt->params[1].ival, stmt->params[2].ival, stmt->params[0].strval);
                }
            } break;
            case IR_AST_ARG:
            case IR_AST_ENTRY:
            case IR_AST_INSN: break;
        }
    }
#undef claim_name

    // Pass 2: References and instructions.
    ir_code_t *cur_code = NULL;
#define expect_str_var     "variable"
#define notfound_str_var   "Variable"
#define expect_str_frame   "stack frame"
#define notfound_str_frame "Stack frame"
#define expect_str_code    "code"
#define notfound_str_code  "Code"
#define get_by_name(type, name)                                                                                        \
    ({                                                                                                                 \
        ir_##type##_t *get_by_name_ = map_get(&func->type##_by_name, (name).strval);                                   \
        if (!get_by_name_ && set_contains(&taken_names, (name).strval)) {                                              \
            cctx_diagnostic(from->cctx, (name).pos, DIAG_ERR, "Expected " expect_str_##type);                          \
        } else if (!get_by_name_) {                                                                                    \
            cctx_diagnostic(from->cctx, (name).pos, DIAG_ERR, notfound_str_##type " not found");                       \
        }                                                                                                              \
        get_by_name_;                                                                                                  \
    })

    for (size_t i = 0; i < ast.params[2].params_len; i++) {
        token_t const *stmt = &ast.params[2].params[i];
        switch (stmt->subtype) {
            default: UNREACHABLE();
            case IR_AST_VAR:
            case IR_AST_FRAME: break;
            case IR_AST_CODE: {
                cur_code = map_get(&func->code_by_name, stmt->params[0].strval);
            } break;
            case IR_AST_ARG: {
                if (stmt->params[0].type == TOKENTYPE_IDENT) {
                    ir_arg_t arg = {.has_var = true, .var = get_by_name(var, stmt->params[0])};
                    if (arg.var && arg.var->arg_index < 0) {
                        arg.var->arg_index = (ptrdiff_t)func->args_len;
                        array_len_insert_strong(&func->args, sizeof(ir_arg_t), &func->args_len, &arg, func->args_len);
                    }
                } else {
                    ir_arg_t arg = {.has_var = false, .type = stmt->params[0].subtype - IR_KEYW_s8 + IR_PRIM_s8};
                    array_len_insert_strong(&func->args, sizeof(ir_arg_t), &func->args_len, &arg, func->args_len);
                }
            } break;
            case IR_AST_ENTRY: {
                if (has_entry) {
                    cctx_diagnostic(from->cctx, stmt->params[0].pos, DIAG_ERR, "Function may only have one entrypoint");
                    has_errors = true;
                } else {
                    func->entry = get_by_name(code, stmt->params[0]);
                    has_entry   = true;
                }
            } break;
            case IR_AST_INSN: {
                bool insn_ok     = true;
                bool operands_ok = true;

                // Find return vars.
                size_t     returns_len = stmt->params[0].params_len;
                ir_var_t **returns     = strong_calloc(returns_len, sizeof(ir_var_t *));
                for (size_t i = 0; i < returns_len; i++) {
                    returns[i]  = get_by_name(var, stmt->params[0].params[i]);
                    insn_ok    &= returns[i] != NULL;
                }

                // Find operands.
                size_t        operands_len = stmt->params[2].params_len;
                ir_operand_t *operands     = strong_calloc(operands_len, sizeof(ir_operand_t));
                for (size_t i = 0; i < operands_len; i++) {
                    if (stmt->params[1].subtype == IR_KEYW_comb) {
                        fprintf(stderr, "[TODO] Deserialize IR combinator\n");
                        abort();
                    }
                    token_t const *operand_ast = &stmt->params[2].params[i];
                    ir_operand_t  *operand     = &operands[i];
                    if (operand_ast->type == TOKENTYPE_IDENT) {
                        // Variable operand.
                        ir_var_t *var = get_by_name(var, *operand_ast);
                        if (!var) {
                            insn_ok     = false;
                            operands_ok = false;
                        }
                        operands[i] = IR_OPERAND_VAR(var);

                    } else if (operand_ast->type == TOKENTYPE_AST) {
                        // Memory operand.
                        token_t const *base_tkn;
                        operand->type  = IR_OPERAND_TYPE_MEM;
                        size_t nontype = 0;
                        if (operand_ast->params[0].type == TOKENTYPE_KEYWORD) {
                            // With a type.
                            nontype                = 1;
                            operand->mem.data_type = operand_ast->params[0].subtype - IR_KEYW_s8 + IR_PRIM_s8;
                        } else {
                            // Without a type.
                            operand->mem.data_type = IR_N_PRIM;
                        }
                        if (operand_ast->params[nontype].type == TOKENTYPE_ICONST) {
                            base_tkn            = &operand_ast->params[nontype + 2];
                            operand->mem.offset = (int64_t)operand_ast->params[nontype].ival;
                            if (operand_ast->params_len > nontype + 1
                                && operand_ast->params[nontype + 1].subtype == IR_TKN_SUB) {
                                operand->mem.offset *= -1;
                            }
                        } else {
                            base_tkn            = &operand_ast->params[nontype];
                            operand->mem.offset = 0;
                        }
                        // TODO: Set correct IR primitive type for this memref.
                        if (!base_tkn) {
                            operand->mem.base_type = IR_MEMBASE_ABS;
                        } else if (base_tkn->subtype == IR_IDENT_GLOBAL) {
                            operand->mem.base_sym  = base_tkn->strval;
                            operand->mem.base_type = IR_MEMBASE_SYM;
                        } else if (map_get(&func->code_by_name, base_tkn->strval)) {
                            operand->mem.base_code = map_get(&func->code_by_name, base_tkn->strval);
                            operand->mem.base_type = IR_MEMBASE_CODE;
                        } else if (map_get(&func->frame_by_name, base_tkn->strval)) {
                            operand->mem.base_frame = map_get(&func->frame_by_name, base_tkn->strval);
                            operand->mem.base_type  = IR_MEMBASE_FRAME;
                        } else if (map_get(&func->var_by_name, base_tkn->strval)) {
                            operand->mem.base_var  = map_get(&func->var_by_name, base_tkn->strval);
                            operand->mem.base_type = IR_MEMBASE_VAR;
                        } else {
                            cctx_diagnostic(
                                from->cctx,
                                base_tkn->pos,
                                DIAG_ERR,
                                "Variable, code, symbol or stack frame not found"
                            );
                            insn_ok     = false;
                            operands_ok = false;
                        }

                    } else {
                        // Constant operand.
                        operands[i] = IR_OPERAND_CONST(((ir_const_t){
                            .prim_type = operand_ast->subtype,
                            .consth    = operand_ast->ivalh,
                            .constl    = operand_ast->ival,
                        }));
                    }
                }

                // Assert SSA preconditions.
                if (func->enforce_ssa) {
                    for (size_t i = 0; i < stmt->params[0].params_len; i++) {
                        if (returns[i] && returns[i]->assigned_at.len) {
                            cctx_diagnostic(
                                from->cctx,
                                stmt->params[0].params[i].pos,
                                DIAG_ERR,
                                "SSA function variable assigned more than once"
                            );
                            insn_ok = false;
                        }
                    }
                }

                // Assert operand count.
                switch (stmt->params[1].subtype) {
                    default: break;
#define IR_OP1_DEF(x) case IR_KEYW_##x:
#include "defs/ir_op1.inc"
                    case IR_KEYW_load:
                    case IR_KEYW_lea:
                    case IR_KEYW_jump:
                        if (operands_len != 1) {
                            cctx_diagnostic(from->cctx, stmt->params[1].pos, DIAG_ERR, "Instruction takes 1 operand");
                            insn_ok = false;
                        }
                        break;
#define IR_OP2_DEF(x) case IR_KEYW_##x:
#include "defs/ir_op2.inc"
                    case IR_KEYW_branch:
                    case IR_KEYW_store:
                        if (operands_len != 2) {
                            cctx_diagnostic(from->cctx, stmt->params[1].pos, DIAG_ERR, "Instruction takes 2 operands");
                            insn_ok = false;
                        }
                        break;
                }

                // Assert return count.
                switch (stmt->params[1].subtype) {
                    default: break;
#define IR_OP1_DEF(x) case IR_KEYW_##x:
#include "defs/ir_op1.inc"
#define IR_OP2_DEF(x) case IR_KEYW_##x:
#include "defs/ir_op2.inc"
                    case IR_KEYW_load:
                    case IR_KEYW_lea:
                    case IR_KEYW_comb:
                        if (returns_len != 1) {
                            cctx_diagnostic(from->cctx, stmt->params[1].pos, DIAG_ERR, "Instruction returns 1 value");
                            insn_ok = false;
                        }
                        break;
                    case IR_KEYW_return:
                    case IR_KEYW_store:
                    case IR_KEYW_branch:
                        if (returns_len != 0) {
                            cctx_diagnostic(from->cctx, stmt->params[1].pos, DIAG_ERR, "Instruction returns nothing");
                            insn_ok = false;
                        }
                        break;
                }

                // Assert instruction preconditions.
                if (!operands_ok) {
                    free(operands);
                    free(returns);
                    break;
                }
                switch (stmt->params[1].subtype) {
                    default: break;

                    case IR_KEYW_branch:
                        // Second operand must be `bool`.
                        if (ir_operand_prim(operands[1]) != IR_PRIM_bool) {
                            cctx_diagnostic(
                                from->cctx,
                                stmt->params[2].params[1].pos,
                                DIAG_ERR,
                                "Branch condition must be `bool`"
                            );
                            insn_ok = false;
                        }
                        if (operands[1].type == IR_OPERAND_TYPE_MEM) {
                            cctx_diagnostic(
                                from->cctx,
                                stmt->params[2].params[1].pos,
                                DIAG_ERR,
                                "Operand must not be a memory operand"
                            );
                            insn_ok = false;
                        }
                        goto operand0_jumpdest;

                    case IR_KEYW_jump:
                    operand0_jumpdest:
                        // First operand must be a memory operand without offset.
                        if (operands[0].type == IR_OPERAND_TYPE_MEM) {
                            if (operands[0].mem.base_type != IR_MEMBASE_CODE) {
                                cctx_diagnostic(
                                    from->cctx,
                                    stmt->params[2].params[0].pos,
                                    DIAG_ERR,
                                    "Operand must relative to code"
                                );
                                insn_ok = false;
                            } else if (operands[0].mem.offset != 0) {
                                cctx_diagnostic(
                                    from->cctx,
                                    stmt->params[2].params[0].pos,
                                    DIAG_ERR,
                                    "Operand must have offset 0"
                                );
                                insn_ok = false;
                            }
                        }
                        // That it definitely is a memory operand is asserted below.
                        goto operand0_mem;

                    case IR_KEYW_load:
                    case IR_KEYW_lea:
                    case IR_KEYW_store:
                    operand0_mem:
                        // First operand must be a memory operand.
                        if (operands[0].type != IR_OPERAND_TYPE_MEM) {
                            cctx_diagnostic(
                                from->cctx,
                                stmt->params[2].params[0].pos,
                                DIAG_ERR,
                                "Operand must be a memory operand"
                            );
                            insn_ok = false;
                        }
                        break;

                    case IR_KEYW_sgt:
                    case IR_KEYW_sle:
                    case IR_KEYW_sge:
                    case IR_KEYW_slt:
                    case IR_KEYW_seq:
                    case IR_KEYW_sne:
                    case IR_KEYW_seqz:
                    case IR_KEYW_snez:
                        // Return type must be bool.
                        if (returns[0] && returns[0]->prim_type != IR_PRIM_bool) {
                            cctx_diagnostic(
                                from->cctx,
                                stmt->params[0].pos,
                                DIAG_ERR,
                                "Instruction always return `bool`"
                            );
                            insn_ok = false;
                        }
                        goto operand_nomem;

                    case IR_KEYW_shl:
                    case IR_KEYW_shr:
                    case IR_KEYW_band:
                    case IR_KEYW_bor:
                    case IR_KEYW_bxor:
                    case IR_KEYW_bneg:
                        // Operand must be an integer or bool.
                        for (size_t i = 0; i < operands_len; i++) {
                            if (ir_operand_prim(operands[i]) > IR_PRIM_bool) {
                                cctx_diagnostic(
                                    from->cctx,
                                    stmt->params[2].params[i].pos,
                                    DIAG_ERR,
                                    "Operand must be an integer type or `bool`"
                                );
                                insn_ok = false;
                            }
                        }
                        goto operand_match;

                    case IR_KEYW_add:
                    case IR_KEYW_sub:
                    case IR_KEYW_mul:
                    case IR_KEYW_div:
                    case IR_KEYW_rem:
                    case IR_KEYW_neg:
                    operand_match:
                        // Operand must equal return type.
                        for (size_t i = 0; i < operands_len; i++) {
                            if (returns[0] && ir_operand_prim(operands[i]) != returns[0]->prim_type) {
                                cctx_diagnostic(
                                    from->cctx,
                                    stmt->params[2].params[i].pos,
                                    DIAG_ERR,
                                    "Operand must match return type"
                                );
                                cctx_diagnostic(
                                    from->cctx,
                                    stmt->params[0].params[0].pos,
                                    DIAG_HINT,
                                    "Return type is %s",
                                    ir_prim_names[returns[0]->prim_type]
                                );
                                insn_ok = false;
                            }
                        }
                        goto operand_nomem;

                    case IR_KEYW_call:
                        // First operand must be a memory operand.
                        if (operands[0].type != IR_OPERAND_TYPE_MEM) {
                            cctx_diagnostic(
                                from->cctx,
                                stmt->params[2].params[0].pos,
                                DIAG_ERR,
                                "Operand must be a memory operand"
                            );
                            insn_ok = false;
                        }
                        goto operand_nomem;

                    case IR_KEYW_return:
                    case IR_KEYW_mov:
                    operand_nomem:
                        // Operand must not be a memory operand.
                        for (size_t i = stmt->params[1].subtype == IR_KEYW_call; i < operands_len; i++) {
                            if (operands[i].type == IR_OPERAND_TYPE_MEM) {
                                cctx_diagnostic(
                                    from->cctx,
                                    stmt->params[2].params[i].pos,
                                    DIAG_ERR,
                                    "Operand must not be a memory operand"
                                );
                                insn_ok = false;
                            }
                        }
                        break;
                }

                // Finally append the instruction.
                if (insn_ok) {
                    switch (stmt->params[1].subtype) {
                        default: UNREACHABLE();

                        case IR_KEYW_comb:
                            fprintf(stderr, "[TODO] Deserialize combinator instruction\n");
                            abort();
                            break;

                        case IR_KEYW_jump: ir_add_jump(IR_APPEND(cur_code), operands[0].mem.base_code); break;

                        case IR_KEYW_branch:
                            ir_add_branch(IR_APPEND(cur_code), operands[1], operands[0].mem.base_code);
                            break;

                        case IR_KEYW_load: ir_add_load(IR_APPEND(cur_code), returns[0], operands[0].mem); break;

                        case IR_KEYW_store: ir_add_store(IR_APPEND(cur_code), operands[1], operands[0].mem); break;

                        case IR_KEYW_lea: ir_add_lea(IR_APPEND(cur_code), returns[0], operands[0].mem); break;

                        case IR_KEYW_call:
                            fprintf(stderr, "[TODO] Deserialize call instruction\n");
                            abort();
                            break;

                        case IR_KEYW_return: ir_add_return(IR_APPEND(cur_code), operands_len, operands); break;

#define IR_OP1_DEF(x) case IR_KEYW_##x:
#include "defs/ir_op1.inc"
                            ir_add_expr1(
                                IR_APPEND(cur_code),
                                returns[0],
                                stmt->params[1].subtype - IR_KEYW_mov + IR_OP1_mov,
                                operands[0]
                            );
                            break;

#define IR_OP2_DEF(x) case IR_KEYW_##x:
#include "defs/ir_op2.inc"
                            ir_add_expr2(
                                IR_APPEND(cur_code),
                                returns[0],
                                stmt->params[1].subtype - IR_KEYW_sgt + IR_OP2_sgt,
                                operands[0],
                                operands[1]
                            );
                            break;
                    }
                }

                free(operands);
                free(returns);
            } break;
        }
    }
#undef expect_str_var
#undef notfound_str_var
#undef expect_str_frame
#undef notfound_str_frame
#undef expect_str_code
#undef notfound_str_code
#undef get_by_name

    if (func->code_list.len == 0) {
        cctx_diagnostic(from->cctx, ast.params[0].pos, DIAG_ERR, "Function has no code");
        has_errors = true;
    } else if (!has_entry) {
        cctx_diagnostic(from->cctx, ast.params[0].pos, DIAG_ERR, "Function has no entrypoint");
        has_errors = true;
    }
    if (has_errors) {
        ir_func_delete(func);
        func = NULL;
    }
    tkn_delete(ast);
    set_clear(&taken_names);

    return func;
}

// Helper to deserialize a single IR function from a string.
// Returns NULL if there are any syntax errors.
ir_func_t *ir_func_deserialize_str(char const *data, size_t data_len, char const *virt_filename) {
    cctx_t      *cctx    = cctx_create();
    srcfile_t   *srcfile = srcfile_create(cctx, virt_filename, data, data_len);
    tokenizer_t *tctx    = ir_tkn_create(srcfile);

    ir_func_t *res = ir_func_deserialize(tctx);

    dlist_foreach_node(diagnostic_t, diag, &cctx->diagnostics) {
        print_diagnostic(diag, stderr);
    }

    tkn_ctx_delete(tctx);
    cctx_delete(cctx);

    return res;
}
