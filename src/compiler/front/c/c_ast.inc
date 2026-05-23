
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

// clang-format off



#ifndef C_AST_DEF
#define C_AST_DEF(name)
#endif

#ifndef C_AST_UNION_DEF
#define C_AST_UNION_DEF(name, ...) C_AST_DEF(name)
#endif

#ifndef C_AST_UNION_FIELD
#define C_AST_UNION_FIELD(parent, type, name, union_tag)
#endif

#ifndef C_AST_UNION_CHILD
#define C_AST_UNION_CHILD(parent, ast_type, name, union_tag) C_AST_UNION_FIELD(parent, c_ast_##ast_type##_t *, name, union_tag)
#endif

#ifndef C_AST_STRUCT_DEF
#define C_AST_STRUCT_DEF(name, ...) C_AST_DEF(name)
#endif

#ifndef C_AST_STRUCT_FIELD
#define C_AST_STRUCT_FIELD(parent, type, name)
#endif

#ifndef C_AST_STRUCT_CHILD
#define C_AST_STRUCT_CHILD(parent, ast_type, name) C_AST_STRUCT_FIELD(parent, c_ast_##ast_type##_t *, name)
#endif

#ifndef C_AST_LIST_DEF
#define C_AST_LIST_DEF(item_type) C_AST_DEF(item_type##_list)
#endif



// Infix expression (e.g. `1 + 2`).
C_AST_STRUCT_DEF(expr_infix,
    // Left-hand side.
    C_AST_STRUCT_CHILD(expr_infix, expr, lhs)
    // Operator token.
    C_AST_STRUCT_FIELD(expr_infix, c_tokentype_t, oper)
    // Operator token position.
    C_AST_STRUCT_FIELD(expr_infix, pos_t, oper_pos)
    // Right-hand side.
    C_AST_STRUCT_CHILD(expr_infix, expr, rhs)
)

// Prefix expression (e.g. `~3`).
C_AST_STRUCT_DEF(expr_prefix,
    // Operator token.
    C_AST_STRUCT_FIELD(expr_prefix, c_tokentype_t, oper)
    // Operator token position.
    C_AST_STRUCT_FIELD(expr_prefix, pos_t, oper_pos)
    // Operand.
    C_AST_STRUCT_CHILD(expr_prefix, expr, val)
)

// Suffix expression (`expr++` or `expr--`).
C_AST_STRUCT_DEF(expr_suffix,
    // Operator token.
    C_AST_STRUCT_FIELD(expr_suffix, c_tokentype_t, oper)
    // Operator token position.
    C_AST_STRUCT_FIELD(expr_suffix, pos_t, oper_pos)
    // Operand.
    C_AST_STRUCT_CHILD(expr_suffix, expr, val)
)

// Index expression (`ptr[index]`).
C_AST_STRUCT_DEF(expr_index,
    // Left-hand side (the thing being indexed).
    C_AST_STRUCT_CHILD(expr_index, expr, lhs)
    // Right-hand side (the index).
    C_AST_STRUCT_CHILD(expr_index, expr, rhs)
)

// Cast expression (`(type) expr`).
C_AST_STRUCT_DEF(expr_cast,
    // Cast type.
    C_AST_STRUCT_CHILD(expr_cast, type_name, type)
    // Value being cast.
    C_AST_STRUCT_CHILD(expr_cast, expr, val)
)

// Function call expression (`expr(args)`).
C_AST_STRUCT_DEF(expr_call,
    // Function being called.
    C_AST_STRUCT_CHILD(expr_call, expr, func)
    // Call arguments.
    C_AST_STRUCT_CHILD(expr_call, expr_list, args)
)

// Identifier expression (e.g. `foo`).
C_AST_STRUCT_DEF(ident,
    // Identifier name (owned, NUL-terminated).
    // `c_ast_cstr_t` is just `typedef char *`; needed it to be one identifier.
    C_AST_STRUCT_FIELD(ident, c_ast_cstr_t, name)
)

// Numeric constant expression (integer or character constant).
C_AST_STRUCT_DEF(expr_iconst,
    // C type of this literal.
    C_AST_STRUCT_FIELD(expr_iconst, c_prim_t, prim)
    // Constant value.
    C_AST_STRUCT_FIELD(expr_iconst, ir_const_t, iconst)
)

// String constant expression.
C_AST_STRUCT_DEF(expr_sconst,
    // String bytes (owned; may contain embedded NULs).
    C_AST_STRUCT_FIELD(expr_sconst, vec_char_t, value)
)

