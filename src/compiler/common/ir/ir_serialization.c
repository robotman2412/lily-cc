
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
#include "ir_tokenizer.h"
#include "ir_types.h"
#include "list.h"
#include "map.h"
#include "tokenizer.h"

#include <asm-generic/errno-base.h>
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

        // TODO: This implicitly assumes IR pointers to be 64-bit, but that information doesn't exist yet.
        if (operand.mem.rel_type == IR_MEMREL_ABS) {
            ir_const_serialize(IR_CONST_U64(operand.mem.offset), to);
        } else if (operand.mem.offset) {
            fputs(" + ", to);
            ir_const_serialize(IR_CONST_S64(operand.mem.offset), to);
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
    if (!func->entry) {
        fprintf(stderr, "[BUG] IR function <%s> has no entrypoint\n", func->name);
        abort();
    }

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

    fprintf(to, "entry %%%s\n", func->entry->name);
}



// Helper function that skips past any EOL tokens.
static void ir_skip_eol(tokenizer_t *from) {
    while (tkn_peek(from).type == TOKENTYPE_EOL) {
        tkn_delete(tkn_next(from));
    }
}

// Helper function that eats tokens until EOL or EOF.
static void ir_eat_eol(tokenizer_t *from) {
    while (1) {
        token_t tkn = tkn_next(from);
        tkn_delete(tkn);
        if (tkn.type == TOKENTYPE_EOF || tkn.type == TOKENTYPE_EOL) {
            return;
        }
    }
}

// Helper function that eats tokens until EOL, EOF or RPAR.
static void ir_eat_rpar(tokenizer_t *from) {
    while (1) {
        token_t tkn = tkn_next(from);
        tkn_delete(tkn);
        if (tkn.type == TOKENTYPE_EOF || tkn.type == TOKENTYPE_EOL
            || (tkn.type == TOKENTYPE_OTHER && tkn.subtype == IR_TKN_RPAR)) {
            return;
        }
    }
}

// Helper function that expects RPAR.
static bool ir_expect_rpar(tokenizer_t *from) {
    bool    ok  = true;
    token_t tkn = tkn_next(from);
    if (tkn.type != TOKENTYPE_OTHER || tkn.subtype != IR_TKN_RPAR) {
        cctx_diagnostic(from->cctx, tkn.pos, DIAG_ERR, "Expected )");
        ir_eat_rpar(from);
        ok = false;
    }
    tkn_delete(tkn);
    return ok;
}

// Helper function that expects EOL or EOF.
static bool ir_expect_eol(tokenizer_t *from) {
    bool    ok  = true;
    token_t tkn = tkn_next(from);
    if (tkn.type != TOKENTYPE_EOF && tkn.type != TOKENTYPE_EOL) {
        cctx_diagnostic(from->cctx, tkn.pos, DIAG_ERR, "Expected end-of-line");
        ir_eat_eol(from);
        ok = false;
    }
    tkn_delete(tkn);
    return ok;
}

// Tests whether the given local name is free and errors if not.
static bool ir_check_free_name(ir_func_t *func, bool *has_errors, cctx_t *cctx, token_t name) {
    if (map_get(&func->code_by_name, name.strval) || map_get(&func->var_by_name, name.strval)
        || map_get(&func->frame_by_name, name.strval)) {
        cctx_diagnostic(cctx, name.pos, DIAG_ERR, "The name %%%s is already in use", name.strval);
        *has_errors = true;
        return false;
    }
    return true;
}

