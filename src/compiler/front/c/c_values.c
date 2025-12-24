
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_values.h"

#include "c_compiler.h"
#include "c_types.h"
#include "compiler.h"
#include "ir.h"
#include "ir_types.h"
#include "map.h"
#include "refcount.h"
#include "strong_malloc.h"
#include "unreachable.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



static c_value_t c_rvalue_create_map(c_compiler_t *ctx, rc_t type_rc) {
    c_type_t const *type = type_rc->data;
    c_comp_t const *comp = type->comp->data;

    map_t map = STR_MAP_EMPTY;
    for (size_t i = 0; i < comp->fields.len; i++) {
        c_field_t const *field     = &comp->fields.arr[i];
        c_value_t       *value_box = strong_malloc(sizeof(c_value_t));
        *value_box                 = c_rvalue_create(ctx, rc_share(field->type_rc));
        map_set(&map, field->name, value_box);
    }

    return (c_value_t){
        .value_type = C_RVALUE_MAP,
        .c_type     = type_rc,
        .rvalue.map = map,
    };
}

static c_value_t c_rvalue_create_arr(c_compiler_t *ctx, rc_t type_rc) {
    c_type_t const *type  = type_rc->data;
    c_type_t const *inner = type->inner->data;
    size_t          elem_size, elem_align;
    if (!c_type_get_size(ctx, inner, &elem_size, &elem_align)) {
        UNREACHABLE();
    }

    fprintf(stderr, "TODO: c_rvalue_create_arr\n");
    abort();
}

// Create a zero-initialized rvalue for some type.
c_value_t c_rvalue_create(c_compiler_t *ctx, rc_t type_rc) {
    c_type_t const *type = type_rc->data;
    size_t          size, align;
    if (!c_type_get_size(ctx, type, &size, &align)) {
        UNREACHABLE();
    }

    if (type->primitive < C_N_PRIM) {
        return (c_value_t){
            .value_type     = C_RVALUE_OPERAND,
            .c_type         = type_rc,
            .rvalue.operand = IR_OPERAND_CONST(((ir_const_t){
                .prim_type = c_prim_to_ir_type(ctx, type->primitive),
                .constl    = 0,
                .consth    = 0,
            })),
        };
    }

    if (size > 1024 || type->primitive == C_COMP_UNION) {
        // Store in blob form.
        return (c_value_t){
            .value_type  = C_RVALUE_BINARY,
            .c_type      = type_rc,
            .rvalue.blob = strong_calloc(1, size),
        };
    }

    if (type->primitive == C_COMP_STRUCT) {
        return c_rvalue_create_map(ctx, type_rc);
    } else if (type->primitive == C_COMP_ARRAY) {
        return c_rvalue_create_arr(ctx, type_rc);
    } else {
        UNREACHABLE();
    }
}

// Convert an rvalue in binary form to array or map as appropriate.
c_value_t c_value_unblob(c_compiler_t *ctx, rc_t type_rc, uint8_t const *blob) {
    c_type_t const *type = type_rc->data;

    if (type->primitive < C_N_PRIM || type->primitive == C_COMP_ENUM || type->primitive == C_COMP_POINTER) {
        // Scalar and primitive types.
        return (c_value_t){
            .value_type = C_RVALUE_OPERAND,
            .c_type     = type_rc,
            .rvalue.operand
            = IR_OPERAND_CONST(ir_const_from_blob(c_type_to_ir_type(ctx, type), blob, ctx->options.big_endian)),
        };

    } else if (type->primitive == C_COMP_STRUCT || type->primitive == C_COMP_UNION) {
        // Struct/union types.
        c_comp_t const *comp = type->comp->data;
        map_t           map  = STR_MAP_EMPTY;
        for (size_t i = 0; i < comp->fields.len; i++) {
            c_field_t const *field     = &comp->fields.arr[i];
            c_value_t       *value_box = strong_malloc(sizeof(c_value_t));
            *value_box                 = c_value_unblob(ctx, rc_share(field->type_rc), blob + field->offset);
            map_set(&map, field->name, value_box);
        }
        return (c_value_t){
            .value_type = C_RVALUE_MAP,
            .c_type     = type_rc,
            .rvalue.map = map,
        };

    } else if (type->primitive == C_COMP_ARRAY) {
        // Array types.
        fprintf(stderr, "TODO: c_value_unblob for an array type\n");
        abort();

    } else {
        UNREACHABLE();
    }
}

