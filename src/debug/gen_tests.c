
#include "gen_tests.h"
#include <asm.h>
#include <asm_postproc.h>
#include <gen.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

static void gen_test_expr();
static void gen_test_func();

void perform_gen_tests(int argc, char **argv) {
	#ifdef EXPR_TEST
	gen_test_expr();
	#endif
	#ifdef FUNC_TEST
	gen_test_func();
	#endif
}

static void gen_test_expr() {
	asm_ctx_t asm_ctx;
	asm_init(&asm_ctx);
	/* The test code:
	var a, b;

	a = b + 3;
	*/
	ident_t ident_a = {
		.strval = "a"
	};
	ident_t ident_b = {
		.strval = "b"
	};
	
	expr_t var_a = {
		.type = EXPR_TYPE_IDENT,
		.ident = &ident_a
	};
	expr_t var_b = {
		.type = EXPR_TYPE_IDENT,
		.ident = &ident_b
	};
	expr_t const_val = {
		.type = EXPR_TYPE_CONST,
		.iconst = 3
	};
	
	expr_t addition = {
		.type = EXPR_TYPE_MATH2,
		.par_a = &var_b,
		.par_b = &const_val,
		.oper = OP_ADD
	};
	expr_t ex = {
		.type = EXPR_TYPE_MATH2,
		.par_a = &var_a,
		.par_b = &addition,
		.oper = OP_ASSIGN
	};
	
	//gen_var_t *hint = gen_expression(&asm_ctx, &var_a, NULL);
	//gen_var_t *out = gen_expression(&asm_ctx, &addition, hint);
	
	gen_var_t *out = gen_expression(&asm_ctx, &ex, NULL);
}

static void gen_test_func() {
	asm_ctx_t asm_ctx;
	asm_init(&asm_ctx);
	/* The test code:
	func epicfunction(a, b) {
		return a + b;
	}
	*/
	
	ident_t func_args[2] = {
		{
			.strval = "a"
		},
		{
			.strval = "b"
		}
	};
	funcdef_t func = {
		.ident = { .strval = "epicfunction" },
		.args  = { .num = 2, .arr = func_args }
	};
	
	expr_t var_a = {
		.type = EXPR_TYPE_IDENT,
		.ident = &func_args[0]
	};
	expr_t var_b = {
		.type = EXPR_TYPE_IDENT,
		.ident = &func_args[1]
	};
	expr_t addition = {
		.type = EXPR_TYPE_MATH2,
		.par_a = &var_a,
		.par_b = &var_b,
		.oper = OP_ADD
	};
	
	stmt_t stmt = {
		.type = STMT_TYPE_RET,
		.expr = &addition
	};
	func.stmts = &(stmts_t) {
		.num = 1,
		.arr = &stmt
	};
	
	gen_function(&asm_ctx, &func);
	int tempfile = open("/tmp/lilly-cc-dbg-bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
	asm_ctx.out_fd = tempfile;
	asm_ppc(&asm_ctx);
	output_native(&asm_ctx);
	close(tempfile);
	system("/usr/bin/xxd -g 1 /tmp/lilly-cc-dbg-bin");
}
