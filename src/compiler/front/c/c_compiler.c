
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"

#include "c_parser.h"
#include "strong_malloc.h"

// Cache of C types by primitive.
static rc_t primitive_cache[C_N_PRIM];

__attribute__((constructor)) static void fill_primitives_cache() {
    for (int i = 0; i < C_N_PRIM; i++) {
        primitive_cache[i] = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), NULL);
        c_type_t *type     = primitive_cache[i]->data;
        type->primitive    = i;
    }
}



// Create a new C compiler context.
c_compiler_t *c_compiler_create(cctx_t *cctx, c_options_t options) {
    c_compiler_t *cc        = strong_calloc(1, sizeof(c_compiler_t));
    cc->cctx                = cctx;
    cc->options             = options;
    cc->global_scope.locals = STR_MAP_EMPTY;
    return cc;
}



// Clear up a `c_type_t`.
static void c_type_free(c_type_t *type) {
    if (type->primitive == C_COMP_FUNCTION) {
        rc_delete(type->func.return_type);
        for (size_t i = 0; i < type->func.args_len; i++) {
            rc_delete(type->func.args[i]);
            free(type->func.arg_names[i]);
        }
    } else if (type->primitive == C_COMP_ARRAY) {
        // TODO: An array size could be here.
        rc_delete(type->inner);
    } else if (type->primitive == C_COMP_POINTER) {
        rc_delete(type->inner);
    }
    free(type);
}

// Create a C type from a specifier-qualifer list.
// Returns a refcount pointer of `c_type_t`.
rc_t c_compile_spec_qual_list(c_compiler_t *ctx, token_t list) {
    token_t *struct_tkn   = NULL;
    int      n_long       = 0;
    bool     has_int      = false;
    bool     has_short    = false;
    bool     has_char     = false;
    bool     has_float    = false;
    bool     has_double   = false;
    bool     has_void     = false;
    bool     has_bool     = false;
    bool     has_unsigned = false;
    bool     has_signed   = false;

    int a = 2;

    rc_t      rc   = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
    c_type_t *type = rc->data;

    // Turn the list into a more manageable format.
    for (size_t i = 0; i < list.params_len; i++) {
        token_t param = list.params[i];
        if (param.type == TOKENTYPE_KEYWORD) {
            switch (param.subtype) {
                case C_KEYW__Atomic: type->is_atomic = true; break;
                case C_KEYW_volatile: type->is_volatile = true; break;
                case C_KEYW_const: type->is_atomic = true; break;
                case C_KEYW_int: has_int = true; break;
                case C_KEYW_short: has_short = true; break;
                case C_KEYW_long: n_long++; break;
                case C_KEYW_char: has_char = true; break;
                case C_KEYW_float: has_float = true; break;
                case C_KEYW_double: has_double = true; break;
                case C_KEYW_void: has_void = true; break;
                case C_KEYW__Bool: has_bool = true; break;
                case C_KEYW_bool: has_bool = true; break;
                case C_KEYW_signed: has_signed = true; break;
                case C_KEYW_unsigned: has_unsigned = true; break;
#ifndef NDEBUG
                default:
                    printf(
                        "Ignoring spec-qual-list token %s because it is unsupported\n",
                        c_tokentype_name[param.subtype]
                    );
                    break;
#endif
            }
        } else if (param.type == TOKENTYPE_AST
                   && (param.subtype == C_AST_ANON_STRUCT || param.subtype == C_AST_NAMED_STRUCT)) {
            if (struct_tkn) {
                // TODO: Diagnostic.
            } else {
                struct_tkn = &list.params[i];
            }
        } else {
#ifndef NDEBUG
            printf("Ignoring spec-qual-list node because it is unsupported:");
            c_tkn_debug_print(param);
#endif
        }
    }

    if (struct_tkn
        && (n_long || has_int || has_short || has_char || has_float || has_double || has_void || has_bool
            || has_unsigned || has_signed)) {
        cctx_diagnostic(ctx->cctx, list.pos, DIAG_ERR, "Cannot be both primitive and compound type");
    }

    // C type parsing is messy, can't do much about that.
    if (struct_tkn) {
        switch (struct_tkn->params[0].subtype) {
            case C_KEYW_struct: type->primitive = C_COMP_STRUCT; break;
            case C_KEYW_union: type->primitive = C_COMP_UNION; break;
            case C_KEYW_enum: type->primitive = C_COMP_ENUM; break;
        }
    } else if (has_char) {
        if (has_unsigned || (!has_signed && !ctx->options.char_is_signed)) {
            type->primitive = C_PRIM_UCHAR;
        } else {
            type->primitive = C_PRIM_SCHAR;
        }
        has_char     = false;
        has_signed   = false;
        has_unsigned = false;
    } else if (has_short) {
        if (has_unsigned) {
            type->primitive = C_PRIM_USHORT;
        } else {
            type->primitive = C_PRIM_SSHORT;
        }
        has_short    = false;
        has_signed   = false;
        has_unsigned = false;
    } else if (n_long == 2) {
        if (has_unsigned) {
            type->primitive = C_PRIM_ULLONG;
        } else {
            type->primitive = C_PRIM_SLLONG;
        }
        n_long       = 0;
        has_signed   = false;
        has_unsigned = false;
    } else if (n_long == 1) {
        if (has_unsigned) {
            type->primitive = C_PRIM_ULONG;
        } else {
            type->primitive = C_PRIM_SLONG;
        }
        n_long       = 0;
        has_signed   = false;
        has_unsigned = false;
    } else if (has_int || has_unsigned || has_signed) {
        if (has_unsigned) {
            type->primitive = C_PRIM_UINT;
        } else {
            type->primitive = C_PRIM_SINT;
        }
        has_int      = false;
        has_signed   = false;
        has_unsigned = false;
    } else if (has_float) {
        type->primitive = C_PRIM_FLOAT;
        has_float       = false;
    } else if (has_double) {
        if (n_long) {
            type->primitive = C_PRIM_LDOUBLE;
        } else {
            type->primitive = C_PRIM_DOUBLE;
        }
        has_double = false;
        n_long--;
    } else if (has_bool) {
        type->primitive = C_PRIM_BOOL;
        has_bool        = false;
    } else if (has_void) {
        type->primitive = C_PRIM_VOID;
        has_void        = false;
    }

    if (!struct_tkn
        && (n_long || has_int || has_short || has_char || has_float || has_double || has_void || has_bool
            || has_unsigned || has_signed)) {
        cctx_diagnostic(ctx->cctx, list.pos, DIAG_ERR, "Invalid primitive type");
    }

    return rc;
}

