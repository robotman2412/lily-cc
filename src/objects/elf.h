
#ifndef ELF_H
#define ELF_H

struct elf32_header;
struct elf32_progheader;

typedef struct elf32_header     elf32_header_t;
typedef struct elf32_progheader elf32_progheader_t;

#define ELF_BITS_32       1
#define ELF_BITS_64       2

#define ELF_LITTLE_ENDIAN 1
#define ELF_BIG_ENDIAN    2

#define ELF_OSABI         0
#define ELF_ABIVER        3

#define ELF_TYPE_NONE     0
#define ELF_TYPE_REL      1
#define ELF_TYPE_EXEC     2
#define ELF_TYPE_DYN      3
#define ELF_TYPE_CORE     4

#define ELF_VERSION       1

#define ELF_PROG_NULL     0
#define ELF_PROG_LOAD     1
#define ELF_PROG_DYNAMIC  2
#define ELF_PROG_INTERP   3
#define ELF_PROG_NOTE     4
#define ELF_PROG_SHLIB    5
#define ELF_PROG_PROGTAB  6
#define ELF_PROG_TLS      7

#define ELF_SECT_NULL     0
#define ELF_SECT_PROGBITS 1
#define ELF_SECT_SYMTAB   2
#define ELF_SECT_STRTAB   3
#define ELF_SECT_RELA     4
#define ELF_SECT_HASH     5
#define ELF_SECT_DYNAMIC  6
#define ELF_SECT_NOTE     7
#define ELF_SECT_NOBITS   8
#define ELF_SECT_REL      9
#define ELF_SECT_SHLIB    10
#define ELF_SECT_DYNSYM   11
#define ELF_SECT_INIT_ARR 14
#define ELF_SECT_FINI_ARR 15
#define ELF_SECT_PREINIT_ARR 16
#define ELF_SECT_GROUP    17
#define ELF_SECT_SYMTABX  18
#define ELF_SECT_NUM      19

#define ELF_SECT_FLAG_WRITE 0x00000001
#define ELF_SECT_FLAG_ALLOC 0x00000002
#define ELF_SECT_FLAG_EXEC  0x00000004
#define ELF_SECT_FLAG_MERGE 0x00000010
#define ELF_SECT_FLAG_STRS  0x00000020
#define ELF_SECT_FLAG_INFO  0x00000040
#define ELF_SECT_FLAG_ORDER 0x00000080
#define ELF_SECT_FLAG_OS_NC 0x00000100
#define ELF_SECT_FLAG_GROUP 0x00000200
#define ELF_SECT_FLAG_TLS   0x00000400

#include "objects.h"

__attribute__((packed))
struct elf32_header {
	/* Elf common fields. */
	char     magic[4];
	uint8_t  bits;
	uint8_t  endianness;
	uint8_t  osabi;
	uint8_t  abiver;
	uint8_t  padding[7];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	/* Elf32 fields. */
	uint32_t entrypoint;
	uint32_t progoffs;
	uint32_t sectoffs;
	/* Elf common fields. */
	uint32_t flags;
	uint16_t headersize;
	uint16_t progentsize;
	uint16_t progentnum;
	uint16_t sectentsize;
	uint16_t sectentnum;
	uint16_t sectnameindex;
};

__attribute__((packed))
struct elf32_progheader {
	uint32_t type;
	uint32_t offset;
	uint32_t virtaddr;
	uint32_t physaddr;
	uint32_t filesize;
	uint32_t memsize;
	uint32_t flags;
	uint32_t alignment;
};

__attribute__((packed))
struct elf32_sectheader {
	uint32_t nameoffs;
	uint32_t type;
	uint32_t flags;
	uint32_t virtaddr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t alignment;
	uint32_t entsize;
};

#endif // ELF_H
