
#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "definitions.h"
#include "tokeniser.h"
#include "main.h"
#include "asm.h"
#include "gen_preproc.h"
#include "ctxalloc.h"

// All context required for the parser to function.
// Stores all data relevant to a translation unit.
struct parser_ctx {
	// Context for tokeniser functions.
	tokeniser_ctx_t *tokeniser_ctx;
	// Context for asm_ and gen_ functions.
	asm_ctx_t       *asm_ctx;
	
	// The number of constants written to .rodata so far.
	size_t           n_const;
	// Memory allocator to use.
	alloc_ctx_t      allocator;
	// Most recently used simple type.
	simple_type_t    s_type;
};

// Integer constant; mostly used in expressions.
struct ival {
	// File position of this object.
	pos_t pos;
	// Integer constant, mostly used in expressions.
	int   ival;
};

// String constant; used either in ident_t or strval_t.
struct strval {
	// File position of this object.
	pos_t pos;
	// String constant, used either in ident_t or strval_t.
	char *strval;
};

// Identity; symbol; mostly used when referring to variables and functions.
struct ident {
	// File position of this object.
	pos_t       pos;
	// Name of the identity.
	char       *strval;
	// Type associated, if any.
	var_type_t *type;
	// The value to assign, if any.
	expr_t     *initialiser;
};

// Expression; used to represent math operations and alike.
struct expr {
	// File position of this object.
	pos_t       pos;
	
	// Type of expression (math1, math2, call, etc.).
	expr_type_t type;
	// Operator for math expressions.
	oper_t      oper;
	
	union {
		// Integer constant EXPR_TYPE_CONST.
		long      iconst;
		// Label reference for EXPR_TYPE_CSTR.
		char     *label;
		// Variable reference for EXPR_TYPE_IDENT.
		strval_t *ident;
		// Function for EXPR_TYPE_CALL.
		expr_t   *func;
		// Parameter for EXPR_TYPE_MATH2 and EXPR_TYPE_MATH1.
		expr_t   *par_a;
	};
	
	union {
		// Parameter for EXPR_TYPE_MATH2.
		expr_t  *par_b;
		// Arguments for EXPR_TYPE_CALL.
		exprs_t *args;
	};
	
	// Whether this expression uses pointers.
	bool uses_pointers;
	// Whether this expression has side effects.
	bool has_side_effects;
	// The amount of expressions going down, including this one.
	size_t operation_count;
};

// Statement; used to represent actions to perform, often evaluation of expressions.
struct stmt {
	// File position of this object.
	pos_t           pos;
	
	// Type of statement (for, while, expression, etc.)
	stmt_type_t     type;
	
	union {
		// A list of statements for STMT_TYPE_MULTI.
		stmts_t    *stmts;
		
		struct {
			// Condition for STMT_TYPE_IF and STMT_TYPE_WHILE.
			expr_t *cond;
			// Code on true for STMT_TYPE_IF, loop code for STMT_TYPE_WHILE.
			stmt_t *code_true;
			// Code on false for STMT_TYPE_IF.
			stmt_t *code_false;
		};
		
		struct {
			// Initialiser for STMT_TYPE_FOR.
			stmt_t  *for_init;
			// Condition for STMT_TYPE_FOR.
			exprs_t *for_cond;
			// Increment for STMT_TYPE_FOR.
			exprs_t *for_next;
			// Loop code for STMT_TYPE_FOR.
			stmt_t  *for_code;
		};
		
		// Expression for STMT_TYPE_EXPR and STMT_TYPE_RET.
		expr_t     *expr;
		// Variable declarations for STMT_TYPE_VAR.
		idents_t   *vars;
		// Inline assembly data for STMT_TYPE_IASM.
		iasm_t     *iasm;
	};
	
	// Preprocessor data for statements.
	preproc_data_t *preproc;
};

// Qualifiers list for inline assembly statements.
struct iasm_qual {
	// File position of this object.
	pos_t           pos;
	
	// Whether the volatile keyword is used.
	bool            is_volatile;
	// Whether the inline keyword is used.
	bool            is_inline;
	// Whether the goto keyword is used.
	bool            is_goto;
};

// Variable reference for inline assembly statements.
struct iasm_reg {
	// File position of this object.
	pos_t           pos;
	// Operand symbol (e.g. [MyLabel] or [fancy_name])
	char           *symbol;
	// Operand constraint string (e.g. "=r" or "+m").
	char           *mode;
	// Operand expression (e.g. a, *b or c[d])
	expr_t         *expr;
	// Result of expression.
	gen_var_t      *expr_result;
	
	/* ==== simple constraints ==== */
	
	// The operand is allowed to be a general register.
	bool            mode_register;
	// The operand is allowed to be any type of memory address which
	// the machine can use on all normal instructions that interact with memory.
	bool            mode_memory;
	// A constant operand with a currently unknown value is allowed.
	bool            mode_unknown_const;
	// A constant operand is allowed only if it's value is currently known.
	bool            mode_known_const;
	
	/* ========= modifiers ======== */
	
	// Whether the operand is accessed in a read-fashion.
	bool            mode_read;
	// Whether the operand is accessed in a write-fashion.
	bool            mode_write;
	// The operand is written before the instruction is finished, and it may not be an input to it.
	bool            mode_early_clobber;
	// The operand is commutative with it's immediate successor, so they may be swapped.
	bool            mode_commutative_next;
	// Also commutative, but to it's predecessor instead.
	bool            mode_commutative_prev;
};

// List of iasm_reg_t.
struct iasm_regs {
	// File position of this object.
	pos_t           pos;
	
	// The amount of iasm_reg_t present in arr.
	size_t          num;
	// The referenced variables.
	iasm_reg_t     *arr;
};

// Inline assembly object; typically used for processor specific behaviour.
struct iasm {
	// File position of this object.
	pos_t           pos;
	
	// The text used to derive the assembly.
	strval_t        text;
	// The input variables.
	iasm_regs_t    *inputs;
	// The output variables.
	iasm_regs_t    *outputs;
	// The clobbered variables and/or registers.
	// iasm_strs_t    *clobbers;
	
	// The qualifiers attached to the asm statement.
	iasm_qual_t     qualifiers;
};

// List of ident_t.
struct idents {
	// File position of this object.
	pos_t           pos;
	
	// The amount of ident_t presen in arr.
	size_t          num;
	// The symbol references stored.
	ident_t        *arr;
};

// List of expr_t.
struct exprs {
	// File position of this object.
	pos_t           pos;
	
	// The amount of expr_t stored in arr.
	size_t          num;
	// The expressions stored.
	expr_t         *arr;
};

// List of stmt_t.
struct stmts {
	// File position of this object.
	pos_t           pos;
	
	// The amount of stmt_t stored in arr.
	size_t          num;
	// The statements stored.
	stmt_t         *arr;
};

// Function declaration and/or implementation.
struct funcdef {
	// File position of this object.
	pos_t           pos;
	
	// Return type.
	var_type_t     *returns;
	// Function name.
	strval_t        ident;
	// Arguments list.
	idents_t        args;
	// Code, if that is defined.
	stmts_t        *stmts;
	
	// Preprocessor data.
	preproc_data_t *preproc;
};

#endif // PARSER_UTIL_H
