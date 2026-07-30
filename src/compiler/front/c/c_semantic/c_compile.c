
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compile.h"

#include "arith128.h"
#include "c_ast.h"
#include "c_ir.h"
#include "c_prim.h"
#include "c_tokenizer.h"
#include "c_types.h"
#include "c_types1.h"
#include "compiler.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "lilycc_malloc.h"
#include "refcount.h"
#include "unreachable.h"
#include "vec.h"

#include <assert.h>
#include <stdio.h>



// Compile one entire C translation unit into C IR.
// Returns `NULL` if a a semantic error occurred (`-Werror` excluded).
cir_trans_unit_t *c_compile2(c_compiler_t *cc, c_ast_def_list_t const *ast) {
    fprintf(stderr, "TODO: c_compile2\n");
    abort();
}


// Compile a static assertion unit.
cir_unit_t *c_compile2_static_assert(c_compiler_t *cc, cir_scope_t *scope, c_ast_def_static_assert_t *s_assert) {
    fprintf(stderr, "TODO: c_compile2_static_assert\n");
    abort();
}

// Compile a declaration unit.
// Produces a function definition or variable definition depending on the type encoded.
cir_unit_t *c_compile2_decl(c_compiler_t *cc, cir_scope_t *scope, rc_t spec_qual_type, c_ast_decl_t const *decl) {
    fprintf(stderr, "TODO: c_compile2_decl\n");
    abort();
}

// Compile a function definition unit.
cir_func_t *c_compile2_func(c_compiler_t *cc, cir_scope_t *scope, c_ast_def_func_t const *def) {
    fprintf(stderr, "TODO: c_compile2_func\n");
    abort();
}



// Compile the body of an enum definition.
static void
    c_compile2_enum_body(c_compiler_t *ctx, c_ast_enum_spec_t const *spec, cir_scope_t *scope, c_enum_type_t *comp) {
    vec_c_ast_enumvar_t const *body    = &spec->definition->items;
    ir_prim_t const            ir_prim = ctx->options.int32 ? IR_PRIM_s32 : IR_PRIM_s16;

    // TODO: Packed enums support.
    comp->prim = C_PRIM_SINT;

    int32_t cur = 0;
    for (size_t i = 0; i < body->len; i++, cur++) {
        c_ast_ident_t const *name  = body->arr[i]->name;
        c_ast_expr_t const  *value = body->arr[i]->value;
        if (value) {
            fprintf(stderr, "TODO: Designated enum variants\n");
            abort();
        }
        if (map_get(&scope->values, name->name)) {
            cctx_diagnostic(ctx->cctx, name->pos, DIAG_ERR, "Redefinition of %s", name->name);
            continue;
        } else {
            ir_const_t   ir_const = ir_cast(ir_prim, IR_CONST_S32(cur));
            cir_const_t *iconst   = cir_const_create(name->pos, C_PRIM_SINT, ir_const);
            cir_scope_add_enum_const(scope, name->name, iconst);

            c_enumvar_t enumvar;
            enumvar.name    = lilycc_strdup(name->name);
            enumvar.ordinal = i128(cur);
            vec_push(&comp->variants, enumvar);
        }
    }
}

// Field adding helper for `c_compile2_struct_body`.
// Returns true on error.
static bool c_compile2_struct_field(
    c_compiler_t *cc, c_struct_type_t *comp, pos_t pos, char const *name, c_type_t type, uint64_t target_size_max
) {
    uint64_t size, align;
    if (!c_type_get_size(cc, type, &size, &align)) {
        cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Use of incomplete type");
        c_type_delete(type);
        return true;
    }

    c_struct_field_t field;
    field.name     = lilycc_strdup(name);
    field.name_pos = pos;
    field.type     = type;
    if (comp->tag == C_COMP_TYPE_UNION) {
        field.offset = 0;
        if (comp->size < size) {
            comp->size = size;
        }
        if (comp->align < align) {
            comp->align = align;
        }
    } else {
        if (comp->size % comp->align) {
            comp->size += comp->align - comp->size % comp->align;
        }
        field.offset = comp->size;
        if (comp->align < align) {
            comp->align = align;
        }
        if (comp->size > target_size_max - size) {
            cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Type size would exceed SIZE_MAX due to this field");
            c_type_delete(type);
            lilycc_free(field.name);
            return true;
        }
        comp->size += size;
    }

    vec_push(&comp->fields, field);

    return false;
}

