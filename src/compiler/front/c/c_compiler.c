
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_compiler.h"

#include "arith128.h"
#include "arrays.h"
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
#include "unreachable.h"

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Guards against actual cleanup happening for the fake RC types.
static void c_compiler_t_fake_cleanup(void *_) {
    (void)_;
    printf("BUG: Refcount ptr to primitive cache destroyed\n");
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
            printf("BUG: C token %d cannot be converted to IR op2\n", subtype);
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
            printf("BUG: C token %d cannot be converted to IR op1\n", subtype);
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
        fprintf(stderr, "TODO: c_var_create C_COMP_FUNCTION\n");
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
            default: UNREACHABLE();
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
        fprintf(stderr, "TODO: c_var_create in global scope\n");
    }

    map_set(&scope->locals, name_tkn->strval, var);
    map_set(&scope->locals_by_decl, name_tkn, var);

    return var;
}



// Describes a store to memory needed to initialize a compound type.
typedef struct {
    uint64_t  offset;
    c_value_t value;
    pos_t     pos;
} c_comp_store_t;

// Helper for `c_compile_comp_init` that keeps track of the field being written.
typedef struct {
    // Stack that indicates field being accessed.
    size_t         *stack;
    size_t          stack_len;
    size_t          stack_cap;
    // Stores needed to initialize without optimization.
    c_comp_store_t *stores;
    size_t          stores_len;
    size_t          stores_cap;
    // Type being initialized.
    rc_t            type_rc;
    // Current field offset.
    uint64_t        field_offset;
    // Current field type.
    rc_t            field_type_rc;
} c_init_cursor_t;

// Step into the first field of the current (compound-typed) field.
// Returns whether there is such a field (the compound type is not zero-sized).
static bool c_init_cursor_step_in(c_init_cursor_t *cursor) {
    c_type_t const *cur = cursor->type_rc->data;
    for (size_t i = 0;; i++) {
        size_t field = (i < cursor->stack_len) ? cursor->stack[i] : 0;

        if (cur->primitive == C_COMP_STRUCT) {
            c_comp_t const *comp = cur->comp->data;
            if (comp->fields.len == 0) {
                return false;
            }
            cur = comp->fields.arr[field].type_rc->data;

        } else if (cur->primitive == C_COMP_UNION) {
            c_comp_t const *comp = cur->comp->data;
            if (comp->fields.len == 0) {
                return false;
            }
            cur = comp->fields.arr[field].type_rc->data;

        } else if (cur->primitive == C_COMP_ARRAY) {
            fprintf(stderr, "TODO: c_init_cursor_step_in with C_COMP_ARRAY\n");
            abort();

        } else {
            // Pointers, enums and primitive types.
            break;
        }

        if (i >= cursor->stack_len) {
            array_lencap_insert_strong(
                &cursor->stack,
                sizeof(size_t),
                &cursor->stack_len,
                &cursor->stack_cap,
                (size_t[]){0},
                cursor->stack_len
            );
        }
    }

    return true;
}

// Helper for `c_compile_comp_init` that selects a named field.
// Returns whether the field exists.
static inline bool
    c_init_cursor_select_named(c_compiler_t *ctx, c_init_cursor_t *cursor, token_t const *name, bool err_notfound) {
    c_type_t const *field_type = cursor->field_type_rc->data;
    if (field_type->primitive != C_COMP_STRUCT && field_type->primitive != C_COMP_UNION) {
        cctx_diagnostic(ctx->cctx, name->pos, DIAG_ERR, "Unexpected named initializer field for non-struct/union type");
        return false;
    }

    c_comp_t const *comp = field_type->comp->data;
    for (size_t i = 0; i < comp->fields.len; i++) {
        c_field_t const *field      = &comp->fields.arr[i];
        c_type_t const  *field_type = field->type_rc->data;

        if (field->name && !strcmp(field->name, name->strval)) {
            // Matching field name found.
            array_lencap_insert_strong(
                &cursor->stack,
                sizeof(size_t),
                &cursor->stack_len,
                &cursor->stack_cap,
                &i,
                cursor->stack_len
            );
            cursor->field_offset += field->offset;
            rc_delete(cursor->field_type_rc);
            cursor->field_type_rc = rc_share(field->type_rc);
            return true;

        } else if (!field->name && (field_type->primitive == C_COMP_STRUCT || field_type->primitive == C_COMP_UNION)) {
            // Recursively search in anonymous nested structs/unions.
            rc_t prev_type_rc = rc_share(cursor->field_type_rc);
            array_lencap_insert_strong(
                &cursor->stack,
                sizeof(size_t),
                &cursor->stack_len,
                &cursor->stack_cap,
                &i,
                cursor->stack_len
            );
            cursor->field_offset += field->offset;
            rc_delete(cursor->field_type_rc);
            cursor->field_type_rc = rc_share(field->type_rc);

            if (c_init_cursor_select_named(ctx, cursor, name, false)) {
                rc_delete(prev_type_rc);
                return true;
            } else {
                // Restore cursor to original value if search fails.
                rc_delete(cursor->field_type_rc);
                cursor->field_type_rc = prev_type_rc;
                cursor->stack_len--;
                cursor->field_offset -= field->offset;
            }
        }
    }

    if (err_notfound) {
        cctx_diagnostic(ctx->cctx, name->pos, DIAG_ERR, "No such struct/union field");
    }

    return false;
}

