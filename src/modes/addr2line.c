
#include "addr2line.h"
#include "array_util.h"
#include "main.h"

// Cleans up an instance of a2l_info_t *.
void a2l_info_free(a2l_info_t *mem) {
	// Clean up pos list.
	for (size_t i = 0; i < mem->pos_count; i++) {
		// Clean up filenames from pos list.
		xfree(mem->allocator, mem->pos_list[i].rel_path);
		xfree(mem->allocator, mem->pos_list[i].abs_path);
	}
	xfree(mem->allocator, mem->pos_list);
	
	// Clean up section map.
	for (size_t i = 0; i < mem->sect_map.numEntries; i++) {
		// Clean up names from section map.
		a2l_sect_t *sect = mem->sect_map.values[i];
		xfree(mem->allocator, sect->name);
	}
	map_delete(&mem->sect_map);
	
	// Clean up label map.
	map_delete(&mem->label_map);
}

// Discards characters from `fd` until a newline is encountered.
void read_until_newline(FILE *fd) {
	while (1) {
		int c = fgetc(fd);
		if (c == EOF) return;
		if (c == '\r') {
			c = fgetc(fd);
			if (c != '\n') {
				fseek(fd, -1, SEEK_CUR);
			}
			return;
		} else if (c == '\n') {
			return;
		}
	}
}

// Dynamically reads a string from `fd` using `allocator`.
char *read_string_until_whitespace(alloc_ctx_t allocator, FILE *fd) {
	char *arr = NULL;
	size_t len = 0;
	size_t cap = 0;
	
	while (1) {
		char c = fgetc(fd);
		
		if (c == '\r') {
			c = fgetc(fd);
			if (c != '\n') {
				fseek(fd, -1, SEEK_CUR);
			}
			break;
		} else if (c == ' ' || c == '\t' || c == '\n') {
			break;
		} else if (c == '\\') {
			c = fgetc(fd);
		}
		
		array_len_cap_concat(allocator, char, arr, cap, len, c);
	}
	
	return arr;
}

// Dumps linenumbering information from a previously generated addr2line file.
// Creates a set of maps representing the stored information.
a2l_info_t mode_addr2line_read(FILE *fd, alloc_ctx_t allocator) {
	a2l_info_t out;
	out.pos_list   = NULL;
	out.pos_count  = 0;
	size_t pos_cap = 0;
	
	// Attempt to iterate lines.
	while (true) {
		// Get type.
		char type_tmp[16];
		int success = fscanf(fd, "%15s", type_tmp);
		
		// Parse 'pos' type.
		if (!strcmp(type_tmp, "pos")) {
			a2l_pos_t pos = {
				.abs_path = read_string_until_whitespace(allocator, fd),
				.rel_path = read_string_until_whitespace(allocator, fd),
			};
			
			// Needs a warning fix
			int success = fscanf(fd, "%x %d,%d %d,%d", &pos.addr, &pos.pos.x0, &pos.pos.y0, &pos.pos.x1, &pos.pos.y1);
			if (success == 5) {
				array_len_cap_concat(allocator, a2l_pos_t, out.pos_list, out.pos_count, pos_cap, pos);
			}
		}
		
		// Parse 'sect' type.
		if (!strcmp(type_tmp, "sect")) {
			
		}
		
		// Parse 'label' type.
		if (!strcmp(type_tmp, "label")) {
			
		}
		
		read_until_newline(fd);
	}
	
	return out;
}