// Deserialize a single IR operand from the file.
bool ir_operand_deserialize(tokenizer_t *from, ir_func_t *func, ir_operand_t *operand_out, pos_t *pos_out) {
    bool res = false;

    token_t const tkn = tkn_next(from);
    *pos_out          = tkn.pos;
    if (tkn.type == TOKENTYPE_IDENT && tkn.subtype == IR_IDENT_LOCAL) {
        // Variable.
        ir_var_t *var = map_get(&func->var_by_name, tkn.strval);
        if (!var) {
            cctx_diagnostic(from->cctx, tkn.pos, DIAG_ERR, "No such variable: %%%s", tkn.strval);
        } else {
            operand_out->type = IR_OPERAND_TYPE_VAR;
            operand_out->var  = var;
            res               = true;
        }

    } else if (tkn.type == TOKENTYPE_ICONST) {
        // Numeric constant.
        operand_out->type             = IR_OPERAND_TYPE_CONST;
        operand_out->iconst.prim_type = tkn.subtype;
        operand_out->iconst.constl    = tkn.ival;
        operand_out->iconst.consth    = tkn.ivalh;
        res                           = true;

    } else if (tkn.type == TOKENTYPE_KEYWORD && tkn.subtype == IR_KEYW_true) {
        // True constant.
        *operand_out = IR_OPERAND_CONST(IR_CONST_BOOL(true));
        res          = true;

    } else if (tkn.type == TOKENTYPE_KEYWORD && tkn.subtype == IR_KEYW_false) {
        // False constant.
        *operand_out = IR_OPERAND_CONST(IR_CONST_BOOL(false));
        res          = true;

    } else if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == IR_TKN_UNDEF) {
        // Undefined operand.
        operand_out->type       = IR_OPERAND_TYPE_UNDEF;
        operand_out->undef_type = tkn.ival;
        res                     = true;

    } else if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == IR_TKN_LPAR) {
        token_t const base = tkn_next(from);
        if (base.type == TOKENTYPE_IDENT && base.subtype != IR_IDENT_BARE) {
            // Relative base address.

        } else if (base.type == TOKENTYPE_ICONST) {
            // Absolute base address.
            ir_const_t iconst = {
                .prim_type = base.subtype,
                .constl    = base.ival,
                .consth    = base.ivalh,
            };
            if (iconst.prim_type > IR_PRIM_u128) {
                cctx_diagnostic(from->cctx, base.pos, DIAG_ERR, "Invalid type for addresses");
            } else {
                if (iconst.prim_type > IR_PRIM_u64) {
                    cctx_diagnostic(
                        from->cctx,
                        base.pos,
                        DIAG_WARN,
                        "[TODO] IR addresses assumed to be 64-bit; this will be truncated"
                    );
                }
                res          = ir_expect_rpar(from);
                *operand_out = IR_OPERAND_CONST(iconst);
            }

        } else {
            cctx_diagnostic(from->cctx, base.pos, DIAG_ERR, "Expected memory base address");
            if (base.type != TOKENTYPE_OTHER || base.subtype != IR_TKN_RPAR) {
                ir_eat_rpar(from);
            }
        }
        tkn_delete(base);

    } else {
        cctx_diagnostic(from->cctx, tkn.pos, DIAG_ERR, "Expected an operand");
    }

    tkn_delete(tkn);
    return res;
}

