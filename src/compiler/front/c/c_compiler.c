
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"

#include "c_parser.h"
#include "c_prepass.h"
#include "ir/ir_interpreter.h"
#include "strong_malloc.h"


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

    cc->prim_types[C_PRIM_BOOL].size    = 1;
    cc->prim_types[C_PRIM_UCHAR].size   = 1;
    cc->prim_types[C_PRIM_SCHAR].size   = 1;
    cc->prim_types[C_PRIM_USHORT].size  = options.short16 ? 2 : 1;
    cc->prim_types[C_PRIM_SSHORT].size  = options.short16 ? 2 : 1;
    cc->prim_types[C_PRIM_UINT].size    = options.int32 ? 4 : 2;
    cc->prim_types[C_PRIM_SINT].size    = options.int32 ? 4 : 2;
    cc->prim_types[C_PRIM_ULONG].size   = options.long64 ? 8 : 4;
    cc->prim_types[C_PRIM_SLONG].size   = options.long64 ? 8 : 4;
    cc->prim_types[C_PRIM_ULLONG].size  = 8;
    cc->prim_types[C_PRIM_SLLONG].size  = 8;
    cc->prim_types[C_PRIM_FLOAT].size   = 4;
    cc->prim_types[C_PRIM_DOUBLE].size  = 8;
    cc->prim_types[C_PRIM_LDOUBLE].size = 8;
    cc->prim_types[C_PRIM_VOID].size    = 0;

    for (size_t i = 0; i < C_N_PRIM; i++) {
        cc->prim_types[i].primitive = i;
        cc->prim_types[i].align     = cc->prim_types[i].size;
        cc->prim_rcs[i].refcount    = 1;
        cc->prim_rcs[i].data        = &cc->prim_types[i];
        cc->prim_rcs[i].cleanup     = c_compiler_t_fake_cleanup;
    }

    return cc;
}

// Destroy a C compiler context.
void c_compiler_destroy(c_compiler_t *cc) {
    map_foreach(ent, &cc->global_scope.locals) {
        c_var_t *local = ent->value;
        rc_delete(local->type);
        free(local);
    }
    map_clear(&cc->global_scope.locals);
    free(cc);
}



// Recursively calculate the layout of a `c_type_t`.
static void c_type_calc_layout(c_compiler_t *ctx, c_type_t *type) {
    if (type->primitive == C_COMP_POINTER) {
        c_type_calc_layout(ctx, type->inner->data);
        type->size  = ctx->prim_types[ctx->options.size_type].size;
        type->align = ctx->prim_types[ctx->options.size_type].align;
    } else if (type->primitive == C_COMP_ARRAY) {
        c_type_calc_layout(ctx, type->inner->data);
        type->align = ((c_type_t *)type->inner->data)->align;
        type->size  = ((c_type_t *)type->inner->data)->size * 1; // TODO: Array bounds.
    } else if (type->primitive == C_COMP_STRUCT || type->primitive == C_COMP_UNION) {
        printf("TODO: Calculate inner layout of structs and unions.\n");
        abort();
    } else {
        type->size  = ctx->prim_types[type->primitive].size;
        type->align = ctx->prim_types[type->primitive].align;
    }
}

