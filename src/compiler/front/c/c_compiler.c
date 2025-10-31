
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"

#include "c_parser.h"
#include "c_prepass.h"
#include "c_tokenizer.h"
#include "c_types.h"
#include "c_values.h"
#include "compiler.h"
#include "ir.h"
#include "ir/ir_interpreter.h"
#include "ir_types.h"
#include "map.h"
#include "refcount.h"
#include "strong_malloc.h"

#include <stdio.h>
#include <stdlib.h>


// Guards against actual cleanup happening for the fake RC types.
static void c_compiler_t_fake_cleanup(void *_) {
    (void)_;
    printf("[BUG] Refcount ptr to primitive cache destroyed\n");
    abort();
}

// Create a new C compiler context.
c_compiler_t *c_compiler_create(cctx_t *cctx, c_options_t options) {
    c_compiler_t *cc                = strong_calloc(1, sizeof(c_compiler_t));
    cc->cctx                        = cctx;
    cc->options                     = options;
    cc->global_scope.locals         = STR_MAP_EMPTY;
    cc->global_scope.locals_by_decl = PTR_MAP_EMPTY;
    cc->global_scope.typedefs       = STR_MAP_EMPTY;
    cc->global_scope.comp_types     = STR_MAP_EMPTY;

    for (size_t i = 0; i < C_N_PRIM; i++) {
        cc->prim_types[i].primitive = i;
        cc->prim_rcs[i].refcount    = 1;
        cc->prim_rcs[i].data        = &cc->prim_types[i];
        cc->prim_rcs[i].cleanup     = c_compiler_t_fake_cleanup;
    }

    return cc;
}

// Destroy a C compiler context.
void c_compiler_destroy(c_compiler_t *cc) {
    c_scope_destroy(cc->global_scope);
    free(cc);
}


// Create a new scope.
c_scope_t c_scope_create(c_scope_t *parent) {
    c_scope_t new_scope      = {0};
    new_scope.parent         = parent;
    new_scope.depth          = parent->depth + 1;
    new_scope.locals         = STR_MAP_EMPTY;
    new_scope.locals_by_decl = PTR_MAP_EMPTY;
    new_scope.typedefs       = STR_MAP_EMPTY;
    new_scope.comp_types     = STR_MAP_EMPTY;
    return new_scope;
}

// Clean up a scope.
void c_scope_destroy(c_scope_t scope) {
    // Delete local variables.
    map_ent_t const *ent = map_next(&scope.locals, NULL);
    while (ent) {
        c_var_t *local = ent->value;
        rc_delete(local->type);
        free(local);
        ent = map_next(&scope.locals, ent);
    }
    map_clear(&scope.locals);
    map_clear(&scope.locals_by_decl);

    // Delete typedefs.
    map_foreach(ent, &scope.typedefs) {
        rc_delete(ent->value);
    }
    map_clear(&scope.typedefs);

    // Delete enums, structs and unions.
    map_foreach(ent, &scope.comp_types) {
        rc_delete(ent->value);
    }
    map_clear(&scope.comp_types);
}

// Look up a variable in scope.
c_var_t *c_scope_lookup(c_scope_t *scope, char const *ident) {
    while (scope) {
        c_var_t *var = map_get(&scope->locals, ident);
        if (var) {
            return var;
        }
        scope = scope->parent;
    }
    return NULL;
}

// Look up a variable in scope by declaration token.
c_var_t *c_scope_lookup_by_decl(c_scope_t *scope, token_t const *decl) {
    while (scope) {
        c_var_t *var = map_get(&scope->locals_by_decl, decl);
        if (var) {
            return var;
        }
        scope = scope->parent;
    }
    return NULL;
}

// Look up a compound type in scope.
rc_t c_scope_lookup_comp(c_scope_t *scope, char const *ident) {
    while (scope) {
        rc_t var = map_get(&scope->comp_types, ident);
        if (var) {
            return rc_share(var);
        }
        scope = scope->parent;
    }
    return NULL;
}

// Look up a typedef in scope.
rc_t c_scope_lookup_typedef(c_scope_t *scope, char const *ident) {
    while (scope) {
        rc_t var = map_get(&scope->typedefs, ident);
        if (var) {
            return rc_share(var);
        }
        scope = scope->parent;
    }
    return NULL;
}



// Convert C binary operator to IR binary operator.
ir_op2_type_t c_op2_to_ir_op2(c_tokentype_t subtype) {
    switch (subtype) {
        case C_TKN_ADD: return IR_OP2_add;
        case C_TKN_SUB: return IR_OP2_sub;
        case C_TKN_MUL: return IR_OP2_mul;
        case C_TKN_DIV: return IR_OP2_div;
        case C_TKN_MOD: return IR_OP2_rem;
        case C_TKN_SHL: return IR_OP2_shl;
        case C_TKN_SHR: return IR_OP2_shr;
        case C_TKN_AND: return IR_OP2_band;
        case C_TKN_OR: return IR_OP2_bor;
        case C_TKN_XOR: return IR_OP2_bxor;
        case C_TKN_EQ: return IR_OP2_seq;
        case C_TKN_NE: return IR_OP2_sne;
        case C_TKN_LT: return IR_OP2_slt;
        case C_TKN_LE: return IR_OP2_sle;
        case C_TKN_GT: return IR_OP2_sgt;
        case C_TKN_GE: return IR_OP2_sge;
        default:
            printf("[BUG] C token %d cannot be converted to IR op2\n", subtype);
            abort();
            break;
    }
}

// Convert C unary operator to IR unary operator.
ir_op1_type_t c_op1_to_ir_op1(c_tokentype_t subtype) {
    switch (subtype) {
        case C_TKN_SUB: return IR_OP1_neg;
        case C_TKN_NOT: return IR_OP1_bneg;
        default:
            printf("[BUG] C token %d cannot be converted to IR op1\n", subtype);
            abort();
            break;
    }
}



// Create a C variable in the current translation unit.
// If in global scope, `code` and `prepass` must be `NULL`.
c_var_t *c_var_create(
    c_compiler_t *ctx, c_prepass_t *prepass, ir_func_t *func, rc_t type_rc, token_t const *name_tkn, c_scope_t *scope
) {
    c_type_t *type = type_rc->data;

    if (type->primitive == C_COMP_FUNCTION) {
        fprintf(stderr, "[TODO] c_var_create C_COMP_FUNCTION\n");
        return NULL;
    }

    // Enforce that it is a complete type.
    size_t size, align;
    if (!c_type_get_size(ctx, type, &size, &align)) {
        c_comp_t   *comp = type->comp->data;
        char const *comp_var;
        switch (comp->type) {
            case C_COMP_TYPE_ENUM: comp_var = "enum"; break;
            case C_COMP_TYPE_STRUCT: comp_var = "struct"; break;
            case C_COMP_TYPE_UNION: comp_var = "union"; break;
            default: __builtin_unreachable();
        }
        cctx_diagnostic(ctx->cctx, name_tkn->pos, DIAG_ERR, "Use of incomplete type %s %s", comp_var, comp->name);
        rc_delete(type_rc);
        return NULL;
    }

    if (map_get(&scope->locals, name_tkn->strval)
        || (scope->local_exclusive && scope->parent && map_get(&scope->parent->locals, name_tkn->strval))) {
        cctx_diagnostic(ctx->cctx, name_tkn->pos, DIAG_ERR, "Redefinition of %s", name_tkn->strval);
        rc_delete(type_rc);
        return NULL;
    }

    c_var_t *var = calloc(1, sizeof(c_var_t));
    var->type    = type_rc;
    if (func) {
        // Can create a valid local variable.
        bool is_struct = type->primitive == C_COMP_STRUCT || type->primitive == C_COMP_UNION;
        if (is_struct || set_contains(&prepass->pointer_taken, name_tkn)) {
            var->storage = C_VAR_STORAGE_FRAME;
            var->frame   = ir_frame_create(func, size, align, NULL);
        } else {
            var->storage = C_VAR_STORAGE_REG;
            var->ir_var  = ir_var_create(func, c_type_to_ir_type(ctx, type), NULL);
        }
    } else {
        fprintf(stderr, "[TODO] c_var_create in global scope\n");
    }

    map_set(&scope->locals, name_tkn->strval, var);
    map_set(&scope->locals_by_decl, name_tkn, var);

    return var;
}