// Clean up an lvalue or rvalue.
void c_value_destroy(c_value_t value) {
    switch (value.value_type) {
        case C_VALUE_ERROR: break;
        case C_LVALUE_MEM:
            if (value.lvalue.memref.base_type == IR_MEMBASE_SYM) {
                free(value.lvalue.memref.base_sym);
            }
            break;
        case C_LVALUE_VAR:
        case C_RVALUE_OPERAND: break;
        case C_RVALUE_ARR: fprintf(stderr, "[TODO] c_value_destroy of C_RVALUE_ARR\n"); break;
        case C_RVALUE_MAP: {
            map_foreach(ent, &value.rvalue.map) {
                rc_delete(ent->value);
            }
            map_clear(&value.rvalue.map);
        } break;
        case C_RVALUE_BINARY: free(value.rvalue.blob); break;
    }
    rc_delete(value.c_type);
}

// Convert a compound typed rvalue into a blob.
static void c_rvalue_to_blob_r(c_compiler_t const *ctx, uint8_t *blob, c_value_t const *rvalue) {
    c_type_t const *rvalue_type = rvalue->c_type->data;

    if (rvalue_type->primitive == C_COMP_ARRAY) {
        // Array copy.
        fprintf(stderr, "[TODO] c_rvalue_to_blob_r with C_COMP_ARRAY\n");
        abort();

    } else if (rvalue_type->primitive == C_COMP_STRUCT) {
        // Struct copy.
        c_comp_t const *rvalue_comp = rvalue_type->comp->data;
        for (size_t i = 0; i < rvalue_comp->fields.len; i++) {
            c_field_t const *field       = &rvalue_comp->fields.arr[i];
            c_value_t const *field_value = map_get(&rvalue->rvalue.map, field->name);
            if (field_value->value_type == C_RVALUE_OPERAND) {
                assert(field_value->rvalue.operand.type == IR_OPERAND_TYPE_CONST);
                ir_const_to_blob(field_value->rvalue.operand.iconst, blob + field->offset, ctx->options.big_endian);
            } else {
                c_rvalue_to_blob_r(ctx, blob + field->offset, field_value);
            }
        }

    } else {
        UNREACHABLE();
    }
}

// Write to a memory lvalue.
static void c_lvalue_write_mem(c_compiler_t *ctx, ir_code_t *code, ir_memref_t lvalue_memref, c_value_t const *rvalue);

// Copy a compound typed rvalue into memory.
static inline void
    c_rvalue_copy_r(c_compiler_t *ctx, ir_code_t *code, ir_memref_t lvalue_memref, c_value_t const *rvalue) {
    c_type_t const *rvalue_type = rvalue->c_type->data;

    if (rvalue_type->primitive == C_COMP_ARRAY) {
        // Array copy.
        fprintf(stderr, "[TODO] c_rvalue_copy_r with C_COMP_ARRAY\n");
        abort();

    } else if (rvalue_type->primitive == C_COMP_STRUCT) {
        // Struct copy.
        c_comp_t const *rvalue_comp = rvalue_type->comp->data;
        for (size_t i = 0; i < rvalue_comp->fields.len; i++) {
            c_field_t const *field        = &rvalue_comp->fields.arr[i];
            c_value_t const *field_value  = map_get(&rvalue->rvalue.map, field->name);
            ir_memref_t      new_memref   = lvalue_memref;
            new_memref.offset            += (int64_t)field->offset;
            c_lvalue_write_mem(ctx, code, new_memref, field_value);
        }

    } else {
        UNREACHABLE();
    }
}

// Write to a memory lvalue.
static void c_lvalue_write_mem(c_compiler_t *ctx, ir_code_t *code, ir_memref_t lvalue_memref, c_value_t const *rvalue) {
    c_type_t const *rvalue_type = rvalue->c_type->data;
    uint64_t        size, align;
    c_type_get_size(ctx, rvalue_type, &size, &align);
    ir_prim_t usize_prim    = c_prim_to_ir_type(ctx, ctx->options.size_type);
    ir_prim_t copy_max_prim = IR_PRIM_u8 + 2 * __builtin_ctzll(ir_prim_sizes[usize_prim] | align);

    switch (rvalue->value_type) {
        case C_VALUE_ERROR: UNREACHABLE();
        case C_LVALUE_MEM: {
            // Copy memory.
            ir_add_memcpy(
                IR_APPEND(code),
                rvalue->lvalue.memref,
                lvalue_memref,
                size,
                usize_prim,
                copy_max_prim,
                true,
                ctx->options.big_endian
            );
        } break;
        case C_LVALUE_VAR:
        case C_RVALUE_OPERAND: {
            // Store a primitive to memory.
            ir_operand_t tmp = c_value_read(ctx, code, rvalue);
            ir_add_store(IR_APPEND(code), tmp, lvalue_memref);
        } break;
        case C_RVALUE_ARR:
        case C_RVALUE_MAP: {
            // Store an array/struct/union to memory.
            if (c_value_is_const(rvalue)) {
                // Trivial write of a constant value.
                uint8_t *blob = calloc(size, 1);
                c_rvalue_to_blob_r(ctx, blob, rvalue);
                ir_add_memcpy_const(
                    IR_APPEND(code),
                    blob,
                    lvalue_memref,
                    size,
                    usize_prim,
                    copy_max_prim,
                    true,
                    ctx->options.big_endian
                );
                free(blob);

            } else {
                // Copy of a non-constant value.
                c_rvalue_copy_r(ctx, code, lvalue_memref, rvalue);
            }
        } break;
        case C_RVALUE_BINARY: {
            // Store a binary blob to memory.
            ir_add_memcpy_const(
                IR_APPEND(code),
                rvalue->rvalue.blob,
                lvalue_memref,
                size,
                usize_prim,
                copy_max_prim,
                true,
                ctx->options.big_endian
            );
        } break;
    }
}