// Deserialize a single IR instruction from the file.
ir_insn_t *ir_insn_deserialize(tokenizer_t *from, ir_func_t *func, bool enforce_ssa, ir_code_t *cur_code) {
    ir_var_t **returns     = NULL;
    size_t     returns_len = 0;
    size_t     returns_cap = 0;

    ir_operand_t *operands     = NULL;
    size_t        operands_len = 0;
    size_t        operands_cap = 0;

    pos_t *operands_pos     = NULL;
    size_t operands_pos_len = 0;
    size_t operands_pos_cap = 0;

    ir_insn_t *insn = NULL;

    // Parse returns list.
    token_t peek = tkn_peek(from);
    if (peek.type == TOKENTYPE_IDENT && peek.subtype == IR_IDENT_LOCAL) {
        tkn_next(from);
        ir_var_t *var = map_get(&func->var_by_name, peek.strval);
        if (!var) {
            cctx_diagnostic(from->cctx, peek.pos, DIAG_ERR, "No such variable: %%%s", peek.strval);
            tkn_delete(peek);
            goto error;
        } else if (enforce_ssa && (var->assigned_at.len || var->arg_index >= 0)) {
            cctx_diagnostic(from->cctx, peek.pos, DIAG_ERR, "SSA IR variable assigned twice: %%%s", peek.strval);
            tkn_delete(peek);
            goto error;
        }
        array_lencap_insert_strong(&returns, sizeof(ir_var_t *), &returns_len, &returns_cap, &var, returns_len);
        tkn_delete(peek);

        while (1) {
            token_t const tkn = tkn_next(from);
            if (tkn.type == TOKENTYPE_OTHER && tkn.subtype == IR_TKN_ASSIGN) {
                break;
            } else if (tkn.type != TOKENTYPE_OTHER || tkn.subtype != IR_TKN_COMMA) {
                cctx_diagnostic(from->cctx, tkn.pos, DIAG_ERR, "Expected ',' or '=' after return variable");
                tkn_delete(tkn);
                goto error;
            }
            tkn_delete(tkn);

            // If it's a comma, expect another identifier.
            token_t const ident = tkn_peek(from);
            if (ident.type == TOKENTYPE_IDENT && ident.subtype == IR_IDENT_LOCAL) {
                var = map_get(&func->var_by_name, peek.strval);
                if (!var) {
                    cctx_diagnostic(from->cctx, peek.pos, DIAG_ERR, "No such variable: %%%s", peek.strval);
                    tkn_delete(ident);
                    goto error;
                } else if (enforce_ssa && (var->assigned_at.len || var->arg_index >= 0)) {
                    cctx_diagnostic(
                        from->cctx,
                        peek.pos,
                        DIAG_ERR,
                        "SSA IR variable assigned twice: %%%s",
                        peek.strval
                    );
                    tkn_delete(ident);
                    goto error;
                }
                array_lencap_insert_strong(&returns, sizeof(ir_var_t *), &returns_len, &returns_cap, &var, returns_len);
                tkn_delete(ident);
            } else {
                cctx_diagnostic(from->cctx, ident.pos, DIAG_ERR, "Expected local identifier after ','");
                tkn_delete(ident);
                goto error;
            }
        }
    }

    // Parse instruction type.
    token_t const mnemonic = tkn_next(from);
    if (mnemonic.type != TOKENTYPE_KEYWORD) {
        cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected an instruction mnemonic");
        goto error;
    }

    // Parse operands.
    peek = tkn_peek(from);
    if (peek.type != TOKENTYPE_EOF && peek.type != TOKENTYPE_EOL) {
        while (1) {
            ir_operand_t operand;
            pos_t        pos;
            if (!ir_operand_deserialize(from, func, &operand, &pos)) {
                goto error;
            }
            array_lencap_insert_strong(
                &operands,
                sizeof(ir_operand_t),
                &operands_len,
                &operands_cap,
                &operand,
                operands_len
            );
            array_lencap_insert_strong(
                &operands_pos,
                sizeof(pos_t),
                &operands_pos_len,
                &operands_pos_cap,
                &pos,
                operands_pos_len
            );

            peek = tkn_peek(from);
            if (peek.type == TOKENTYPE_EOF || peek.type == TOKENTYPE_EOL) {
                break;
            } else if (peek.type == TOKENTYPE_OTHER && peek.subtype == IR_TKN_COMMA) {
                tkn_next(from);
            } else {
                cctx_diagnostic(from->cctx, peek.pos, DIAG_ERR, "Expected ',' or end of line after operand");
                goto error;
            }
        }
    }

    // Create instruction.
    switch (mnemonic.subtype) {
        case IR_KEYW_sgt ... IR_KEYW_bxor:
            if (operands_len != 2) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected two operands");
                goto error;
            } else if (returns_len != 1) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected one assignment");
                goto error;
            }
            insn = ir_add_expr2(
                IR_APPEND(cur_code),
                returns[0],
                mnemonic.subtype - IR_KEYW_sgt + IR_OP2_sgt,
                operands[0],
                operands[1]
            );
            break;
        case IR_KEYW_mov ... IR_KEYW_bneg:
            if (operands_len != 1) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected one operand");
                goto error;
            } else if (returns_len != 1) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected one assignment");
                goto error;
            }
            insn = ir_add_expr1(
                IR_APPEND(cur_code),
                returns[0],
                mnemonic.subtype - IR_KEYW_mov + IR_OP1_mov,
                operands[0]
            );
            break;
        case IR_KEYW_load:
            if (operands_len != 1) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected one operand");
                goto error;
            } else if (returns_len != 1) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected one assignment");
                goto error;
            } else if (operands[0].type != IR_OPERAND_TYPE_MEM) {
                cctx_diagnostic(from->cctx, operands_pos[0], DIAG_ERR, "Expected a memory operand");
                goto error;
            }
            insn = ir_add_load(IR_APPEND(cur_code), returns[0], operands[0]);
            break;
        case IR_KEYW_lea:
            if (operands_len != 1) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected one operand");
                goto error;
            } else if (returns_len != 1) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected one assignment");
                goto error;
            } else if (operands[0].type != IR_OPERAND_TYPE_MEM) {
                cctx_diagnostic(from->cctx, operands_pos[0], DIAG_ERR, "Expected a memory operand");
                goto error;
            }
            insn = ir_add_load(IR_APPEND(cur_code), returns[0], operands[0]);
            break;
        case IR_KEYW_store:
            if (operands_len != 2) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected two operands");
                goto error;
            } else if (returns_len != 0) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Unexpected assignment");
                goto error;
            } else if (operands[1].type != IR_OPERAND_TYPE_MEM) {
                cctx_diagnostic(from->cctx, operands_pos[1], DIAG_ERR, "Expected a memory operand");
                goto error;
            }
            insn = ir_add_store(IR_APPEND(cur_code), operands[0], operands[1]);
            break;
        // case IR_KEYW_comb: break; // TODO: This requires different operand parsing.
        case IR_KEYW_jump:
            if (operands_len != 1) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected one operand");
                goto error;
            } else if (returns_len != 0) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Unexpected assignment");
                goto error;
            } else if (operands[0].type != IR_OPERAND_TYPE_MEM || operands[0].mem.rel_type != IR_MEMREL_CODE) {
                cctx_diagnostic(from->cctx, operands_pos[0], DIAG_ERR, "Expected a code block memory operand");
                goto error;
            }
            insn = ir_add_jump(IR_APPEND(cur_code), operands[0].mem.base_code);
            break;
        case IR_KEYW_branch:
            if (operands_len != 2) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected two operands");
                goto error;
            } else if (returns_len != 0) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Unexpected assignment");
                goto error;
            } else if (ir_operand_prim(operands[0]) != IR_PRIM_bool) {
                cctx_diagnostic(from->cctx, operands_pos[0], DIAG_ERR, "Expected a boolean operand");
                goto error;
            } else if (operands[1].type != IR_OPERAND_TYPE_MEM || operands[1].mem.rel_type != IR_MEMREL_CODE) {
                cctx_diagnostic(from->cctx, operands_pos[1], DIAG_ERR, "Expected a code block memory operand");
                goto error;
            }
            insn = ir_add_branch(IR_APPEND(cur_code), operands[0], operands[1].mem.base_code);
            break;
        case IR_KEYW_return:
            if (returns_len != 0) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Unexpected assignment");
                goto error;
            } else if (operands_len >= 2) {
                cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Expected one or zero operands");
                goto error;
            } else if (operands_len == 1) {
                if (returns_len != 0) {
                    cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "Unexpected assignment");
                    goto error;
                }
                insn = ir_add_return1(IR_APPEND(cur_code), operands[0]);
            } else {
                insn = ir_add_return0(IR_APPEND(cur_code));
            }
            break;
        default:
            cctx_diagnostic(from->cctx, mnemonic.pos, DIAG_ERR, "No such instruction");
            goto error;
            break;
    }

