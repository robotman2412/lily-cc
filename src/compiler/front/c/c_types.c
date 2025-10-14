
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_types.h"

#include "c_compiler.h"
#include "c_parser.h"
#include "ir_interpreter.h"
#include "strong_malloc.h"



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
    } else if (type->primitive == C_COMP_ARRAY) { // NOLINT
        // TODO: An array size could be here.
        rc_delete(type->inner);
    } else if (type->primitive == C_COMP_POINTER) {
        rc_delete(type->inner);
    } else if (type->primitive == C_COMP_ENUM || type->primitive == C_COMP_STRUCT || type->primitive == C_COMP_UNION) {
        rc_delete(type->comp);
    }
    free(type);
}

// Delete a compound type.
static void c_comp_free(c_comp_t *comp) {
    if (comp->type == C_COMP_TYPE_ENUM) {
        map_foreach(ent, &comp->variants) {
            c_enumvar_t *value = ent->value;
            free(value->name);
            free(value);
        }
        map_clear(&comp->variants);
    } else {
        map_foreach(ent, &comp->fields) {
            c_field_t *value = ent->value;
            free(value->name);
            rc_delete(value->type_rc);
            free(value);
        }
        map_clear(&comp->fields);
    }
    free(comp->name);
    free(comp);
}



// Compile the body of an enum definition.
static void
    c_compile_enum_body(c_compiler_t *ctx, token_t const *body, size_t body_len, c_scope_t *scope, c_comp_t *comp) {
    // TODO: Packed enums support.
    int cur = 0;
    for (size_t i = 0; i < body_len; i++, cur++) {
        token_t const *name = &body[i].params[0];
        if (body[i].params_len == 2) {
            // Enum variant with specific index.
            c_compile_expr_t res = c_compile_expr(ctx, NULL, NULL, scope, &body[i].params[1]);
            if (res.res.value_type == C_RVALUE) {
                cur = (int)ir_cast(c_prim_to_ir_type(ctx, C_PRIM_SINT), res.res.rvalue.iconst).constl;
            }
        }
        if (map_get(&scope->locals, name->strval)) {
            cctx_diagnostic(ctx->cctx, name->pos, DIAG_ERR, "Redefinition of %s", name->strval);
            continue;
        } else {
            c_var_t *var      = strong_calloc(1, sizeof(c_var_t));
            var->storage      = C_VAR_STORAGE_ENUM_VARIANT;
            var->type         = rc_share(&ctx->prim_rcs[C_PRIM_SINT]);
            var->enum_variant = cur;
            map_set(&scope->locals, name->strval, var);
            map_set(&scope->locals_by_decl, name, var);

            c_enumvar_t *enumvar = strong_calloc(1, sizeof(c_enumvar_t));
            enumvar->name        = strong_strdup(name->strval);
            enumvar->ordinal     = cur;
            map_set(&comp->variants, enumvar->name, enumvar);
        }
    }

    comp->align = comp->size = ctx->options.int32 ? 4 : 2;
}

// Compile the body of a struct/union definition.
static void
    c_compile_struct_body(c_compiler_t *ctx, token_t const *body, size_t body_len, c_scope_t *scope, c_comp_t *comp) {
    (void)ctx;
    (void)body;
    (void)body_len;
    (void)scope;
    (void)comp;
    // TODO: Nested struct/union definitions.
    fprintf(stderr, "[TODO] c_compile_struct_body\n");
    abort();
}

// Compile a C enum/struct/union specification.
// Returns a refcount pointer of `c_comp_t`.
static rc_t c_compile_comp_spec(c_compiler_t *ctx, token_t const *struct_spec, c_scope_t *scope) {
    token_t const *name;
    token_t const *body;
    size_t         body_len;
    if (struct_spec->subtype == C_AST_NAMED_STRUCT) {
        name     = &struct_spec->params[1];
        body     = &struct_spec->params[2];
        body_len = struct_spec->params_len - 2;
    } else {
        name     = NULL;
        body     = &struct_spec->params[1];
        body_len = struct_spec->params_len - 1;
    }

    // What tag type this specifier has.
    c_comp_type_t comp_type;
    switch (struct_spec->params[0].subtype) {
        case C_KEYW_enum: comp_type = C_COMP_TYPE_ENUM; break;
        case C_KEYW_struct: comp_type = C_COMP_TYPE_STRUCT; break;
        case C_KEYW_union: comp_type = C_COMP_TYPE_UNION; break;
        default: __builtin_unreachable();
    }

    // Get or create the compound type.
    rc_t      comp_rc = NULL;
    c_comp_t *comp;
    if (name) {
        comp_rc = c_scope_lookup_comp(scope, name->strval);
    }
    if (!comp_rc) {
        comp         = strong_calloc(1, sizeof(c_comp_t));
        comp_rc      = rc_new_strong(comp, (void (*)(void *))c_comp_free);
        comp->name   = name ? strong_strdup(name->strval) : NULL;
        comp->type   = comp_type;
        comp->fields = STR_MAP_EMPTY;
        if (name) {
            map_set(&scope->comp_types, name->strval, rc_share(comp_rc));
        }
    } else {
        comp = comp_rc->data;
    }

    // Assert that the tag type matches.
    if (comp->type != comp_type) {
        static char const *const names[] = {
            [C_COMP_TYPE_ENUM]   = "an enum",
            [C_COMP_TYPE_STRUCT] = "a struct",
            [C_COMP_TYPE_UNION]  = "a union",
        };
        cctx_diagnostic(
            ctx->cctx,
            name->pos, // Non-NULL because it's impossible to get this error with anonymous structs
            DIAG_ERR,
            "Use of %s (which is %s) as %s",
            name->strval,
            names[comp->type],
            names[comp_type]
        );
        rc_delete(comp_rc);
        return NULL;
    }

    // Finally compile the body with the appropriate type.
    if (comp_type == C_COMP_TYPE_ENUM) {
        c_compile_enum_body(ctx, body, body_len, scope, comp);
    } else {
        c_compile_struct_body(ctx, body, body_len, scope, comp);
    }

    return comp_rc;
}