// Compound literal expression (e.g. `(int[]){1, 2, 3}`).
C_AST_STRUCT_DEF(expr_compliteral,
    // Type name.
    C_AST_STRUCT_CHILD(expr_compliteral, type_name, type)
    // Initializer.
    C_AST_STRUCT_CHILD(expr_compliteral, init_list, init)
)

// One expression; tagged union of the variants above.
C_AST_UNION_DEF(expr,
    C_AST_UNION_CHILD(expr, expr_index,       index,       EXPR_INDEX)
    C_AST_UNION_CHILD(expr, expr_infix,       infix,       EXPR_INFIX)
    C_AST_UNION_CHILD(expr, expr_prefix,      prefix,      EXPR_PREFIX)
    C_AST_UNION_CHILD(expr, expr_suffix,      suffix,      EXPR_SUFFIX)
    C_AST_UNION_CHILD(expr, expr_cast,        cast,        EXPR_CAST)
    C_AST_UNION_CHILD(expr, expr_call,        call,        EXPR_CALL)
    C_AST_UNION_CHILD(expr, ident,            ident,       EXPR_IDENT)
    C_AST_UNION_CHILD(expr, expr_iconst,      iconst,      EXPR_ICONST)
    C_AST_UNION_CHILD(expr, expr_sconst,      sconst,      EXPR_SCONST)
    C_AST_UNION_CHILD(expr, expr_compliteral, compliteral, EXPR_COMPLITERAL)
    C_AST_UNION_CHILD(expr, garbage,          garbage,     EXPR_GARBAGE)
    C_AST_UNION_CHILD(expr, expr_list,        exprs,       EXPRS)
)

// List of expressions.
C_AST_LIST_DEF(expr)



// Failed-parse carrier; the parser emits diagnostics and drops the offending tokens,
// leaving only the position of the failed range.
C_AST_STRUCT_DEF(garbage,
)



// Initializer value; either an expression or compound initializer.
C_AST_UNION_DEF(initval,
    C_AST_UNION_CHILD(initval, expr,      expr,     INITVAL_EXPR)
    C_AST_UNION_CHILD(initval, init_list, compound, INITVAL_COMPOUND)
)

// Designated initializer with a field name (e.g. `.field = value`).
C_AST_STRUCT_DEF(init_named,
    // Field name.
    C_AST_STRUCT_CHILD(init_named, ident, name)
    // Initializer value.
    C_AST_STRUCT_CHILD(init_named, init, value)
)

// Designated initializer with an array index (e.g. `[expr] = value`).
C_AST_STRUCT_DEF(init_indexed,
    // Index expression.
    C_AST_STRUCT_CHILD(init_indexed, expr, index)
    // Initializer value.
    C_AST_STRUCT_CHILD(init_indexed, init, value)
)

// One initializer; tagged union of the variants above.
C_AST_UNION_DEF(init,
    C_AST_UNION_CHILD(init, init_list,     compound, INIT_COMPOUND)
    C_AST_UNION_CHILD(init, init_named,    named,    INIT_NAMED)
    C_AST_UNION_CHILD(init, init_indexed,  indexed,  INIT_INDEXED)
    C_AST_UNION_CHILD(init, initval,       val,      INIT_VAL)
    C_AST_UNION_CHILD(init, garbage,       garbage,  INIT_GARBAGE)
)

// List of initializers.
C_AST_LIST_DEF(init)



// Array type/declarator (e.g. `int[10]`).
C_AST_STRUCT_DEF(decl_array,
    // Inner type/declarator (nullable for abstract declarators).
    C_AST_STRUCT_CHILD(decl_array, decl, inner)
    // Array size (nullable for unsized arrays like `int[]`).
    C_AST_STRUCT_CHILD(decl_array, expr, size)
)

// Pointer type/declarator (e.g. the `*` in `int *foo`).
C_AST_STRUCT_DEF(decl_ptr,
    // Position of the `*` token.
    C_AST_STRUCT_FIELD(decl_ptr, pos_t, ptr_pos)
    // Type qualifiers (nullable; e.g. for `int *const`).
    C_AST_STRUCT_CHILD(decl_ptr, spec_qual_list, spec_qual)
    // Inner type/declarator (nullable).
    C_AST_STRUCT_CHILD(decl_ptr, decl, inner)
)

// Function type/declarator.
C_AST_STRUCT_DEF(decl_func,
    // Parameter declarations.
    C_AST_STRUCT_CHILD(decl_func, arg_def_list, params)
    // Whether the parameter list ends with `, ...`.
    C_AST_STRUCT_FIELD(decl_func, bool, variadic)
)

