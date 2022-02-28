#!/bin/bash

if [ ! -r build/current_arch ]; then
	echo "Error: Not configured."
	echo "Use 'configure.sh' to configure."
	exit 2
fi

ARCH=$(cat build/current_arch)
VER=$(cat version)

if [ "$ARCH" == "template" ]; then
	echo "Error: Configured for 'template', this is not allowed."
	echo "Use 'configure.sh' to configure."
	exit 2
fi

echo "Building lilly-cc $ARCH v$VER"

# Convert our grammar file to actual C code.
echo "BISON src/parser.bison"
bison src/parser.bison -v -Wnone -Wconflicts-sr -Wconflicts-rr -o src/parser.c --defines=src/parser.h || exit 1

# Files to link.
FILES=""

# Options passed to compiler
CCOPTIONS="-Isrc -Isrc/debug -Isrc/arch/$ARCH -Isrc/asm -Ibuild"

for i in $*; do
	case $i in
		debug|--debug)
			CCOPTIONS="$CCOPTIONS -DENABLE_DEBUG_LOGS -DDEBUG_COMPILER -ggdb"
			;;
		--ccoptions=*)
			CCOPTIONS="$CCOPTIONS ${i#*=}"
			;;
		-D*)
			CCOPTIONS="$CCOPTIONS $i"
			;;
	esac
done

echo "Building lilly-cc v0.1"
echo "CCOPTIONS: $CCOPTIONS"

errors=0

# Compile macro.
CC() {
	for i in $*; do
		echo "CC $1"
		paf="build/$i"
		mkdir -p "${paf%/*}"
		gcc $CCOPTIONS -c "src/$i" -o "build/$i.o" || errors=1
		FILES="$FILES build/$i.o"
	done
}

# Compile all the files.
CC main.c
CC parser.c
CC strmap.c
CC tokeniser.c
CC parser-util.c

CC asm/asm.c
CC asm/asm_postproc.c
CC asm/gen_util.c
CC asm/gen_fallbacks.c
CC asm/gen_preproc.c

# Compile all the architecture-specific files.
for i in $(find src/arch/$ARCH/ -type f -name "*.c"); do
	CC arch/$ARCH/${i##*/}
done

CC debug/pront.c
CC debug/gen_tests.c

if [ $errors -gt 0 ]; then
	exit $errors
fi

# Link the compiled files.
echo "LN$FILES"
gcc $FILES -o comp || errors=1

exit $errors
