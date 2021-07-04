#!/bin/bash

# Convert our grammar file to actual C code.
echo "BISON src/parser.bison"
bison src/parser.bison -Wnone -Wconflicts-sr -Wconflicts-rr -o src/parser.c --defines=src/parser.h

# Files to link.
FILES=""

# Options passed to compiler
ARCH=$(cat current_arch)
CCOPTIONS="-Isrc -Ibuild -Isrc/asm -Isrc/arch -Isrc/arch/$ARCH"

errors=0

# Compile macro.
CC() {
	for i in $*; do
		echo "CC $1"
		mkdir -p "build/${i%/*}"
		gcc $CCOPTIONS -c "src/$i" -o "build/$i.o" || errors=1
		FILES="$FILES build/$i.o"
	done
}

# Compile all the files.
# mkdir -p build/asm build/arch
CC parser.c
CC main.c
CC asm/asm.c
CC asm/softstack.c
CC arch/gen.c
CC tokeniser.c
CC strmap.c

if [ $errors -gt 0 ]; then
	exit $errors
fi

# Link the compiled files.
gcc $FILES -o comp || errors=1

exit $errors