// Write to an lvalue.
void c_value_write(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue, c_value_t const *rvalue) {
    switch (lvalue->value_type) {
        default: UNREACHABLE(); break;
        case C_VALUE_ERROR:
            fprintf(stderr, "[BUG] c_value_write called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE_OPERAND:
        case C_RVALUE_ARR:
        case C_RVALUE_MAP:
        case C_RVALUE_BINARY:
            fprintf(stderr, "[BUG] c_value_write called with an `rvalue` destination");
            abort();
            break;
        case C_LVALUE_MEM: c_lvalue_write_mem(ctx, code, lvalue->lvalue.memref, rvalue); break;
        case C_LVALUE_VAR: {
            ir_operand_t tmp = c_value_read(ctx, code, rvalue);
            ir_add_expr1(IR_APPEND(code), lvalue->lvalue.ir_var, IR_OP1_mov, tmp);
        } break;
    }
}

// Get the memory location of an lvalue.
ir_memref_t c_value_memref(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue) {
    (void)ctx;
    (void)code;

    if (lvalue->value_type == C_VALUE_ERROR) {
        fprintf(stderr, "[BUG] c_value_addrof called on C_VALUE_ERROR\n");
        abort();
    } else if (lvalue->value_type == C_RVALUE_OPERAND) {
        fprintf(stderr, "[BUG] c_value_addrof called on C_RVALUE\n");
        abort();
    }

    ir_memref_t memref;
    if (lvalue->value_type == C_LVALUE_MEM) {
        // Directly use the pointer from the lvalue.
        memref = lvalue->lvalue.memref;

    } else if (lvalue->value_type == C_LVALUE_VAR) {
        // Take pointer of C variable.
        fprintf(stderr, "[BUG] Address taken of C variable but the variable was not marked as such\n");
        abort();

    } else {
        UNREACHABLE();
    }

    return memref;
}

// Read a value for scalar arithmetic.
ir_operand_t c_value_read(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value) {
    switch (value->value_type) {
        default: UNREACHABLE(); break;
        case C_VALUE_ERROR:
            fprintf(stderr, "[BUG] c_value_read called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE_ARR:
        case C_RVALUE_MAP:
        case C_RVALUE_BINARY:
            fprintf(stderr, "[BUG] c_value_read called on compound type rvalue\n");
            abort();
            break;
        case C_RVALUE_OPERAND:
            // Rvalues don't need any reading because they're already in an `ir_operand_t`.
            return value->rvalue.operand;
        case C_LVALUE_MEM: {
            c_type_t *type = value->c_type->data;
            if (type->primitive >= C_N_PRIM) {
                fprintf(stderr, "[BUG] c_value_read called on compound type lvalue\n");
                abort();
            }
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

// Access the field of a struct/union value.
c_value_t c_value_field(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value, token_t const *field_name) {
    (void)code; // Will be needed later for bitfields.
    c_type_t const *type = value->c_type->data;
    if (type->primitive != C_COMP_STRUCT && type->primitive != C_COMP_UNION) {
        fprintf(stderr, "[BUG] c_value_field called on non-struct/union type\n");
        abort();
    }
    c_comp_t const *comp = type->comp->data;

    c_field_t const *field = NULL;
    for (size_t i = 0; i < comp->fields.len; i++) {
        if (!strcmp(comp->fields.arr[i].name, field_name->strval)) {
            field = &comp->fields.arr[i];
            break;
        }
    }
    if (!field) {
        cctx_diagnostic(ctx->cctx, field_name->pos, DIAG_ERR, "No such struct/union field");
        return (c_value_t){0};
    }

    switch (value->value_type) {
        case C_VALUE_ERROR: UNREACHABLE();
        case C_LVALUE_MEM: {
            ir_memref_t memref  = value->lvalue.memref;
            memref.offset      += (int64_t)field->offset;
            return (c_value_t){
                .value_type    = C_LVALUE_MEM,
                .c_type        = rc_share(field->type_rc),
                .lvalue.memref = memref,
            };
        }
        case C_LVALUE_VAR:
        case C_RVALUE_OPERAND:
        case C_RVALUE_ARR: abort();
        case C_RVALUE_MAP: return c_value_clone(ctx, map_get(&value->rvalue.map, field));
        case C_RVALUE_BINARY: {
            size_t size, align;
            if (!c_type_get_size(ctx, field->type_rc->data, &size, &align)) {
                UNREACHABLE();
            }
            uint8_t *blob = strong_malloc(size);
            memcpy(blob, value->rvalue.blob + field->offset, size);

            c_type_t const *field_type = field->type_rc->data;
            if (field_type->primitive < C_N_PRIM || field_type->primitive == C_COMP_POINTER) {
                // Primitives must be converted into IR constants.
                ir_const_t value
                    = ir_const_from_blob(c_type_to_ir_type(ctx, field_type), blob, ctx->options.big_endian);
                free(blob);
                return (c_value_t){
                    .value_type     = C_RVALUE_OPERAND,
                    .c_type         = rc_share(field->type_rc),
                    .rvalue.operand = IR_OPERAND_CONST(value),
                };

            } else {
                // Not a primitive, gets to be a blob.
                return (c_value_t){
                    .value_type  = C_RVALUE_BINARY,
                    .c_type      = rc_share(field->type_rc),
                    .rvalue.blob = blob,
                };
            }
        }
    }
    UNREACHABLE();
}

// Clone a C value.
c_value_t c_value_clone(c_compiler_t *ctx, c_value_t const *value) {
    switch (value->value_type) {
        case C_VALUE_ERROR:
        case C_LVALUE_MEM:
        case C_LVALUE_VAR:
        case C_RVALUE_OPERAND: rc_share(value->c_type); return *value;
        case C_RVALUE_ARR: abort();
        case C_RVALUE_MAP: {
            map_t fields = STR_MAP_EMPTY;
            map_foreach(ent, &value->rvalue.map) {
                c_value_t *value = strong_malloc(sizeof(c_value_t));
                *value           = c_value_clone(ctx, ent->value);
                map_set(&fields, ent->key, value);
            }
            return (c_value_t){
                .value_type = C_RVALUE_BINARY,
                .c_type     = rc_share(value->c_type),
                .rvalue.map = fields,
            };
        }
        case C_RVALUE_BINARY: {
            size_t size, align;
            c_type_get_size(ctx, value->c_type->data, &size, &align);
            uint8_t *blob = strong_malloc(size);
            memcpy(blob, value->rvalue.blob, size);
            return (c_value_t){
                .value_type  = C_RVALUE_BINARY,
                .c_type      = rc_share(value->c_type),
                .rvalue.blob = blob,
            };
        }
    }
    UNREACHABLE();
}

// Determine whether a value is assignable.
// Produces a diagnostic if it is not.
bool c_value_assignable(c_compiler_t *ctx, c_value_t const *value, pos_t diag_pos) {
    c_type_t const *type = value->c_type->data;
    if (type->is_const || (value->value_type != C_LVALUE_MEM && value->value_type != C_LVALUE_VAR)) {
        cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Expression must be a modifiable lvalue");
        return false;
    }
    switch (type->primitive) {
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
        case C_PRIM_LDOUBLE: return true;
        case C_PRIM_VOID: cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Cannot assign to void"); return false;
        case C_COMP_STRUCT:
        case C_COMP_UNION:
        case C_COMP_ENUM:
        case C_COMP_POINTER: return true;
        case C_COMP_ARRAY: cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Cannot assign to array type"); return false;
        case C_COMP_FUNCTION:
            cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Cannot assign to function type");
            return false;
    }
    UNREACHABLE();
}

// Determine whether the value is a constant rvalue.
bool c_value_is_const(c_value_t const *value) {
    switch (value->value_type) {
        case C_VALUE_ERROR:
        case C_LVALUE_MEM:
        case C_LVALUE_VAR: return false;
        case C_RVALUE_OPERAND: return value->rvalue.operand.type == IR_OPERAND_TYPE_CONST;
        case C_RVALUE_ARR: fprintf(stderr, "[TODO] c_value_is_const on C_RVALUE_ARR\n"); break;
        case C_RVALUE_MAP:
            map_foreach(ent, &value->rvalue.map) {
                if (!c_value_is_const(ent->value)) {
                    return false;
                }
            }
            return true;
        case C_RVALUE_BINARY: return true;
    }
    UNREACHABLE();
}
