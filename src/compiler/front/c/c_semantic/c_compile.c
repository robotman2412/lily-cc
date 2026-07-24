
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compile.h"

#include "arith128.h"
#include "arrays.h"
#include "c_ast.h"
#include "c_compile_expr.h"
#include "c_ir.h"
#include "c_parser.h"
#include "c_tokenizer.h"
#include "c_types.h"
#include "compiler.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "lilycc_malloc.h"
#include "refcount.h"

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
static void c_compile2_enum_body(c_compiler_t *ctx, c_ast_enum_spec_t const *spec, cir_scope_t *scope, c_comp_t *comp) {
    vec_c_ast_enumvar_t const *body    = &spec->definition->items;
    ir_prim_t const            ir_prim = ctx->options.int32 ? IR_PRIM_s32 : IR_PRIM_s16;

    // TODO: Packed enums support.

    size_t  cap = 0;
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
            enumvar.ordinal = cur;
            array_lencap_insert_strong(
                &comp->variants.arr,
                sizeof(c_enumvar_t),
                &comp->variants.len,
                &cap,
                &enumvar,
                comp->variants.len
            );
        }
    }

    comp->align = comp->size = ctx->options.int32 ? 4 : 2;
}

// Compile the body of a struct/union definition.
static void
    c_compile2_struct_body(c_compiler_t *ctx, c_ast_struct_spec_t const *spec, cir_scope_t *scope, c_comp_t *comp) {
    vec_c_ast_def_t const *body = &spec->definition->items;

    size_t   cap    = 0;
    uint64_t offset = 0;
    uint64_t size   = 0;
    uint64_t align  = 1;
    bool     errors = false;

    for (size_t i = 0; i < body->len; i++) {
        if (body->arr[i]->tag != C_AST_TAG_DEFS) {
            continue;
        }
        c_ast_defs_t const          *def      = body->arr[i]->def_defs;
        vec_c_ast_init_decl_t const *decls    = &def->decls->items;
        rc_t                         inner_rc = c_compile2_spec_qual_list(ctx, def->spec_qual, scope);
        if (!inner_rc) {
            errors = true;
            continue;
        }

        c_type_t const *inner_type = inner_rc->data;
        // TODO: This check does not exclude `struct a` in `struct { struct a; }`.
        if (decls->len == 0 && (inner_type->primitive == C_COMP_STRUCT || inner_type->primitive == C_COMP_UNION)) {
            c_comp_t const *inner_comp = inner_type->comp->data;

            // Pad to alignment of inner type.
            if (offset % inner_comp->align) {
                offset += inner_comp->align - offset % inner_comp->align;
            }

            c_field_t field;
            field.type_rc = rc_share(inner_rc);
            field.name    = NULL;
            field.offset  = offset;
            array_lencap_insert_strong(
                &comp->fields.arr,
                sizeof(c_field_t),
                &comp->fields.len,
                &cap,
                &field,
                comp->fields.len
            );

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
            for (size_t x = 0; x < decls->len; x++) {
                c_ast_ident_t const *name_ast = NULL;
                rc_t field_type = c_compile2_type(ctx, scope, rc_share(inner_rc), decls->arr[x]->decl, &name_ast);
                if (!field_type) {
                    errors = true;
                    continue;
                }
                uint64_t field_size, field_align;
                if (c_type_get_size(ctx, field_type->data, &field_size, &field_align)) {
                    if (offset % field_align) {
                        offset += field_align - offset % field_align;
                    }
                    c_field_t field;
                    field.type_rc  = field_type;
                    field.name     = lilycc_strdup(name_ast->name);
                    field.name_pos = name_ast->pos;
                    field.offset   = offset;
                    array_lencap_insert_strong(
                        &comp->fields.arr,
                        sizeof(c_field_t),
                        &comp->fields.len,
                        &cap,
                        &field,
                        comp->fields.len
                    );
                } else {
                    cctx_diagnostic(ctx->cctx, name_ast->pos, DIAG_ERR, "Use of incomplete type");
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
static rc_t c_compile2_comp_spec(c_compiler_t *ctx, c_ast_spec_qual_t const *comp_spec, cir_scope_t *scope) {
    // What tag type this specifier has.
    c_comp_type_t        comp_type;
    c_ast_ident_t const *name;
    if (comp_spec->tag == C_AST_TAG_SPEC_QUAL_ENUM) {
        name      = comp_spec->spec_qual_enum->name;
        comp_type = C_COMP_TYPE_ENUM;
    } else {
        assert(comp_spec->tag == C_AST_TAG_SPEC_QUAL_STRUCT);
        name      = comp_spec->spec_qual_struct->name;
        comp_type = comp_spec->spec_qual_struct->is_union ? C_COMP_TYPE_UNION : C_COMP_TYPE_STRUCT;
    }

    // Get or create the compound type.
    rc_t      comp_rc = NULL;
    c_comp_t *comp;
    if (name) {
        comp_rc = cir_scope_lookup_tag(scope, name->name);
    }
    if (!comp_rc) {
        comp       = lilycc_calloc(1, sizeof(c_comp_t));
        comp_rc    = rc_new_strong(comp, (void (*)(void *))c_comp_free);
        comp->name = name ? lilycc_strdup(name->name) : NULL;
        comp->type = comp_type;
        if (name) {
            cir_scope_add_tag(scope, name->name, rc_share(comp_rc));
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
            name->name,
            names[comp->type],
            names[comp_type]
        );
        rc_delete(comp_rc);
        return NULL;
    }

    // Finally compile the body with the appropriate type.
    if (comp_type == C_COMP_TYPE_ENUM) {
        c_ast_enum_spec_t const *enum_spec = comp_spec->spec_qual_enum;
        if (enum_spec->definition) {
            c_compile2_enum_body(ctx, enum_spec, scope, comp);
        }
    } else {
        c_ast_struct_spec_t const *struct_spec = comp_spec->spec_qual_struct;
        if (struct_spec->definition) {
            c_compile2_struct_body(ctx, struct_spec, scope, comp);
        }
    }

    return comp_rc;
}

// Create a C type from a specifier-qualifer list.
// Returns a refcount pointer of `c_type_t`.
rc_t c_compile2_spec_qual_list(c_compiler_t *ctx, c_ast_spec_qual_list_t const *list, cir_scope_t *scope) {
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

    rc_t      type_rc = rc_new_strong(lilycc_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
    c_type_t *type    = type_rc->data;

    // Turn the list into a more manageable format.
    for (size_t i = 0; i < list->items.len; i++) {
        c_ast_spec_qual_t const *param = list->items.arr[i];
        if (param->tag == C_AST_TAG_SPEC_QUAL_KEYW) {
            c_keyw_t keyw = param->spec_qual_keyw;
            switch (keyw) {
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
        rc_t comp_rc = c_compile2_comp_spec(ctx, comp, scope);
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

    } else if (typedef_name) {
        fprintf(stderr, "TODO: c_compile2_spec_qual_list with typedef'd name\n");
        abort();

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
    } else if (has_int128) {
        if (has_unsigned) {
            type->primitive = C_PRIM_U128;
        } else {
            type->primitive = C_PRIM_S128;
        }
        has_int128   = false;
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

    if (n_long || has_int || has_short || has_char || has_float || has_double || has_void || has_bool || has_unsigned
        || has_signed || has_int128) {
        cctx_diagnostic(ctx->cctx, list->pos, DIAG_ERR, "Invalid combination of type specifiers");
    }

    return type_rc;
}

// Compile the type encoded by a declaration or type name.
// Returns a refcount ptr of `c_type_t` if successful.
rc_t c_compile2_type(
    c_compiler_t *ctx, cir_scope_t *scope, rc_t spec_qual_type, c_ast_decl_t const *decl, c_ast_ident_t const **name_out
) {
    rc_t cur = spec_qual_type;
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
            rc_t      func       = rc_new_strong(lilycc_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *func_type  = func->data;
            func_type->primitive = C_COMP_FUNCTION;
            func_type->func.return_type = cur;
            func_type->func.args_len    = decl->decl_func->params->items.len;
            if (func_type->func.args_len > 0) {
                func_type->func.args          = lilycc_calloc(func_type->func.args_len, sizeof(rc_t));
                func_type->func.arg_names     = lilycc_calloc(func_type->func.args_len, sizeof(void *));
                // TODO: This will be removed later!
                func_type->func.arg_name_tkns = lilycc_calloc(func_type->func.args_len, sizeof(void *));
            }

            for (size_t i = 0; i < func_type->func.args_len; i++) {
                c_ast_arg_def_t const *param = decl->decl_func->params->items.arr[i];
                func_type->func.args[i]      = c_compile2_spec_qual_list(ctx, param->spec_qual, scope);
                if (param->decl) {
                    c_ast_ident_t const *name_tmp;
                    func_type->func.args[i]
                        = c_compile2_type(ctx, scope, func_type->func.args[i], param->decl, &name_tmp);
                    func_type->func.arg_names[i] = lilycc_strdup(name_tmp->name); // NOLINT.
                }
            }

            decl = decl->decl_func->inner;
            cur  = func;

        } else if (decl->tag == C_AST_TAG_DECL_ARRAY) {
            rc_t      next       = rc_new_strong(lilycc_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *next_type  = next->data;
            next_type->primitive = C_COMP_ARRAY;
            next_type->inner     = cur;
            next_type->length    = -1;

            uint64_t inner_size, inner_align;
            if (!c_type_get_size(ctx, cur->data, &inner_size, &inner_align)) {
                cctx_diagnostic(ctx->cctx, decl->pos, DIAG_ERR, "Array has incomplete element type");
            } else if (decl->decl_array->size) {
                // Has size expression.
                fprintf(stderr, "TODO: Sized arrays\n");
                abort();
                /*
                c_compile_expr_t res = c_compile_expr(ctx, NULL, NULL, scope, &decl->params[1]);
                if (c_value_is_const(&res.res)) {
                    if (!c_type_is_scalar(res.res.c_type->data)) {
                        cctx_diagnostic(
                            ctx->cctx,
                            decl->params[1].pos,
                            DIAG_ERR,
                            "Expected scalar type for array bound"
                        );
                    } else {
                        // Check that the evaluated array length is within bounds.
                        ir_const_t iconst       = c_value_read(ctx, NULL, &res.res).iconst;
                        i128_t     len          = ir_cast(IR_PRIM_u128, iconst).const128;
                        int        sizeof_usize = ir_prim_sizes[c_prim_to_ir_type(ctx, ctx->options.size_type)];
                        uint64_t   usize_max;
                        if (sizeof_usize == 8) {
                            usize_max = UINT64_MAX;
                        } else {
                            usize_max = (1llu << (sizeof_usize * 8)) - 1;
                        }

                        if (ir_prim_is_signed(iconst.prim_type) && cmp128s(len, I128_ZERO) < 0) {
                            char buf[40];
                            itoa128(neg128(len), 0, buf);
                            cctx_diagnostic(
                                ctx->cctx,
                                decl->params[1].pos,
                                DIAG_ERR,
                                "Negative array length of -%s is not allowed",
                                buf
                            );
                        } else if (cmp128u(len, ui128(INT64_MAX / 2)) > 0) {
                            char buf[40];
                            itoa128(len, 0, buf);
                            cctx_diagnostic(
                                ctx->cctx,
                                decl->params[1].pos,
                                DIAG_ERR,
                                "Array length of %s is not supported by Lily-CC",
                                buf
                            );
                        } else if (cmp128u(mul128(len, ui128(inner_size)), ui128(usize_max)) > 0) {
                            char buf[40];
                            itoa128(mul128(len, ui128(inner_size)), 0, buf);
                            cctx_diagnostic(
                                ctx->cctx,
                                decl->params[1].pos,
                                DIAG_ERR,
                                "Array length of %" PRIu64 " would cause its size (%s) exceed __SIZE_MAX__ (%" PRIu64
                                ")",
                                lo64(len),
                                buf,
                                usize_max
                            );
                        } else {
                            // All checks passed, the array length is valid.
                            next_type->length = (int64_t)lo64(len);
                        }
                    }
                }
                c_value_destroy(res.res);
                */
            }

            decl = decl->decl_array->inner;
            cur  = next;

        } else if (decl->tag == C_AST_TAG_DECL_PTR) {
            rc_t      next       = rc_new_strong(lilycc_calloc(1, sizeof(c_type_t)), (void (*)(void *))c_type_free);
            c_type_t *next_type  = next->data;
            next_type->primitive = C_COMP_POINTER;
            next_type->inner     = cur;

            // Add specifiers to pointer.
            vec_c_ast_spec_qual_t const *list = &decl->decl_ptr->spec_qual->items;
            for (size_t i = 0; i < list->len; i++) {
                assert(list->arr[i]->tag == C_AST_TAG_SPEC_QUAL_KEYW);
                if (list->arr[i]->spec_qual_keyw == C_KEYW_volatile) {
                    next_type->is_volatile = true;
                } else if (list->arr[i]->spec_qual_keyw == C_KEYW_const) {
                    next_type->is_const = true;
                } else if (list->arr[i]->spec_qual_keyw == C_KEYW__Atomic) {
                    next_type->is_atomic = true;
                } else if (list->arr[i]->spec_qual_keyw == C_KEYW_restrict) {
                    next_type->is_restrict = true;
                } else {
                    fprintf(stderr, "BUG: Unhandled pointer qualifier %s\n", c_keyw_name[list->arr[i]->spec_qual_keyw]);
                    abort();
                }
            }

            decl = decl->decl_ptr->inner;
            cur  = next;

        } else if (decl->tag == C_AST_TAG_DECL_GARBAGE) {
            rc_delete(cur);
            return NULL;

        } else {
            abort();
        }
    }
}
