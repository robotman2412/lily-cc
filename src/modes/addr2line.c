
#include "addr2line.h"
#include "array_util.h"
#include "main.h"
#include "errno.h"
#include "stdlib.h"



#define A2L_ACCEPT_TYPE "abcdefghijklmnopqrtsuvwxyz_0123456789"

typedef struct {
	// Show the command-line help text.
	bool       showHelp;
	// Show the version number.
	bool       showVersion;
	// Abort by exiting with code 1,
	bool       abort;
	// Filename to use for for linenumber information.
	char      *exeFile;
	// Number of addresses to interpret.
	size_t     addrCount;
	// Addresses to interpret.
	address_t *addrs;
} options_t;

// Attempt to convert hexadecimal string into unsigned long long.
// Returns true on success.
static bool unhex(const char *raw, unsigned long long *out) {
	*out = 0;
	
	while (*raw) {
		*out <<= 4;
		
		if (*raw >= '0' && *raw <= '9') {
			*out |= *raw - '0';
		} else if (*raw >= 'A' && *raw <= 'F') {
			*out |= *raw - 'A' + 0x0A;
		} else if (*raw >= 'a' && *raw <= 'f') {
			*out |= *raw - 'a' + 0x0A;
		} else {
			*out = 0;
			return false;
		}
		
		raw ++;
	}
	
	return true;
}

// Parse arguments for addr2line mode.
static void parse_options(options_t *options, int argc, char **argv) {
	// Set defaults.
	*options = (options_t) {
		.showHelp    = false,
		.showVersion = false,
		.abort       = false,
		.exeFile     = NULL,
		.addrCount   = 0,
		.addrs       = NULL,
	};
	
	// Iterate argv.
	for (int argIndex = 1; argIndex < argc; argIndex ++) {
		if (!strcmp(argv[argIndex], "-V") || !strcmp(argv[argIndex], "--version")) {
			// Show version.
			options->showVersion = true;
			
		} else if (!strcmp(argv[argIndex], "-H") || !strcmp(argv[argIndex], "--help")) {
			// Show help.
			options->showHelp = true;
			
		} else if (!strcmp(argv[argIndex], "-e")) {
			// Executable file.
			if (options->exeFile != NULL) {
				printf("Error: An executable file was already specified.\n");
				options->abort = true;
				
			} else if (argIndex + 1 >= argc) {
				printf("Error: No filename to match '-e'.\n");
				options->abort = true;
				
			} else {
				options->exeFile = argv[++argIndex];
			}
			
		} else if (!strncmp(argv[argIndex], "--exe=", 6)) {
			// Executable file.
			if (options->exeFile != NULL) {
				printf("Error: An executable file was already specified.\n");
				options->abort = true;
				
			} else {
				options->exeFile = argv[argIndex] + 6;
			}
			
		} else if (*argv[argIndex] == '-') {
			// Unrecognised option.
			printf("Error: Invalid option: '%s'.\n", argv[argIndex]);
			options->abort = true;
			
		} else {
			// Assume an address.
			// Extract raw number string.
			char *raw = argv[argIndex];
			if (!strncasecmp(argv[argIndex], "0x", 2)) {
				raw += 2;
			}
			
			// Attempt to parse hex.
			unsigned long long num;
			if (!unhex(raw, &num)) {
				printf("Error: Not a hexadecimal number: '%s'.\n", argv[argIndex]);
				options->abort = true;
				continue;
			}
			
			// Add to the great big address list.
			array_len_concat(global_alloc, address_t, options->addrs, options->addrCount, num);
		}
	}
	
	if (!options->exeFile) {
		options->exeFile = "a.out";
	}
}

// Show help on the command line.
static void show_help(int argc, char **argv) {
	printf("%s [--mode=...] [options] address...\n", *argv);
	printf("Options:\n");
	printf("  --mode=<compile|addr2line>\n");
	printf("                Specify the application mode, default is compile, current is addr2line.\n");
	printf("  -V  --version\n");
	printf("                Show the version.\n");
	printf("  -H  --help\n");
	printf("                Show this list.\n");
	printf("  -e filename --exe=filename\n");
	printf("                Specify the file to use for linenumber information.\n");
}