// Create a C type and get the name from an (abstract) declarator.
// Takes ownership of the `spec_qual_type` share passed.
rc_t c_compile_decl(c_compiler_t *ctx, token_t decl, rc_t spec_qual_type, char const **name_out) {
    rc_t cur = spec_qual_type;
    if (name_out) {
        *name_out = NULL;
    }

    while (1) {
        if (decl.type == TOKENTYPE_IDENT) {
            if (name_out) {
                *name_out = decl.strval;
            }
            return cur;

        } else if (decl.type == TOKENTYPE_AST && decl.subtype == C_AST_TYPE_FUNC) {
            rc_t      func       = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *func_type  = func->data;
            func_type->primitive = C_COMP_FUNCTION;
            func_type->func.return_type = cur;
            func_type->func.args_len    = decl.params_len - 1;
            func_type->func.args        = strong_malloc(sizeof(rc_t) * (decl.params_len - 1));
            func_type->func.arg_names   = strong_malloc(sizeof(void *) * (decl.params_len - 1));

            for (size_t i = 0; i < decl.params_len - 1; i++) {
                token_t param = decl.params[i + 1];
                if (param.subtype == C_AST_SPEC_QUAL_LIST) {
                    func_type->func.args[i]      = c_compile_spec_qual_list(ctx, param);
                    func_type->func.arg_names[i] = NULL;
                } else {
                    rc_t        list = c_compile_spec_qual_list(ctx, param.params[0]);
                    char const *name_tmp;
                    func_type->func.args[i]      = c_compile_decl(ctx, param.params[1], list, &name_tmp);
                    func_type->func.arg_names[i] = strong_strdup(name_tmp);
                }
            }

            decl = decl.params[0];
            cur  = func;

        } else if (decl.type == TOKENTYPE_AST && decl.subtype == C_AST_TYPE_ARRAY) {
            rc_t      next       = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *next_type  = next->data;
            next_type->primitive = C_COMP_ARRAY;
            next_type->inner     = cur;

            if (decl.params_len) {
                // Inner node.
                decl = decl.params[0];
            } else {
                // No inner node.
                return next;
            }

            cur = next;

        } else if (decl.type == TOKENTYPE_AST && decl.subtype == C_AST_TYPE_PTR_TO) {
            rc_t      next       = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *next_type  = next->data;
            next_type->primitive = C_COMP_POINTER;
            next_type->inner     = cur;

            if (decl.params_len > 1 && decl.params[1].type == TOKENTYPE_AST
                && decl.params[1].subtype == C_AST_SPEC_QUAL_LIST) {
                // Add specifiers to pointer.
                token_t list = decl.params[1];
                for (size_t i = 0; i < list.params_len; i++) {
                    if (list.params[i].subtype == C_KEYW_volatile) {
                        next_type->is_volatile = true;
                    } else if (list.params[i].subtype == C_KEYW_const) {
                        next_type->is_const = true;
                    } else if (list.params[i].subtype == C_KEYW__Atomic) {
                        next_type->is_atomic = true;
                    } else if (list.params[i].subtype == C_KEYW_restrict) {
                        next_type->is_restrict = true;
                    }
                }

                if (decl.params_len > 2) {
                    // Inner node.
                    decl = decl.params[2];
                } else {
                    // No inner node.
                    return next;
                }
            } else if (decl.params_len > 1) {
                // Inner node.
                decl = decl.params[1];
            } else {
                // No inner node.
                return next;
            }

            cur = next;
        }
    }
}



