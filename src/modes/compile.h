
#pragma once

#include <asm.h>
#include <gen.h>
#include <parser-util.h>

// Run in compilation/linking mode.
int mode_compile(int argc, char **argv);

// Parse -m arguments, the '-m' removed.
// Returns true on success.
bool machine_argparse(const char *arg);
// Parse -f arguments, the '-f' removed.
// Returns true on success.
bool flag_argparse   (const char *arg);

// Compile a file of unknown type.
asm_ctx_t *compile       (char *filename, tokeniser_ctx_t *tkn_ctx);
// Compile a C source file.
asm_ctx_t *compile_c     (char *filename, tokeniser_ctx_t *tkn_ctx);
// Assembles an assembly source file.
asm_ctx_t *assemble_s    (char *filename, tokeniser_ctx_t *tkn_ctx);

// Bison tokeniser callback.
int  yylex  (parser_ctx_t *ctx);
// Bison error callback.
void yyerror(parser_ctx_t *ctx, char *msg);

// Compile a function after parsing.
void function_added(parser_ctx_t *ctx, funcdef_t *func);