// Addr2line / linenumber dump mode.
int mode_addr2line(int argc, char **argv) {
	options_t options;
	parse_options(&options, argc, argv);
	
	if (options.showHelp) {
		printf("lily-addr2line " ARCH_ID " " COMPILER_VER "\n");
		show_help(argc, argv);
		return options.abort;
	}
	if (options.showVersion) {
		printf("lily-addr2line " ARCH_ID " " COMPILER_VER "\n");
	}
	if (options.abort) {
		return 1;
	}
	
	// Open input file.
	FILE *fd = fopen(options.exeFile, "rb");
	if (!fd) {
		printf("Cannot open %s: %s\n", options.exeFile, strerror(errno));
		return 1;
	}
	
	// Try to parse it.
	a2l_info_t info = mode_addr2line_read(fd, global_alloc);
	if (!info.valid) {
		printf("%s: Cannot read linenumber information\n", options.exeFile);
		fclose(fd);
		return 1;
	}
	
	// Locate addresses.
	for (size_t i = 0; i < options.addrCount; i++) {
		mode_addr2line_report(&info, options.addrs[i]);
	}
	
}



// Cleans up an instance of a2l_info_t *.
void a2l_info_free(a2l_info_t *mem) {
	// Clean up pos list.
	for (size_t i = 0; i < mem->pos_count; i++) {
		// Clean up filenames from pos list.
		xfree(mem->allocator, (void *) mem->pos_list[i].rel_path);
		xfree(mem->allocator, (void *) mem->pos_list[i].abs_path);
	}
	xfree(mem->allocator, mem->pos_list);
	
	// Clean up section map.
	for (size_t i = 0; i < mem->sect_map.numEntries; i++) {
		// Clean up names from section map.
		a2l_sect_t *sect = (a2l_sect_t *) mem->sect_map.values[i];
		xfree(mem->allocator, (void *) sect->name);
		xfree(mem->allocator, (void *) sect);
	}
	map_delete(&mem->sect_map);
	
	// Clean up label map.
	map_delete(&mem->label_map);
}


// Discards characters from `fd` until a newline is encountered.
static void read_until_newline(FILE *fd) {
	while (1) {
		int c = fgetc(fd);
		if (c == EOF) return;
		if (c == '\r') {
			c = fgetc(fd);
			if (c != '\n') {
				fseek(fd, -1, SEEK_CUR);
			}
			return;
		} else if (c == '\n' || c == -1) {
			return;
		}
	}
}

// Dynamically reads a string from `fd` using `allocator`.
static char *read_string_until_whitespace(alloc_ctx_t allocator, FILE *fd) {
	char *arr = NULL;
	size_t len = 0;
	size_t cap = 0;
	
	while (1) {
		int c = fgetc(fd);
		
		if (c == '\r') {
			c = fgetc(fd);
			if (c != '\n') {
				fseek(fd, -1, SEEK_CUR);
			}
			break;
		} else if (c == ' ' || c == '\t' || c == '\n' || c == -1) {
			break;
		} else if (c == '\\') {
			c = fgetc(fd);
		}
		
		array_len_cap_concat(allocator, char, arr, cap, len, c);
	}
	
	array_len_cap_concat(allocator, char, arr, cap, len, 0);
	return arr;
}

// Determine whether a string contains non-printable characters.
// Assumes >=0x7f is non-printable.
static bool str_non_printable(const char *str) {
	while (*str) {
		if (*str < 0x20 || *str > 0x7e) return true;
		str ++;
	}
	return false;
}

// Comparator for sorting positions list by address.
// Returns a.addr - b.addr.
static int a2l_pos_addr_cmp(const void *a, const void *b) {
	long long diff = ((a2l_pos_t *) a)->addr - ((a2l_pos_t *) b)->addr;
	if (diff > 0) return 1;
	if (diff < 0) return -1;
	return 0;
}

