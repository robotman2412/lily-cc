
#include "main.h"
#include "asm_postproc.h"
#include "ansi_codes.h"

// Entrypoint label name.
// If not null, the entry vector is set to this label.
const char *entrypoint = NULL;
const char *irqvector  = NULL;
const char *nmivector  = NULL;

// Parse -m arguments, the '-m' removed.
// Returns true on success.
bool machine_argparse(const char *arg) {
    if (!strncmp(arg, "entrypoint", 10)) {
        if (arg[10] != '=' || !arg[11]) {
            // Missing required argument.
            printf(ANSI_RED_FG "Error: Option '-mentrypoint' requires '=<label name>'!" ANSI_DEFAULT "\n");
            return false;
        } else {
            // Set entrypoint.
            entrypoint = arg + 11;
            return true;
        }
        
    } else if (!strncmp(arg, "irqhandler", 10)) {
        if (arg[10] != '=' || !arg[11]) {
            // Missing required argument.
            printf(ANSI_RED_FG "Error: Option '-mirqhandler' requires '=<label name>'!" ANSI_DEFAULT "\n");
            return false;
        } else {
            // Set entrypoint.
            irqvector = arg + 11;
            return true;
        }
        
    } else if (!strncmp(arg, "nmihandler", 10)) {
        if (arg[10] != '=' || !arg[11]) {
            // Missing required argument.
            printf(ANSI_RED_FG "Error: Option '-mnmihandler' requires '=<label name>'!" ANSI_DEFAULT "\n");
            return false;
        } else {
            // Set entrypoint.
            nmivector = arg + 11;
            return true;
        }
        
    } else {
        // Unknown option.
        fflush(stdout);
        fprintf(stderr, ANSI_RED_FG "Error: Unknown option '-m%s'!" ANSI_DEFAULT "\n", arg);
    }
}

static inline void output_native_padd(FILE *fd, address_t n) {
	char *buf = malloc(256);
	memset(buf, 0, 256);
	while (n > 256) {
		// Write a bit at a time.
		fwrite(buf, 1, 256, fd);
		n -= 256;
	}
	if (n) {
		fwrite(buf, 1, n, fd);
	}
	free(buf);
}