// Compile the body of a struct/union definition.
static void c_compile2_struct_body(
    c_compiler_t *ctx, c_ast_struct_spec_t const *spec, cir_scope_t *scope, c_struct_type_t *comp
) {
    vec_c_ast_def_t const *body = &spec->definition->items;

    uint64_t target_size_max = c_target_size_max_64(ctx);

    comp->size  = 0;
    comp->align = 1;
    bool errors = false;

    for (size_t i = 0; i < body->len; i++) {
        if (body->arr[i]->tag != C_AST_TAG_DEFS) {
            continue;
        }
        c_ast_defs_t const          *def   = body->arr[i]->def_defs;
        vec_c_ast_init_decl_t const *decls = &def->decls->items;

        c_type_opt_t inner = c_compile2_spec_qual_list(ctx, def->spec_qual, scope);
        if (!c_type_is_valid(inner)) {
            errors = true;
            continue;
        } else if (inner.qual.s_typedef) {
            cctx_diagnostic(ctx->cctx, def->spec_qual->pos, DIAG_ERR, "typedef not allowed here");
            c_type_delete(inner);
            errors = true;
            continue;
        }

        if (
            decls->len == 0 && (inner.prim == C_COMP_STRUCT || inner.prim == C_COMP_UNION)
            && inner.extra->comp_type->name == NULL
            && inner.extra->comp_type->refcount == 1 // Refcount check excludes typedef'd names.
        ) {
            errors |= c_compile2_struct_field(ctx, comp, (pos_t){0}, NULL, inner, target_size_max);

        } else {
            // Normal field.
            for (size_t x = 0; x < decls->len; x++) {
                c_ast_ident_t const *name_ast = NULL;
                c_type_opt_t         field_type
                    = c_compile2_type(ctx, scope, c_type_clone(inner), decls->arr[x]->decl, &name_ast);
                if (!c_type_is_valid(field_type)) {
                    errors = true;
                    continue;
                }
                errors |= c_compile2_struct_field(
                    ctx,
                    comp,
                    decls->arr[x]->decl->pos,
                    name_ast->name,
                    field_type,
                    target_size_max
                );
            }
            c_type_delete(inner);
        }
    }

    if (errors) {
        comp->size  = 0;
        comp->align = 0;
    } else if (comp->size % comp->align) {
        comp->size += comp->align - comp->size % comp->align;
    }
}

// Compile a C enum/struct/union specification.
static c_comp_type_t *c_compile2_comp_spec(c_compiler_t *ctx, c_ast_spec_qual_t const *comp_spec, cir_scope_t *scope) {
    // What tag type this specifier has.
    c_comp_type_tag_t    tag;
    c_ast_ident_t const *name;
    if (comp_spec->tag == C_AST_TAG_SPEC_QUAL_ENUM) {
        name = comp_spec->spec_qual_enum->name;
        tag  = C_COMP_TYPE_ENUM;
    } else {
        assert(comp_spec->tag == C_AST_TAG_SPEC_QUAL_STRUCT);
        name = comp_spec->spec_qual_struct->name;
        tag  = comp_spec->spec_qual_struct->is_union ? C_COMP_TYPE_UNION : C_COMP_TYPE_STRUCT;
    }

    // Get or create the compound type.
    c_comp_type_t *comp = NULL;
    if (name) {
        comp = cir_scope_lookup_tag(scope, name->name);
    }
    if (!comp) {
        comp       = lilycc_calloc(1, sizeof(c_comp_type_t));
        comp->name = name ? lilycc_strdup(name->name) : NULL;
        comp->tag  = tag;
        if (name) {
            cir_scope_add_tag(scope, name->name, comp);
            comp->refcount++;
        }
    }

    // Assert that the tag type matches.
    if (comp->tag != tag) {
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
            name->name,
            names[comp->tag],
            names[tag]
        );
        c_comp_type_delete(comp);
        return NULL;
    }

    // Finally compile the body with the appropriate type.
    if (tag == C_COMP_TYPE_ENUM) {
        c_ast_enum_spec_t const *enum_spec = comp_spec->spec_qual_enum;
        if (enum_spec->definition) {
            c_compile2_enum_body(ctx, enum_spec, scope, &comp->enum_type);
        }
    } else {
        c_ast_struct_spec_t const *struct_spec = comp_spec->spec_qual_struct;
        if (struct_spec->definition) {
            c_compile2_struct_body(ctx, struct_spec, scope, &comp->struct_type);
        }
    }

    return comp;
}

