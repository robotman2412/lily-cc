
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
}

static void gen_test_func() {
}