// Dumps linenumbering information from a previously generated addr2line file.
// Creates a set of maps representing the stored information.
a2l_info_t mode_addr2line_read(FILE *fd, alloc_ctx_t allocator) {
	a2l_info_t out;
	out.valid      = true;
	out.pos_list   = NULL;
	out.pos_count  = 0;
	size_t pos_cap = 0;
	map_create(&out.label_map);
	map_create(&out.sect_map);
	
	// Attempt to iterate lines.
	while (true) {
		// Get type.
		char type_tmp[16];
		int success = fscanf(fd, "%15s ", type_tmp);
		
		if (!strcmp(type_tmp, "pos")) {
			// Parse 'pos' type.
			// Scan paths.
			a2l_pos_t pos = {
				.abs_path = read_string_until_whitespace(allocator, fd),
				.rel_path = read_string_until_whitespace(allocator, fd),
			};
			
			// Scan numbers.
			unsigned long long addr;
			int success = fscanf(fd, "%llx %d,%d %d,%d", &addr, &pos.pos.x0, &pos.pos.y0, &pos.pos.x1, &pos.pos.y1);
			pos.addr = addr;
			
			// Check for scanf success.
			if (success == 5) {
				// Store into array.
				array_len_cap_concat(allocator, a2l_pos_t, out.pos_list, pos_cap, out.pos_count, pos);
				
			} else {
				// Clean up.
				xfree(allocator, pos.abs_path);
				xfree(allocator, pos.rel_path);
			}
			
		} else if (!strcmp(type_tmp, "sect")) {
			// Parse 'sect' type.
			// Scan name.
			char *name = read_string_until_whitespace(allocator, fd);
			
			// Scan numbers.
			unsigned long long addr, size, align;
			int success = fscanf(fd, "%llx %llx %llx", &addr, &size, &align);
			
			// Check for scanf success.
			if (success == 3) {
				// Allocate section data.
				a2l_sect_t *sect = xalloc(allocator, sizeof(a2l_sect_t));
				*sect = (a2l_sect_t) {
					.name  = name,
					.addr  = addr,
					.size  = size,
					.align = align,
				};
				// Store in map.
				map_set(&out.sect_map, name, sect);
				
			} else {
				// Clean up.
				xfree(allocator, name);
			}
			
		} else if (!strcmp(type_tmp, "label")) {
			// Parse 'label' type.
			// Scan name.
			char *name = read_string_until_whitespace(allocator, fd);
			
			// Scan address.
			unsigned long long addr;
			int success = fscanf(fd, "%llx", &addr);
			
			// Scheck for scanf success.
			if (success == 1) {
				// Store in label map.
				map_set(&out.label_map, name, (void*) addr);
			}
			
			// Clean up.
			xfree(allocator, name);
			
		} else if (strspn(type_tmp, A2L_ACCEPT_TYPE) < strlen(type_tmp)) {
			// It has wrong chars, assume something went wrong.
			out.valid = false;
			
		}
		
		read_until_newline(fd);
		if (success != 1) break;
	}
	
	if (out.valid) {
		// Sort positions list by addresses.
		qsort(out.pos_list, out.pos_count, sizeof(a2l_pos_t), a2l_pos_addr_cmp);
	}
	
	return out;
}

// Report found linenumber for given address.
void mode_addr2line_report(a2l_info_t *info, address_t addr) {
	// Test whether the address lies in a known section.
	bool sect_found = false;
	for (size_t i = 0; i < info->sect_map.numEntries; i++) {
		a2l_sect_t *sect = (a2l_sect_t *) info->sect_map.values[i];
		if (addr >= sect->addr && addr < sect->addr + sect->size) {
			sect_found = true;
			break;
		}
	}
	
	// If not in a known section, assume unknown address.
	if (!sect_found) {
		printf("??:0\n");
		return;
	}
	
	// Look for closest address.
	a2l_pos_t addr_tmp = { .addr = addr };
	a2l_pos_t *closest = array_find_closest(a2l_pos_t, info->pos_list, info->pos_count, a2l_pos_addr_cmp, addr_tmp);
	
	// If closest has smaller address, increment the index.
	if (closest && closest->addr < addr) {
		closest ++;
		if (closest >= &info->pos_list[info->pos_count]) closest = NULL;
	}
	
	// If there is a close enough match, report findings.
	if (closest) {
		printf("%s:%d\n", closest->rel_path, closest->pos.y0);
	} else {
		printf("??:0\n");
	}
}
