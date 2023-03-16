
#ifndef SYNTAXTREE_H
#define SYNTAXTREE_H

#include <vector>
#include <map>
#include <string>
#include <definitions.h>

namespace syntax {

struct SimpleType {
	// Is this const?
	bool isConst;
	// Simple type this represents.
	simple_type_t type;
};

struct Expression {
	enum Type {
		// Unary math like `++lvalue` and `*pointer`.
		EXPR_MATH1,
		// Binary math like `rvalue + rvalue` and `array[index]`.
		EXPR_MATH2,
		// Function calls.
		EXPR_CALL,
		// Variable and/or label reference.
		EXPR_LABEL,
		// Numeric constants.
		EXPR_ICONST,
		// String constants.
		EXPR_SCONST,
	};
	
	// Position in source file.
	// First member of all types in abstract syntax tree.
	pos_t pos;
	
	// What type of expression is stored.
	Type type;
	// Expressions for EXPR_MATH* and EXPR_CALL.
	std::vector<Expression> exprs;
	// Value of numeric constant for EXPR_ICONST.
	long long ival;
	// Value of string constant for EXPR_SCONST,
	// Label name for EXPR_LABEL.
	std::string strval;
};

} // namespace syntax

#endif // SYNTAXTREE_H
