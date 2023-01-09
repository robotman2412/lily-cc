
#include "main.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <asm.h>
#include <config.h>
#include "tokeniser.h"
#include "parser.h"
#include <gen.h>
#include "asm_postproc.h"
#include "modes.h"

#include "ctxalloc.h"

#include "stdlib.h"
#include "errno.h"

#if __WORDSIZE < WORD_BITS
#warning "The target has a larger word size than the current machine, the compiler might not be able to handle it."
#endif

// Filling in of an external array.
char *reg_names[] = REG_NAMES;

// Filling in of another external array.
size_t simple_type_size[] = arrSSIZE_BY_INDEX;

int main(int argc, char **argv) {
	alloc_init();
	
	// Check for explicit mode switches.
	if (argc >= 2 && !strcmp(argv[1], "--mode=addr2line")) {
		argv[1] = argv[0];
		return mode_addr2line(argc-1, argv+1);
		
	} else if (argc >= 2 && !strcmp(argv[1], "--mode=compile")) {
		argv[1] = argv[0];
		return mode_addr2line(argc-1, argv+1);
		
	}
	
	// Check for mode by name.
	if (strlen(*argv) >= 9 && !strcmp(*argv + strlen(*argv) - 9, "addr2line")) {
		return mode_addr2line(argc, argv);
	} else {
		return mode_compile(argc, argv);
	}
}


// Check wether a file exists and is a directory.
bool isdir(char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) != 0) return 0;
	return S_ISDIR(statbuf.st_mode);
}