// Decay an array value into its pointer.
static c_value_t c_array_decay(c_compiler_t *ctx, ir_code_t *code, c_value_t value) {
    c_type_t const *type = value.c_type->data;
    if (type->primitive != C_COMP_ARRAY) {
        return value;
    }
    ir_prim_t ptr_prim = c_prim_to_ir_type(ctx, ctx->options.size_type);

    if (c_is_rvalue(&value)) {
        fprintf(stderr, "[TODO] Decay rvalue array into pointer");
        abort();
    }

    rc_t        ptr_rc = c_type_pointer(ctx, rc_share(type->inner));
    ir_memref_t memref = c_value_memref(ctx, code, &value);
    c_value_destroy(value);
    if (memref.base_type == IR_MEMBASE_ABS) {
        // Can (and must, for design reasons) optimize array address into a constant.
        return (c_value_t){
            .value_type     = C_RVALUE_OPERAND,
            .c_type         = ptr_rc,
            .rvalue.operand = {
                .type   = IR_OPERAND_TYPE_CONST,
                .iconst = {
                    .prim_type = ptr_prim,
                    .consth    = 0,
                    .constl    = memref.offset,
                },
            },
        };
    } else {
        // The array address is not constant.
        ir_var_t *tmp = ir_var_create(code->func, ptr_prim, NULL);
        ir_add_lea(IR_APPEND(code), tmp, memref);
        return (c_value_t){
            .value_type     = C_RVALUE_OPERAND,
            .c_type         = ptr_rc,
            .rvalue.operand = IR_OPERAND_VAR(tmp),
        };
    }
}

// Compile an infix arithmetic expression into IR.
static inline c_compile_expr_t
    c_compile_expr2_arith(c_compiler_t *ctx, ir_code_t *code, token_t const *expr, c_value_t lhs, c_value_t rhs) {
    bool            is_assign = expr->params[0].subtype >= C_TKN_ADD_S && expr->params[0].subtype <= C_TKN_XOR_S;
    c_type_t const *lhs_type  = lhs.c_type->data;
    c_type_t const *rhs_type  = rhs.c_type->data;

    // Determine promotion.
    c_tokentype_t op2     = is_assign ? expr->params[0].subtype + C_TKN_ADD - C_TKN_ADD_S : expr->params[0].subtype;
    c_prim_t      c_prim  = c_prim_promote(lhs_type->primitive, rhs_type->primitive);
    ir_prim_t     ir_prim = c_prim_to_ir_type(ctx, c_prim);
    bool const    is_cmp  = op2 >= C_TKN_EQ && op2 <= C_TKN_GE;

    // Cast the variables if needed.
    ir_operand_t ir_lhs = c_cast_ir_operand(code, c_value_read(ctx, code, &lhs), ir_prim);
    ir_operand_t ir_rhs = c_cast_ir_operand(code, c_value_read(ctx, code, &rhs), ir_prim);

    ir_op2_type_t ir_op2 = c_op2_to_ir_op2(op2);
    if ((ir_op2 == IR_OP2_div || ir_op2 == IR_OP2_rem) && ir_calc1(IR_OP1_seqz, ir_rhs.iconst).constl) {
        cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Constant division by zero");
        return (c_compile_expr_t){
            .code = code,
            .res  = (c_value_t){0},
        };
    }

    if (ir_lhs.type == IR_OPERAND_TYPE_CONST && ir_rhs.type == IR_OPERAND_TYPE_CONST) {
        // Resulting constant can be evaluated at compile-time.
        c_value_destroy(lhs);
        c_value_destroy(rhs);
        ir_const_t tmp = ir_calc2(ir_op2, ir_lhs.iconst, ir_rhs.iconst);
        if (is_cmp) {
            tmp    = ir_cast(c_prim_to_ir_type(ctx, C_PRIM_SINT), tmp);
            c_prim = C_PRIM_SINT;
        }
        return (c_compile_expr_t){
            .code = code,
            .res  = {
                 .value_type     = C_RVALUE_OPERAND,
                 .c_type         = rc_share(&ctx->prim_rcs[c_prim]),
                 .rvalue.operand = IR_OPERAND_CONST(tmp),
            },
        };
    }

    // Add math instruction.
    ir_var_t *tmpvar = ir_var_create(code->func, is_cmp ? IR_PRIM_bool : ir_prim, NULL);
    ir_add_expr2(IR_APPEND(code), tmpvar, ir_op2, ir_lhs, ir_rhs);
    if (is_cmp) {
        ir_var_t *tmpvar2 = ir_var_create(code->func, c_prim_to_ir_type(ctx, C_PRIM_SINT), NULL);
        ir_add_expr1(IR_APPEND(code), tmpvar2, IR_OP1_mov, IR_OPERAND_VAR(tmpvar));
        tmpvar = tmpvar2;
        c_prim = C_PRIM_SINT;
    }

    c_value_t rvalue = {
        .value_type     = C_RVALUE_OPERAND,
        .c_type         = rc_share(&ctx->prim_rcs[c_prim]),
        .rvalue.operand = IR_OPERAND_VAR(tmpvar),
    };

    if (expr->params[0].subtype >= C_TKN_ADD_S && expr->params[0].subtype <= C_TKN_XOR_S) {
        // Assignment arithmetic expression; write back.
        if (c_value_assignable(ctx, &lhs, expr->params[1].pos)) {
            c_value_write(ctx, code, &lhs, &rvalue);
        }
    }

    c_value_destroy(lhs);
    c_value_destroy(rhs);
    return (c_compile_expr_t){
        .code = code,
        .res  = rvalue,
    };
}

