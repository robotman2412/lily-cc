
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"

#include "c_parser.h"
#include "c_prepass.h"
#include "c_tokenizer.h"
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



// Clean up an lvalue or rvalue.
void c_value_destroy(c_value_t value) {
    rc_delete(value.c_type);
    if (value.value_type == C_LVALUE_MEM && value.lvalue.memref.base_type == IR_MEMBASE_SYM) {
        free(value.lvalue.memref.base_sym);
    }
}

// Write to an lvalue.
void c_value_write(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue, c_value_t const *rvalue) {
    ir_operand_t tmp = c_value_read(ctx, code, rvalue);
    switch (lvalue->value_type) {
        case C_VALUE_ERROR:
            printf("[BUG] c_value_write called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE:
            printf("[BUG] c_value_write called with an `rvalue` destination");
            abort();
            break;
        case C_LVALUE_MEM:
            // Store to the pointer.
            ir_add_store(IR_APPEND(code), tmp, lvalue->lvalue.memref);
            break;
        case C_LVALUE_VAR:
            // Store to the IR variable.
            ir_add_expr1(IR_APPEND(code), lvalue->lvalue.ir_var, IR_OP1_mov, tmp);
            break;
    }
}

// Get the memory location of an lvalue.
ir_memref_t c_value_memref(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue) {
    (void)ctx;
    (void)code;

    if (lvalue->value_type == C_VALUE_ERROR) {
        printf("[BUG] c_value_addrof called on C_VALUE_ERROR\n");
        abort();
    } else if (lvalue->value_type == C_RVALUE) {
        printf("[BUG] c_value_addrof called on C_RVALUE\n");
        abort();
    }

    ir_memref_t memref;
    if (lvalue->value_type == C_LVALUE_MEM) {
        // Directly use the pointer from the lvalue.
        memref = lvalue->lvalue.memref;

    } else if (lvalue->value_type == C_LVALUE_VAR) {
        // Take pointer of C variable.
        printf("[BUG] Address taken of C variable but the variable was not marked as such\n");
        abort();

    } else {
        __builtin_unreachable();
    }

    return memref;
}

// Read a value for scalar arithmetic.
ir_operand_t c_value_read(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value) {
    switch (value->value_type) {
        default: abort(); break;
        case C_VALUE_ERROR:
            printf("[BUG] c_value_read called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE:
            // Rvalues don't need any reading because they're already in an `ir_operand_t`.
            return value->rvalue;
        case C_LVALUE_MEM: {
            // Pointer lvalue is read from memory.
            ir_var_t *tmp = ir_var_create(code->func, c_type_to_ir_type(ctx, value->c_type->data), NULL);
            ir_add_load(IR_APPEND(code), tmp, c_value_memref(ctx, code, value));
            return IR_OPERAND_VAR(tmp);
        }
        case C_LVALUE_VAR:
            // C variables with no pointer taken are stored in IR variables.
            return IR_OPERAND_VAR(value->lvalue.ir_var);
    }
}

// Create a C variable in the current translation unit.
// If in global scope, `code` and `prepass` must be `NULL`.
c_var_t *c_var_create(
    c_compiler_t *ctx, c_prepass_t *prepass, ir_func_t *func, rc_t type_rc, token_t const *name_tkn, c_scope_t *scope
) {
    c_type_t *type = type_rc->data;

    // Enforce that it is a complete type.
    size_t size, align;
    if (!c_type_get_size(ctx, type, &size, &align)) {
        c_comp_t   *comp = type->comp->data;
        char const *comp_var;
        switch (comp->type) {
            case C_COMP_TYPE_ENUM: comp_var = "enum"; break;
            case C_COMP_TYPE_STRUCT: comp_var = "struct"; break;
            case C_COMP_TYPE_UNION: comp_var = "union"; break;
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
        if (set_contains(&prepass->pointer_taken, name_tkn)) {
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
                     .value_type = C_RVALUE,
                     .c_type     = rc_share(&ctx->prim_rcs[C_PRIM_SINT]),
                     .rvalue     = IR_OPERAND_CONST(iconst),
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
                 .value_type = C_RVALUE,
                 .c_type     = rc_share(&ctx->prim_rcs[expr->subtype]),
                 .rvalue     = {
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
            c_value_write(ctx, code, &lvalue, &rvalue);
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
                return (c_compile_expr_t){
                    .code = code,
                    .res  = {
                         .value_type = C_RVALUE,
                         .c_type     = type,
                         .rvalue     = IR_OPERAND_CONST(ir_calc2(ir_op2, ir_lhs.iconst, ir_rhs.iconst)),
                    },
                };
            }

            // Add math instruction.
            ir_var_t *tmpvar = ir_var_create(code->func, c_type_to_ir_type(ctx, res_type->data), NULL);
            ir_add_expr2(IR_APPEND(code), tmpvar, ir_op2, ir_lhs, ir_rhs);

            c_value_t rvalue = {
                .value_type = C_RVALUE,
                .c_type     = res_type,
                .rvalue     = IR_OPERAND_VAR(tmpvar),
            };

            if (expr->params[0].subtype >= C_TKN_ADD_S && expr->params[0].subtype <= C_TKN_XOR_S) {
                // Assignment arithmetic expression; write back.
                if (lhs.value_type == C_RVALUE || ((c_type_t *)lhs.c_type->data)->is_const) {
                    cctx_diagnostic(ctx->cctx, expr->params[1].pos, DIAG_ERR, "Expression must be a modifiable lvalue");
                } else {
                    c_value_write(ctx, code, &lhs, &rvalue);
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
                c_value_read(ctx, code, &res.res),
                (ir_operand_t){.type   = IR_OPERAND_TYPE_CONST,
                               .iconst = {.prim_type = tmpvar->prim_type, .constl = 1, .consth = 0}}
            );

            // After modifying, write back and return value.
            c_value_t rvalue
                = {.value_type = C_RVALUE, .c_type = rc_share(res.res.c_type), .rvalue = IR_OPERAND_VAR(tmpvar)};
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
            ptr_type->inner     = rc_share(res.res.c_type);
            rc_t ptr_rc         = rc_new_strong(ptr_type, (void (*)(void *))c_type_free);

            // Get address into an rvalue.
            ir_var_t *ir_ptr = ir_var_create(code->func, c_prim_to_ir_type(ctx, ctx->options.size_type), NULL);
            ir_add_lea(IR_APPEND(code), ir_ptr, c_value_memref(ctx, code, &res.res));
            c_value_t rvalue = {
                .value_type = C_RVALUE,
                .c_type     = ptr_rc,
                .rvalue     = IR_OPERAND_VAR(ir_ptr),
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
            c_value_t rvalue = {
                .value_type    = C_LVALUE_MEM,
                .c_type        = rc_share(type->inner),
                .lvalue.memref = c_value_memref(ctx, code, &res.res),
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
                         .value_type = C_RVALUE,
                         .c_type     = type,
                         .rvalue = IR_OPERAND_CONST(ir_calc1(c_op1_to_ir_op1(expr->params[0].subtype), ir_value.iconst)),
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
                .rvalue     = IR_OPERAND_VAR(tmpvar),
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
        ir_operand_t read_value = c_value_read(ctx, code, &res.res);
        ir_var_t    *oldvar     = ir_var_create(code->func, ir_prim, NULL);
        ir_add_expr1(IR_APPEND(code), oldvar, IR_OP1_mov, read_value);

        // Add or subtract one.
        ir_var_t *tmpvar = ir_var_create(code->func, ir_prim, NULL);
        ir_add_expr2(
            IR_APPEND(code),
            tmpvar,
            is_inc ? IR_OP2_add : IR_OP2_sub,
            read_value,
            (ir_operand_t){.type   = IR_OPERAND_TYPE_CONST,
                           .iconst = {.prim_type = tmpvar->prim_type, .constl = 1, .consth = 0}}
        );

        // After modifying, write back.
        c_value_t write_rvalue = {
            .value_type = C_RVALUE,
            .c_type     = res.res.c_type,
            .rvalue     = IR_OPERAND_VAR(tmpvar),
        };
        c_value_write(ctx, code, &res.res, &write_rvalue);

        // Return old value.
        c_value_t old_rvalue = {
            .value_type = C_RVALUE,
            .c_type     = rc_share(res.res.c_type),
            .rvalue     = IR_OPERAND_VAR(tmpvar),
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
                func->args[i].has_var = true;
                func->args[i].var     = var->ir_var;
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
