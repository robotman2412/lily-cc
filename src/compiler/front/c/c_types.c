
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_types.h"

#include "c_compiler.h"
#include "c_parser.h"
#include "compiler.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "map.h"
#include "refcount.h"
#include "strong_malloc.h"
#include "unreachable.h"

#include <stdio.h>



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
            if (res.res.value_type == C_RVALUE_OPERAND) {
                cur = (int)ir_cast(c_prim_to_ir_type(ctx, C_PRIM_SINT), res.res.rvalue.operand.iconst).constl;
            } else if (c_is_rvalue(&res.res)) {
                cctx_diagnostic(ctx->cctx, body->params[1].pos, DIAG_ERR, "Expected scalar type");
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
    uint64_t offset = 0;
    uint64_t size   = 0;
    uint64_t align  = 1;
    bool     errors = false;

    for (size_t i = 0; i < body_len; i++) {
        token_t const *decl     = &body[i];
        rc_t           inner_rc = c_compile_spec_qual_list(ctx, &decl->params[0], scope);
        if (!inner_rc) {
            errors = true;
            continue;
        }

        c_type_t const *inner_type = inner_rc->data;
        // TODO: This check does not exclude `struct a` in `struct { struct a; }`.
        if (decl->params_len == 1
            && (inner_type->primitive == C_COMP_STRUCT || inner_type->primitive == C_COMP_UNION)) {
            c_comp_t const *inner_comp = inner_type->comp->data;

            // Pad to alignment of inner type.
            if (offset % inner_comp->align) {
                offset += inner_comp->align - offset % inner_comp->align;
            }

            // Copy fields in with current offset added.
            map_foreach(ent, &inner_comp->fields) {
                c_field_t const *field    = ent->value;
                c_field_t const *existing = map_get(&comp->fields, ent->key);
                if (existing) {
                    cctx_diagnostic(ctx->cctx, field->name_tkn->pos, DIAG_ERR, "Redefinition of %s", ent->key);
                    errors = true;
                    continue;
                }

                c_field_t *copy = strong_calloc(1, sizeof(c_field_t));
                copy->name_tkn  = field->name_tkn;
                copy->offset    = field->offset + offset;
                copy->type_rc   = rc_share(field->type_rc);
                map_set(&comp->fields, ent->key, copy);
            }

            // Adjust size and alignment accordingly.
            if (align < inner_comp->align) {
                align = inner_comp->align;
            }
            if (comp->type == C_COMP_TYPE_STRUCT) {
                offset += inner_comp->size;
                size    = offset;
            } else if (size < inner_comp->size) {
                size = inner_comp->size;
            }

        } else {
            // Normal field.
            for (size_t x = 1; x < decl->params_len; x++) {
                token_t const *name_tkn;
                rc_t           field_type = c_compile_decl(ctx, &decl->params[x], scope, rc_share(inner_rc), &name_tkn);
                if (!field_type) {
                    errors = true;
                    continue;
                }
                size_t field_size, field_align;
                if (c_type_get_size(ctx, field_type->data, &field_size, &field_align)) {
                    if (offset % field_align) {
                        offset += field_align - offset % field_align;
                    }
                    c_field_t *field = strong_malloc(sizeof(c_field_t));
                    field->type_rc   = field_type;
                    field->name_tkn  = name_tkn;
                    field->offset    = offset;
                    map_set(&comp->fields, name_tkn->strval, field);
                } else {
                    cctx_diagnostic(ctx->cctx, name_tkn->pos, DIAG_ERR, "Use of incomplete type");
                    rc_delete(field_type);
                    errors = true;
                }

                if (align < field_align) {
                    align = field_align;
                }
                if (comp->type == C_COMP_TYPE_STRUCT) {
                    offset += field_size;
                    size    = offset;
                } else if (size < field_size) {
                    size = field_size;
                }
            }
        }

        rc_delete(inner_rc);
    }

    if (size % align) {
        size += align - size % align;
    }
    if (!errors) {
        comp->size  = size;
        comp->align = align;
    }
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
        default: UNREACHABLE();
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
    if (body_len) {
        if (comp_type == C_COMP_TYPE_ENUM) {
            c_compile_enum_body(ctx, body, body_len, scope, comp);
        } else {
            c_compile_struct_body(ctx, body, body_len, scope, comp);
        }
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
        if (has_unsigned) {
            type->primitive = C_PRIM_UCHAR;
        } else if (has_signed) {
            type->primitive = C_PRIM_SCHAR;
        } else {
            type->primitive = C_PRIM_CHAR;
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
c_prim_t c_prim_promote(c_prim_t a, c_prim_t b) {
    c_prim_t tmp;
    if (a >= C_N_PRIM || b >= C_N_PRIM) {
        UNREACHABLE();
    } else if (a > b) {
        tmp = a;
    } else {
        tmp = b;
    }

    if (tmp < C_PRIM_SINT) {
        // Promote smaller types to `int`.
        return C_PRIM_SINT;
    } else {
        // Otherwise, merely use the wider of the two types.
        return tmp;
    }
}

// Determine whether a value of type `old_type` can be cast to `new_type`.
bool c_type_castable(c_compiler_t *ctx, c_type_t const *new_type, c_type_t const *old_type) {
    if (c_type_identical(ctx, new_type, old_type, false)) {
        return true;
    }
    if (new_type->primitive == C_PRIM_VOID) {
        return true;
    }
    return old_type->primitive < C_N_PRIM && new_type->primitive < C_N_PRIM && old_type->primitive != C_PRIM_VOID
           && new_type->primitive != C_PRIM_VOID;
}

// Determine whether two types are the same.
// If `strict`, then modifiers like `_Atomic` and `volatile` also apply.
bool c_type_identical(c_compiler_t *ctx, c_type_t const *a, c_type_t const *b, bool strict) {
    if (a == b) {
        return true;
    }
    if (a->primitive != b->primitive) {
        return false;
    }
    if (strict) {
        if (a->is_restrict != b->is_restrict || a->is_atomic != b->is_atomic || a->is_const != b->is_const
            || a->is_volatile != b->is_volatile) {
            return false;
        }
    }
    switch (a->primitive) {
        case C_PRIM_BOOL:
        case C_PRIM_CHAR:
        case C_PRIM_UCHAR:
        case C_PRIM_SCHAR:
        case C_PRIM_USHORT:
        case C_PRIM_SSHORT:
        case C_PRIM_UINT:
        case C_PRIM_SINT:
        case C_PRIM_ULONG:
        case C_PRIM_SLONG:
        case C_PRIM_ULLONG:
        case C_PRIM_SLLONG:
        case C_PRIM_FLOAT:
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE:
        case C_PRIM_VOID: return true;
        case C_COMP_STRUCT:
        case C_COMP_UNION: return a->comp == b->comp;
        case C_COMP_ENUM:
        case C_COMP_POINTER: return true;
        case C_COMP_ARRAY: return c_type_compatible(ctx, a->inner->data, b->inner->data);
        case C_COMP_FUNCTION: return false;
    }
    UNREACHABLE();
}

// Determine whether two types are compatible.
bool c_type_compatible(c_compiler_t *ctx, c_type_t const *a, c_type_t const *b) {
    if (a == b) {
        return true;
    }
    if (a->primitive != b->primitive) {
        return (a->primitive < C_N_PRIM || a->primitive == C_COMP_ENUM)
               && (b->primitive < C_N_PRIM || b->primitive == C_COMP_ENUM);
    }
    switch (a->primitive) {
        case C_PRIM_BOOL:
        case C_PRIM_CHAR:
        case C_PRIM_UCHAR:
        case C_PRIM_SCHAR:
        case C_PRIM_USHORT:
        case C_PRIM_SSHORT:
        case C_PRIM_UINT:
        case C_PRIM_SINT:
        case C_PRIM_ULONG:
        case C_PRIM_SLONG:
        case C_PRIM_ULLONG:
        case C_PRIM_SLLONG:
        case C_PRIM_FLOAT:
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE:
        case C_PRIM_VOID: return true;
        case C_COMP_STRUCT:
        case C_COMP_UNION: return a->comp == b->comp;
        case C_COMP_ENUM:
        case C_COMP_POINTER: return true;
        case C_COMP_ARRAY: return c_type_compatible(ctx, a->inner->data, b->inner->data);
        case C_COMP_FUNCTION: return false;
    }
    UNREACHABLE();
}

// Helper for `c_type_arith_compatible` that determines pointer arithmetic compatibility.
static bool c_type_ptrarith_compatible(
    c_compiler_t   *ctx,
    c_type_t const *ptr,
    c_type_t const *other,
    c_tokentype_t   oper_tkn,
    pos_t           diag_pos,
    bool            is_swapped
) {
    switch (oper_tkn) {
        case C_TKN_LAND:
        case C_TKN_LOR: return true;

        case C_TKN_ADD:
            if (!c_prim_is_int(other->primitive)) {
                // Pointer addition requires a pointer and an integer.
                cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Invalid operands to %s", c_tokens[oper_tkn]);
                return false;
            }
            return true;

        case C_TKN_ADD_S:
            if (is_swapped || ptr->primitive != C_COMP_POINTER || !c_prim_is_int(other->primitive)) {
                // Pointer addition requires a pointer and an integer.
                cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Invalid operands to %s", c_tokens[oper_tkn]);
                return false;
            }
            return true;

        case C_TKN_SUB:
            if (c_prim_is_ptr(other->primitive)) {
                // Pointers must be to compatible types.
                c_type_t const *ptr_inner   = ptr->inner->data;
                c_type_t const *other_inner = other->inner->data;
                if (ptr_inner->primitive != C_PRIM_VOID && other_inner->primitive != C_PRIM_VOID
                    && !c_type_compatible(ctx, ptr_inner, other_inner)) {
                    cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Difference of pointers to incompatible types");
                    return false;
                }
                return true;
            }
            if (!c_prim_is_int(other->primitive)) {
                // Pointer subtraction requires a pointer and an integer or another pointer.
                cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Invalid operands to %s", c_tokens[oper_tkn]);
                return false;
            }
            return true;

        case C_TKN_SUB_S:
            if (is_swapped || ptr->primitive != C_COMP_POINTER || !c_prim_is_int(other->primitive)) {
                // Pointer subtraction requires a pointer and an integer or another pointer.
                cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Invalid operands to %s", c_tokens[oper_tkn]);
                return false;
            }
            return true;

        case C_TKN_MUL:
        case C_TKN_MUL_S:
        case C_TKN_DIV:
        case C_TKN_DIV_S:
        case C_TKN_MOD:
        case C_TKN_MOD_S:
        case C_TKN_SHL:
        case C_TKN_SHL_S:
        case C_TKN_SHR:
        case C_TKN_SHR_S:
        case C_TKN_AND:
        case C_TKN_AND_S:
        case C_TKN_OR:
        case C_TKN_OR_S:
        case C_TKN_XOR:
        case C_TKN_XOR_S:
            // Cannot do any of these operations on a pointer.
            cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Invalid operands to %s", c_tokens[oper_tkn]);
            return false;

        case C_TKN_EQ:
        case C_TKN_NE:
        case C_TKN_LT:
        case C_TKN_LE:
        case C_TKN_GT:
        case C_TKN_GE:
            if (c_prim_is_ptr(other->primitive)) {
                // Warn if incompatible pointers.
                c_type_t const *ptr_inner   = ptr->inner->data;
                c_type_t const *other_inner = other->inner->data;
                if (ptr_inner->primitive != C_PRIM_VOID && other_inner->primitive != C_PRIM_VOID
                    && !c_type_compatible(ctx, ptr_inner, other_inner)) {
                    cctx_diagnostic(ctx->cctx, diag_pos, DIAG_WARN, "Comparison of pointers to incompatible types");
                }
            } else if (!c_prim_is_int(other->primitive)) {
                // Comparison can only be integers and pointers.
                cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Invalid operands to %s", c_tokens[oper_tkn]);
                return false;
            } else {
                // Warn of integer-pointer comparison.
                cctx_diagnostic(ctx->cctx, diag_pos, DIAG_WARN, "Comparison of pointer and integral types");
            }
            return true;

        default: UNREACHABLE();
    }
}

// Determine whether two types can be used with a certain operator token.
// Produces a diagnostic if they cannot.
bool c_type_arith_compatible(
    c_compiler_t *ctx, c_type_t const *a, c_type_t const *b, c_tokentype_t oper_tkn, pos_t diag_pos
) {
    // There are additional checks and restrictions to pointer arithmetic.
    if (c_prim_is_ptr(a->primitive)) {
        return c_type_ptrarith_compatible(ctx, a, b, oper_tkn, diag_pos, false);
    } else if (c_prim_is_ptr(b->primitive)) {
        return c_type_ptrarith_compatible(ctx, b, a, oper_tkn, diag_pos, true);
    }

    // Otherwise, they are arithmetic compatible if both types are primitives.
    if (a->primitive >= C_N_PRIM || b->primitive >= C_N_PRIM) {
        cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Invalid operands to %s", c_tokens[oper_tkn]);
        return false;
    }

    return true;
}

// Get the alignment and size of a C type.
bool c_type_get_size(c_compiler_t *ctx, c_type_t const *type, uint64_t *size_out, uint64_t *align_out) {
    c_prim_t prim = type->primitive;
    if (prim == C_COMP_POINTER) {
        prim = ctx->options.size_type;
    }
    switch (prim) {
        case C_PRIM_BOOL:
        case C_PRIM_CHAR:
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
        case C_COMP_POINTER: UNREACHABLE();
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: return false;
    }
    return true;
}

// Convert C primitive or pointer type to IR primitive type.
ir_prim_t c_prim_to_ir_type(c_compiler_t *ctx, c_prim_t prim) {
    switch (prim) {
        case C_PRIM_BOOL: return IR_PRIM_bool;
        case C_PRIM_CHAR: return ctx->options.char_is_signed ? IR_PRIM_s8 : IR_PRIM_u8;
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
        case C_PRIM_VOID:
        case C_COMP_STRUCT:
        case C_COMP_UNION:
        case C_COMP_ENUM:
        case C_COMP_POINTER:
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: return IR_N_PRIM;
    }
    UNREACHABLE();
}

// Convert C primitive or pointer type to IR primitive type.
ir_prim_t c_type_to_ir_type(c_compiler_t *ctx, c_type_t const *type) {
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