// Compile an infix pointer arithmetic expression into IR.
static inline c_compile_expr_t
    c_compile_expr2_ptrarith(c_compiler_t *ctx, ir_code_t *code, token_t const *expr, c_value_t lhs, c_value_t rhs) {
    ir_prim_t     ptr_prim = c_prim_to_ir_type(ctx, ctx->options.size_type);
    ir_op2_type_t op2      = c_op2_to_ir_op2(expr->params[0].subtype);

    // Perform array decay, if needed.
    lhs = c_array_decay(ctx, code, lhs);
    rhs = c_array_decay(ctx, code, rhs);

    c_type_t const *lhs_type = lhs.c_type->data;
    c_type_t const *rhs_type = rhs.c_type->data;
    bool const      lhs_ptr  = lhs_type->primitive == C_COMP_POINTER;
    bool const      rhs_ptr  = rhs_type->primitive == C_COMP_POINTER;

    uint64_t size = 0, align = 0;
    if (op2 == IR_OP2_sub || op2 == IR_OP2_add) {
        // Assert that pointers are complete types with nonzero size.
        if (lhs_ptr) {
            if (!c_type_get_size(ctx, lhs_type->inner->data, &size, &align)) {
                cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Pointer arithmetic with incomplete type");
                c_value_destroy(lhs);
                c_value_destroy(rhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            } else if (size == 0) {
                cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Pointer arithmetic with zero-size type");
                c_value_destroy(lhs);
                c_value_destroy(rhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
        }
        if (rhs_ptr) {
            if (!c_type_get_size(ctx, rhs_type->inner->data, &size, &align)) {
                cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Pointer arithmetic with incomplete type");
                c_value_destroy(lhs);
                c_value_destroy(rhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            } else if (size == 0) {
                cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Pointer arithmetic with zero-size type");
                c_value_destroy(lhs);
                c_value_destroy(rhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
        }
    }
    ir_const_t size_iconst = (ir_const_t){.prim_type = ptr_prim, .consth = 0, .constl = size};

    ir_operand_t lhs_ir   = c_value_read(ctx, code, &lhs);
    ir_operand_t rhs_ir   = c_value_read(ctx, code, &rhs);
    bool const   is_const = lhs_ir.type == IR_OPERAND_TYPE_CONST && rhs_ir.type == IR_OPERAND_TYPE_CONST;

    ir_operand_t value;
    rc_t         type_rc;
    if (op2 == IR_OP2_sub && lhs_ptr && rhs_ptr) {
        // Pointer difference.
        type_rc = rc_share(&ctx->prim_rcs[ptr_prim]);
        if (is_const) {
            value = IR_OPERAND_CONST(
                ir_calc2(IR_OP2_div, ir_calc2(IR_OP2_sub, lhs_ir.iconst, rhs_ir.iconst), size_iconst)
            );
        } else {
            ir_var_t *sub = ir_var_create(code->func, ptr_prim, NULL);
            ir_add_expr2(IR_APPEND(code), sub, IR_OP2_sub, lhs_ir, rhs_ir);
            ir_var_t *div = ir_var_create(code->func, ptr_prim, NULL);
            ir_add_expr2(IR_APPEND(code), div, IR_OP2_div, IR_OPERAND_VAR(sub), IR_OPERAND_CONST(size_iconst));
            value = IR_OPERAND_VAR(div);
        }

    } else if (op2 == IR_OP2_sub) {
        // Pointer offset (subtract edition).
        type_rc = rc_share(lhs.c_type);
        if (is_const) {
            value = IR_OPERAND_CONST(
                ir_calc2(IR_OP2_sub, lhs_ir.iconst, ir_calc2(IR_OP2_mul, rhs_ir.iconst, size_iconst))
            );
        } else {
            ir_var_t *mul = ir_var_create(code->func, ptr_prim, NULL);
            ir_add_expr2(IR_APPEND(code), mul, IR_OP2_sub, rhs_ir, IR_OPERAND_CONST(size_iconst));
            ir_var_t *sub = ir_var_create(code->func, ptr_prim, NULL);
            ir_add_expr2(IR_APPEND(code), sub, IR_OP2_div, lhs_ir, IR_OPERAND_VAR(mul));
            value = IR_OPERAND_VAR(sub);
        }

    } else if (op2 == IR_OP2_add) {
        // Pointer offset.
        if (lhs_ptr) {
            type_rc = rc_share(lhs.c_type);
            if (is_const) {
                value = IR_OPERAND_CONST(
                    ir_calc2(IR_OP2_add, lhs_ir.iconst, ir_calc2(IR_OP2_mul, rhs_ir.iconst, size_iconst))
                );
            } else {
                ir_var_t *mul = ir_var_create(code->func, ptr_prim, NULL);
                ir_add_expr2(IR_APPEND(code), mul, IR_OP2_add, rhs_ir, IR_OPERAND_CONST(size_iconst));
                ir_var_t *sub = ir_var_create(code->func, ptr_prim, NULL);
                ir_add_expr2(IR_APPEND(code), sub, IR_OP2_div, lhs_ir, IR_OPERAND_VAR(mul));
                value = IR_OPERAND_VAR(sub);
            }
        } else {
            type_rc = rc_share(rhs.c_type);
            if (is_const) {
                value = IR_OPERAND_CONST(
                    ir_calc2(IR_OP2_add, rhs_ir.iconst, ir_calc2(IR_OP2_mul, lhs_ir.iconst, size_iconst))
                );
            } else {
                ir_var_t *mul = ir_var_create(code->func, ptr_prim, NULL);
                ir_add_expr2(IR_APPEND(code), mul, IR_OP2_add, lhs_ir, IR_OPERAND_CONST(size_iconst));
                ir_var_t *sub = ir_var_create(code->func, ptr_prim, NULL);
                ir_add_expr2(IR_APPEND(code), sub, IR_OP2_div, rhs_ir, IR_OPERAND_VAR(mul));
                value = IR_OPERAND_VAR(sub);
            }
        }

    } else {
        // Pointer comparison.
        type_rc = rc_share(&ctx->prim_rcs[C_PRIM_SINT]);
        if (is_const) {
            value = IR_OPERAND_CONST(ir_calc2(op2, lhs_ir.iconst, rhs_ir.iconst));
        } else {
            ir_var_t *cmp = ir_var_create(code->func, IR_PRIM_bool, NULL);
            ir_add_expr2(IR_APPEND(code), cmp, op2, lhs_ir, rhs_ir);
            value = IR_OPERAND_VAR(cmp);
        }
    }

    c_value_destroy(lhs);
    c_value_destroy(rhs);
    return (c_compile_expr_t){
        .res = {
            .value_type = C_RVALUE_OPERAND,
            .rvalue.operand = value,
            .c_type = type_rc,
        },
        .code = code,
    };
}

// Compile an expression into IR.
c_compile_expr_t
    c_compile_expr(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *expr) {
    if (expr->type == TOKENTYPE_IDENT) {
        // Look up variable in scope.
        c_var_t *c_var = c_scope_lookup(scope, expr->strval);
        if (!c_var) {
            cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Identifier %s is undefined", expr->strval);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        }

        if (c_var->storage == C_VAR_STORAGE_ENUM_VARIANT) {
            // Enum variants looked up like this provide an rvalue.
            ir_const_t iconst = {
                .prim_type = c_prim_to_ir_type(ctx, C_PRIM_SINT),
                .constl    = c_var->enum_variant,
            };
            return (c_compile_expr_t){
                .code = code,
                .res  = {
                     .value_type     = C_RVALUE_OPERAND,
                     .c_type         = rc_share(&ctx->prim_rcs[C_PRIM_SINT]),
                     .rvalue.operand = IR_OPERAND_CONST(iconst),
                },
            };
        }

        if (!code) {
            cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Cannot access a variable in a constant expression");
            return (c_compile_expr_t){0};
        }

        // This is an actual variable and so an lvalue.
        c_value_t res;
        res.c_type = rc_share(c_var->type);
        switch (c_var->storage) {
            case C_VAR_STORAGE_REG:
                res.value_type    = C_LVALUE_VAR;
                res.lvalue.ir_var = c_var->ir_var;
                break;
            case C_VAR_STORAGE_FRAME:
                res.value_type    = C_LVALUE_MEM;
                res.lvalue.memref = IR_MEMREF(c_type_to_ir_type(ctx, c_var->type->data), IR_BADDR_FRAME(c_var->frame));
                break;
            case C_VAR_STORAGE_GLOBAL:
                res.value_type = C_LVALUE_MEM;
                res.lvalue.memref
                    = IR_MEMREF(c_type_to_ir_type(ctx, c_var->type->data), IR_BADDR_SYM(strong_strdup(c_var->sym)));
                break;
            case C_VAR_STORAGE_ENUM_VARIANT: __builtin_unreachable();
        }
        return (c_compile_expr_t){
            .code = code,
            .res  = res,
        };

    } else if (expr->type == TOKENTYPE_ICONST || expr->type == TOKENTYPE_CCONST) {
        return (c_compile_expr_t){
            .code = code,
            .res  = {
                 .value_type     = C_RVALUE_OPERAND,
                 .c_type         = rc_share(&ctx->prim_rcs[expr->subtype]),
                 .rvalue.operand = {
                     .type   = IR_OPERAND_TYPE_CONST,
                     .iconst = {
                         .prim_type = c_prim_to_ir_type(ctx, expr->subtype),
                         .constl    = expr->ival,
                         .consth    = expr->ivalh,
                    },
                },
            },
        };

    } else if (expr->type == TOKENTYPE_SCONST) {
        printf("TODO: String constants\n");
        abort();

    } else if (expr->subtype == C_AST_EXPRS) {
        // Multiple expressions; return value from the last one.
        for (size_t i = 0; i < expr->params_len - 1; i++) {
            c_compile_expr_t res;
            res = c_compile_expr(ctx, prepass, code, scope, &expr->params[i]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            code = res.code;
            c_value_destroy(res.res);
        }
        return c_compile_expr(ctx, prepass, code, scope, &expr->params[expr->params_len - 1]);

    } else if (expr->subtype == C_AST_EXPR_CALL) {
        if (!code) {
            cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Cannot call a function in a constant expression");
            return (c_compile_expr_t){0};
        }

        ir_operand_t funcptr;
        rc_t         functype_rc;
        if (expr->params[0].type != TOKENTYPE_IDENT) {
            // If not an ident, compile function addr expression.
            c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &expr->params[0]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            code        = res.code;
            funcptr     = c_value_read(ctx, code, &res.res);
            functype_rc = res.res.c_type;
        } else {
            // If it is an ident, it should be a function in the global scope.
            c_var_t *funcvar = c_scope_lookup(scope, expr->params[0].strval);
            functype_rc      = funcvar->type;
        }
        c_type_t *functype = functype_rc->data;

        if (functype->primitive == C_COMP_POINTER
            && ((c_type_t *)functype->inner->data)->primitive == C_COMP_FUNCTION) {
            // Function pointer type.
            (void)funcptr;
            printf("TODO: Function pointer types\n");
            abort();
        }

        // Validate compatibility.
        if (functype->primitive != C_COMP_FUNCTION) {
            cctx_diagnostic(ctx->cctx, expr->params[0].pos, DIAG_ERR, "Expected a function type");
            rc_delete(functype_rc);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        } else if (functype->func.args_len < expr->params_len - 1) {
            cctx_diagnostic(ctx->cctx, expr->params[functype->func.args_len + 1].pos, DIAG_ERR, "Too many arguments");
            rc_delete(functype_rc);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        } else if (functype->func.args_len > expr->params_len - 1) {
            cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Not enough arguments");
            rc_delete(functype_rc);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        }

        ir_operand_t *params = strong_calloc(sizeof(ir_operand_t), expr->params_len - 1);
        for (size_t i = 0; i < expr->params_len - 1; i++) {
            c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &expr->params[i + 1]);
            if (res.res.value_type == C_VALUE_ERROR) {
                rc_delete(functype_rc);
                free(params);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            // TODO: Do casting here.
            // code = res.code;
            // params[i] = res.res;
        }

        printf("TODO: Call expressions\n");
        abort();
        // rc_t return_type = rc_share(functype->func.return_type);
        // rc_delete(functype_rc);
        // return (c_compile_expr_t){
        //     .code = code,
        //     .type = return_type,
        //     .res = (ir_operand_t){0},
        // };

    } else if (expr->subtype == C_AST_EXPR_INDEX) {
        printf("TODO: Index expressions\n");
        abort();

    } else if (expr->subtype == C_AST_EXPR_INFIX && expr->params[0].subtype == C_TKN_ARROW) {
        // Compile struct/union pointer expression.
        c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
        if (res.res.value_type == C_VALUE_ERROR) {
            return (c_compile_expr_t){
                .code = res.code,
                .res  = {0},
            };
        }
        code             = res.code;
        ir_operand_t ptr = c_value_read(ctx, code, &res.res);

        // Get the target struct/union field.
        c_type_t const *ptr_type = res.res.c_type->data;
        if (ptr_type->primitive != C_COMP_POINTER) {
            cctx_diagnostic(
                ctx->cctx,
                expr->params[1].pos,
                DIAG_ERR,
                "Expression is not a pointer to struct/union type"
            );
            if (ptr_type->primitive == C_COMP_STRUCT || ptr_type->primitive == C_COMP_UNION) {
                cctx_diagnostic(ctx->cctx, expr->params[1].pos, DIAG_HINT, "Did you mean to use the `.` operator?");
            }
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = res.code,
                .res  = {0},
            };
        }
        c_type_t const *inner_type = ptr_type->inner->data;
        if (inner_type->primitive != C_COMP_STRUCT && inner_type->primitive != C_COMP_UNION) {
            cctx_diagnostic(
                ctx->cctx,
                expr->params[1].pos,
                DIAG_ERR,
                "Expression is not a pointer to struct/union type"
            );
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = res.code,
                .res  = {0},
            };
        }
        c_comp_t const  *comp  = inner_type->comp->data;
        c_field_t const *field = map_get(&comp->fields, expr->params[2].strval);
        if (!field) {
            cctx_diagnostic(ctx->cctx, expr->params[2].pos, DIAG_ERR, "Unknown field %s", expr->params[2].strval);
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = res.code,
                .res  = {0},
            };
        }

        // Get field as an offset pointer.
        c_value_t lvalue = {
            .value_type    = C_LVALUE_MEM,
            .c_type        = rc_share(field->type_rc),
            .lvalue.memref = {
                .data_type = c_type_to_ir_type(ctx, field->type_rc->data),
                .offset    = (int64_t)field->offset,
            },
        };
        switch (ptr.type) {
            case IR_OPERAND_TYPE_CONST:
                lvalue.lvalue.memref.base_type  = IR_MEMBASE_ABS;
                lvalue.lvalue.memref.offset    += (int64_t)ptr.iconst.constl;
                break;
            case IR_OPERAND_TYPE_UNDEF: __builtin_unreachable();
            case IR_OPERAND_TYPE_VAR:
                lvalue.lvalue.memref.base_type = IR_MEMBASE_VAR;
                lvalue.lvalue.memref.base_var  = ptr.var;
                break;
            case IR_OPERAND_TYPE_MEM:
            case IR_OPERAND_TYPE_REG: __builtin_unreachable();
        }

        c_value_destroy(res.res);
        return (c_compile_expr_t){
            .code = code,
            .res  = lvalue,
        };

    } else if (expr->subtype == C_AST_EXPR_INFIX && expr->params[0].subtype == C_TKN_DOT) {
        // Compile struct/union reference expression.
        c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
        if (res.res.value_type == C_VALUE_ERROR) {
            return (c_compile_expr_t){
                .code = res.code,
                .res  = {0},
            };
        }
        code = res.code;

        // Get the target struct/union field.
        c_type_t const *struct_type = res.res.c_type->data;
        if (struct_type->primitive != C_COMP_STRUCT && struct_type->primitive != C_COMP_UNION) {
            cctx_diagnostic(ctx->cctx, expr->params[1].pos, DIAG_ERR, "Expression is not a struct/union type");
            if (struct_type->primitive == C_COMP_POINTER) {
                c_type_t const *inner_type = struct_type->inner->data;
                if (inner_type->primitive == C_COMP_UNION || inner_type->primitive == C_COMP_STRUCT) {
                    cctx_diagnostic(
                        ctx->cctx,
                        expr->params[1].pos,
                        DIAG_HINT,
                        "Did you mean to use the `->` operator?"
                    );
                }
            }
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = res.code,
                .res  = {0},
            };
        }
        c_comp_t const  *comp  = struct_type->comp->data;
        c_field_t const *field = map_get(&comp->fields, expr->params[2].strval);
        if (!field) {
            cctx_diagnostic(ctx->cctx, expr->params[2].pos, DIAG_ERR, "Unknown field %s", expr->params[2].strval);
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = res.code,
                .res  = {0},
            };
        }

        return (c_compile_expr_t){
            .code = code,
            .res  = c_value_field(ctx, code, &res.res, expr->params[2].strval),
        };

    } else if (expr->subtype == C_AST_EXPR_INFIX
               && (expr->params[0].subtype == C_TKN_LOR || expr->params[0].subtype == C_TKN_LAND)) {
        // Logical ANR/OR expression.
        c_compile_expr_t res;
        bool const       is_land = expr->params[0].subtype == C_TKN_LAND;

        ir_code_t *cont_code  = ir_code_create(code->func, NULL);
        ir_code_t *break_code = ir_code_create(code->func, NULL);
        ir_code_t *exit_code  = ir_code_create(code->func, NULL);

        // Get operands.
        res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
        if (res.res.value_type == C_VALUE_ERROR) {
            return (c_compile_expr_t){
                .code = res.code,
                .res  = (c_value_t){0},
            };
        }
        c_value_t  lhs         = res.res;
        ir_code_t *branch_code = res.code;

        res = c_compile_expr(ctx, prepass, cont_code, scope, &expr->params[2]);
        if (res.res.value_type == C_VALUE_ERROR) {
            c_value_destroy(lhs);
            return (c_compile_expr_t){
                .code = res.code,
                .res  = (c_value_t){0},
            };
        }
        c_value_t rhs = res.res;

        // Determine compatibility with this operator.
        if (!c_type_arith_compatible(
                ctx,
                lhs.c_type->data,
                rhs.c_type->data,
                expr->params[0].subtype,
                expr->params[0].pos
            )) {
            ir_code_delete(exit_code);
            ir_code_delete(cont_code);
            ir_code_delete(break_code);
            c_value_destroy(lhs);
            c_value_destroy(rhs);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        }

        if (c_value_is_const(&lhs) && c_value_is_const(&rhs)) {
            // Constant expression.
            ir_code_delete(exit_code);
            ir_code_delete(cont_code);
            ir_code_delete(break_code);
            ir_const_t lhs_val = ir_calc1(IR_OP1_snez, lhs.rvalue.operand.iconst);
            ir_const_t rhs_val = ir_calc1(IR_OP1_snez, rhs.rvalue.operand.iconst);
            bool       res     = is_land ? (lhs_val.constl & rhs_val.constl) : (lhs_val.constl | rhs_val.constl);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){
                     .value_type     = C_RVALUE_OPERAND,
                     .c_type         = rc_share(&ctx->prim_rcs[C_PRIM_SINT]),
                     .rvalue.operand = IR_OPERAND_CONST(((ir_const_t){
                         .prim_type = c_prim_to_ir_type(ctx, C_PRIM_SINT),
                         .consth    = 0,
                         .constl    = res,
                    })),
                },
            };
        }

        // Not a constant expression.
        code              = res.code;
        ir_var_t *resvar  = ir_var_create(code->func, IR_PRIM_bool, NULL);
        ir_var_t *resvar2 = ir_var_create(code->func, c_prim_to_ir_type(ctx, C_PRIM_SINT), NULL);
        ir_var_t *condvar = ir_var_create(code->func, IR_PRIM_bool, NULL);

        ir_add_expr1(
            IR_APPEND(branch_code),
            condvar,
            is_land ? IR_OP1_seqz : IR_OP1_snez,
            c_value_read(ctx, branch_code, &lhs)
        );
        ir_add_branch(IR_APPEND(branch_code), IR_OPERAND_VAR(condvar), break_code);
        ir_add_jump(IR_APPEND(branch_code), cont_code);

        ir_add_expr1(IR_APPEND(break_code), resvar, IR_OP1_mov, IR_OPERAND_CONST(IR_CONST_BOOL(!is_land)));
        ir_add_jump(IR_APPEND(break_code), exit_code);

        ir_add_expr1(IR_APPEND(cont_code), resvar, IR_OP1_snez, c_value_read(ctx, cont_code, &rhs));
        ir_add_jump(IR_APPEND(cont_code), exit_code);

        ir_add_expr1(IR_APPEND(exit_code), resvar2, IR_OP1_mov, IR_OPERAND_VAR(resvar));

        return (c_compile_expr_t){
            .code = exit_code,
            .res  = (c_value_t){
                 .value_type     = C_RVALUE_OPERAND,
                 .c_type         = rc_share(&ctx->prim_rcs[C_PRIM_SINT]),
                 .rvalue.operand = IR_OPERAND_VAR(resvar2),
            },
        };

    } else if (expr->subtype == C_AST_EXPR_INFIX) {
        if (expr->params[0].subtype == C_TKN_ASSIGN) {
            // Simple assignment expression.
            c_compile_expr_t res;

            // Compile the rvalue.
            res = c_compile_expr(ctx, prepass, code, scope, &expr->params[2]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            code          = res.code;
            c_value_t rhs = res.res;

            // Compile the lvalue.
            res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            code          = res.code;
            c_value_t lhs = res.res;

            // Assert that it's actually a writable lvalue.
            if (!c_value_assignable(ctx, &lhs, expr->params[1].pos)) {
                c_value_destroy(lhs);
                c_value_destroy(rhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            // Determine compatibility with this operator.
            if (!c_type_compatible(ctx, lhs.c_type->data, rhs.c_type->data)) {
                cctx_diagnostic(ctx->cctx, expr->params[0].pos, DIAG_ERR, "Cannot assign incompatible type");
                c_value_destroy(lhs);
                c_value_destroy(rhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            // Write into the lvalue.
            c_value_write(ctx, code, &lhs, &rhs);
            // The C semantics specify that the result of an assignment expression is an rvalue.
            // Therefor, clean up the lvalue and return the rvalue.
            c_value_destroy(lhs);
            return (c_compile_expr_t){
                .code = code,
                .res  = rhs,
            };

        } else {
            // Other expressions.
            c_compile_expr_t res;

            // Get operands.
            res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = res.code,
                    .res  = (c_value_t){0},
                };
            }
            c_value_t lhs = res.res;
            code          = res.code;
            res           = c_compile_expr(ctx, prepass, code, scope, &expr->params[2]);
            if (res.res.value_type == C_VALUE_ERROR) {
                c_value_destroy(lhs);
                return (c_compile_expr_t){
                    .code = res.code,
                    .res  = (c_value_t){0},
                };
            }
            c_value_t rhs = res.res;
            code          = res.code;

            // Determine compatibility with this operator.
            if (!c_type_arith_compatible(
                    ctx,
                    lhs.c_type->data,
                    rhs.c_type->data,
                    expr->params[0].subtype,
                    expr->params[0].pos
                )) {
                c_value_destroy(lhs);
                c_value_destroy(rhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            c_type_t const *lhs_type = lhs.c_type->data;
            c_type_t const *rhs_type = rhs.c_type->data;
            if (c_prim_is_ptr(lhs_type->primitive) || c_prim_is_ptr(rhs_type->primitive)) {
                // Pointer arithmetic is more restricted and has special handling.
                return c_compile_expr2_ptrarith(ctx, code, expr, lhs, rhs);
            } else {
                // All other arithmetic translates directly to the corresponding IR instructions.
                return c_compile_expr2_arith(ctx, code, expr, lhs, rhs);
            }
        }

    } else if (expr->subtype == C_AST_EXPR_PREFIX) {
        if (expr->params[0].subtype == C_TKN_INC || expr->params[0].subtype == C_TKN_DEC) {
            // Pre-increment / pre-decrement.
            bool             is_inc = expr->params[0].subtype == C_TKN_INC;
            c_compile_expr_t res;
            res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            code                     = res.code;
            c_type_t const *res_type = res.res.c_type->data;

            // Assert that it's writeable.
            if (!c_prim_is_scalar(res_type->primitive)) {
                cctx_diagnostic(ctx->cctx, expr->params[0].pos, DIAG_ERR, "Cannot increment this type");
                c_value_destroy(res.res);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            } else if (!c_value_assignable(ctx, &res.res, expr->params[1].pos)) {
                c_value_destroy(res.res);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            // Determine increment, which is 1 for non-pointer types.
            uint64_t increment = 1, dummy;
            if (c_prim_is_ptr(res_type->primitive)) {
                if (!c_type_get_size(ctx, res_type->inner->data, &increment, &dummy)) {
                    c_value_destroy(res.res);
                    return (c_compile_expr_t){
                        .code = code,
                        .res  = (c_value_t){0},
                    };
                }
            }

            // Add or subtract the increment.
            ir_var_t *tmpvar = ir_var_create(code->func, c_type_to_ir_type(ctx, res_type), NULL);
            ir_add_expr2(
                IR_APPEND(code),
                tmpvar,
                is_inc ? IR_OP2_add : IR_OP2_sub,
                c_value_read(ctx, code, &res.res),
                (ir_operand_t){
                    .type   = IR_OPERAND_TYPE_CONST,
                    .iconst = {
                        .prim_type = tmpvar->prim_type,
                        .constl    = increment,
                        .consth    = 0,
                    },
                }
            );

            // After modifying, write back and return value.
            c_value_t rvalue = {
                .value_type     = C_RVALUE_OPERAND,
                .c_type         = rc_share(res.res.c_type),
                .rvalue.operand = IR_OPERAND_VAR(tmpvar),
            };
            c_value_write(ctx, code, &res.res, &rvalue);
            c_value_destroy(res.res);

            return (c_compile_expr_t){
                .code = code,
                .res  = rvalue,
            };

        } else if (expr->params[0].subtype == C_TKN_AND) {
            // Address of operator.
            c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            } else if (res.res.value_type == C_RVALUE_OPERAND) {
                cctx_diagnostic(ctx->cctx, expr->params[0].pos, DIAG_ERR, "Lvalue required as unary `&` operand");
                c_value_destroy(res.res);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            // Create pointer type.
            rc_t ptr_rc = c_type_pointer(ctx, rc_share(res.res.c_type));

            // Get address into an rvalue.
            ir_var_t *ir_ptr = ir_var_create(code->func, c_prim_to_ir_type(ctx, ctx->options.size_type), NULL);
            ir_add_lea(IR_APPEND(code), ir_ptr, c_value_memref(ctx, code, &res.res));
            c_value_t rvalue = {
                .value_type     = C_RVALUE_OPERAND,
                .c_type         = ptr_rc,
                .rvalue.operand = IR_OPERAND_VAR(ir_ptr),
            };
            c_value_destroy(res.res);

            return (c_compile_expr_t){
                .code = code,
                .res  = rvalue,
            };

        } else if (expr->params[0].subtype == C_TKN_MUL) {
            // Pointer deref operator.
            c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            code           = res.code;
            c_type_t *type = res.res.c_type->data;

            // Enforce inner expression to be a pointer type.
            if (type->primitive != C_COMP_POINTER && type->primitive != C_COMP_ARRAY) {
                cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Expected a pointer or array type");
                c_value_destroy(res.res);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            // Create pointer lvalue.
            ir_operand_t ptrval = c_value_read(ctx, code, &res.res);
            ir_memref_t  memref = {0};
            memref.data_type    = c_type_to_ir_type(ctx, type->inner->data);
            switch (ptrval.type) {
                case IR_OPERAND_TYPE_CONST:
                    memref.base_type = IR_MEMBASE_ABS;
                    memref.offset    = (int64_t)ptrval.iconst.constl;
                    break;
                case IR_OPERAND_TYPE_UNDEF: __builtin_unreachable();
                case IR_OPERAND_TYPE_VAR:
                    memref.base_type = IR_MEMBASE_VAR;
                    memref.base_var  = ptrval.var;
                    break;
                case IR_OPERAND_TYPE_MEM:
                case IR_OPERAND_TYPE_REG: __builtin_unreachable();
            }
            c_value_t rvalue = {
                .value_type    = C_LVALUE_MEM,
                .c_type        = rc_share(type->inner),
                .lvalue.memref = memref,
            };
            c_value_destroy(res.res);

            return (c_compile_expr_t){
                .code = code,
                .res  = rvalue,
            };

        } else {
            // Normal unary operator.
            c_compile_expr_t res;
            res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            code = res.code;

            // Apply unary operator.
            ir_operand_t ir_value = c_value_read(ctx, code, &res.res);
            if (ir_value.type == IR_OPERAND_TYPE_CONST) {
                // Resulting constant can be evaluated at compile-time.
                rc_t type = rc_share(res.res.c_type);
                c_value_destroy(res.res);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = {
                         .value_type = C_RVALUE_OPERAND,
                         .c_type     = type,
                         .rvalue.operand
                        = IR_OPERAND_CONST(ir_calc1(c_op1_to_ir_op1(expr->params[0].subtype), ir_value.iconst)),
                    },
                };
            }

            ir_prim_t ir_prim;
            if (expr->params[0].subtype == C_TKN_LNOT) {
                ir_prim = IR_PRIM_bool;
            } else {
                ir_prim = c_type_to_ir_type(ctx, res.res.c_type->data);
            }
            ir_var_t *tmpvar = ir_var_create(code->func, ir_prim, NULL);
            ir_add_expr1(IR_APPEND(code), tmpvar, c_op1_to_ir_op1(expr->params[0].subtype), ir_value);

            // Return temporary value.
            c_value_t rvalue = {
                .value_type     = C_RVALUE_OPERAND,
                .c_type         = rc_share(res.res.c_type),
                .rvalue.operand = IR_OPERAND_VAR(tmpvar),
            };

            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = code,
                .res  = rvalue,
            };
        }
    } else if (expr->subtype == C_AST_EXPR_SUFFIX) {
        // Post-increment / post-decrement.
        bool             is_inc = expr->params[0].subtype == C_TKN_INC;
        c_compile_expr_t res;
        res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
        if (res.res.value_type == C_VALUE_ERROR) {
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        }
        code                     = res.code;
        c_type_t const *res_type = res.res.c_type->data;

        // Assert that it's actually a writable lvalue.
        if (!c_prim_is_scalar(res_type->primitive)) {
            cctx_diagnostic(ctx->cctx, expr->params[0].pos, DIAG_ERR, "Cannot increment this type");
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        } else if (!c_value_assignable(ctx, &res.res, expr->params[1].pos)) {
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        }

        // Save value for later use.
        ir_prim_t    ir_prim    = c_type_to_ir_type(ctx, res.res.c_type->data);
        ir_operand_t read_value = c_value_read(ctx, code, &res.res);
        ir_var_t    *oldvar     = ir_var_create(code->func, ir_prim, NULL);
        ir_add_expr1(IR_APPEND(code), oldvar, IR_OP1_mov, read_value);

        // Determine increment, which is 1 for non-pointer types.
        uint64_t increment = 1, dummy;
        if (c_prim_is_ptr(res_type->primitive)) {
            if (!c_type_get_size(ctx, res_type->inner->data, &increment, &dummy)) {
                c_value_destroy(res.res);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
        }

        // Add or subtract the increment.
        ir_var_t *tmpvar = ir_var_create(code->func, c_type_to_ir_type(ctx, res_type), NULL);
        ir_add_expr2(
            IR_APPEND(code),
            tmpvar,
            is_inc ? IR_OP2_add : IR_OP2_sub,
            c_value_read(ctx, code, &res.res),
            (ir_operand_t){
                .type   = IR_OPERAND_TYPE_CONST,
                .iconst = {
                    .prim_type = tmpvar->prim_type,
                    .constl    = increment,
                    .consth    = 0,
                },
            }
        );

        // After modifying, write back.
        c_value_t write_rvalue = {
            .value_type     = C_RVALUE_OPERAND,
            .c_type         = res.res.c_type,
            .rvalue.operand = IR_OPERAND_VAR(tmpvar),
        };
        c_value_write(ctx, code, &res.res, &write_rvalue);

        // Return old value.
        c_value_t old_rvalue = {
            .value_type     = C_RVALUE_OPERAND,
            .c_type         = rc_share(res.res.c_type),
            .rvalue.operand = IR_OPERAND_VAR(tmpvar),
        };
        c_value_destroy(res.res);

        return (c_compile_expr_t){
            .code = code,
            .res  = old_rvalue,
        };
    } else if (expr->subtype == C_AST_GARBAGE) {
        return (c_compile_expr_t){
            .code = code,
            .res  = (c_value_t){0},
        };
    } else {
        __builtin_unreachable();
    }
}

// Compile a statement node into IR.
// Returns the code path linearly after this.
ir_code_t *
    c_compile_stmt(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *stmt) {
    if (stmt->subtype == C_AST_STMTS) {
        // Multiple statements in scope.
        c_scope_t new_scope = c_scope_create(scope);

        // Compile statements in this scope.
        for (size_t i = 0; i < stmt->params_len; i++) {
            code = c_compile_stmt(ctx, prepass, code, &new_scope, &stmt->params[i]);
        }

        // Clean up the scope.
        c_scope_destroy(new_scope);

    } else if (stmt->subtype == C_AST_EXPRS) {
        // Expression statement.
        c_compile_expr_t expr = c_compile_expr(ctx, prepass, code, scope, stmt);
        code                  = expr.code;
        if (expr.res.value_type != C_VALUE_ERROR) {
            c_value_destroy(expr.res);
        }

    } else if (stmt->subtype == C_AST_DECLS) {
        // Local declarations.
        code = c_compile_decls(ctx, prepass, code, scope, stmt);

    } else if (stmt->subtype == C_AST_DO_WHILE || stmt->subtype == C_AST_WHILE) {
        // While or do...while loop.
        ir_code_t *loop_body = ir_code_create(code->func, NULL);
        ir_code_t *cond_body = ir_code_create(code->func, NULL);
        ir_code_t *after     = ir_code_create(code->func, NULL);

        // Compile expression.
        c_compile_expr_t expr = c_compile_expr(ctx, prepass, cond_body, scope, &stmt->params[0]);
        if (expr.res.value_type != C_VALUE_ERROR) {
            ir_add_branch(
                IR_APPEND(expr.code),
                c_cast_ir_operand(expr.code, c_value_read(ctx, expr.code, &expr.res), IR_PRIM_bool),
                loop_body
            );
            c_value_destroy(expr.res);
        }
        ir_add_jump(IR_APPEND(expr.code), after);

        // Compile loop body.
        loop_body = c_compile_stmt(ctx, prepass, loop_body, scope, &stmt->params[1]);
        ir_add_jump(IR_APPEND(loop_body), cond_body);

        if (stmt->subtype == C_AST_DO_WHILE) {
            // Jump to loop first for do...while.
            ir_add_jump(IR_APPEND(code), loop_body);
        } else {
            // Jump to condition first for while.
            ir_add_jump(IR_APPEND(code), cond_body);
        }

        code = after;

    } else if (stmt->subtype == C_AST_IF_ELSE) {
        // If or if...else statement.
        c_compile_expr_t expr = c_compile_expr(ctx, prepass, code, scope, &stmt->params[0]);
        code                  = expr.code;

        // Allocate code paths.
        ir_code_t *if_body = ir_code_create(code->func, NULL);
        ir_code_t *after   = ir_code_create(code->func, NULL);
        c_compile_stmt(ctx, prepass, if_body, scope, &stmt->params[1]);
        ir_add_jump(IR_APPEND(if_body), after);
        if (expr.res.value_type != C_VALUE_ERROR) {
            ir_add_branch(
                IR_APPEND(code),
                c_cast_ir_operand(code, c_value_read(ctx, code, &expr.res), IR_PRIM_bool),
                if_body
            );
        }
        if (stmt->params_len == 3) {
            // If...else statement.
            ir_code_t *else_body = ir_code_create(code->func, NULL);
            c_compile_stmt(ctx, prepass, else_body, scope, &stmt->params[2]);
            ir_add_jump(IR_APPEND(else_body), after);
            ir_add_jump(IR_APPEND(code), else_body);
        } else {
            // If statement.
            ir_add_jump(IR_APPEND(code), after);
        }

        if (expr.res.value_type != C_VALUE_ERROR) {
            c_value_destroy(expr.res);
        }

        // Continue after the if statement.
        code = after;

    } else if (stmt->subtype == C_AST_FOR_LOOP) {
        // For loop.

        if (stmt->params[0].type == TOKENTYPE_AST && stmt->params[0].subtype == C_AST_DECLS) {
            // Setup is a decls.
            code = c_compile_decls(ctx, prepass, code, scope, &stmt->params[0]);
        } else if (stmt->params[0].type != TOKENTYPE_AST || stmt->params[0].subtype != C_AST_NOP) {
            // Setup is an expr.
            c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &stmt->params[0]);
            c_value_destroy(res.res);
            code = res.code;
        }

        ir_code_t *for_body = ir_code_create(code->func, NULL);
        ir_code_t *for_cond;
        ir_code_t *after = ir_code_create(code->func, NULL);

        // Compile condition.
        if (stmt->params[1].type == TOKENTYPE_AST && stmt->params[1].subtype == C_AST_NOP) {
            for_cond = for_body;
        } else {
            for_cond             = ir_code_create(code->func, NULL);
            c_compile_expr_t res = c_compile_expr(ctx, prepass, for_cond, scope, &stmt->params[1]);
            if (res.res.value_type != C_VALUE_ERROR) {
                ir_add_branch(
                    IR_APPEND(res.code),
                    c_cast_ir_operand(res.code, c_value_read(ctx, res.code, &res.res), IR_PRIM_bool),
                    for_body
                );
            }
            ir_add_jump(IR_APPEND(res.code), after);
            c_value_destroy(res.res);
        }
        ir_add_jump(IR_APPEND(code), for_cond);

        // Compile body.
        code = c_compile_stmt(ctx, prepass, for_body, scope, &stmt->params[3]);

        // Compile increment.
        if (stmt->params[2].type != TOKENTYPE_AST || stmt->params[2].subtype != C_AST_NOP) {
            c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &stmt->params[2]);
            code                 = res.code;
            c_value_destroy(res.res);
        }
        ir_add_jump(IR_APPEND(code), for_cond);

        return after;

    } else if (stmt->subtype == C_AST_RETURN) {
        // Return statement.
        if (stmt->params_len) {
            // With value.
            c_compile_expr_t expr = c_compile_expr(ctx, prepass, code, scope, &stmt->params[0]);
            code                  = expr.code;
            if (expr.res.value_type != C_VALUE_ERROR) {
                ir_add_return1(IR_APPEND(code), c_value_read(ctx, code, &expr.res));
                c_value_destroy(expr.res);
            }
        } else {
            // Without.
            ir_add_return0(IR_APPEND(code));
        }
    }
    return code;
}

// Compile a C function definition into IR.
ir_func_t *c_compile_func_def(c_compiler_t *ctx, token_t const *def, c_prepass_t *prepass) {
    rc_t inner_type = c_compile_spec_qual_list(ctx, &def->params[0], &ctx->global_scope);
    if (!inner_type) {
        return NULL;
    }

    token_t const *name;
    rc_t           func_type_rc = c_compile_decl(ctx, &def->params[1], &ctx->global_scope, inner_type, &name);
    if (!name) {
        // A diagnostic will already have been created; skip this.
        rc_delete(func_type_rc);
        return NULL;
    }
    c_type_t *func_type = func_type_rc->data;

    // Create function and scope.
    ir_func_t *func  = ir_func_create(name->strval, NULL, func_type->func.args_len);
    c_scope_t  scope = c_scope_create(&ctx->global_scope);

    // Bring parameters into scope.
    for (size_t i = 0; i < func_type->func.args_len; i++) {
        if (func_type->func.arg_names[i]) {
            c_var_t *var = c_var_create(
                ctx,
                prepass,
                func,
                rc_share(func_type->func.args[i]),
                func_type->func.arg_name_tkns[i],
                &scope
            );
            if (!var) {
                // A diagnostic will have already been created.
                continue;
            }

            if (var->storage == C_VAR_STORAGE_REG) {
                func->args[i].has_var  = true;
                func->args[i].var      = var->ir_var;
                var->ir_var->arg_index = (ptrdiff_t)i;
            } else {
                func->args[i].has_var = false;
                func->args[i].type    = c_type_to_ir_type(ctx, func_type->func.args[i]->data);
            }
        } else {
            func->args[i].type = c_type_to_ir_type(ctx, func_type->func.args[i]->data);
        }
    }

    // Compile the statements inside this same scope.
    token_t const *body = &def->params[2];
    ir_code_t     *code = (ir_code_t *)func->code_list.head;
    for (size_t i = 0; i < body->params_len; i++) {
        code = c_compile_stmt(ctx, prepass, code, &scope, &body->params[i]);
    }

    c_scope_destroy(scope);

    // Add implicit empty return statement.
    ir_add_return0(IR_APPEND(code));

    // Add function into global scope.
    c_var_t *var = calloc(1, sizeof(c_var_t));
    var->storage = C_VAR_STORAGE_GLOBAL;
    var->type    = func_type_rc;
    map_set(&ctx->global_scope.locals, name->strval, var);

    return func;
}

// Compile a declaration statement.
// If in global scope, `code` and `prepass` must be `NULL`, and will return `NULL`.
// If not global, all must not be `NULL` and will return the code path linearly after this.
ir_code_t *
    c_compile_decls(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *decls) {
    // TODO: Typedef support.
    rc_t inner_type = c_compile_spec_qual_list(ctx, &decls->params[0], scope);
    if (!inner_type) {
        return code;
    }

    for (size_t i = 1; i < decls->params_len; i++) {
        token_t const *name      = NULL;
        rc_t           decl_type = c_compile_decl(ctx, &decls->params[i], scope, rc_share(inner_type), &name);
        if (!decl_type) {
            // A diagnostic will already have been created; skip this.
            continue;
        }
        if (!name) {
            // A diagnostic will already have been created; skip this.
            rc_delete(decl_type);
            continue;
        }

        // Create the C variable.
        c_var_t *var = c_var_create(ctx, prepass, code ? code->func : NULL, decl_type, name, scope);
        if (!var) {
            // A diagnostic will have already been created.
            rc_delete(decl_type);
            continue;
        }

        // If the declaration has an assignment, compile it too.
        if (decls->params[i].type == TOKENTYPE_AST && decls->params[i].subtype == C_AST_ASSIGN_DECL) {
            if (scope->depth == 0) {
                printf("TODO: Initialized variables in the global scope\n");
                abort();
            }

            c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &decls->params[1].params[1]);
            code                 = res.code;
            if (res.res.value_type != C_VALUE_ERROR) {
                ir_operand_t tmp = c_value_read(ctx, code, &res.res);
                switch (var->storage) {
                    case C_VAR_STORAGE_REG: ir_add_expr1(IR_APPEND(code), var->ir_var, IR_OP1_mov, tmp); break;
                    case C_VAR_STORAGE_FRAME:
                        ir_add_store(IR_APPEND(code), tmp, IR_MEMREF(ir_operand_prim(tmp), IR_BADDR_FRAME(var->frame)));
                        break;
                    case C_VAR_STORAGE_GLOBAL: abort(); break;
                    case C_VAR_STORAGE_ENUM_VARIANT: __builtin_unreachable();
                }
                c_value_destroy(res.res);
            }
        }
    }
    rc_delete(inner_type);

    return code;
}



// Explain a C type.
static void c_type_explain_impl(c_type_t *type, FILE *to) {
start:
    if (type->primitive == C_COMP_FUNCTION) {
        fputs("function(", to);
        for (size_t i = 0; i < type->func.args_len; i++) {
            if (i) {
                fputs(", ", to);
            }
            c_type_explain_impl(type->func.args[i]->data, to);
        }
        fputs(") returning ", to);
        type = (c_type_t *)type->func.return_type->data;
        goto start;
    } else if (type->primitive == C_COMP_ARRAY) {
        fputs("array of ", to);
        type = (c_type_t *)type->inner->data;
        goto start;
    } else if (type->primitive == C_COMP_POINTER) {
        if (type->is_atomic) {
            fputs("_Atomic ", to);
        }
        if (type->is_const) {
            fputs("const ", to);
        }
        if (type->is_volatile) {
            fputs("volatile ", to);
        }
        if (type->is_restrict) {
            fputs("restrict ", to);
        }
        fputs("pointer to ", to);
        type = (c_type_t *)type->inner->data;
        goto start;
    }

    if (type->is_atomic) {
        fputs("_Atomic ", to);
    }
    switch (type->primitive) {
        case C_PRIM_UCHAR: fputs("unsigned char", to); break;
        case C_PRIM_SCHAR: fputs("signed char", to); break;
        case C_PRIM_USHORT: fputs("unsigned short", to); break;
        case C_PRIM_SSHORT: fputs("short", to); break;
        case C_PRIM_UINT: fputs("unsigned int", to); break;
        case C_PRIM_SINT: fputs("int", to); break;
        case C_PRIM_ULONG: fputs("unsigned long", to); break;
        case C_PRIM_SLONG: fputs("signed long", to); break;
        case C_PRIM_ULLONG: fputs("unsigned long long", to); break;
        case C_PRIM_SLLONG: fputs("long long", to); break;
        case C_PRIM_BOOL: fputs("_Bool", to); break;
        case C_PRIM_FLOAT: fputs("float", to); break;
        case C_PRIM_DOUBLE: fputs("double", to); break;
        case C_PRIM_LDOUBLE: fputs("long double", to); break;
        case C_PRIM_VOID: fputs("void", to); break;
        case C_COMP_STRUCT: fputs("struct", to); break;
        case C_COMP_UNION: fputs("union", to); break;
        case C_COMP_ENUM: fputs("enum", to); break;
        default: break;
    }
    if (type->is_const) {
        fputs(" const", to);
    }
    if (type->is_volatile) {
        fputs(" volatile", to);
    }
}

// Explain a C type.
void c_type_explain(c_type_t *type, FILE *to) {
    c_type_explain_impl(type, to);
    fputc('\n', to);
}