// Create a C type from a specifier-qualifer list.
// Returns a refcount pointer of `c_type_t`.
c_type_opt_t c_compile2_spec_qual_list(c_compiler_t *ctx, c_ast_spec_qual_list_t const *list, cir_scope_t *scope) {
    // A typedef'd name.
    c_ast_ident_t const     *typedef_name = NULL;
    // An enum/struct/union spec.
    c_ast_spec_qual_t const *comp         = NULL;

    int  n_long       = 0;
    bool has_int      = false;
    bool has_short    = false;
    bool has_char     = false;
    bool has_float    = false;
    bool has_double   = false;
    bool has_void     = false;
    bool has_bool     = false;
    bool has_unsigned = false;
    bool has_signed   = false;
    bool has_int128   = false;

    c_type_t type = C_TYPE_INVALID;

    // Turn the list into a more manageable format.
    for (size_t i = 0; i < list->items.len; i++) {
        c_ast_spec_qual_t const *param = list->items.arr[i];
        if (param->tag == C_AST_TAG_SPEC_QUAL_KEYW) {
            c_keyw_t keyw = param->spec_qual_keyw;
            switch (keyw) {
                case C_KEYW__Atomic: type.qual.q_atomic = true; break;
                case C_KEYW_volatile: type.qual.q_volatile = true; break;
                case C_KEYW_const: type.qual.q_const = true; break;
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
                case C_KEYW___int128: has_int128 = true; break;
                default:
                    fprintf(stderr, "BUG: Unhandled specifier-qualifier keyword %s\n", c_keyw_name[keyw]);
                    abort();
                    break;
            }
        } else if (typedef_name || comp) {
            pos_t pos;
            if (typedef_name) {
                pos = typedef_name->pos;
            } else {
                assert(comp);
                if (comp->tag == C_AST_TAG_SPEC_QUAL_ENUM) {
                    pos = comp->spec_qual_enum->keyw_pos;
                } else {
                    assert(comp->tag == C_AST_TAG_SPEC_QUAL_STRUCT);
                    pos = comp->spec_qual_struct->keyw_pos;
                }
            }
            cctx_diagnostic(ctx->cctx, param->pos, DIAG_ERR, "Multiple types in specifier-qualifier list");
            cctx_diagnostic(ctx->cctx, pos, DIAG_INFO, "Previous type in this list");
        } else if (param->tag == C_AST_TAG_SPEC_QUAL_ENUM || param->tag == C_AST_TAG_SPEC_QUAL_STRUCT) {
            comp = param;
        } else {
            assert(param->tag == C_AST_TAG_SPEC_QUAL_TYPEDEF);
            typedef_name = param->spec_qual_typedef;
        }
    }

    if (has_signed && has_unsigned) {
        cctx_diagnostic(ctx->cctx, list->pos, DIAG_ERR, "Type cannot be both signed and unsigned");
    }

    // C type parsing is messy, can't do much about that.
    if (comp) {
        c_comp_type_t *inner = c_compile2_comp_spec(ctx, comp, scope);
        if (!inner) {
            return C_TYPE_INVALID;
        }
        switch (comp->tag) {
            case C_AST_TAG_SPEC_QUAL_ENUM: type.prim = C_COMP_ENUM; break;
            case C_AST_TAG_SPEC_QUAL_STRUCT:
                type.prim = comp->spec_qual_struct->is_union ? C_COMP_UNION : C_COMP_STRUCT;
                break;
            case C_AST_TAG_SPEC_QUAL_KEYW:
            case C_AST_TAG_SPEC_QUAL_TYPEDEF: UNREACHABLE();
        }
        type.extra            = lilycc_calloc(1, sizeof(c_bigtype_t));
        type.extra->refcount  = 1;
        type.extra->comp_type = inner;

    } else if (typedef_name) {
        fprintf(stderr, "TODO: c_compile2_spec_qual_list with typedef'd name\n");
        abort();

    } else if (has_char) {
        if (has_unsigned) {
            type.prim = C_PRIM_UCHAR;
        } else if (has_signed) {
            type.prim = C_PRIM_SCHAR;
        } else {
            type.prim = C_PRIM_CHAR;
        }
        has_char     = false;
        has_signed   = false;
        has_unsigned = false;
    } else if (has_short) {
        if (has_unsigned) {
            type.prim = C_PRIM_USHORT;
        } else {
            type.prim = C_PRIM_SSHORT;
        }
        has_short    = false;
        has_signed   = false;
        has_unsigned = false;
    } else if (has_int128) {
        if (has_unsigned) {
            type.prim = C_PRIM_U128;
        } else {
            type.prim = C_PRIM_S128;
        }
        has_int128   = false;
        has_signed   = false;
        has_unsigned = false;
    } else if (n_long == 2) {
        if (has_unsigned) {
            type.prim = C_PRIM_ULLONG;
        } else {
            type.prim = C_PRIM_SLLONG;
        }
        n_long       = 0;
        has_signed   = false;
        has_unsigned = false;
    } else if (n_long == 1) {
        if (has_unsigned) {
            type.prim = C_PRIM_ULONG;
        } else {
            type.prim = C_PRIM_SLONG;
        }
        n_long       = 0;
        has_signed   = false;
        has_unsigned = false;
    } else if (has_int || has_unsigned || has_signed) {
        if (has_unsigned) {
            type.prim = C_PRIM_UINT;
        } else {
            type.prim = C_PRIM_SINT;
        }
        has_int      = false;
        has_signed   = false;
        has_unsigned = false;
    } else if (has_float) {
        type.prim = C_PRIM_FLOAT;
        has_float = false;
    } else if (has_double) {
        if (n_long) {
            type.prim = C_PRIM_LDOUBLE;
        } else {
            type.prim = C_PRIM_DOUBLE;
        }
        has_double = false;
        n_long--;
    } else if (has_bool) {
        type.prim = C_PRIM_BOOL;
        has_bool  = false;
    } else if (has_void) {
        type.prim = C_PRIM_VOID;
        has_void  = false;
    } else {
        type.prim = C_PRIM_SINT;
    }

    if (n_long || has_int || has_short || has_char || has_float || has_double || has_void || has_bool || has_unsigned
        || has_signed || has_int128) {
        cctx_diagnostic(ctx->cctx, list->pos, DIAG_ERR, "Invalid combination of type specifiers");
    }

    return type;
}