// Helper for `c_compile_comp_init` that selects an indexed field.
// Returns whether the field exists.
static inline bool
    c_init_cursor_select_indexed(c_compiler_t *ctx, c_init_cursor_t *cursor, c_value_t index, pos_t index_pos) {
    c_type_t const *field_type = cursor->field_type_rc->data;
    if (field_type->primitive != C_COMP_ARRAY) {
        cctx_diagnostic(ctx->cctx, index_pos, DIAG_ERR, "Unexpected indexed initializer field for non-array type");
        c_value_destroy(index);
        return false;
    }

    assert(c_value_is_const(&index));

    ir_const_t ir_index = c_value_read(ctx, NULL, &index).iconst;
    if (ir_prim_is_signed(ir_index.prim_type)) {
        i128_t s_index = ir_cast(IR_PRIM_s128, ir_index).const128;
        if (cmp128s(s_index, int128(0, 0)) < 0) {
            char buf[40];
            itoa128(neg128(s_index), 0, buf);
            cctx_diagnostic(ctx->cctx, index_pos, DIAG_ERR, "Negative initializer index -%s is not allowed", buf);
            c_value_destroy(index);
            return false;
        }
    }
    i128_t u_index = ir_cast(IR_PRIM_u128, ir_index).const128;
    if (cmp128u(u_index, int128(0, field_type->length)) > 0) {
        char buf[40];
        itoa128(u_index, 0, buf);
        cctx_diagnostic(
            ctx->cctx,
            index_pos,
            DIAG_ERR,
            "Initializer index %s exceeds array bounds (length %" PRIu64 ")",
            buf,
            field_type->length
        );
        c_value_destroy(index);
        return false;
    }

    uint64_t inner_size, inner_align;
    if (!c_type_get_size(ctx, field_type->inner->data, &inner_size, &inner_align)) {
        UNREACHABLE();
    }

    uint64_t index_64 = lo64(u_index);
    array_lencap_insert_strong(
        &cursor->stack,
        sizeof(size_t),
        &cursor->stack_len,
        &cursor->stack_cap,
        &index_64,
        cursor->stack_len
    );
    cursor->field_offset += index_64 * inner_size;
    rc_delete(cursor->field_type_rc);
    cursor->field_type_rc = rc_share(field_type->inner);
    c_value_destroy(index);
    return true;
}

// Helper for `c_init_field` that moves the cursor to the next field.
static void c_init_cursor_next(c_init_cursor_t *cursor) {
    while (cursor->stack_len) {
        bool            has_next = false;
        c_type_t const *cur      = cursor->type_rc->data;
        for (size_t depth = 0; depth < cursor->stack_len; depth++) {
            size_t index = cursor->stack[depth];
            if (cur->primitive == C_COMP_STRUCT) {
                c_comp_t const  *comp  = cur->comp->data;
                c_field_t const *field = &comp->fields.arr[index];
                rc_delete(cursor->field_type_rc);
                cursor->field_type_rc = rc_share(field->type_rc);
                cur                   = field->type_rc->data;
                cursor->field_offset  = field->offset;
                has_next              = index + 1 < comp->fields.len;

            } else if (cur->primitive == C_COMP_UNION) {
                c_comp_t const  *comp  = cur->comp->data;
                c_field_t const *field = &comp->fields.arr[index];
                rc_delete(cursor->field_type_rc);
                cursor->field_type_rc = rc_share(field->type_rc);
                cur                   = field->type_rc->data;
                cursor->field_offset  = field->offset;
                has_next              = false;

            } else if (cur->primitive == C_COMP_ARRAY) {
                fprintf(stderr, "TODO: c_init_cursor_next with C_COMP_ARRAY\n");
                abort();

            } else {
                // Pointers, enums and primitive types.
                has_next = false;
            }
            if (!has_next) {
                break;
            }
        }
        if (has_next) {
            break;
        }

        cursor->stack_len--;
    }

    if (cursor->stack_len) {
        cursor->stack[cursor->stack_len - 1]++;
    }
}

// Compile a compound initializer for a scalar type.
static c_compile_expr_t c_compile_scalar_init(
    c_compiler_t *ctx, c_prepass_t *prepass, ir_code_t *code, c_scope_t *scope, token_t const *init, rc_t type_rc
) {
    if (init->params_len == 0) {
        UNREACHABLE();
    }

    for (size_t i = 0; i < init->params_len; i++) {
        if (init->params[i].type == TOKENTYPE_AST
            && (init->params[i].subtype == C_AST_COMPINIT_INDEX || init->params[i].subtype == C_AST_COMPINIT_NAME)) {
            cctx_diagnostic(ctx->cctx, init->params[i].pos, DIAG_ERR, "Designated initializer used with a scalar type");
            return (c_compile_expr_t){.code = code, .res = {0}};
        }
    }

    c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &init->params[0]);
    if (res.res.value_type == C_VALUE_ERROR) {
        return res;
    }
    code          = res.code;
    bool is_const = c_value_is_const(&res.res);

    if (init->params_len > 1) {
        cctx_diagnostic(ctx->cctx, init->params[1].pos, DIAG_WARN, "Excess elements in scalar initializer");
        bool       error      = false;
        // Need a dummy code to compile, but it will not run.
        ir_code_t *dummy_code = code ? ir_code_create(code->func, NULL) : NULL;
        for (size_t i = 0; i < init->params_len; i++) {
            c_compile_expr_t tmp = c_compile_expr(ctx, prepass, dummy_code, scope, &init->params[i]);
            if (tmp.res.value_type == C_VALUE_ERROR) {
                error = true;
            }
            if (!c_value_is_const(&tmp.res)) {
                is_const = false;
            }
            c_value_destroy(tmp.res);
            dummy_code = tmp.code;
        }
        if (error) {
            c_value_destroy(res.res);
            res.res.value_type = C_VALUE_ERROR;
        }
    }

    ir_operand_t cast_operand;
    ir_prim_t    dest_prim = c_type_to_ir_type(ctx, type_rc->data);
    if (is_const) {
        cast_operand = IR_OPERAND_CONST(ir_cast(dest_prim, res.res.rvalue.operand.iconst));
    } else {
        ir_var_t *tmp = ir_var_create(code->func, dest_prim, NULL);
        ir_add_expr1(IR_APPEND(code), tmp, IR_OP1_mov, c_value_read(ctx, code, &res.res));
        cast_operand = IR_OPERAND_VAR(tmp);
    }
    c_value_destroy(res.res);
    c_value_t cast_value = {
        .value_type     = C_RVALUE_OPERAND,
        .rvalue.operand = cast_operand,
    };

    return (c_compile_expr_t){
        .code = code,
        .res  = cast_value,
    };
}