error:
    free(returns);
    free(operands);
    free(operands_pos);

    if (!insn) {
        ir_eat_eol(from);
    } else {
        bool err  = false;
        err      |= !ir_expect_eol(from);
        if (err) {
            ir_insn_delete(insn);
            return NULL;
        }
    }

    return insn;
}

// Deserialize a single IR function from the file.
// Call multiple times on a single file if you want all the functions.
ir_func_t *ir_func_deserialize(tokenizer_t *from) {
    ir_skip_eol(from);

    // Parse the function declaration.
    token_t function = tkn_next(from);
    if (function.type != TOKENTYPE_KEYWORD
        || (function.subtype != IR_KEYW_ssa_function && function.subtype != IR_KEYW_function)) {
        cctx_diagnostic(from->cctx, function.pos, DIAG_ERR, "Expected `function` or `ssa_function`");
        return NULL;
    }

    token_t funcname = tkn_next(from);
    if (funcname.type != TOKENTYPE_IDENT || funcname.subtype != IR_IDENT_GLOBAL) {
        cctx_diagnostic(from->cctx, function.pos, DIAG_ERR, "Expected a global identifier");
        return NULL;
    }

    // Actually create the function.
    ir_func_t *func        = ir_func_create_empty(funcname.strval);
    bool       enforce_ssa = function.subtype == IR_KEYW_ssa_function;
    bool       has_entry   = false;
    bool       has_errors  = false;
    ir_code_t *cur_code    = NULL;
    while (1) {
        ir_skip_eol(from);
        token_t const tkn = tkn_peek(from);
        if (tkn.type == TOKENTYPE_EOF
            || (tkn.type == TOKENTYPE_KEYWORD
                && (tkn.subtype == IR_KEYW_function || tkn.subtype == IR_KEYW_ssa_function))) {
            // End of this function.
            break;
        }

        if (tkn.type == TOKENTYPE_KEYWORD && tkn.subtype == IR_KEYW_code) {
            tkn_next(from);
            // Create a new code block.
            token_t const name = tkn_next(from);
            if (name.type != TOKENTYPE_IDENT || name.subtype != IR_IDENT_LOCAL) {
                cctx_diagnostic(from->cctx, name.pos, DIAG_ERR, "Expected a local identifier");
                ir_eat_eol(from);
                has_errors = true;
            } else {
                if (ir_check_free_name(func, &has_errors, from->cctx, name)) {
                    cur_code = ir_code_create(func, name.strval);
                }
                has_errors |= !ir_expect_eol(from);
            }
            tkn_delete(name);

        } else if (tkn.type == TOKENTYPE_KEYWORD && tkn.subtype == IR_KEYW_entry) {
            tkn_next(from);
            // Set the function's entrypoint.
            token_t const name = tkn_next(from);
            if (name.type != TOKENTYPE_IDENT || name.subtype != IR_IDENT_LOCAL) {
                cctx_diagnostic(from->cctx, name.pos, DIAG_ERR, "Expected a local identifier");
                ir_eat_eol(from);
                has_errors = true;
            } else {
                if (has_entry) {
                    cctx_diagnostic(
                        from->cctx,
                        pos_between(tkn.pos, name.pos),
                        DIAG_ERR,
                        "Function entrypoint specified twice"
                    );
                    has_errors = true;
                }
                has_entry   = true;
                func->entry = map_get(&func->code_by_name, name.strval);
                if (!func->entry) {
                    cctx_diagnostic(from->cctx, name.pos, DIAG_ERR, "No such code block: %%%s", name.strval);
                    has_errors = true;
                }
                has_errors |= !ir_expect_eol(from);
            }
            tkn_delete(name);

        } else if (tkn.type == TOKENTYPE_KEYWORD && tkn.subtype == IR_KEYW_var) {
            tkn_next(from);
            // Create a new variable.
            token_t const name = tkn_next(from);
            if (name.type != TOKENTYPE_IDENT || name.subtype != IR_IDENT_LOCAL) {
                cctx_diagnostic(from->cctx, name.pos, DIAG_ERR, "Expected a local identifier");
                ir_eat_eol(from);
                has_errors = true;
            } else {
                token_t const prim = tkn_next(from);
                if (prim.type != TOKENTYPE_KEYWORD || prim.subtype < IR_KEYW_s8 || prim.subtype >= IR_KEYW_comb) {
                    cctx_diagnostic(from->cctx, prim.pos, DIAG_ERR, "Expected a type");
                    ir_eat_eol(from);
                    has_errors = true;
                } else {
                    if (ir_check_free_name(func, &has_errors, from->cctx, name)) {
                        ir_var_create(func, prim.subtype - IR_KEYW_s8 + IR_PRIM_s8, name.strval);
                    }
                    has_errors |= !ir_expect_eol(from);
                }
                tkn_delete(prim);
            }
            tkn_delete(name);

        } else if (tkn.type == TOKENTYPE_KEYWORD && tkn.subtype == IR_KEYW_arg) {
            tkn_next(from);
            // Make a function argument.
            token_t const arg = tkn_next(from);
            if (arg.type == TOKENTYPE_KEYWORD && arg.subtype >= IR_KEYW_s8 && arg.subtype <= IR_KEYW_f64) {
                // An ignored argument; just has the type.
                ir_arg_t entry = {
                    .has_var = false,
                    .type    = arg.subtype - IR_KEYW_s8 + IR_PRIM_s8,
                };
                array_len_insert_strong(&func->args, sizeof(ir_arg_t), &func->args_len, &entry, func->args_len);
                has_errors |= !ir_expect_eol(from);

            } else if (arg.type == TOKENTYPE_IDENT) {
                // A normal argument; has a variable.
                ir_arg_t entry = {
                    .has_var = true,
                    .var     = map_get(&func->var_by_name, arg.strval),
                };
                if (!entry.var) {
                    cctx_diagnostic(from->cctx, arg.pos, DIAG_ERR, "No such variable: %%%s", arg.strval);
                    has_errors = true;
                } else if (entry.var->arg_index >= 0) {
                    cctx_diagnostic(
                        from->cctx,
                        arg.pos,
                        DIAG_ERR,
                        "Variable is already used as an argument: %%%s",
                        arg.strval
                    );
                    has_errors = true;
                } else {
                    entry.var->arg_index = func->args_len;
                    array_len_insert_strong(&func->args, sizeof(ir_arg_t), &func->args_len, &entry, func->args_len);
                }
                has_errors |= !ir_expect_eol(from);

            } else {
                cctx_diagnostic(from->cctx, arg.pos, DIAG_ERR, "Expected a local identifier or a type");
                ir_eat_eol(from);
                has_errors = true;
            }
            tkn_delete(arg);

        } else if (tkn.type == TOKENTYPE_KEYWORD && tkn.subtype == IR_KEYW_frame) {
            tkn_next(from);
            // Make a new stack frame.
            token_t const name = tkn_next(from);
            if (name.type != TOKENTYPE_IDENT || name.subtype != IR_IDENT_LOCAL) {
                cctx_diagnostic(from->cctx, name.pos, DIAG_ERR, "Expected a local identifier");
                ir_eat_eol(from);
                has_errors = true;
            } else {
                token_t const size = tkn_next(from);
                if (size.type != TOKENTYPE_ICONST || size.subtype > IR_PRIM_u64 || size.ival >> 63) {
                    cctx_diagnostic(
                        from->cctx,
                        size.pos,
                        DIAG_ERR,
                        "Expected a positive numeric constant no bigger than 64-bit"
                    );
                    ir_eat_eol(from);
                    has_errors = true;
                } else {
                    token_t const align = tkn_next(from);
                    if (align.type != TOKENTYPE_ICONST || align.subtype > IR_PRIM_u64 || align.ival >> 63) {
                        cctx_diagnostic(
                            from->cctx,
                            align.pos,
                            DIAG_ERR,
                            "Expected a positive numeric constant no bigger than 64-bit"
                        );
                        ir_eat_eol(from);
                        has_errors = true;
                    } else {
                        if (ir_check_free_name(func, &has_errors, from->cctx, name)) {
                            ir_frame_create(func, size.ival, align.ival, name.strval);
                        }
                        has_errors |= !ir_expect_eol(from);
                    }
                }
            }
            tkn_delete(name);
        } else {
            has_errors |= !ir_insn_deserialize(from, func, enforce_ssa, cur_code);
        }
    }

    func->enforce_ssa = enforce_ssa;
    if (func->code_list.len == 0) {
        cctx_diagnostic(from->cctx, pos_between(function.pos, funcname.pos), DIAG_ERR, "Function has no code");
        has_errors = true;
    } else if (!has_entry) {
        cctx_diagnostic(from->cctx, pos_between(function.pos, funcname.pos), DIAG_ERR, "Function has no entrypoint");
        has_errors = true;
    }
    if (has_errors) {
        ir_func_delete(func);
        func = NULL;
    }
    return func;
}

// Helper to deserialize a single IR function from a string.
// Returns NULL if there are any syntax errors.
ir_func_t *ir_func_deserialize_str(char const *data, size_t data_len, char const *virt_filename) {
    cctx_t      *cctx    = cctx_create();
    srcfile_t   *srcfile = srcfile_create(cctx, virt_filename, data, data_len);
    tokenizer_t *tctx    = ir_tkn_create(srcfile);

    ir_func_t *res = ir_func_deserialize(tctx);

    if (cctx->diagnostics.len && res) {
        ir_func_delete(res);
        res = NULL;
    }
    dlist_foreach_node(diagnostic_t, diag, &cctx->diagnostics) {
        print_diagnostic(diag);
    }

    tkn_ctx_delete(tctx);
    cctx_delete(cctx);

    return res;
}