// One type/declarator; tagged union of the variants above.
C_AST_UNION_DEF(decl,
    C_AST_UNION_CHILD(decl, decl_array, array,   DECL_ARRAY)
    C_AST_UNION_CHILD(decl, decl_ptr,   ptr,     DECL_PTR)
    C_AST_UNION_CHILD(decl, decl_func,  func,    DECL_FUNC)
    C_AST_UNION_CHILD(decl, ident,      ident,   DECL_IDENT)
    C_AST_UNION_CHILD(decl, garbage,    garbage, DECL_GARBAGE)
)

// List of declarators.
C_AST_LIST_DEF(decl)

// Init declarator (e.g. `foo = 1`).
C_AST_STRUCT_DEF(init_decl,
    // Declarator.
    C_AST_STRUCT_CHILD(init_decl, decl, decl)
    // Optional initializer.
    C_AST_STRUCT_CHILD(init_decl, initval, init)
)

// List of init declarators (e.g. `foo = 1, bar = 2`).
C_AST_LIST_DEF(init_decl)

// Full type name (e.g. `int *[2]` in `sizeof(int *[2])` or a cast target).
C_AST_STRUCT_DEF(type_name,
    // Type specifier/qualifier list.
    C_AST_STRUCT_CHILD(type_name, spec_qual_list, spec_qual)
    // Declarator (nullable for abstract declarators).
    C_AST_STRUCT_CHILD(type_name, decl, decl)
)

// Struct/union specifier (e.g. `struct foo` or `struct foo { ... }`).
// Either `name` or `definition` (but not both) may be NULL.
C_AST_STRUCT_DEF(struct_spec,
    // Is `union` (as opposed to `struct`).
    C_AST_STRUCT_FIELD(struct_spec, bool, is_union)
    // Position of the struct/union keyword.
    C_AST_STRUCT_FIELD(struct_spec, pos_t, keyw_pos)
    // Tag name.
    C_AST_STRUCT_CHILD(struct_spec, ident, name)
    // Body (nullable; absent for forward declarations and uses).
    C_AST_STRUCT_CHILD(struct_spec, def_list, definition)
)

// Enum specifier (e.g. `enum foo` or `enum foo { ... }`).
C_AST_STRUCT_DEF(enum_spec,
    // Position of the enum keyword.
    C_AST_STRUCT_FIELD(enum_spec, pos_t, keyw_pos)
    // Tag name.
    C_AST_STRUCT_CHILD(enum_spec, ident, name)
    // Body (nullable; absent for forward declarations and uses).
    C_AST_STRUCT_CHILD(enum_spec, enumvar_list, definition)
)

// Enum variant (e.g. `FOO = 1` in an enum definition).
C_AST_STRUCT_DEF(enumvar,
    // Variant name.
    C_AST_STRUCT_CHILD(enumvar, ident, name)
    // Variant value (nullable).
    C_AST_STRUCT_CHILD(enumvar, expr, value)
)

// Enum variant list (enum body).
C_AST_LIST_DEF(enumvar)

// One element of a type specifier/qualifier list.
C_AST_UNION_DEF(spec_qual,
    C_AST_UNION_FIELD(spec_qual, c_keyw_t,    keyw,    SPEC_QUAL_KEYW)
    C_AST_UNION_CHILD(spec_qual, struct_spec, struct,  SPEC_QUAL_STRUCT)
    C_AST_UNION_CHILD(spec_qual, enum_spec,   enum,    SPEC_QUAL_ENUM)
    C_AST_UNION_CHILD(spec_qual, ident,       typedef, SPEC_QUAL_TYPEDEF)
)

// Type specifier/qualifier list (e.g. `const int`).
C_AST_LIST_DEF(spec_qual)



// Function argument declaration
C_AST_STRUCT_DEF(arg_def,
    // Type specifier/qualifier list.
    C_AST_STRUCT_CHILD(arg_def, spec_qual_list, spec_qual)
    // Declarator.
    C_AST_STRUCT_CHILD(arg_def, decl, decl)
)

// Function argument declaration list.
C_AST_LIST_DEF(arg_def)

// Declaration list (e.g. `int x, y = 3;`).
C_AST_STRUCT_DEF(defs,
    // Type specifier/qualifier list.
    C_AST_STRUCT_CHILD(defs, spec_qual_list, spec_qual)
    // Declarators (each is a `c_ast_def_t` of variant assign).
    C_AST_STRUCT_CHILD(defs, init_decl_list, decls)
)

// Function definition.
C_AST_STRUCT_DEF(def_func,
    // Type specifier/qualifier list.
    C_AST_STRUCT_CHILD(def_func, spec_qual_list, spec_qual)
    // Declarator (typically a function-type declarator wrapping an identifier).
    C_AST_STRUCT_CHILD(def_func, decl, declarator)
    // Function body.
    C_AST_STRUCT_CHILD(def_func, stmt_list, body)
)