// Compile excess nested initializers.
// Used to check for errors, but the code will effectively never run.
// Returns true if a compile error occurred.
static int c_compile_excess_init(
    c_compiler_t *ctx, c_prepass_t *prepass, ir_func_t *func, c_scope_t *scope, token_t const *init
) {
    if (init->type == TOKENTYPE_AST && init->subtype == C_AST_COMPINIT) {
        bool err = false;
        for (size_t i = 0; i < init->params_len; i++) {
            err |= c_compile_excess_init(ctx, prepass, func, scope, &init->params[i]);
        }
        return err;
    } else {
        // Detached code created here means that it will compile but never run.
        ir_code_t       *code = func ? ir_code_create(func, NULL) : NULL;
        c_compile_expr_t res  = c_compile_expr(ctx, prepass, code, scope, init);
        bool             err  = res.res.value_type == C_VALUE_ERROR;
        c_value_destroy(res.res);
        return err;
    }
}

// Compile a compound initializer into IR.
c_compile_expr_t c_compile_comp_init(
    c_compiler_t  *ctx,
    c_prepass_t   *prepass,
    ir_code_t     *code,
    c_scope_t     *scope,
    token_t const *init,
    rc_t           type_rc,
    pos_t          type_pos
) {
    c_type_t const *type = type_rc->data;

    // Check type completeness.
    uint64_t size, align;
    if (!c_type_get_size(ctx, type, &size, &align)) {
        cctx_diagnostic(ctx->cctx, type_pos, DIAG_ERR, "Usage of incomplete type");
        return (c_compile_expr_t){
            .code = code,
            .res  = {0},
        };
    }

    // Compile-time optimization for zero-initializations.
    ir_prim_t prim = c_type_to_ir_type(ctx, type);
    if (init->params_len == 0
        || (init->params_len == 1 && init->params[0].type == TOKENTYPE_ICONST && init->params[0].ival == 0
            && init->params[0].ivalh == 0)) {

        c_value_t zeroed;
        if (prim < IR_N_PRIM) {
            zeroed = (c_value_t){
                .value_type     = C_RVALUE_OPERAND,
                .c_type         = type_rc,
                .rvalue.operand = IR_OPERAND_CONST(((ir_const_t){
                    .prim_type = prim,
                    .constl    = 0,
                    .consth    = 0,
                })),
            };
        } else {
            zeroed = (c_value_t){
                .value_type  = C_RVALUE_BINARY,
                .c_type      = type_rc,
                .rvalue.blob = strong_calloc(1, size),
            };
        }

        return (c_compile_expr_t){.code = code, .res = zeroed};
    }

    // Initializer of scalar types.
    if (prim < IR_N_PRIM) {
        return c_compile_scalar_init(ctx, prepass, code, scope, init, type_rc);
    }

    c_init_cursor_t cursor = {
        .stack        = strong_calloc(1, sizeof(size_t)),
        .stack_len    = 1,
        .stack_cap    = 1,
        .stores       = NULL,
        .stores_len   = 0,
        .stores_cap   = 0,
        .type_rc      = rc_share(type_rc),
        .field_offset = 0,
    };
    if (type->primitive == C_COMP_ARRAY) {
        cursor.field_type_rc = rc_share(type->inner);
    } else {
        c_comp_t const *comp = type->comp->data;
        if (comp->fields.len > 0) {
            cursor.field_type_rc = rc_share(comp->fields.arr[0].type_rc);
        }
    }

    // Collect unoptimized stores from the initializer.
    bool error    = false;
    bool is_const = true;
    for (size_t i = 0; i < init->params_len; i++) {
        bool           field_error = false;
        bool           can_step_in = true;
        token_t const *field       = &init->params[i];

        if (field->type == TOKENTYPE_AST
            && (field->subtype == C_AST_COMPINIT_NAME || field->subtype == C_AST_COMPINIT_INDEX)) {
            can_step_in         = false;
            cursor.stack_len    = 0;
            cursor.field_offset = 0;
            rc_delete(cursor.field_type_rc);
            cursor.field_type_rc = rc_share(cursor.type_rc);
        }

        while (1) {
            if (field->type == TOKENTYPE_AST && field->subtype == C_AST_COMPINIT_NAME) {
                // Named initializer field, e.g. `.foo = bar`.
                if (!c_init_cursor_select_named(ctx, &cursor, &field->params[0], true)) {
                    field_error = true;
                }

            } else if (field->type == TOKENTYPE_AST && field->subtype == C_AST_COMPINIT_INDEX) {
                // Indexed initializer field, e.g. `[foo] = bar`.
                c_compile_expr_t res = c_compile_expr(ctx, NULL, NULL, scope, &field->params[0]);
                if (res.res.value_type == C_VALUE_ERROR
                    || !c_init_cursor_select_indexed(ctx, &cursor, res.res, field->params[0].pos)) {
                    field_error = true;
                }

            } else {
                break;
            }
            field = &field->params[1];
        }

        if (field_error) {
            error = true;
            break;

        } else if (cursor.stack_len == 0) {
            // An excess initializer; the expressions are compiled but will not run.
            error |= c_compile_excess_init(ctx, prepass, code ? code->func : NULL, scope, field);

        } else if (field->type == TOKENTYPE_AST && field->subtype == C_AST_COMPINIT) {
            // Nested initializer.
            c_compile_expr_t res;
            if (c_type_is_scalar(cursor.field_type_rc->data)) {
                // Directly compile scalar initializer.
                res = c_compile_scalar_init(ctx, prepass, code, scope, field, cursor.field_type_rc);

            } else {
                // Recursively compile compound initializer.
                res = c_compile_comp_init(ctx, prepass, code, scope, field, rc_share(cursor.field_type_rc), (pos_t){0});
            }

            is_const &= c_value_is_const(&res.res);
            code      = res.code;
            if (res.res.value_type == C_VALUE_ERROR) {
                error = true;
            } else {
                c_comp_store_t entry = {
                    .offset = cursor.field_offset,
                    .pos    = field->pos,
                    .value  = res.res,
                };
                array_lencap_insert_strong(
                    &cursor.stores,
                    sizeof(*cursor.stores),
                    &cursor.stores_len,
                    &cursor.stores_cap,
                    &entry,
                    cursor.stores_len
                );
            }
            c_init_cursor_next(&cursor);

        } else {
            // Field expression.
            c_compile_expr_t res  = c_compile_expr(ctx, prepass, code, scope, field);
            is_const             &= c_value_is_const(&res.res);
            code                  = res.code;

            if (res.res.value_type == C_VALUE_ERROR) {
                c_value_destroy(res.res);
                error = true;
            } else {
                if (can_step_in && !c_type_is_compatible(ctx, cursor.field_type_rc->data, res.res.c_type->data)) {
                    c_init_cursor_step_in(&cursor);
                }

                if (!c_type_is_compatible(ctx, cursor.field_type_rc->data, res.res.c_type->data)) {
                    cctx_diagnostic(ctx->cctx, field->pos, DIAG_ERR, "Initializer with value of incompatible type");
                    c_value_destroy(res.res);
                    error = true;
                } else {
                    c_comp_store_t entry = {
                        .pos    = field->pos,
                        .offset = cursor.field_offset,
                        .value  = res.res,
                    };
                    array_lencap_insert_strong(
                        &cursor.stores,
                        sizeof(*cursor.stores),
                        &cursor.stores_len,
                        &cursor.stores_cap,
                        &entry,
                        cursor.stores_len
                    );
                }
            }
            c_init_cursor_next(&cursor);
        }
    }

    if (error) {
        goto out;
    }

    // Emit warnings about reinitializations of fields.
    uint32_t *bytes_init = calloc((size + 3) / 4, sizeof(uint32_t));
    for (size_t i = 0; i < cursor.stores_len; i++) {
        c_comp_store_t const *store = &cursor.stores[i];
        uint64_t              write_size, write_align;
        if (!c_type_get_size(ctx, store->value.c_type->data, &write_size, &write_align)) {
            UNREACHABLE();
        }

        bool overlap = false;
        for (size_t x = 0; x < write_size; x++) {
            size_t byte            = x + store->offset;
            overlap               |= (bytes_init[byte / 32] >> (byte % 32)) & 1;
            bytes_init[byte / 32] |= 1 << (byte % 32);
        }

        if (overlap) {
            cctx_diagnostic(ctx->cctx, store->pos, DIAG_WARN, "Initializer overwrites previous value");
        }
    }
    free(bytes_init);

    // Collect the writes.
    c_value_t value;
    value.c_type = type_rc;
    if (is_const) {
        uint8_t *blob = calloc(size, sizeof(uint8_t));
        for (size_t i = 0; i < cursor.stores_len; i++) {
            c_comp_store_t const *store = &cursor.stores[i];
            if (store->value.value_type == C_RVALUE_OPERAND) {
                assert(store->value.rvalue.operand.type == IR_OPERAND_TYPE_CONST);
                ir_const_to_blob(store->value.rvalue.operand.iconst, blob + store->offset, ctx->options.big_endian);
            } else {
                assert(store->value.value_type == C_RVALUE_BINARY);
                size_t write_size, write_align;
                if (!c_type_get_size(ctx, store->value.c_type->data, &write_size, &write_align)) {
                    UNREACHABLE();
                }
                memcpy(blob + store->offset, store->value.rvalue.blob, write_size);
            }
        }

        value.value_type  = C_RVALUE_BINARY;
        value.rvalue.blob = blob;

    } else {
        ir_frame_t *frame = ir_frame_create(code->func, size, align, NULL);
        ir_add_memset(
            IR_APPEND(code),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame)),
            IR_OPERAND_CONST(IR_CONST_U8(0)),
            IR_OPERAND_CONST(((ir_const_t){
                .prim_type = c_prim_to_ir_type(ctx, ctx->options.size_type),
                .constl    = size,
            }))
        );

        for (size_t i = 0; i < cursor.stores_len; i++) {
            c_comp_store_t const *store = &cursor.stores[i];
            c_value_memcpy(
                ctx,
                code,
                &store->value,
                IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame), .offset = store->offset)
            );
        }

        value.value_type   = C_RVALUE_STACK;
        value.rvalue.frame = frame;
    }