// Clean up a `c_type_t`.
static void c_type_free(c_type_t *type) {
    if (type->primitive == C_COMP_FUNCTION) {
        rc_delete(type->func.return_type);
        for (size_t i = 0; i < type->func.args_len; i++) {
            rc_delete(type->func.args[i]);
            free(type->func.arg_names[i]);
        }
        free(type->func.args);
        free(type->func.arg_names);
        free(type->func.arg_name_tkns);
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
rc_t c_compile_spec_qual_list(c_compiler_t *ctx, token_t const *list) {
    token_t const *struct_tkn   = NULL;
    int            n_long       = 0;
    bool           has_int      = false;
    bool           has_short    = false;
    bool           has_char     = false;
    bool           has_float    = false;
    bool           has_double   = false;
    bool           has_void     = false;
    bool           has_bool     = false;
    bool           has_unsigned = false;
    bool           has_signed   = false;

    rc_t      rc   = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
    c_type_t *type = rc->data;

    // Turn the list into a more manageable format.
    for (size_t i = 0; i < list->params_len; i++) {
        token_t param = list->params[i];
        if (param.type == TOKENTYPE_KEYWORD) {
            switch (param.subtype) {
                case C_KEYW__Atomic: type->is_atomic = true; break;
                case C_KEYW_volatile: type->is_volatile = true; break;
                case C_KEYW_const: type->is_const = true; break;
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
                struct_tkn = &list->params[i];
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
        cctx_diagnostic(ctx->cctx, list->pos, DIAG_ERR, "Cannot be both primitive and compound type");
    }

    // C type parsing is messy, can't do much about that.
    if (struct_tkn) {
        fprintf(stderr, "[TODO] C compiler doesn't support structs, enums or unions yet\n");
        abort();
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
        cctx_diagnostic(ctx->cctx, list->pos, DIAG_ERR, "Invalid primitive type");
    }

    c_type_calc_layout(ctx, type);

    return rc;
}

// Create a C type and get the name from an (abstract) declarator.
// Takes ownership of the `spec_qual_type` share passed.
rc_t c_compile_decl(c_compiler_t *ctx, token_t const *decl, rc_t spec_qual_type, token_t const **name_out) {
    rc_t cur = spec_qual_type;
    if (name_out) {
        *name_out = NULL;
    }

    if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_ASSIGN_DECL) {
        decl = &decl->params[0];
    }
    while (1) {
        if (decl->type == TOKENTYPE_IDENT) {
            if (name_out) {
                *name_out = decl;
            }
            c_type_calc_layout(ctx, cur->data);
            return cur;

        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_NOP) {
            return cur;

        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_TYPE_FUNC) {
            rc_t      func       = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *func_type  = func->data;
            func_type->primitive = C_COMP_FUNCTION;
            func_type->func.return_type = cur;
            func_type->func.args_len    = decl->params_len - 1;
            if (decl->params_len > 1) {
                func_type->func.args          = strong_calloc((decl->params_len - 1), sizeof(rc_t));
                func_type->func.arg_names     = strong_calloc((decl->params_len - 1), sizeof(void *));
                func_type->func.arg_name_tkns = strong_calloc((decl->params_len - 1), sizeof(void *));
            }

            for (size_t i = 0; i < decl->params_len - 1; i++) {
                token_t const *param = &decl->params[i + 1];
                if (param->subtype == C_AST_SPEC_QUAL_LIST) {
                    func_type->func.args[i] = c_compile_spec_qual_list(ctx, param);
                } else {
                    token_t const *name_tmp;
                    rc_t           list              = c_compile_spec_qual_list(ctx, &param->params[0]);
                    func_type->func.args[i]          = c_compile_decl(ctx, &param->params[1], list, &name_tmp);
                    func_type->func.arg_names[i]     = strong_strdup(name_tmp->strval);
                    func_type->func.arg_name_tkns[i] = name_tmp;
                }
            }

            decl = &decl->params[0];
            cur  = func;

        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_TYPE_ARRAY) {
            rc_t      next       = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *next_type  = next->data;
            next_type->primitive = C_COMP_ARRAY;
            next_type->inner     = cur;

            if (decl->params_len) {
                // Inner node.
                decl = &decl->params[0];
            } else {
                // No inner node.
                c_type_calc_layout(ctx, next->data);
                return next;
            }

            cur = next;

        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_TYPE_PTR_TO) {
            rc_t      next       = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *next_type  = next->data;
            next_type->primitive = C_COMP_POINTER;
            next_type->size      = ctx->prim_types[ctx->options.size_type].size;
            next_type->align     = ctx->prim_types[ctx->options.size_type].align;
            next_type->inner     = cur;

            if (decl->params_len > 1 && decl->params[1].type == TOKENTYPE_AST
                && decl->params[1].subtype == C_AST_SPEC_QUAL_LIST) {
                // Add specifiers to pointer.
                token_t const *list = &decl->params[1];
                for (size_t i = 0; i < list->params_len; i++) {
                    if (list->params[i].subtype == C_KEYW_volatile) {
                        next_type->is_volatile = true;
                    } else if (list->params[i].subtype == C_KEYW_const) {
                        next_type->is_const = true;
                    } else if (list->params[i].subtype == C_KEYW__Atomic) {
                        next_type->is_atomic = true;
                    } else if (list->params[i].subtype == C_KEYW_restrict) {
                        next_type->is_restrict = true;
                    }
                }

                if (decl->params_len > 2) {
                    // Inner node.
                    decl = &decl->params[2];
                } else {
                    // No inner node.
                    return next;
                }
            } else if (decl->params_len > 1) {
                // Inner node.
                decl = &decl->params[1];
            } else {
                // No inner node.
                return next;
            }

            cur = next;
        } else {
            abort();
        }
    }
}



// Create a new scope.
c_scope_t *c_scope_create(c_scope_t *parent) {
    c_scope_t *new_scope      = strong_calloc(1, sizeof(c_scope_t));
    new_scope->parent         = parent;
    new_scope->depth          = parent->depth + 1;
    new_scope->locals         = STR_MAP_EMPTY;
    new_scope->locals_by_decl = PTR_MAP_EMPTY;
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
    map_clear(&scope->locals_by_decl);
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



// Create a type that is a pointer to an existing type.
rc_t c_type_pointer(c_compiler_t *ctx, rc_t inner) {
    c_type_t *data  = strong_calloc(1, sizeof(c_type_t));
    data->primitive = C_COMP_POINTER;
    data->inner     = inner;
    data->size      = ctx->prim_types[ctx->options.size_type].size;
    return rc_new_strong(data, (void (*)(void *))c_type_free);
}

// Determine type promotion to apply in an infix context.
// Note: This does not take ownership of the refcount ptr;
// It will call `rc_share` on the callers behalf only if necessary.
rc_t c_type_promote(c_compiler_t *ctx, c_tokentype_t oper, rc_t a_rc, rc_t b_rc) {
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
        return rc_share(&ctx->prim_rcs[C_PRIM_SINT - is_unsigned]);
    } else {
        // Otherwise, merely drop stuff like `volatile` and `const`.
        return rc_share(&ctx->prim_rcs[tmp->primitive]);
    }
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
        case C_TKN_LNOT: return IR_OP1_bneg;
        default:
            printf("[BUG] C token %d cannot be converted to IR op1\n", subtype);
            abort();
            break;
    }
}

// Convert C primitive or pointer type to IR primitive type.
ir_prim_t c_prim_to_ir_type(c_compiler_t *ctx, c_prim_t prim) {
    switch (prim) {
        case C_PRIM_BOOL: return IR_PRIM_bool;
        case C_PRIM_UCHAR: return IR_PRIM_u8;
        case C_PRIM_SCHAR: return IR_PRIM_s8;
        case C_PRIM_USHORT: return !ctx->options.short16 ? IR_PRIM_u8 : IR_PRIM_u16;
        case C_PRIM_SSHORT: return !ctx->options.short16 ? IR_PRIM_s8 : IR_PRIM_s16;
        case C_PRIM_UINT: return !ctx->options.int32 ? IR_PRIM_u16 : IR_PRIM_u32;
        case C_PRIM_SINT: return !ctx->options.int32 ? IR_PRIM_s16 : IR_PRIM_s32;
        case C_PRIM_ULONG: return !ctx->options.long64 ? IR_PRIM_u32 : IR_PRIM_u64;
        case C_PRIM_SLONG: return !ctx->options.long64 ? IR_PRIM_s32 : IR_PRIM_s64;
        case C_PRIM_ULLONG: return IR_PRIM_u64;
        case C_PRIM_SLLONG: return IR_PRIM_s64;
        case C_PRIM_FLOAT: return IR_PRIM_f32;
        case C_PRIM_DOUBLE: return IR_PRIM_f64;
        case C_PRIM_LDOUBLE: return IR_PRIM_f64;
        default: return IR_PRIM_u8;
    }
}

// Convert C primitive or pointer type to IR primitive type.
ir_prim_t c_type_to_ir_type(c_compiler_t *ctx, c_type_t *type) {
    c_prim_t c_prim = type->primitive;
    if (c_prim == C_COMP_POINTER) {
        c_prim = ctx->options.size_type;
    }
    return c_prim_to_ir_type(ctx, c_prim);
}

// Cast one IR type to another according to the C rules for doing so.
// TODO: This may not be sufficient as the type system evolves.
ir_operand_t c_cast_ir_operand(ir_code_t *code, ir_operand_t operand, ir_prim_t type) {
    if (operand.is_const) {
        // Casting constants.
        if (operand.iconst.prim_type == type) {
            return operand;
        }
        return (ir_operand_t){
            .is_const = true,
            .iconst   = ir_cast(type, operand.iconst),
        };
    } else {
        // Casting variables or expressions.
        ir_op1_type_t op1 = type == IR_PRIM_bool ? IR_OP1_snez : IR_OP1_mov;
        if (operand.var->prim_type == type) {
            return operand;
        }
        ir_var_t *dest = ir_var_create(code->func, type, NULL);
        ir_add_expr1(IR_APPEND(code), dest, op1, operand);
        return (ir_operand_t){
            .is_const = false,
            .var      = dest,
        };
    }
}



// Clean up an lvalue or rvalue.
void c_value_destroy(c_value_t value) {
    rc_delete(value.c_type);
}

// Write to an lvalue.
void c_value_write(
    c_compiler_t *ctx, ir_code_t *code, c_scope_t *scope, c_value_t const *lvalue, c_value_t const *rvalue
) {
    ir_operand_t tmp = c_value_read(ctx, code, scope, rvalue);
    switch (lvalue->value_type) {
        case C_VALUE_ERROR:
            printf("[BUG] c_value_write called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE:
            printf("[BUG] c_value_write called with an `rvalue` destination");
            abort();
            break;
        case C_LVALUE_PTR:
            // Store to the pointer.
            ir_add_store(IR_APPEND(code), tmp, lvalue->lvalue.ptr);
            c_clobber_memory(ctx, code, scope, true);
            break;
        case C_LVALUE_VAR:
            // Store to the IR variable.
            ir_add_expr1(IR_APPEND(code), lvalue->lvalue.c_var->ir_var, IR_OP1_mov, tmp);
            // We always store to the register first, copying back to memory can happen later.
            lvalue->lvalue.c_var->register_up_to_date = true;
            lvalue->lvalue.c_var->memory_up_to_date   = false;
            break;
    }
}

// Get the address of an lvalue.
ir_operand_t c_value_addrof(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue) {
    if (lvalue->value_type == C_VALUE_ERROR) {
        printf("[BUG] c_value_addrof called on C_VALUE_ERROR\n");
        abort();
    } else if (lvalue->value_type == C_RVALUE) {
        printf("[BUG] c_value_addrof called on C_RVALUE\n");
        abort();
    }

    ir_prim_t    ptr_prim = c_prim_to_ir_type(ctx, ctx->options.size_type);
    ir_operand_t addr;
    if (lvalue->value_type == C_LVALUE_PTR) {
        // Directly use the pointer from the lvalue.
        addr = lvalue->lvalue.ptr;

    } else if (lvalue->value_type == C_LVALUE_VAR) {
        // Take pointer of C variable.
        if (!lvalue->lvalue.c_var->pointer_taken) {
            printf("[BUG] Address taken of C variable but the variable was not marked as such\n");
            abort();
        }
        addr.is_const = false;
        addr.var      = ir_var_create(code->func, ptr_prim, NULL);
        if (lvalue->lvalue.c_var->is_global) {
            ir_add_lea_symbol(IR_APPEND(code), addr.var, lvalue->lvalue.c_var->symbol, 0);
        } else {
            ir_add_lea_stack(IR_APPEND(code), addr.var, lvalue->lvalue.c_var->ir_frame, 0);
        }

    } else {
        abort();
    }

    return addr;
}

// Read a value for scalar arithmetic.
ir_operand_t c_value_read(c_compiler_t *ctx, ir_code_t *code, c_scope_t *scope, c_value_t const *value) {
    switch (value->value_type) {
        default: abort(); break;
        case C_VALUE_ERROR:
            printf("[BUG] c_value_read called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE:
            // Rvalues don't need any reading because they're already in an `ir_operand_t`.
            return value->rvalue;
        case C_LVALUE_PTR:
            // Pointer lvalue is read from memory.
            c_clobber_memory(ctx, code, scope, false);
            ir_var_t *tmp = ir_var_create(code->func, c_type_to_ir_type(ctx, value->c_type->data), NULL);
            ir_add_load(IR_APPEND(code), tmp, c_value_addrof(ctx, code, value));
            return (ir_operand_t){
                .is_const = false,
                .var      = tmp,
            };
        case C_LVALUE_VAR:
            // C variables are stored in IR variables.
            if (!value->lvalue.c_var->register_up_to_date) {
                // Load the variable into a register if it's not in one already.
                if (!value->lvalue.c_var->memory_up_to_date) {
                    printf("[BUG] Variable was lost; neither memory nor register up to date\n");
                    abort();
                } else if (!value->lvalue.c_var->pointer_taken) {
                    printf("[BUG] Variable is marked as not register-resident but no pointer was taken\n");
                    abort();
                }
                ir_var_t *addr = ir_var_create(code->func, c_prim_to_ir_type(ctx, ctx->options.size_type), NULL);
                if (value->lvalue.c_var->is_global) {
                    ir_add_lea_symbol(IR_APPEND(code), addr, value->lvalue.c_var->symbol, 0);
                } else {
                    ir_add_lea_stack(IR_APPEND(code), addr, value->lvalue.c_var->ir_frame, 0);
                }
                ir_add_load(
                    IR_APPEND(code),
                    value->lvalue.c_var->ir_var,
                    (ir_operand_t){.is_const = false, .var = addr}
                );
                value->lvalue.c_var->register_up_to_date = true;
            }
            // After this point, the variable is guaranteed to be in a register.
            return (ir_operand_t){
                .is_const = false,
                .var      = value->lvalue.c_var->ir_var,
            };
    }
}



// Clobber memory in the current scope.
// If `do_load` is `true`, clobbered variables will be loaded. Otherwise, they will be stored.
void c_clobber_memory(c_compiler_t *ctx, ir_code_t *code, c_scope_t *scope, bool do_load) {
    while (scope) {
        map_foreach(entry, &scope->locals) {
            c_var_t *var = (c_var_t *)entry->value;
            if (!var->pointer_taken) {
                // Skip if the variable is not aliased.
                continue;
            }
            if (do_load) {
                // Mark register as out-of-date.
                var->register_up_to_date = false;
                var->memory_up_to_date   = true;
            } else {
                // Store to memory.
                if (!var->memory_up_to_date) {
                    ir_var_t *addr = ir_var_create(code->func, c_prim_to_ir_type(ctx, ctx->options.size_type), NULL);
                    if (var->is_global) {
                        ir_add_lea_symbol(IR_APPEND(code), addr, var->symbol, 0);
                    } else {
                        ir_add_lea_stack(IR_APPEND(code), addr, var->ir_frame, 0);
                    }
                    ir_add_store(
                        IR_APPEND(code),
                        (ir_operand_t){.is_const = false, .var = var->ir_var},
                        (ir_operand_t){.is_const = false, .var = addr}
                    );
                }
                // And mark memory as up-to-date.
                var->memory_up_to_date   = true;
                var->register_up_to_date = false;
            }
        }
        scope = scope->parent;
    }
}

// Transfer affected local variables to registers and global variables to memory.
// Only applies to variables that are aliased by a pointer.
// Used before/after branching paths such as if statements.
void c_create_branch_consistency(c_compiler_t *ctx, ir_code_t *code, c_scope_t *scope, set_t const *affected_vars) {
    if (!affected_vars) {
        return;
    }
    set_foreach(token_t const, var_ident, affected_vars) {
        c_var_t *var = c_scope_lookup_by_decl(scope, var_ident);
        if (!var) {
            printf("[BUG] Affected variables set for c_create_branch_consistency contains an undefined variable\n");
            abort();
        }
        if (!var->pointer_taken) {
            // Variable isn't aliased so it's always in a register.
            continue;
        } else if (var->is_global && !var->memory_up_to_date) {
            // Global needs to be copied to memory.
            ir_var_t *addr = ir_var_create(code->func, c_prim_to_ir_type(ctx, ctx->options.size_type), NULL);
            ir_add_lea_symbol(IR_APPEND(code), addr, var->symbol, 0);
            ir_add_store(
                IR_APPEND(code),
                (ir_operand_t){.is_const = false, .var = var->ir_var},
                (ir_operand_t){.is_const = false, .var = addr}
            );
            var->memory_up_to_date = true;
        } else if (!var->is_global && !var->register_up_to_date) {
            // Variable needs to be copied to a register.
            ir_var_t *addr = ir_var_create(code->func, c_prim_to_ir_type(ctx, ctx->options.size_type), NULL);
            ir_add_lea_stack(IR_APPEND(code), addr, var->ir_frame, 0);
            ir_add_load(IR_APPEND(code), var->ir_var, (ir_operand_t){.is_const = false, .var = addr});
            var->register_up_to_date = true;
        }
        // Preventively clobber the location the variable isn't supposed to be in.
        if (var->is_global) {
            var->register_up_to_date = false;
        } else {
            var->memory_up_to_date = false;
        }
    }
}

// Assume the variables are in the state that would be created by `c_create_branch_consistency`.
void c_assume_branch_consistency(c_compiler_t *ctx, c_scope_t *scope, set_t const *affected_vars) {
    if (!affected_vars) {
        return;
    }
    set_foreach(token_t const, var_ident, affected_vars) {
        c_var_t *var = c_scope_lookup_by_decl(scope, var_ident);
        if (!var) {
            printf("[BUG] Affected variables set for c_create_branch_consistency contains an undefined variable\n");
            abort();
        }
        if (!var->pointer_taken) {
            // Variable isn't aliased so it's always in a register.
            continue;
        } else if (var->is_global && !var->memory_up_to_date) {
            // Global assumed to be copied to memory.
            var->memory_up_to_date = true;
        } else if (!var->is_global && !var->register_up_to_date) {
            // Variable assumed to be copied to a register.
            var->register_up_to_date = true;
        }
        // Preventively clobber the location the variable isn't supposed to be in.
        if (var->is_global) {
            var->register_up_to_date = false;
        } else {
            var->memory_up_to_date = false;
        }
    }
}



// Compile an expression into IR.
c_compile_expr_t
    c_compile_expr(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *expr) {
    if (expr->type == TOKENTYPE_IDENT) {
        // Look up variable in scope.
        // TODO: Implement enums here.
        c_var_t *c_var = c_scope_lookup(scope, expr->strval);
        if (!c_var) {
            cctx_diagnostic(ctx->cctx, expr->pos, DIAG_ERR, "Identifier %s is undefined", expr->strval);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        }

        return (c_compile_expr_t){
            .code = code,
            .res  = {
                 .value_type = C_LVALUE_VAR,
                 .c_type     = rc_share(c_var->type),
                 .lvalue     = {
                         .c_var = c_var,
                },
            },
        };

    } else if (expr->type == TOKENTYPE_ICONST || expr->type == TOKENTYPE_CCONST) {
        return (c_compile_expr_t){
            .code = code,
            .res  = {
                 .value_type = C_RVALUE,
                 .c_type     = rc_share(&ctx->prim_rcs[expr->subtype]),
                 .rvalue
                = {.is_const = true,
                    .iconst   = {
                          .prim_type = c_prim_to_ir_type(ctx, expr->subtype),
                          .constl    = expr->ival,
                   }},
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
            funcptr     = c_value_read(ctx, code, scope, &res.res);
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
        printf("TODO: Arrow operator\n");
        abort();

    } else if (expr->subtype == C_AST_EXPR_INFIX && expr->params[0].subtype == C_TKN_DOT) {
        printf("TODO: Dot operator\n");
        abort();

    } else if (expr->subtype == C_AST_EXPR_INFIX && expr->params[0].subtype == C_TKN_LOR) {
        printf("TODO: Logical OR operator\n");
        abort();

    } else if (expr->subtype == C_AST_EXPR_INFIX && expr->params[0].subtype == C_TKN_LAND) {
        printf("TODO: Logical AND operator\n");
        abort();

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
            code             = res.code;
            c_value_t rvalue = res.res;

            // Compile the lvalue.
            res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            code             = res.code;
            c_value_t lvalue = res.res;

            // Assert that it's actually a writable lvalue.
            if (lvalue.value_type == C_RVALUE || ((c_type_t *)lvalue.c_type->data)->is_const) {
                cctx_diagnostic(ctx->cctx, expr->params[1].pos, DIAG_ERR, "Expression must be a modifiable lvalue");
                c_value_destroy(lvalue);
                c_value_destroy(rvalue);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            // Write into the lvalue.
            c_value_write(ctx, code, scope, &lvalue, &rvalue);
            // The C semantics specify that the result of an assignment expression is an rvalue.
            // Therefor, clean up the lvalue and return the rvalue.
            c_value_destroy(lvalue);
            return (c_compile_expr_t){
                .code = code,
                .res  = rvalue,
            };

        } else {
            // Other expressions.
            c_compile_expr_t res;
            bool is_assign = expr->params[0].subtype >= C_TKN_ADD_S && expr->params[0].subtype <= C_TKN_XOR_S;

            // Get operands.
            res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
            if (res.res.value_type == C_VALUE_ERROR) {
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            c_value_t lhs = res.res;
            code          = res.code;
            res           = c_compile_expr(ctx, prepass, code, scope, &expr->params[2]);
            if (res.res.value_type == C_VALUE_ERROR) {
                c_value_destroy(lhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }
            c_value_t rhs = res.res;
            code          = res.code;

            // Determine promotion.
            c_tokentype_t op2 = is_assign ? expr->params[0].subtype + C_TKN_ADD - C_TKN_ADD_S : expr->params[0].subtype;
            rc_t          type = c_type_promote(ctx, op2, lhs.c_type, rhs.c_type);
            rc_t          res_type;

            // Cast the variables if needed.
            ir_prim_t ir_prim = c_type_to_ir_type(ctx, type->data);
            if (op2 >= C_TKN_EQ && op2 <= C_TKN_GE) {
                res_type = rc_share(&ctx->prim_rcs[C_PRIM_BOOL]);
            } else {
                res_type = rc_share(type);
            }
            ir_operand_t ir_lhs = c_cast_ir_operand(code, c_value_read(ctx, code, scope, &lhs), ir_prim);
            ir_operand_t ir_rhs = c_cast_ir_operand(code, c_value_read(ctx, code, scope, &rhs), ir_prim);

            if (ir_lhs.is_const && ir_rhs.is_const) {
                // Resulting constant can be evaluated at compile-time.
                c_value_destroy(lhs);
                c_value_destroy(rhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = {
                         .value_type = C_RVALUE,
                         .c_type     = type,
                         .rvalue     = {
                                 .is_const = true,
                                 .iconst   = ir_calc2(c_op2_to_ir_op2(op2), ir_lhs.iconst, ir_rhs.iconst),
                        },
                    },
                };
            }

            // Add math instruction.
            ir_var_t *tmpvar = ir_var_create(code->func, c_type_to_ir_type(ctx, res_type->data), NULL);
            ir_add_expr2(IR_APPEND(code), tmpvar, c_op2_to_ir_op2(op2), ir_lhs, ir_rhs);

            c_value_t rvalue = {
                .value_type = C_RVALUE,
                .c_type     = res_type,
                .rvalue     = {
                        .is_const = false,
                        .var      = tmpvar,
                },
            };

            if (expr->params[0].subtype >= C_TKN_ADD_S && expr->params[0].subtype <= C_TKN_XOR_S) {
                // Assignment arithmetic expression; write back.
                if (lhs.value_type == C_RVALUE || ((c_type_t *)lhs.c_type->data)->is_const) {
                    cctx_diagnostic(ctx->cctx, expr->params[1].pos, DIAG_ERR, "Expression must be a modifiable lvalue");
                } else {
                    c_value_write(ctx, code, scope, &lhs, &rvalue);
                }
            }

            c_value_destroy(lhs);
            c_value_destroy(rhs);
            rc_delete(type);
            return (c_compile_expr_t){
                .code = code,
                .res  = rvalue,
            };
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
            code = res.code;

            // Assert that it's writeable.
            if (res.res.value_type == C_RVALUE || ((c_type_t *)res.res.c_type->data)->is_const) {
                cctx_diagnostic(ctx->cctx, expr->params[1].pos, DIAG_ERR, "Expression must be a modifiable lvalue");
                c_value_destroy(res.res);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            // Add or subtract one.
            ir_var_t *tmpvar = ir_var_create(code->func, c_type_to_ir_type(ctx, res.res.c_type->data), NULL);
            ir_add_expr2(
                IR_APPEND(code),
                tmpvar,
                is_inc ? IR_OP2_add : IR_OP2_sub,
                c_value_read(ctx, code, scope, &res.res),
                (ir_operand_t){.is_const = true, .iconst = {.prim_type = tmpvar->prim_type, .constl = 1, .consth = 0}}
            );

            // After modifying, write back and return value.
            c_value_t rvalue = {
                .value_type = C_RVALUE,
                .c_type     = rc_share(res.res.c_type),
                .rvalue     = {
                        .is_const = false,
                        .var      = tmpvar,
                },
            };
            c_value_write(ctx, code, scope, &res.res, &rvalue);
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
            } else if (res.res.value_type == C_RVALUE) {
                cctx_diagnostic(ctx->cctx, expr->params[0].pos, DIAG_ERR, "Lvalue required as unary `&` operand");
                c_value_destroy(res.res);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            // Create pointer type.
            c_type_t *ptr_type  = strong_calloc(1, sizeof(c_type_t));
            ptr_type->primitive = C_COMP_POINTER;
            ptr_type->size      = ctx->prim_types[ctx->options.size_type].size;
            ptr_type->align     = ctx->prim_types[ctx->options.size_type].align;
            ptr_type->inner     = rc_share(res.res.c_type);
            rc_t ptr_rc         = rc_new_strong(ptr_type, (void (*)(void *))c_type_free);

            // Get address into an rvalue.
            c_value_t rvalue = {
                .value_type = C_RVALUE,
                .c_type     = ptr_rc,
                .rvalue     = c_value_addrof(ctx, code, &res.res),
            };
            c_value_destroy(res.res);

            return (c_compile_expr_t){
                .code = code,
                .res  = rvalue,
            };

        } else if (expr->params[0].subtype == C_TKN_MUL) {
            // Address of operator.
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
            c_value_t rvalue = {
                .value_type = C_LVALUE_PTR,
                .c_type     = rc_share(type->inner),
                .lvalue.ptr = c_value_read(ctx, code, scope, &res.res),
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
            ir_operand_t ir_value = c_value_read(ctx, code, scope, &res.res);
            if (ir_value.is_const) {
                // Resulting constant can be evaluated at compile-time.
                rc_t type = rc_share(res.res.c_type);
                c_value_destroy(res.res);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = {
                         .value_type = C_RVALUE,
                         .c_type     = type,
                         .rvalue     = {
                                 .is_const = true,
                                 .iconst   = ir_calc1(c_op1_to_ir_op1(expr->params[0].subtype), ir_value.iconst),
                        },
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
                .value_type = C_RVALUE,
                .c_type     = rc_share(res.res.c_type),
                .rvalue     = {
                        .is_const = false,
                        .var      = tmpvar,
                },
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
        code = res.code;

        // Assert that it's actually a writable lvalue.
        if (res.res.value_type == C_RVALUE || ((c_type_t *)res.res.c_type->data)->is_const) {
            cctx_diagnostic(ctx->cctx, expr->params[1].pos, DIAG_ERR, "Expression must be a modifiable lvalue");
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        }

        // Save value for later use.
        ir_prim_t    ir_prim    = c_type_to_ir_type(ctx, res.res.c_type->data);
        ir_operand_t read_value = c_value_read(ctx, code, scope, &res.res);
        ir_var_t    *oldvar     = ir_var_create(code->func, ir_prim, NULL);
        ir_add_expr1(IR_APPEND(code), oldvar, IR_OP1_mov, read_value);

        // Add or subtract one.
        ir_var_t *tmpvar = ir_var_create(code->func, ir_prim, NULL);
        ir_add_expr2(
            IR_APPEND(code),
            tmpvar,
            is_inc ? IR_OP2_add : IR_OP2_sub,
            read_value,
            (ir_operand_t){.is_const = true, .iconst = {.prim_type = tmpvar->prim_type, .constl = 1, .consth = 0}}
        );

        // After modifying, write back.
        c_value_t write_rvalue = {
            .value_type = C_RVALUE,
            .c_type     = res.res.c_type,
            .rvalue     = {
                    .is_const = false,
                    .var      = tmpvar,
            },
        };
        c_value_write(ctx, code, scope, &res.res, &write_rvalue);

        // Return old value.
        c_value_t old_rvalue = {
            .value_type = C_RVALUE,
            .c_type     = rc_share(res.res.c_type),
            .rvalue     = {
                    .is_const = false,
                    .var      = tmpvar,
            },
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
    }
    abort();
}

// Compile a statement node into IR.
// Returns the code path linearly after this.
ir_code_t *
    c_compile_stmt(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *stmt) {
    if (stmt->subtype == C_AST_STMTS) {
        // Multiple statements in scope.
        c_scope_t *new_scope = c_scope_create(scope);

        // Compile statements in this scope.
        for (size_t i = 0; i < stmt->params_len; i++) {
            code = c_compile_stmt(ctx, prepass, code, new_scope, &stmt->params[i]);
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
        // Consistency points are before and after condition eval.
        // For do...while: before -> body, body -> condition, condition -> body/after.
        // For while: before -> condition, body -> condition, condition -> body/after.
        set_t const *affected = map_get(&prepass->branch_vars, stmt);

        ir_code_t *loop_body = ir_code_create(code->func, NULL);
        ir_code_t *cond_body = ir_code_create(code->func, NULL);
        ir_code_t *after     = ir_code_create(code->func, NULL);

        // Before -> body (do...while) or before -> condition (while).
        c_create_branch_consistency(ctx, code, scope, affected);

        // Compile expression.
        c_compile_expr_t expr = c_compile_expr(ctx, prepass, cond_body, scope, &stmt->params[0]);
        // Condition -> body/after.
        c_create_branch_consistency(ctx, expr.code, scope, affected);
        if (expr.res.value_type != C_VALUE_ERROR) {
            ir_add_branch(
                IR_APPEND(expr.code),
                c_cast_ir_operand(expr.code, c_value_read(ctx, expr.code, scope, &expr.res), IR_PRIM_bool),
                loop_body
            );
            c_value_destroy(expr.res);
        }
        ir_add_jump(IR_APPEND(expr.code), after);

        // Compile loop body.
        loop_body = c_compile_stmt(ctx, prepass, loop_body, scope, &stmt->params[1]);
        /// Body -> condition.
        c_create_branch_consistency(ctx, loop_body, scope, affected);
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
        // Consistency points are after condition and after convergence.
        // That means condition -> if/else, if -> after, else -> after.
        set_t const *affected = map_get(&prepass->branch_vars, stmt);

        // Evaluate condition.
        c_compile_expr_t expr = c_compile_expr(ctx, prepass, code, scope, &stmt->params[0]);
        code                  = expr.code;
        // Condition -> if/else.
        c_create_branch_consistency(ctx, code, scope, affected);

        // Allocate code paths.
        ir_code_t *if_body = ir_code_create(code->func, NULL);
        ir_code_t *after   = ir_code_create(code->func, NULL);
        c_compile_stmt(ctx, prepass, if_body, scope, &stmt->params[1]);
        // If -> after.
        c_create_branch_consistency(ctx, if_body, scope, affected);
        ir_add_jump(IR_APPEND(if_body), after);
        if (expr.res.value_type != C_VALUE_ERROR) {
            ir_add_branch(
                IR_APPEND(code),
                c_cast_ir_operand(code, c_value_read(ctx, code, scope, &expr.res), IR_PRIM_bool),
                if_body
            );
        }
        if (stmt->params_len == 3) {
            // If...else statement.
            ir_code_t *else_body = ir_code_create(code->func, NULL);
            c_compile_stmt(ctx, prepass, else_body, scope, &stmt->params[2]);
            // Else -> after.
            c_create_branch_consistency(ctx, else_body, scope, affected);
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
        // Consistency points are before and after condition eval.
        // That means setup -> condition, body -> condition and condition -> body/after.
        set_t const *affected = map_get(&prepass->branch_vars, stmt);

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

        // Setup -> condition consistency.
        c_create_branch_consistency(ctx, code, scope, affected);

        // Compile condition.
        if (stmt->params[1].type == TOKENTYPE_AST && stmt->params[1].subtype == C_AST_NOP) {
            // Condition -> body consistency is not needed because the body will have already made it so.
            for_cond = for_body;
        } else {
            for_cond             = ir_code_create(code->func, NULL);
            c_compile_expr_t res = c_compile_expr(ctx, prepass, for_cond, scope, &stmt->params[1]);
            // Condition -> body consistency.
            c_create_branch_consistency(ctx, res.code, scope, affected);
            if (res.res.value_type != C_VALUE_ERROR) {
                ir_add_branch(
                    IR_APPEND(res.code),
                    c_cast_ir_operand(res.code, c_value_read(ctx, res.code, scope, &res.res), IR_PRIM_bool),
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

        // Body -> condition consistency.
        c_create_branch_consistency(ctx, code, scope, affected);
        ir_add_jump(IR_APPEND(code), for_cond);

        return after;

    } else if (stmt->subtype == C_AST_RETURN) {
        // Return statement.
        if (stmt->params_len) {
            // With value.
            c_compile_expr_t expr = c_compile_expr(ctx, prepass, code, scope, &stmt->params[0]);
            code                  = expr.code;
            if (expr.res.value_type != C_VALUE_ERROR) {
                ir_add_return1(IR_APPEND(code), c_value_read(ctx, code, scope, &expr.res));
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
    rc_t inner_type = c_compile_spec_qual_list(ctx, &def->params[0]);
    if (!inner_type) {
        return NULL;
    }

    token_t const *name;
    rc_t           func_type_rc = c_compile_decl(ctx, &def->params[1], inner_type, &name);
    if (!name) {
        // A diagnostic will already have been created; skip this.
        rc_delete(func_type_rc);
        return NULL;
    }
    c_type_t *func_type = func_type_rc->data;

    // Create function and scope.
    ir_func_t *func  = ir_func_create(name->strval, NULL, func_type->func.args_len, NULL);
    c_scope_t *scope = c_scope_create(&ctx->global_scope);

    // Bring parameters into scope.
    for (size_t i = 0; i < func_type->func.args_len; i++) {
        if (func_type->func.arg_names[i]) {
            c_type_t *c_type         = (c_type_t *)func_type->func.args[i]->data;
            c_var_t  *var            = calloc(1, sizeof(c_var_t));
            var->type                = rc_share(func_type->func.args[i]);
            var->ir_var              = func->args[i].var;
            var->ir_frame            = ir_frame_create(func, c_type->size, c_type->align, NULL);
            var->ir_var->prim_type   = c_type_to_ir_type(ctx, var->type->data);
            var->register_up_to_date = true;
            var->pointer_taken       = set_contains(&prepass->pointer_taken, func_type->func.arg_name_tkns[i]);
            map_set(&scope->locals, func_type->func.arg_names[i], var);
            map_set(&scope->locals_by_decl, func_type->func.arg_name_tkns[i], var);
        }
    }

    // Compile the statements inside this same scope.
    token_t const *body = &def->params[2];
    ir_code_t     *code = (ir_code_t *)func->code_list.head;
    for (size_t i = 0; i < body->params_len; i++) {
        code = c_compile_stmt(ctx, prepass, code, scope, &body->params[i]);
    }

    c_scope_destroy(scope);

    // Add implicit empty return statement.
    ir_add_return0(IR_APPEND(code));

    // Add function into global scope.
    c_var_t *var   = calloc(1, sizeof(c_var_t));
    var->is_global = true;
    var->type      = func_type_rc;
    map_set(&ctx->global_scope.locals, name->strval, var);

    return func;
}

// Compile a declaration statement.
// If in global scope, `code` and `prepass` must be `NULL`, and will return `NULL`.
// If not global, all must not be `NULL` and will return the code path linearly after this.
ir_code_t *
    c_compile_decls(c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *decls) {
    // TODO: Typedef support.
    rc_t inner_type = c_compile_spec_qual_list(ctx, &decls->params[0]);
    if (!inner_type) {
        return code;
    }

    for (size_t i = 1; i < decls->params_len; i++) {
        token_t const *name      = NULL;
        rc_t           decl_type = c_compile_decl(ctx, &decls->params[i], rc_share(inner_type), &name);
        if (!name) {
            // A diagnostic will already have been created; skip this.
            rc_delete(decl_type);
            continue;
        }

        // Create the C variable.
        c_var_t  *var         = calloc(1, sizeof(c_var_t));
        c_type_t *c_decl_type = decl_type->data;
        var->is_global        = scope->depth == 0;
        var->type             = decl_type;
        var->pointer_taken    = var->is_global || set_contains(&prepass->pointer_taken, name);
        var->ir_var           = code ? ir_var_create(code->func, c_type_to_ir_type(ctx, decl_type->data), NULL) : NULL;
        var->ir_frame         = code ? ir_frame_create(code->func, c_decl_type->size, c_decl_type->size, NULL) : NULL;
        var->register_up_to_date = !var->is_global;
        var->memory_up_to_date   = var->is_global;
        map_set(&scope->locals, name->strval, var);
        map_set(&scope->locals_by_decl, name, var);

        // If the declaration has an assignment, compile it too.
        if (decls->params[i].type == TOKENTYPE_AST && decls->params[i].subtype == C_AST_ASSIGN_DECL) {
            if (scope->depth == 0) {
                printf("TODO: Initialized variables in the global scope\n");
                abort();
            }

            c_value_t lvalue = {
                .value_type   = C_LVALUE_VAR,
                .c_type       = var->type,
                .lvalue.c_var = var,
            };
            c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &decls->params[1].params[1]);
            code                 = res.code;
            if (res.res.value_type != C_VALUE_ERROR) {
                c_value_write(ctx, code, scope, &lvalue, &res.res);
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