// One declaration; tagged union of the variants above.
C_AST_UNION_DEF(def,
    C_AST_UNION_CHILD(def, defs,     defs,    DEFS)
    C_AST_UNION_CHILD(def, def_func, func,    DEF_FUNC)
    C_AST_UNION_CHILD(def, garbage,  garbage, DEF_GARBAGE)
)

// List of declarations.
C_AST_LIST_DEF(def)



// For-loop statement.
C_AST_STRUCT_DEF(stmt_for,
    // Initializer (nullable; can be an expression statement or a declaration).
    C_AST_STRUCT_CHILD(stmt_for, stmt, init)
    // Condition (nullable).
    C_AST_STRUCT_CHILD(stmt_for, expr_list, cond)
    // Increment (nullable).
    C_AST_STRUCT_CHILD(stmt_for, expr_list, inc)
    // Body.
    C_AST_STRUCT_CHILD(stmt_for, stmt, body)
)

// While-loop statement.
C_AST_STRUCT_DEF(stmt_while,
    // Condition.
    C_AST_STRUCT_CHILD(stmt_while, expr, cond)
    // Body.
    C_AST_STRUCT_CHILD(stmt_while, stmt, body)
    // Is of `do...while` form.
    C_AST_STRUCT_FIELD(stmt_while, bool, is_do_while)
)

// If statement, with optional else branch.
C_AST_STRUCT_DEF(stmt_if,
    // Condition.
    C_AST_STRUCT_CHILD(stmt_if, expr, cond)
    // If body.
    C_AST_STRUCT_CHILD(stmt_if, stmt, body)
    // Else body (nullable).
    C_AST_STRUCT_CHILD(stmt_if, stmt, else_body)
)

// Switch statement.
C_AST_STRUCT_DEF(stmt_switch,
    // Switch value.
    C_AST_STRUCT_CHILD(stmt_switch, expr, value)
    // Body.
    C_AST_STRUCT_CHILD(stmt_switch, stmt, body)
)

// Case label (e.g. `case 1:` or `case 1 ... 5:`).
C_AST_STRUCT_DEF(stmt_case,
    // Case value, or lower limit of a GCC case range.
    C_AST_STRUCT_CHILD(stmt_case, expr, lo)
    // Upper limit of a GCC case range (nullable).
    C_AST_STRUCT_CHILD(stmt_case, expr, hi)
)

// Named label (e.g. `foo:`).
C_AST_STRUCT_DEF(stmt_label,
    // Label name.
    C_AST_STRUCT_CHILD(stmt_label, ident, name)
    // Labeled statement.
    C_AST_STRUCT_CHILD(stmt_label, stmt, body)
)

// Return statement.
C_AST_STRUCT_DEF(stmt_return,
    // Return value (nullable for `return;`).
    C_AST_STRUCT_CHILD(stmt_return, expr_list, value)
)

// Goto statement.
C_AST_STRUCT_DEF(stmt_goto,
    // Target label.
    C_AST_STRUCT_CHILD(stmt_goto, ident, target)
)

// One statement; tagged union of the variants above.
C_AST_UNION_DEF(stmt,
    C_AST_UNION_CHILD(stmt, stmt_list,    stmts,    STMTS)
    C_AST_UNION_CHILD(stmt, stmt_for,     for,     STMT_FOR)
    C_AST_UNION_CHILD(stmt, stmt_while,   while,   STMT_WHILE)
    C_AST_UNION_CHILD(stmt, stmt_if,      if,      STMT_IF)
    C_AST_UNION_CHILD(stmt, stmt_switch,  switch,  STMT_SWITCH)
    C_AST_UNION_CHILD(stmt, stmt_case,    case,    STMT_CASE)
    C_AST_UNION_CHILD(stmt, stmt_label,   label,   STMT_LABEL)
    C_AST_UNION_CHILD(stmt, stmt_return,  return,  STMT_RETURN)
    C_AST_UNION_CHILD(stmt, stmt_goto,    goto,    STMT_GOTO)
    C_AST_UNION_CHILD(stmt, expr_list,    expr,    STMT_EXPR)
    C_AST_UNION_CHILD(stmt, def,          def,     STMT_DEF)
    C_AST_UNION_CHILD(stmt, garbage,      garbage, STMT_GARBAGE)
)

// List of statements.
C_AST_LIST_DEF(stmt)



#undef C_AST_DEF
#undef C_AST_UNION_DEF
#undef C_AST_UNION_CHILD
#undef C_AST_UNION_FIELD
#undef C_AST_STRUCT_DEF
#undef C_AST_STRUCT_CHILD
#undef C_AST_STRUCT_FIELD
#undef C_AST_LIST_DEF