out:
    // Final cleanup.
    rc_delete(cursor.type_rc);
    rc_delete(cursor.field_type_rc);
    free(cursor.stack);
    free(cursor.stores);

    if (error) {
        return (c_compile_expr_t){.code = code, .res = {0}};
    } else {
        return (c_compile_expr_t){.code = code, .res = value};
    }
}

// Decay an array value into its pointer.
static c_value_t c_array_decay(c_compiler_t *ctx, ir_code_t *code, c_value_t value) {
    c_type_t const *type = value.c_type->data;
    if (type->primitive != C_COMP_ARRAY) {
        return value;
    }
    ir_prim_t ptr_prim = c_prim_to_ir_type(ctx, ctx->options.size_type);
    rc_t      ptr_rc   = c_type_to_pointer(ctx, rc_share(type->inner));

    ir_memref_t memref;
    if (c_is_rvalue(&value)) {
        // R-values.
        if (value.value_type == C_RVALUE_STACK) {
            memref = IR_MEMREF(c_type_to_ir_type(ctx, type->inner->data), IR_BADDR_FRAME(value.rvalue.frame));
        } else {
            assert(value.value_type == C_RVALUE_BINARY);
            // TODO: This could (and should) be put into `.rodata` instead.
            uint64_t size, align;
            if (!c_type_get_size(ctx, type, &size, &align)) {
                UNREACHABLE();
            }
            ir_prim_t usize_prim    = c_prim_to_ir_type(ctx, ctx->options.size_type);
            ir_prim_t copy_max_prim = IR_PRIM_u8 + 2 * __builtin_ctzll(ir_prim_sizes[usize_prim] | align);

            ir_frame_t *frame = ir_frame_create(code->func, size, align, NULL);
            memref            = IR_MEMREF(c_type_to_ir_type(ctx, type->inner->data), IR_BADDR_FRAME(frame));
            ir_gen_memcpy_const(
                IR_APPEND(code),
                value.rvalue.blob,
                memref,
                size,
                copy_max_prim,
                usize_prim,
                true,
                ctx->options.big_endian
            );
        }

    } else {
        // L-values.
        memref = c_value_memref(ctx, code, &value);
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
        }
    }

    c_value_destroy(value);
    ir_var_t *tmp = ir_var_create(code->func, ptr_prim, NULL);
    ir_add_lea(IR_APPEND(code), tmp, memref);
    return (c_value_t){
        .value_type     = C_RVALUE_OPERAND,
        .c_type         = ptr_rc,
        .rvalue.operand = IR_OPERAND_VAR(tmp),
    };
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
        if (c_value_is_assignable(ctx, &lhs, expr->params[1].pos)) {
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
    ir_prim_t const uptr_prim = c_prim_to_ir_type(ctx, ctx->options.size_type);
    ir_prim_t const sptr_prim = ir_prim_as_signed(uptr_prim);
    ir_op2_type_t   op2;
    if (expr->subtype == C_AST_EXPR_INDEX) {
        op2 = IR_OP2_add;
    } else {
        op2 = c_op2_to_ir_op2(expr->params[0].subtype);
    }

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
    ir_const_t usize_iconst = (ir_const_t){.prim_type = uptr_prim, .consth = 0, .constl = size};
    ir_const_t ssize_iconst = (ir_const_t){.prim_type = ir_prim_as_signed(uptr_prim), .consth = 0, .constl = size};

    ir_operand_t lhs_ir   = c_value_read(ctx, code, &lhs);
    ir_operand_t rhs_ir   = c_value_read(ctx, code, &rhs);
    bool const   is_const = lhs_ir.type == IR_OPERAND_TYPE_CONST && rhs_ir.type == IR_OPERAND_TYPE_CONST;

    // Ensure the types match for the following IR operations.
    if (ir_operand_prim(lhs_ir) != ir_operand_prim(rhs_ir)) {
        ir_prim_t large_prim
            = ir_operand_prim(lhs_ir) > ir_operand_prim(rhs_ir) ? ir_operand_prim(lhs_ir) : ir_operand_prim(rhs_ir);
        if (is_const) {
            lhs_ir = IR_OPERAND_CONST(ir_cast(large_prim, lhs_ir.iconst));
            rhs_ir = IR_OPERAND_CONST(ir_cast(large_prim, rhs_ir.iconst));
        } else {
            ir_var_t *lhs_tmp = ir_var_create(code->func, large_prim, NULL);
            ir_add_expr1(IR_APPEND(code), lhs_tmp, IR_OP1_mov, lhs_ir);
            lhs_ir            = IR_OPERAND_VAR(lhs_tmp);
            ir_var_t *rhs_tmp = ir_var_create(code->func, large_prim, NULL);
            ir_add_expr1(IR_APPEND(code), rhs_tmp, IR_OP1_mov, rhs_ir);
            rhs_ir = IR_OPERAND_VAR(rhs_tmp);
        }
    }

    ir_operand_t value;
    rc_t         type_rc;
    if (op2 == IR_OP2_sub && lhs_ptr && rhs_ptr) {
        // Pointer difference.
        type_rc = rc_share(&ctx->prim_rcs[uptr_prim]);
        if (is_const) {
            value = IR_OPERAND_CONST(ir_calc2(
                IR_OP2_div,
                ir_cast(sptr_prim, ir_calc2(IR_OP2_sub, lhs_ir.iconst, rhs_ir.iconst)),
                ssize_iconst
            ));
        } else {
            ir_var_t *sub = ir_var_create(code->func, uptr_prim, NULL);
            ir_add_expr2(IR_APPEND(code), sub, IR_OP2_sub, lhs_ir, rhs_ir);
            ir_var_t *cast = ir_var_create(code->func, sptr_prim, NULL);
            ir_add_expr1(IR_APPEND(code), cast, IR_OP1_mov, IR_OPERAND_VAR(sub));
            ir_var_t *div = ir_var_create(code->func, uptr_prim, NULL);
            ir_add_expr2(IR_APPEND(code), div, IR_OP2_div, IR_OPERAND_VAR(cast), IR_OPERAND_CONST(ssize_iconst));
            value = IR_OPERAND_VAR(div);
        }

    } else if (op2 == IR_OP2_sub) {
        // Pointer offset (subtract edition).
        type_rc = rc_share(lhs.c_type);
        if (is_const) {
            value = IR_OPERAND_CONST(
                ir_calc2(IR_OP2_sub, lhs_ir.iconst, ir_calc2(IR_OP2_mul, rhs_ir.iconst, usize_iconst))
            );
        } else {
            ir_var_t *mul = ir_var_create(code->func, uptr_prim, NULL);
            ir_add_expr2(IR_APPEND(code), mul, IR_OP2_mul, rhs_ir, IR_OPERAND_CONST(usize_iconst));
            ir_var_t *sub = ir_var_create(code->func, uptr_prim, NULL);
            ir_add_expr2(IR_APPEND(code), sub, IR_OP2_sub, lhs_ir, IR_OPERAND_VAR(mul));
            value = IR_OPERAND_VAR(sub);
        }

    } else if (op2 == IR_OP2_add) {
        // Pointer offset.
        if (lhs_ptr) {
            type_rc = rc_share(lhs.c_type);
            if (is_const) {
                value = IR_OPERAND_CONST(
                    ir_calc2(IR_OP2_add, lhs_ir.iconst, ir_calc2(IR_OP2_mul, rhs_ir.iconst, usize_iconst))
                );
            } else {
                ir_var_t *mul = ir_var_create(code->func, uptr_prim, NULL);
                ir_add_expr2(IR_APPEND(code), mul, IR_OP2_mul, rhs_ir, IR_OPERAND_CONST(usize_iconst));
                ir_var_t *add = ir_var_create(code->func, uptr_prim, NULL);
                ir_add_expr2(IR_APPEND(code), add, IR_OP2_add, lhs_ir, IR_OPERAND_VAR(mul));
                value = IR_OPERAND_VAR(add);
            }
        } else {
            type_rc = rc_share(rhs.c_type);
            if (is_const) {
                value = IR_OPERAND_CONST(
                    ir_calc2(IR_OP2_add, rhs_ir.iconst, ir_calc2(IR_OP2_mul, lhs_ir.iconst, usize_iconst))
                );
            } else {
                ir_var_t *mul = ir_var_create(code->func, uptr_prim, NULL);
                ir_add_expr2(IR_APPEND(code), mul, IR_OP2_mul, lhs_ir, IR_OPERAND_CONST(usize_iconst));
                ir_var_t *add = ir_var_create(code->func, uptr_prim, NULL);
                ir_add_expr2(IR_APPEND(code), add, IR_OP2_add, rhs_ir, IR_OPERAND_VAR(mul));
                value = IR_OPERAND_VAR(add);
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

        // TODO: This should allow for `const` variables whose value is known at compile-time.
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
            case C_VAR_STORAGE_ENUM_VARIANT: UNREACHABLE();
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

    } else if (expr->subtype == C_AST_COMPLITERAL) {
        // Compile target type.
        rc_t type_rc;
        if (expr->params[0].type == TOKENTYPE_AST && expr->params[0].subtype == C_AST_TYPE_NAME) {
            rc_t inner_rc = c_compile_spec_qual_list(ctx, &expr->params[0].params[0], scope);
            if (!inner_rc) {
                return (c_compile_expr_t){
                    .res  = (c_value_t){0},
                    .code = code,
                };
            }
            token_t const *name_tkn;
            type_rc = c_compile_decl(ctx, &expr->params[0].params[1], scope, inner_rc, &name_tkn);
            if (name_tkn) {
                cctx_diagnostic(ctx->cctx, name_tkn->pos, DIAG_ERR, "Spurious identifier in type name");
            }
        } else {
            type_rc = c_compile_spec_qual_list(ctx, &expr->params[0], scope);
        }

        // Compile the actual initializer.
        return c_compile_comp_init(ctx, prepass, code, scope, &expr->params[1], type_rc, expr->params[0].pos);

    } else if (expr->subtype == C_AST_EXPR_CAST) {
        // Compile cast target type.
        rc_t cast_rc;
        if (expr->params[0].type == TOKENTYPE_AST && expr->params[0].subtype == C_AST_TYPE_NAME) {
            rc_t inner_rc = c_compile_spec_qual_list(ctx, &expr->params[0].params[0], scope);
            if (!inner_rc) {
                return (c_compile_expr_t){
                    .res  = (c_value_t){0},
                    .code = code,
                };
            }
            token_t const *name_tkn;
            cast_rc = c_compile_decl(ctx, &expr->params[0].params[1], scope, inner_rc, &name_tkn);
            if (name_tkn) {
                cctx_diagnostic(ctx->cctx, name_tkn->pos, DIAG_ERR, "Spurious identifier in type name");
            }
        } else {
            cast_rc = c_compile_spec_qual_list(ctx, &expr->params[0], scope);
        }

        // Compile cast source expression.
        c_compile_expr_t res = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
        if (res.res.value_type == C_VALUE_ERROR
            || c_type_is_identical(ctx, cast_rc->data, res.res.c_type->data, false)) {
            rc_delete(cast_rc);
            return res;
        }

        // Determine castability.
        c_type_t const *new_type = cast_rc->data;
        c_type_t const *old_type = res.res.c_type->data;
        if (!c_type_is_castable(ctx, new_type, old_type)) {
            cctx_diagnostic(ctx->cctx, expr->params[1].pos, DIAG_ERR, "Cannot cast between these types");
            return (c_compile_expr_t){
                .res  = (c_value_t){0},
                .code = code,
            };
        }

        // If the new type is (effectively) `void`, then produce a `void` typed dummy rvalue.
        if (new_type->primitive == C_PRIM_VOID) {
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = code,
                .res  = {
                     .value_type     = C_RVALUE_OPERAND,
                     .c_type         = cast_rc,
                     .rvalue.operand = IR_OPERAND_UNDEF(IR_PRIM_u8),
                },
            };
        }

        // Anything that gets here can be represented as a cast with IR primitives.
        if (res.res.value_type == C_RVALUE_OPERAND && res.res.rvalue.operand.type == IR_OPERAND_TYPE_CONST) {
            // Can be evaluated at compile time.
            ir_const_t new_const = ir_cast(c_type_to_ir_type(ctx, new_type), res.res.rvalue.operand.iconst);
            c_value_t  rvalue    = {
                    .value_type     = C_RVALUE_OPERAND,
                    .c_type         = cast_rc,
                    .rvalue.operand = IR_OPERAND_CONST(new_const),
            };
            c_value_destroy(res.res);
            return (c_compile_expr_t){
                .code = code,
                .res  = rvalue,
            };
        }

        // Otherwise emit a mov instruction.
        ir_var_t *tmpvar = ir_var_create(code->func, c_type_to_ir_type(ctx, new_type), NULL);
        ir_add_expr1(IR_APPEND(code), tmpvar, IR_OP1_mov, c_value_read(ctx, code, &res.res));
        c_value_t rvalue = {
            .value_type     = C_RVALUE_OPERAND,
            .c_type         = cast_rc,
            .rvalue.operand = IR_OPERAND_VAR(tmpvar),
        };
        c_value_destroy(res.res);
        return (c_compile_expr_t){
            .code = code,
            .res  = rvalue,
        };

    } else if (expr->subtype == C_AST_EXPR_INDEX) {
        c_compile_expr_t res;

        // Get operands.
        res = c_compile_expr(ctx, prepass, code, scope, &expr->params[0]);
        if (res.res.value_type == C_VALUE_ERROR) {
            return (c_compile_expr_t){
                .code = res.code,
                .res  = (c_value_t){0},
            };
        }
        c_value_t lhs = res.res;
        code          = res.code;
        res           = c_compile_expr(ctx, prepass, code, scope, &expr->params[1]);
        if (res.res.value_type == C_VALUE_ERROR) {
            c_value_destroy(lhs);
            return (c_compile_expr_t){
                .code = res.code,
                .res  = (c_value_t){0},
            };
        }
        c_value_t rhs = res.res;
        code          = res.code;

        // Validate expression types.
        if (c_type_is_pointer(lhs.c_type->data) == c_type_is_pointer(rhs.c_type->data)
            || (!c_type_is_scalar(lhs.c_type->data) && !c_type_is_scalar(rhs.c_type->data))) {
            cctx_diagnostic(
                ctx->cctx,
                expr->pos,
                DIAG_ERR,
                "Expected one pointer or array type and one scalar type for index expression"
            );
            c_value_destroy(lhs);
            c_value_destroy(rhs);
            return (c_compile_expr_t){
                .code = code,
                .res  = (c_value_t){0},
            };
        }

        res = c_compile_expr2_ptrarith(ctx, code, expr, lhs, rhs);
        if (res.res.value_type == C_VALUE_ERROR) {
            c_value_destroy(lhs);
            c_value_destroy(rhs);
            return (c_compile_expr_t){
                .code = res.code,
                .res  = (c_value_t){0},
            };
        }
        c_value_t pointer = res.res;
        code              = res.code;

        c_type_t const *pointer_type = pointer.c_type->data;
        ir_operand_t    ir_ptr       = c_value_read(ctx, code, &pointer);

        ir_memref_t memref = IR_MEMREF(c_type_to_ir_type(ctx, pointer_type->inner->data));
        switch (ir_ptr.type) {
            case IR_OPERAND_TYPE_CONST:
                memref.base_type = IR_MEMBASE_ABS;
                memref.offset    = (int64_t)ir_ptr.iconst.constl;
                break;
            case IR_OPERAND_TYPE_UNDEF: UNREACHABLE();
            case IR_OPERAND_TYPE_VAR:
                memref.base_type = IR_MEMBASE_VAR;
                memref.base_var  = ir_ptr.var;
                break;
            case IR_OPERAND_TYPE_MEM: UNREACHABLE();
            case IR_OPERAND_TYPE_REG:
                memref.base_type  = IR_MEMBASE_REG;
                memref.base_regno = ir_ptr.regno;
                break;
        }

        c_value_t lvalue = {
            .value_type    = C_LVALUE_MEM,
            .c_type        = rc_share(pointer_type->inner),
            .lvalue.memref = memref,
        };
        c_value_destroy(pointer);
        return (c_compile_expr_t){
            .code = code,
            .res  = lvalue,
        };

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
        uint64_t         field_offset;
        c_field_t const *field = c_type_get_field(ctx, inner_type, expr->params[2].strval, &field_offset);
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
                .offset    = (int64_t)field_offset,
            },
        };
        switch (ptr.type) {
            case IR_OPERAND_TYPE_CONST:
                lvalue.lvalue.memref.base_type  = IR_MEMBASE_ABS;
                lvalue.lvalue.memref.offset    += (int64_t)ptr.iconst.constl;
                break;
            case IR_OPERAND_TYPE_UNDEF: UNREACHABLE();
            case IR_OPERAND_TYPE_VAR:
                lvalue.lvalue.memref.base_type = IR_MEMBASE_VAR;
                lvalue.lvalue.memref.base_var  = ptr.var;
                break;
            case IR_OPERAND_TYPE_MEM:
            case IR_OPERAND_TYPE_REG: UNREACHABLE();
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

        return (c_compile_expr_t){
            .code = code,
            .res  = c_value_access_field(ctx, &res.res, &expr->params[2]),
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
            if (!c_value_is_assignable(ctx, &lhs, expr->params[1].pos)) {
                c_value_destroy(lhs);
                c_value_destroy(rhs);
                return (c_compile_expr_t){
                    .code = code,
                    .res  = (c_value_t){0},
                };
            }

            // Determine compatibility with this operator.
            if (!c_type_is_compatible(ctx, lhs.c_type->data, rhs.c_type->data)) {
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
            } else if (!c_value_is_assignable(ctx, &res.res, expr->params[1].pos)) {
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
            rc_t ptr_rc = c_type_to_pointer(ctx, rc_share(res.res.c_type));

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
                case IR_OPERAND_TYPE_UNDEF: UNREACHABLE();
                case IR_OPERAND_TYPE_VAR:
                    memref.base_type = IR_MEMBASE_VAR;
                    memref.base_var  = ptrval.var;
                    break;
                case IR_OPERAND_TYPE_MEM:
                case IR_OPERAND_TYPE_REG: UNREACHABLE();
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
        } else if (!c_value_is_assignable(ctx, &res.res, expr->params[1].pos)) {
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
        UNREACHABLE();
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
            }
            token_t const *init = &decls->params[1].params[1];

            // TODO: Clean this up a bit?

            c_compile_expr_t res;
            if (init->type == TOKENTYPE_AST && init->subtype == C_AST_COMPINIT) {
                res = c_compile_comp_init(ctx, prepass, code, scope, init, rc_share(decl_type), name->pos);
            } else {
                res = c_compile_expr(ctx, prepass, code, scope, init);
            }
            code = res.code;
            if (res.res.value_type != C_VALUE_ERROR) {
                c_value_t lvalue;
                // Type refcount intentionally not increased.
                switch (var->storage) {
                    case C_VAR_STORAGE_REG:
                        lvalue = (c_value_t){
                            .value_type    = C_LVALUE_VAR,
                            .c_type        = var->type,
                            .lvalue.ir_var = var->ir_var,
                        };
                        break;
                    case C_VAR_STORAGE_FRAME:
                        lvalue = (c_value_t){
                            .value_type    = C_LVALUE_MEM,
                            .c_type        = var->type,
                            .lvalue.memref = IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(var->frame)),
                        };
                        break;
                    case C_VAR_STORAGE_GLOBAL: fprintf(stderr, "TODO: Initialized globals\n"); abort();
                    case C_VAR_STORAGE_ENUM_VARIANT: UNREACHABLE();
                }
                c_value_write(ctx, code, &lvalue, &res.res);
                c_value_destroy(res.res);
            }
        }
    }
    rc_delete(inner_type);

    return code;
}



