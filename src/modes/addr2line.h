
#pragma once

typedef struct a2l_info  a2l_info_t;
typedef struct a2l_pos   a2l_pos_t;
typedef struct a2l_sect  a2l_sect_t;

#include <stdint.h>
#include <stdio.h>
#include <strmap.h>
#include <parser-util.h>
#include <config.h>

struct a2l_info {
	// Indicates whether the file is a valid linenumbers dump.
	bool        valid;
	// Allocator used to create this info.
	alloc_ctx_t allocator;
	// Map of label name to address_t.
	map_t       label_map;
	// List of positions.
	a2l_pos_t  *pos_list;
	// Amount of positions stored.
	size_t      pos_count;
	// Map of section name to a2l_sect_t.
	map_t       sect_map;
};

struct a2l_pos {
	// Absolute filename.
	char     *abs_path;
	// Relative filename.
	char     *rel_path;
	// Position information encoded.
	// Uses relative filename.
	pos_t     pos;
	// Address of position.
	address_t addr;
};

struct a2l_sect {
	// Name of the section.
	const char *name;
	// Starting address of section.
	address_t   addr;
	// Size in memory words of section.
	address_t   size;
	// Alignment of section.
	address_t   align;
};

// Cleans up an instance of a2l_info_t *.
void a2l_info_free(a2l_info_t *mem);

// Dumps linenumbering information from a previously generated addr2line file.
// Creates a set of maps representing the stored information.
a2l_info_t mode_addr2line_read(FILE *fd, alloc_ctx_t allocator);
// Report found linenumber for given address.
void mode_addr2line_report(a2l_info_t *info, address_t addr);

// Addr2line / linenumber dump mode.
int mode_addr2line(int argc, char **argv);
