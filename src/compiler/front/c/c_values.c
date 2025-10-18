
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>



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
        case C_RVALUE_BINARY: rc_delete(value.rvalue.binary.blob); break;
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
        map_foreach(ent, &rvalue_comp->fields) {
            c_field_t const *field       = ent->value;
            c_value_t const *field_value = map_get(&rvalue->rvalue.map, ent->key);
            if (field_value->value_type == C_RVALUE_OPERAND) {
                assert(field_value->rvalue.operand.type == IR_OPERAND_TYPE_CONST);
                ir_const_to_blob(field_value->rvalue.operand.iconst, blob + field->offset, ctx->options.big_endian);
            } else {
                c_rvalue_to_blob_r(ctx, blob + field->offset, field_value);
            }
        }

    } else {
        __builtin_unreachable();
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
        map_foreach(ent, &rvalue_comp->fields) {
            c_field_t const *field        = ent->value;
            c_value_t const *field_value  = map_get(&rvalue->rvalue.map, ent->key);
            ir_memref_t      new_memref   = lvalue_memref;
            new_memref.offset            += (int64_t)field->offset;
            c_lvalue_write_mem(ctx, code, new_memref, field_value);
        }

    } else {
        __builtin_unreachable();
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
        case C_VALUE_ERROR: __builtin_unreachable();
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
                rvalue->rvalue.binary.blob->data + rvalue->rvalue.binary.offset,
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
        default: __builtin_unreachable(); break;
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
        __builtin_unreachable();
    }

    return memref;
}

// Read a value for scalar arithmetic.
ir_operand_t c_value_read(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value) {
    switch (value->value_type) {
        default: __builtin_unreachable(); break;
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
c_value_t c_value_field(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value, char const *field_name) {
    c_type_t const *type = value->c_type->data;
    if (type->primitive != C_COMP_STRUCT && type->primitive != C_COMP_UNION) {
        fprintf(stderr, "[BUG] c_value_field called on non-struct/union type\n");
        abort();
    }
    c_comp_t const  *comp  = type->comp->data;
    c_field_t const *field = map_get(&comp->fields, field_name);
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
    __builtin_unreachable();
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
    __builtin_unreachable();
}
