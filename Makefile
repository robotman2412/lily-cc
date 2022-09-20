
CC=gcc
LD=gcc
YACC=bison

TARGET		=$(shell cat build/current_arch)
VERSION		=$(shell cat version)
SOURCES		=./build/parser.c\
			$(shell find ./src ! -path './src/debug/*' ! -path './src/arch/*' -name '*.c')\
			$(shell find ./src/arch/$(TARGET) -name '*.c')
HEADERS		=./build/config.h ./build/parser.h ./build/version_number.h\
			$(shell find ./src ! -path './src/debug/*' ! -path './src/arch/*' -name '*.h')\
			$(shell find ./src/arch/$(TARGET) -name '*.h')
OBJECTS		=$(shell echo $(SOURCES) | sed -e 's/src/build/g;s/\.c/.c.o/g')
OBJ_DEBUG	=$(shell echo $(SOURCES) | sed -e 's/src/build/g;s/\.c/.c.debug.o/g')
INCLUDES	=-Isrc -Isrc/arch/$(TARGET) -Isrc/asm -Isrc/objects -Isrc/util -Isrc/debug -Ibuild

OUTFILE     =comp
CCFLAGS     =$(INCLUDES)
FLAGS_DEBUG =$(INCLUDES) -ggdb -DENABLE_DEBUG_LOGS -DDEBUG_COMPILER -DDEBUG_GENERATOR
LDFLAGS=
YACCFLAGS=-v -Wnone -Wconflicts-sr -Wconflicts-rr

CFGFILES=build build/config.h build/current_arch build/

.PHONY: all config debug debugsettings clean config install

# Commands for the user.
all: config ./build/main.o
	$(LD) ./build/main.o -o $(OUTFILE) $(LDFLAGS)

debug: config ./build/debug.o
	$(LD) -ggdb ./build/debug.o -o $(OUTFILE) $(LDFLAGS)

# Checks
config: $(CFGFILES)

$(CFGFILES):
	./configure.sh --check

# Compilation
./build/main.o: $(OBJECTS)
	$(LD) -r $^ -o $@
	
./build/debug.o: $(OBJ_DEBUG)
	$(LD) -ggdb -r $^ -o $@

./build/parser.c: ./src/parser.bison
	@mkdir -p $(shell dirname $@)
	$(YACC) $< $(YACCFLAGS) -o build/parser.c --defines=build/parser.h

./build/parser.c.o: ./build/parser.c $(HEADERS)
	@mkdir -p $(shell dirname $@)
	$(CC) -c $< $(CCFLAGS) -o $@

./build/parser.c.debug.o: ./build/parser.c $(HEADERS)
	@mkdir -p $(shell dirname $@)
	$(CC) -c $< $(FLAGS_DEBUG) -o $@

./build/%.o: ./src/% $(HEADERS)
	@mkdir -p $(shell dirname $@)
	$(CC) -c $< $(CCFLAGS) -o $@

./build/%.debug.o: ./src/% $(HEADERS)
	@mkdir -p $(shell dirname $@)
	$(CC) -c $< $(CCFLAGS) -o $@

# Clean
clean:
	rm -f $(OBJECTS) ./comp ./build/parser.*
	rm -rf $(shell find build/* -type d)

# Install the thing
install: config ./comp
	sudo ./install.sh
