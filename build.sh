#!/bin/bash

# Convert our grammar file to actual C code.
bison src/parser.bison -Wall --report=all -o build/parser.c --defines=build/parser.h

# Files to link.
FILES="build/parser.c.o"

# Options passed to compiler
ARCH=$(cat current_arch)
CCOPTIONS="-Isrc -Ibuild -Isrc/asm -Isrc/arch -Isrc/arch/$ARCH"

# Compile macro.
CC() {
gcc $CCOPTIONS -c "src/$1" -o "build/$1.o"
FILES="$FILES build/$1.o"
}

# Compile all the files.
mkdir -p build/asm build/arch
gcc $CCOPTIONS -c "build/parser.c" -o "build/parser.c.o" && \
CC main.c && \
CC asm/asm.c && \
CC asm/softstack.c && \
CC arch/gen.c && \
CC tokeniser.c && \
CC strmap.c || exit 1

# Link the compiled files.
gcc $FILES -o comp