// Create a C type from a specifier-qualifer list.
// Returns a refcount pointer of `c_type_t`.
rc_t c_compile_spec_qual_list(c_compiler_t *ctx, token_t const *list, c_scope_t *scope) {
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

    rc_t      type_rc = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
    c_type_t *type    = type_rc->data;

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
                case C_KEYW__Bool:
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
        rc_t comp_rc = c_compile_comp_spec(ctx, struct_tkn, scope);
        if (!comp_rc) {
            rc_delete(type_rc);
            return NULL;
        }
        c_comp_t *comp = comp_rc->data;
        type->comp     = comp_rc;
        switch (comp->type) {
            case C_COMP_TYPE_ENUM: type->primitive = C_COMP_ENUM; break;
            case C_COMP_TYPE_STRUCT: type->primitive = C_COMP_STRUCT; break;
            case C_COMP_TYPE_UNION: type->primitive = C_COMP_UNION; break;
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
        cctx_diagnostic(ctx->cctx, list->pos, DIAG_ERR, "Invalid combination of type specifiers");
    }

    return type_rc;
}

// Create a C type and get the name from an (abstract) declarator.
// Takes ownership of the `spec_qual_type` share passed.
rc_t c_compile_decl(
    c_compiler_t *ctx, token_t const *decl, c_scope_t *scope, rc_t spec_qual_type, token_t const **name_out
) {
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
                    func_type->func.args[i] = c_compile_spec_qual_list(ctx, param, scope);
                } else {
                    token_t const *name_tmp;
                    rc_t           list              = c_compile_spec_qual_list(ctx, &param->params[0], scope);
                    func_type->func.args[i]          = c_compile_decl(ctx, &param->params[1], scope, list, &name_tmp);
                    func_type->func.arg_names[i]     = strong_strdup(name_tmp->strval); // NOLINT.
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
                return next;
            }

            cur = next;

        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_TYPE_PTR_TO) {
            rc_t      next       = rc_new_strong(strong_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *next_type  = next->data;
            next_type->primitive = C_COMP_POINTER;
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
        } else if (decl->type == TOKENTYPE_AST && decl->subtype == C_AST_GARBAGE) {
            rc_delete(cur);
            return NULL;
        } else {
            abort();
        }
    }
}


// Create a type that is a pointer to an existing type.
rc_t c_type_pointer(c_compiler_t *ctx, rc_t inner) {
    (void)ctx;
    c_type_t *data  = strong_calloc(1, sizeof(c_type_t));
    data->primitive = C_COMP_POINTER;
    data->inner     = inner;
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

// Get the alignment and size of a C type.
bool c_type_get_size(c_compiler_t *ctx, c_type_t const *type, uint64_t *size_out, uint64_t *align_out) {
    c_prim_t prim = type->primitive;
    if (prim == C_COMP_POINTER) {
        prim = ctx->options.size_type;
    }
    switch (prim) {
        case C_PRIM_BOOL:
        case C_PRIM_UCHAR:
        case C_PRIM_SCHAR: *align_out = *size_out = 1; break;
        case C_PRIM_USHORT:
        case C_PRIM_SSHORT: *align_out = *size_out = ctx->options.short16 ? 2 : 1; break;
        case C_PRIM_UINT:
        case C_PRIM_SINT: *align_out = *size_out = ctx->options.int32 ? 4 : 2; break;
        case C_PRIM_ULONG:
        case C_PRIM_SLONG: *align_out = *size_out = ctx->options.long64 ? 8 : 4; break;
        case C_PRIM_ULLONG:
        case C_PRIM_SLLONG: *align_out = *size_out = 8; break;
        case C_PRIM_FLOAT: *align_out = *size_out = 4; break;
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE: *align_out = *size_out = 8; break;
        case C_PRIM_VOID: *align_out = 1, *size_out = 0; break;
        case C_COMP_STRUCT:
        case C_COMP_ENUM:
        case C_COMP_UNION: {
            c_comp_t *comp = type->comp->data;
            if (comp->align == 0) {
                return false;
            }
            *size_out  = comp->size;
            *align_out = comp->align;
        } break;
        case C_COMP_POINTER: __builtin_unreachable();
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: return false;
    }
    return true;
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
        case C_PRIM_DOUBLE:
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
    if (operand.type == IR_OPERAND_TYPE_CONST) {
        // Casting constants.
        if (operand.iconst.prim_type == type) {
            return operand;
        }
        return IR_OPERAND_CONST(ir_cast(type, operand.iconst));
    } else {
        // Casting variables or expressions.
        ir_op1_type_t op1 = type == IR_PRIM_bool ? IR_OP1_snez : IR_OP1_mov;
        if (operand.var->prim_type == type) {
            return operand;
        }
        ir_var_t *dest = ir_var_create(code->func, type, NULL);
        ir_add_expr1(IR_APPEND(code), dest, op1, operand);
        return IR_OPERAND_VAR(dest);
    }
}