// Create a new scope.
c_scope_t *c_scope_create(c_scope_t *parent) {
    c_scope_t *new_scope = strong_calloc(1, sizeof(c_scope_t));
    new_scope->parent    = parent;
    new_scope->depth     = parent->depth + 1;
    new_scope->locals    = STR_MAP_EMPTY;
    return new_scope;
}

// Clean up a scope.
void c_scope_destroy(c_scope_t *scope) {
    map_ent_t const *ent = map_next(&scope->locals, NULL);
    while (ent) {
        // TODO: This probably needs to be changed with struct support.
        c_var_t *local = ent->value;
        rc_delete(local->type);
        free(local);
        ent = map_next(&scope->locals, ent);
    }
    map_clear(&scope->locals);
    free(scope);
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



// Determine type promotion to apply in an infix context.
rc_t c_type_promote(c_tokentype_t oper, rc_t a_rc, rc_t b_rc) {
    c_type_t *a = a_rc->data;
    c_type_t *b = b_rc->data;
    c_type_t *tmp;
    if (a->primitive >= C_N_PRIM || b->primitive >= C_N_PRIM) {
        return NULL;
    } else if (a->primitive > b->primitive) {
        tmp = a;
    } else {
        tmp = b;
    }

    if (oper > C_TKN_LOR && tmp->primitive < C_PRIM_UINT) {
        // If a non-boolean operator is used on a type smaller than int, promote to it.
        bool is_unsigned = tmp->primitive & 1;
        return rc_share(primitive_cache[C_PRIM_SINT - is_unsigned]);
    } else {
        // Otherwise, merely drop stuff like `volatile` and `const`.
        return rc_share(primitive_cache[tmp->primitive]);
    }
}

// Convert C binary operator to IR binary operator.
ir_op2_type_t c_op2_to_ir_op2(c_tokentype_t subtype) {
    switch (subtype) {
        case C_TKN_ADD: return IR_OP2_ADD;
        case C_TKN_SUB: return IR_OP2_SUB;
        case C_TKN_MUL: return IR_OP2_MUL;
        case C_TKN_DIV: return IR_OP2_DIV;
        case C_TKN_MOD: return IR_OP2_MOD;
        case C_TKN_SHL: return IR_OP2_SHL;
        case C_TKN_SHR: return IR_OP2_SHR;
        case C_TKN_AND: return IR_OP2_BAND;
        case C_TKN_OR: return IR_OP2_BOR;
        case C_TKN_XOR: return IR_OP2_BXOR;
        case C_TKN_EQ: return IR_OP2_SEQ;
        case C_TKN_NE: return IR_OP2_SNE;
        case C_TKN_LT: return IR_OP2_SLT;
        case C_TKN_LE: return IR_OP2_SLE;
        case C_TKN_GT: return IR_OP2_SGT;
        case C_TKN_GE: return IR_OP2_SGE;
        default:
            printf("[BUG] C token %d cannot be converted to IR op2\n", subtype);
            abort();
            break;
    }
}

// Convert C unary operator to IR unary operator.
ir_op1_type_t c_op1_to_ir_op1(c_tokentype_t subtype) {
    switch (subtype) {
        case C_TKN_SUB: return IR_OP1_NEG;
        case C_TKN_NOT: return IR_OP1_BNEG;
        case C_TKN_LNOT: return IR_OP1_BNEG;
        default:
            printf("[BUG] C token %d cannot be converted to IR op1\n", subtype);
            abort();
            break;
    }
}

// Convert C primitive or pointer type to IR primitive type.
ir_prim_t c_type_to_ir_type(c_compiler_t *ctx, c_type_t *type) {
    c_prim_t c_prim = type->primitive;
    if (c_prim == C_COMP_POINTER) {
        c_prim = ctx->options.size_type;
    }
    switch (c_prim) {
        case C_PRIM_BOOL: return IR_PRIM_BOOL;
        case C_PRIM_UCHAR: return IR_PRIM_U8;
        case C_PRIM_SCHAR: return IR_PRIM_S8;
        case C_PRIM_USHORT: return !ctx->options.short16 ? IR_PRIM_U8 : IR_PRIM_U16;
        case C_PRIM_SSHORT: return !ctx->options.short16 ? IR_PRIM_S8 : IR_PRIM_S16;
        case C_PRIM_UINT: return !ctx->options.int32 ? IR_PRIM_U16 : IR_PRIM_U32;
        case C_PRIM_SINT: return !ctx->options.int32 ? IR_PRIM_S16 : IR_PRIM_S32;
        case C_PRIM_ULONG: return !ctx->options.long64 ? IR_PRIM_U32 : IR_PRIM_U64;
        case C_PRIM_SLONG: return !ctx->options.long64 ? IR_PRIM_S32 : IR_PRIM_S64;
        case C_PRIM_ULLONG: return IR_PRIM_U64;
        case C_PRIM_SLLONG: return IR_PRIM_S64;
        case C_PRIM_FLOAT: return IR_PRIM_F32;
        case C_PRIM_DOUBLE: return IR_PRIM_F64;
        case C_PRIM_LDOUBLE: return IR_PRIM_F64;
        default: __builtin_trap();
    }
}

// Cast one IR type to another according to the C rules for doing so.
ir_var_t *c_cast_ir_var(ir_code_t *code, ir_var_t *var, ir_prim_t type) {
    if (var->prim_type == type) {
        return var;
    }
    ir_var_t *dest = ir_var_create(code->func, type, NULL);
    ir_add_expr1(
        code,
        dest,
        type == IR_PRIM_BOOL ? IR_OP1_SNEZ : IR_OP1_MOV,
        (ir_operand_t){.is_const = false, .var = var}
    );
    return dest;
}



// Compile an expression into IR.
// If `assign` is `NULL`, then the expression is read; otherwise, it is written, and the expression must be an lvalue.
c_compile_expr_t c_compile_expr(
    c_compiler_t *ctx, ir_func_t *func, ir_code_t *code, c_scope_t *scope, token_t expr, ir_var_t *assign
) {
    if (expr.type == TOKENTYPE_IDENT) {
        // Look up variable in scope.
        c_var_t *c_var = c_scope_lookup(scope, expr.strval);
        if (!c_var) {
            cctx_diagnostic(ctx->cctx, expr.pos, DIAG_ERR, "Identifier %s is undefined", expr.strval);
            return (c_compile_expr_t){
                .code = code,
                .type = NULL,
                .var  = NULL,
            };
        }
        if (assign && ((c_type_t *)c_var->type->data)->is_const) {
            cctx_diagnostic(ctx->cctx, expr.pos, DIAG_ERR, "Expression must be a modifiable lvalue");
        } else if (assign) {
            ir_add_expr1(code, c_var->ir_var, IR_OP1_MOV, (ir_operand_t){.is_const = false, .var = assign});
        }
        return (c_compile_expr_t){
            .code = code,
            .type = rc_share(c_var->type),
            .var  = assign ?: c_var->ir_var,
        };

    } else if (expr.type == TOKENTYPE_ICONST || expr.type == TOKENTYPE_CCONST) {
        // Create integer constant.
        if (assign) {
            cctx_diagnostic(ctx->cctx, expr.pos, DIAG_ERR, "Expression must be a modifiable lvalue");
        }

        // TODO: Typed literals.
        ir_var_t *var = ir_var_create(func, ctx->options.int32 ? IR_PRIM_S32 : IR_PRIM_S16, NULL);
        ir_add_expr1(
            code,
            var,
            IR_OP1_MOV,
            (ir_operand_t){.is_const = true, .iconst = {.prim_type = var->prim_type, .constl = expr.ival}}
        );
        return (c_compile_expr_t){
            .code = code,
            .type = rc_share(primitive_cache[C_PRIM_SINT]),
            .var  = var,
        };

    } else if (expr.type == TOKENTYPE_SCONST) {
        printf("TODO: String constants\n");
        abort();

    } else if (expr.subtype == C_AST_EXPRS) {
        // Multiple expressions; return value from the last one.
        if (assign) {
            cctx_diagnostic(ctx->cctx, expr.pos, DIAG_ERR, "Expression must be a modifiable lvalue");
        }
        for (size_t i = 0; i < expr.params_len - 1; i++) {
            c_compile_expr_t res;
            res = c_compile_expr(ctx, func, code, scope, expr.params[i], NULL);
            if (!res.var) {
                return (c_compile_expr_t){
                    .code = code,
                    .type = NULL,
                    .var  = NULL,
                };
            }
            code = res.code;
            rc_delete(res.type);
        }
        return c_compile_expr(ctx, func, code, scope, expr.params[expr.params_len - 1], NULL);

    } else if (expr.subtype == C_AST_EXPR_CALL) {
        if (assign) {
            cctx_diagnostic(ctx->cctx, expr.pos, DIAG_ERR, "Expression must be a modifiable lvalue");
        }

        ir_var_t *funcptr;
        rc_t      functype_rc;
        if (expr.params[0].type != TOKENTYPE_IDENT) {
            // If not an ident, compile function addr expression.
            c_compile_expr_t res = c_compile_expr(ctx, func, code, scope, expr.params[0], NULL);
            if (!res.var) {
                return (c_compile_expr_t){
                    .code = code,
                    .type = NULL,
                    .var  = NULL,
                };
            }
            code        = res.code;
            funcptr     = res.var;
            functype_rc = res.type;
        } else {
            // If it is an ident, it should be a function in the global scope.
            c_var_t *funcvar = c_scope_lookup(scope, expr.params[0].strval);
            functype_rc      = funcvar->type;
        }
        c_type_t *functype = functype_rc->data;

        if (functype->primitive == C_COMP_POINTER
            && ((c_type_t *)functype->inner->data)->primitive == C_COMP_FUNCTION) {
            // Function pointer type.
            printf("TODO: Function pointer types\n");
            abort();
        }

        // Validate compatibility.
        if (functype->primitive != C_COMP_FUNCTION) {
            cctx_diagnostic(ctx->cctx, expr.params[0].pos, DIAG_ERR, "Expected a function type");
            rc_delete(functype_rc);
            return (c_compile_expr_t){
                .code = code,
                .type = NULL,
                .var  = NULL,
            };
        } else if (functype->func.args_len < expr.params_len - 1) {
            cctx_diagnostic(ctx->cctx, expr.params[functype->func.args_len + 1].pos, DIAG_ERR, "Too many arguments");
            rc_delete(functype_rc);
            return (c_compile_expr_t){
                .code = code,
                .type = NULL,
                .var  = NULL,
            };
        } else if (functype->func.args_len > expr.params_len - 1) {
            cctx_diagnostic(ctx->cctx, expr.pos, DIAG_ERR, "Not enough arguments");
            rc_delete(functype_rc);
            return (c_compile_expr_t){
                .code = code,
                .type = NULL,
                .var  = NULL,
            };
        }

        ir_operand_t *params = strong_calloc(sizeof(ir_operand_t), expr.params_len - 1);
        for (size_t i = 0; i < expr.params_len - 1; i++) {
            c_compile_expr_t res = c_compile_expr(ctx, func, code, scope, expr.params[i + 1], NULL);
            if (!res.var) {
                rc_delete(functype_rc);
                free(params);
                return (c_compile_expr_t){
                    .code = code,
                    .type = NULL,
                    .var  = NULL,
                };
            }
            // TODO: Do casting here.
            code               = res.code;
            params[i].is_const = false;
            params[i].var      = res.var;
        }

        // rc_t return_type = rc_share(functype->func.return_type);
        // rc_delete(functype_rc);
        // return (c_compile_expr_t){
        //     .code = code,
        //     .type = return_type,
        //     .var  = NULL,
        // };

    } else if (expr.subtype == C_AST_EXPR_INDEX) {
        printf("TODO: Index expressions\n");
        abort();

    } else if (expr.subtype == C_AST_EXPR_INFIX && expr.params[0].subtype == C_TKN_ARROW) {
        printf("TODO: Arrow operator\n");
        abort();

    } else if (expr.subtype == C_AST_EXPR_INFIX && expr.params[0].subtype == C_TKN_DOT) {
        printf("TODO: Dot operator\n");
        abort();

    } else if (expr.subtype == C_AST_EXPR_INFIX && expr.params[0].subtype == C_TKN_LOR) {
        printf("TODO: Logical OR operator\n");
        abort();

    } else if (expr.subtype == C_AST_EXPR_INFIX && expr.params[0].subtype == C_TKN_LAND) {
        printf("TODO: Logical AND operator\n");
        abort();

    } else if (expr.subtype == C_AST_EXPR_INFIX) {
        if (assign) {
            cctx_diagnostic(ctx->cctx, expr.pos, DIAG_ERR, "Expression must be a modifiable lvalue");
        }
        if (expr.params[0].subtype == C_TKN_ASSIGN) {
            // Simple assignment expression.
            c_compile_expr_t res;
            res = c_compile_expr(ctx, func, code, scope, expr.params[2], NULL);
            if (!res.var) {
                return (c_compile_expr_t){
                    .code = code,
                    .type = NULL,
                    .var  = NULL,
                };
            }
            code = res.code;
            return c_compile_expr(ctx, func, code, scope, expr.params[1], res.var);

        } else {
            // Other expressions.
            c_compile_expr_t res;
            bool             is_assign = expr.params[0].subtype >= C_TKN_ADD_S && expr.params[0].subtype <= C_TKN_XOR_S;

            // Get operands.
            res = c_compile_expr(ctx, func, code, scope, expr.params[1], NULL);
            if (!res.var) {
                return (c_compile_expr_t){
                    .code = code,
                    .type = NULL,
                    .var  = NULL,
                };
            }
            rc_t      type0 = res.type;
            ir_var_t *var0  = res.var;
            code            = res.code;
            res             = c_compile_expr(ctx, func, code, scope, expr.params[2], NULL);
            if (!res.var) {
                rc_delete(type0);
                return (c_compile_expr_t){
                    .code = code,
                    .type = NULL,
                    .var  = NULL,
                };
            }
            rc_t      type1 = res.type;
            ir_var_t *var1  = res.var;
            code            = res.code;

            // Determine promotion.
            c_tokentype_t op2  = is_assign ? expr.params[0].subtype + C_TKN_ADD - C_TKN_ADD_S : expr.params[0].subtype;
            rc_t          type = c_type_promote(op2, type0, type1);
            rc_delete(type0);
            rc_delete(type1);

            // Cast the variables if needed.
            ir_prim_t ir_prim;
            if (op2 >= C_TKN_EQ && op2 <= C_TKN_GE) {
                ir_prim = IR_PRIM_BOOL;
            } else {
                ir_prim = c_type_to_ir_type(ctx, type->data);
            }
            var0 = c_cast_ir_var(code, var0, ir_prim);
            var1 = c_cast_ir_var(code, var1, ir_prim);

            // Add math instruction.
            ir_var_t *tmpvar = ir_var_create(func, ir_prim, NULL);
            ir_add_expr2(
                code,
                tmpvar,
                c_op2_to_ir_op2(op2),
                (ir_operand_t){.is_const = false, .var = var0},
                (ir_operand_t){.is_const = false, .var = var1}
            );

            if (is_assign) {
                // Write back.
                return c_compile_expr(ctx, func, code, scope, expr.params[1], tmpvar);
            } else {
                return (c_compile_expr_t){
                    .code = code,
                    .type = type,
                    .var  = tmpvar,
                };
            }
        }

    } else if (expr.subtype == C_AST_EXPR_PREFIX) {
        if (expr.params[0].subtype == C_TKN_INC || expr.params[0].subtype == C_TKN_DEC) {
            // Pre-increment / pre-decrement.
            bool             is_inc = expr.params[0].subtype == C_TKN_INC;
            c_compile_expr_t res;
            res = c_compile_expr(ctx, func, code, scope, expr.params[1], NULL);
            if (!res.var) {
                return (c_compile_expr_t){
                    .code = code,
                    .type = NULL,
                    .var  = NULL,
                };
            }
            code = res.code;

            // Add or subtract one.
            ir_var_t *tmpvar = ir_var_create(func, c_type_to_ir_type(ctx, res.type->data), NULL);
            ir_add_expr2(
                code,
                tmpvar,
                is_inc ? IR_OP2_ADD : IR_OP2_SUB,
                (ir_operand_t){.is_const = false, .var = res.var},
                (ir_operand_t){.is_const = true, .iconst = {.prim_type = tmpvar->prim_type, .constl = 1, .consth = 0}}
            );

            // After modifying, write back and return value.
            return c_compile_expr(ctx, func, code, scope, expr.params[1], tmpvar);

        } else {
            // Normal unary operator.
            c_compile_expr_t res;
            res = c_compile_expr(ctx, func, code, scope, expr.params[1], NULL);
            if (!res.var) {
                return (c_compile_expr_t){
                    .code = code,
                    .type = NULL,
                    .var  = NULL,
                };
            }
            code = res.code;

            // Apply unary operator.
            ir_prim_t ir_prim;
            if (expr.params[0].subtype == C_TKN_LNOT) {
                ir_prim = IR_PRIM_BOOL;
            } else {
                ir_prim = c_type_to_ir_type(ctx, res.type->data);
            }
            ir_var_t *tmpvar = ir_var_create(func, ir_prim, NULL);
            ir_add_expr1(
                code,
                tmpvar,
                c_op1_to_ir_op1(expr.params[0].subtype),
                (ir_operand_t){.is_const = false, .var = res.var}
            );

            // Return temporary value.
            return (c_compile_expr_t){
                .code = code,
                .type = res.type,
                .var  = tmpvar,
            };
        }
    } else if (expr.subtype == C_AST_EXPR_SUFFIX) {
        // Post-increment / post-decrement.
        bool             is_inc = expr.params[0].subtype == C_TKN_INC;
        c_compile_expr_t res;
        res = c_compile_expr(ctx, func, code, scope, expr.params[1], NULL);
        if (!res.var) {
            return (c_compile_expr_t){
                .code = code,
                .type = NULL,
                .var  = NULL,
            };
        }
        code = res.code;

        // Save value for later use.
        ir_var_t *oldvar = ir_var_create(func, c_type_to_ir_type(ctx, res.type->data), NULL);
        ir_add_expr1(code, oldvar, IR_OP1_MOV, (ir_operand_t){.is_const = false, .var = res.var});

        // Add or subtract one.
        ir_var_t *tmpvar = ir_var_create(func, c_type_to_ir_type(ctx, res.type->data), NULL);
        ir_add_expr2(
            code,
            tmpvar,
            is_inc ? IR_OP2_ADD : IR_OP2_SUB,
            (ir_operand_t){.is_const = false, .var = res.var},
            (ir_operand_t){.is_const = true, .iconst = {.prim_type = tmpvar->prim_type, .constl = 1, .consth = 0}}
        );

        // After modifying, write back.
        res = c_compile_expr(ctx, func, code, scope, expr.params[1], tmpvar);
        if (!res.var) {
            return (c_compile_expr_t){
                .code = code,
                .type = NULL,
                .var  = NULL,
            };
        }
        code = res.code;

        // Return previous value.
        return (c_compile_expr_t){
            .code = code,
            .type = res.type,
            .var  = oldvar,
        };
    }
    abort();
}

// Compile a statement node into IR.
// Returns the code path linearly after this.
ir_code_t *c_compile_stmt(c_compiler_t *ctx, ir_func_t *func, ir_code_t *code, c_scope_t *scope, token_t stmt) {
    if (stmt.subtype == C_AST_STMTS) {
        // Multiple statements in scope.
        c_scope_t *new_scope = c_scope_create(scope);

        // Compile statements in this scope.
        for (size_t i = 0; i < stmt.params_len; i++) {
            code = c_compile_stmt(ctx, func, code, new_scope, stmt.params[i]);
        }

        // Clean up the scope.
        c_scope_destroy(new_scope);

    } else if (stmt.subtype == C_AST_EXPRS) {
        // Expression statement.
        code = c_compile_expr(ctx, func, code, scope, stmt, NULL).code;

    } else if (stmt.subtype == C_AST_DECLS) {
        // Local declarations.
        // TODO: Init declarator support.
        c_compile_decls(ctx, func, scope, stmt);

    } else if (stmt.subtype == C_AST_DO_WHILE || stmt.subtype == C_AST_WHILE) {
        // While or do...while loop.
        ir_code_t *loop_body = ir_code_create(func, NULL);
        ir_code_t *cond_body = ir_code_create(func, NULL);
        ir_code_t *after     = ir_code_create(func, NULL);

        // Compile expression.
        c_compile_expr_t expr = c_compile_expr(ctx, func, cond_body, scope, stmt.params[0], NULL);
        cond_body             = expr.code;
        if (expr.var) {
            ir_add_branch(
                cond_body,
                (ir_operand_t){.is_const = false, .var = c_cast_ir_var(cond_body, expr.var, IR_PRIM_BOOL)},
                loop_body
            );
        }
        ir_add_jump(cond_body, after);

        // Compile loop body.
        loop_body = c_compile_stmt(ctx, func, loop_body, scope, stmt.params[1]);
        ir_add_jump(loop_body, cond_body);

        if (stmt.subtype == C_AST_DO_WHILE) {
            // Jump to loop first for do...while.
            ir_add_jump(code, loop_body);
        } else {
            // Jump to condition first for while.
            ir_add_jump(code, cond_body);
        }

        code = after;

    } else if (stmt.subtype == C_AST_IF_ELSE) {
        // If or if...else statement.
        // Evaluate condition.
        c_compile_expr_t expr = c_compile_expr(ctx, func, code, scope, stmt.params[0], NULL);
        code                  = expr.code;

        // Allocate code paths.
        ir_code_t *if_body = ir_code_create(func, NULL);
        ir_code_t *after   = ir_code_create(func, NULL);
        c_compile_stmt(ctx, func, if_body, scope, stmt.params[1]);
        ir_add_jump(if_body, after);
        if (expr.var) {
            ir_add_branch(
                code,
                (ir_operand_t){.is_const = false, .var = c_cast_ir_var(code, expr.var, IR_PRIM_BOOL)},
                if_body
            );
        }
        if (stmt.params_len == 3) {
            // If...else statement.
            ir_code_t *else_body = ir_code_create(func, NULL);
            c_compile_stmt(ctx, func, else_body, scope, stmt.params[2]);
            ir_add_jump(else_body, after);
            ir_add_jump(code, else_body);
        } else {
            // If statement.
            ir_add_jump(code, after);
        }

        if (expr.type) {
            rc_delete(expr.type);
        }

        // Continue after the if statement.
        code = after;

    } else if (stmt.subtype == C_AST_FOR_LOOP) {
        // For loop.
        if (stmt.params[0].type == TOKENTYPE_AST && stmt.params[0].subtype == C_AST_DECLS) {
            // Setup is a decls.
        } else if (stmt.params[0].type != TOKENTYPE_AST || stmt.params[0].subtype != C_AST_NOP) {
            // Setup is an expr.
        }

        ir_code_t *for_cond = ir_code_create(func, NULL);
        ir_code_t *for_body = ir_code_create(func, NULL);

    } else if (stmt.subtype == C_AST_RETURN) {
        // Return statement.
        if (stmt.params_len) {
            // With value.
            c_compile_expr_t expr = c_compile_expr(ctx, func, code, scope, stmt.params[0], NULL);
            code                  = expr.code;
            if (expr.var) {
                ir_add_return1(code, (ir_operand_t){.is_const = false, .var = expr.var});
            }
            if (expr.type) {
                rc_delete(expr.type);
            }
        } else {
            // Without.
            ir_add_return0(code);
        }
    }
    return code;
}

// Compile a C function definition into IR.
ir_func_t *c_compile_func_def(c_compiler_t *ctx, token_t def) {
    rc_t inner_type = c_compile_spec_qual_list(ctx, def.params[0]);
    if (!inner_type) {
        return NULL;
    }

    char const *name;
    rc_t        func_type_rc = c_compile_decl(ctx, def.params[1], inner_type, &name);
    if (!name) {
        // A diagnostic will already have been created; skip this.
        rc_delete(func_type_rc);
        return NULL;
    }
    c_type_t *func_type = func_type_rc->data;

    // Create function and scope.
    ir_func_t *func  = ir_func_create(name, NULL, func_type->func.args_len, NULL);
    c_scope_t *scope = c_scope_create(&ctx->global_scope);

    // Bring parameters into scope.
    for (size_t i = 0; i < func_type->func.args_len; i++) {
        if (func_type->func.arg_names[i]) {
            c_var_t *var           = calloc(1, sizeof(c_var_t));
            var->type              = rc_share(func_type->func.args[i]);
            var->ir_var            = func->args[i];
            var->ir_var->prim_type = c_type_to_ir_type(ctx, var->type->data);
            map_set(&scope->locals, func_type->func.arg_names[i], var);
        }
    }

    // Compile the statements inside this same scope.
    token_t    body = def.params[2];
    ir_code_t *code = (ir_code_t *)func->code_list.head;
    for (size_t i = 0; i < body.params_len; i++) {
        code = c_compile_stmt(ctx, func, code, scope, body.params[i]);
    }

    c_scope_destroy(scope);

    // Add implicit empty return statement.
    ir_add_return0(code);
    rc_delete(func_type_rc);

    return func;
}

// Compila a declaration statement.
void c_compile_decls(c_compiler_t *ctx, ir_func_t *func, c_scope_t *scope, token_t decls) {
    // TODO: Typedef support.
    rc_t inner_type = c_compile_spec_qual_list(ctx, decls.params[0]);
    if (!inner_type) {
        return;
    }

    for (size_t i = 1; i < decls.params_len; i++) {
        char const *name      = NULL;
        rc_t        decl_type = c_compile_decl(ctx, decls.params[i], rc_share(inner_type), &name);
        if (!name) {
            // A diagnostic will already have been created; skip this.
            rc_delete(decl_type);
            continue;
        }

        c_var_t *var   = calloc(1, sizeof(c_var_t));
        var->is_global = scope->depth == 0;
        var->type      = decl_type;
        var->ir_var    = func ? ir_var_create(func, c_type_to_ir_type(ctx, decl_type->data), NULL) : NULL;
        map_set(&scope->locals, name, var);
    }
    rc_delete(inner_type);
}



// Explain a C type.
void c_type_explain(c_type_t *type, FILE *to) {
start:
    if (type->primitive == C_COMP_FUNCTION) {
        fputs("function(", to);
        for (size_t i = 0; i < type->func.args_len; i++) {
            if (i) {
                fputs(", ", to);
            }
            c_type_explain(type->func.args[i]->data, to);
        }
        fputs(") -> ", to);
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
    }
    if (type->is_const) {
        fputs(" const", to);
    }
    if (type->is_volatile) {
        fputs(" volatile", to);
    }
}