// Explain a C type.
static void c_type_explain_impl(c_type_t const *type, FILE *to) {
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
        if (type->length < 0) {
            fputs("array (unsized) of ", to);
        } else {
            fprintf(to, "array (length %" PRId64 ") of ", type->length);
        }
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
        case C_COMP_STRUCT:
            fprintf(to, "struct %s", ((c_comp_t const *)type->comp->data)->name ?: "<anonymous>");
            goto print_members;
        case C_COMP_UNION:
            fprintf(to, "union %s", ((c_comp_t const *)type->comp->data)->name ?: "<anonymous>");
            goto print_members;
        print_members: {
            c_comp_t const *comp = type->comp->data;
            if (comp->align != 0) {
                fputs(" { ", to);
                for (size_t i = 0; i < comp->fields.len; i++) {
                    if (i) {
                        fputs(", ", to);
                    }
                    if (comp->fields.arr[i].name) {
                        fprintf(to, "%s: ", comp->fields.arr[i].name);
                    }
                    c_type_explain_impl(comp->fields.arr[i].type_rc->data, to);
                }
                fputs(" }", to);
            }
        } break;
        case C_COMP_ENUM: fprintf(to, "enum %s", ((c_comp_t const *)type->comp->data)->name ?: "<anonymous>"); break;
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
void c_type_explain(c_type_t const *type, FILE *to) {
    c_type_explain_impl(type, to);
    fputc('\n', to);
}