// Compile the type encoded by a declaration or type name.
// Returns a refcount ptr of `c_type_t` if successful.
c_type_opt_t c_compile2_type(
    c_compiler_t         *ctx,
    cir_scope_t          *scope,
    c_type_t              spec_qual_type,
    c_ast_decl_t const   *decl,
    c_ast_ident_t const **name_out
) {
    c_type_t cur = spec_qual_type;
    if (name_out) {
        *name_out = NULL;
    }

    while (1) {
        if (!decl) {
            return cur;

        } else if (decl->tag == C_AST_TAG_DECL_IDENT) {
            if (name_out) {
                *name_out = decl->decl_ident;
            }
            return cur;

        } else if (decl->tag == C_AST_TAG_DECL_FUNC) {
            c_func_type_t *func = lilycc_calloc(1, sizeof(c_func_type_t));
            func->returns       = cur;
            c_bigtype_t *extra  = lilycc_calloc(1, sizeof(c_bigtype_t));
            extra->refcount     = 1;
            extra->inner        = cur;
            c_type_t next       = {
                .extra = extra,
                .prim  = C_COMP_POINTER,
                .qual  = {0},
            };

            bool errors = false;
            for (size_t i = 0; i < decl->decl_func->params->items.len; i++) {
                c_ast_arg_def_t const *def  = decl->decl_func->params->items.arr[i];
                c_ast_ident_t const   *name = NULL;
                c_type_opt_t           type = c_compile2_spec_qual_list(ctx, def->spec_qual, scope);
                if (!c_type_is_valid(type)) {
                    errors = true;
                    continue;
                }
                if (def->decl) {
                    type = c_compile2_type(ctx, scope, type, def->decl, &name);
                    if (!c_type_is_valid(type)) {
                        errors = true;
                        continue;
                    }
                }
                c_func_arg_t arg = {0};
                arg.type         = type;
                if (name) {
                    arg.name     = name->name;
                    arg.name_pos = name->pos;
                }
                vec_push(&func->args, arg);
            }

            if (errors) {
                c_type_delete(cur);
                return C_TYPE_INVALID;
            }

            decl = decl->decl_func->inner;
            cur  = next;

        } else if (decl->tag == C_AST_TAG_DECL_ARRAY) {
            uint64_t inner_size, inner_align;
            if (!c_type_get_size(ctx, cur, &inner_size, &inner_align)) {
                cctx_diagnostic(ctx->cctx, decl->pos, DIAG_ERR, "Array has incomplete element type");
                c_type_delete(cur);
                return C_TYPE_INVALID;
            }

            c_bigtype_t *extra = lilycc_calloc(1, sizeof(c_bigtype_t));
            extra->refcount    = 1;
            extra->inner       = cur;
            c_type_t next      = {
                .extra = extra,
                .prim  = C_COMP_ARRAY,
                .qual  = {0},
            };

            if (decl->decl_array->size) {
                // Has size expression.
                fprintf(stderr, "TODO: Sized arrays\n");
                abort();
            }

            decl = decl->decl_array->inner;
            cur  = next;

        } else if (decl->tag == C_AST_TAG_DECL_PTR) {
            c_bigtype_t *extra = lilycc_calloc(1, sizeof(c_bigtype_t));
            extra->refcount    = 1;
            extra->inner       = cur;
            c_type_t next      = {
                .extra = extra,
                .prim  = C_COMP_POINTER,
                .qual  = {0},
            };

            // Add specifiers to pointer.
            vec_c_ast_spec_qual_t const *list = &decl->decl_ptr->spec_qual->items;
            for (size_t i = 0; i < list->len; i++) {
                assert(list->arr[i]->tag == C_AST_TAG_SPEC_QUAL_KEYW);
                if (list->arr[i]->spec_qual_keyw == C_KEYW_volatile) {
                    next.qual.q_volatile = true;
                } else if (list->arr[i]->spec_qual_keyw == C_KEYW_const) {
                    next.qual.q_const = true;
                } else if (list->arr[i]->spec_qual_keyw == C_KEYW__Atomic) {
                    next.qual.q_atomic = true;
                } else if (list->arr[i]->spec_qual_keyw == C_KEYW_restrict) {
                    next.qual.q_restrict = true;
                } else {
                    fprintf(stderr, "BUG: Unhandled pointer qualifier %s\n", c_keyw_name[list->arr[i]->spec_qual_keyw]);
                    abort();
                }
            }

            decl = decl->decl_ptr->inner;
            cur  = next;

        } else if (decl->tag == C_AST_TAG_DECL_GARBAGE) {
            c_type_delete(cur);
            return C_TYPE_INVALID;

        } else {
            abort();
        }
    }
}
