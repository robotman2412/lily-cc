
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
	char *outputType;
};

// Check wether a file exists and is a directory.
bool       isdir         (char *path);
// Show help on the command line.
void       show_help     (int argc, char **argv);
// Apply default options for options not already set.
void       apply_defaults(options_t *options);
// Process a function.
void       function_added(parser_ctx_t *ctx, funcdef_t *func);

// Compile a file of unknown type.
asm_ctx_t *compile       (char *filename, FILE *file);
// Compile a C source file.
asm_ctx_t *compile_c     (char *filename, FILE *file);
// Assembles an assembly source file.
asm_ctx_t *assemble_s    (char *filename, FILE *file);

// Bison tokeniser callback.
int        yylex         (parser_ctx_t *ctx);
// Bison error callback.
void       yyerror       (parser_ctx_t *ctx, char *msg);

#endif //MAIN_H
