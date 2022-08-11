
#ifndef MAIN_H
#define MAIN_H

struct options;

typedef struct options options_t;

#include <config.h>

extern char *reg_names[];

#include <stddef.h>
#include <stdbool.h>
#include <parser-util.h>
#include <tokeniser.h>
#include <asm.h>

struct options {
	bool abort;
	bool showHelp;
	bool showVersion;
	int numSourceFiles;
	char **sourceFiles;
	int numIncludeDirs;
	char **includeDirs;
	char *outputFile;
};

// Check wether a file exists and is a directory.
bool       isdir         (char *path);
// Show help on the command line.
void       show_help     (int argc, char **argv);
// Apply default options for options not already set.
void       apply_defaults(options_t *options);
// Compile a function after parsing.
void       function_added(parser_ctx_t *ctx, funcdef_t *func);

// Parse -m arguments, the '-m' removed.
// Returns true on success.
bool       machine_argparse(const char *arg);
// Parse -f arguments, the '-f' removed.
// Returns true on success.
bool       flag_argparse (const char *arg);

// Compile a file of unknown type.
asm_ctx_t *compile       (char *filename, tokeniser_ctx_t *tkn_ctx);
// Compile a C source file.
asm_ctx_t *compile_c     (char *filename, tokeniser_ctx_t *tkn_ctx);
// Assembles an assembly source file.
asm_ctx_t *assemble_s    (char *filename, tokeniser_ctx_t *tkn_ctx);

// Bison tokeniser callback.
int        yylex         (parser_ctx_t *ctx);
// Bison error callback.
void       yyerror       (parser_ctx_t *ctx, char *msg);

#endif //MAIN_H
