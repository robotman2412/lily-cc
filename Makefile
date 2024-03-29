
MAKEFLAGS  += --jobs=$(shell nproc)

CC			= gcc
LD			= gcc
YACC		= bison

TARGET		= $(shell cat build/current_arch)
VERSION		= $(shell cat version)
SOURCES		= ./build/parser.c\
				$(shell find ./src ! -path './src/debug/*' ! -path './src/arch/*' -name '*.c')\
				$(shell find ./src/arch/$(TARGET) -name '*.c')
HEADERS		= ./build/config.h ./build/parser.h ./build/version_number.h\
				$(shell find ./src ! -path './src/debug/*' ! -path './src/arch/*' -name '*.h')\
				$(shell find ./src/arch/$(TARGET) -name '*.h')
OBJECTS		= $(shell echo $(SOURCES) | sed -e 's/src/build/g;s/\.c/.c.o/g')
SRC_DEBUG	= $(SOURCES) $(shell find ./src -path './src/debug/*' ! -path './src/arch/*' -name '*.c')
HDR_DEBUG	= $(HEADERS) $(shell find ./src -path './src/debug/*' ! -path './src/arch/*' -name '*.h')
OBJ_DEBUG	= $(shell echo $(SRC_DEBUG) | sed -e 's/src/build/g;s/\.c/.c.debug.o/g')
INCLUDES	= -Isrc -Isrc/arch/$(TARGET) -Isrc/asm -Isrc/objects -Isrc/util -Isrc/modes -Isrc/debug -Ibuild

OUTFILE		= comp
CCFLAGS		= $(INCLUDES)
FLAGS_DEBUG	= $(CCFLAGS) -ggdb -DENABLE_DEBUG_LOGS -DDEBUG_COMPILER -DDEBUG_GENERATOR
LDFLAGS		=
YACCFLAGS	= -v -Wnone -Wconflicts-sr -Wconflicts-rr

CFGFILES	= build build/config.h build/current_arch build/

.PHONY: all config debug debugsettings clean config install

# Commands for the user.
all: config ./build/main.o
	@$(LD) ./build/main.o -o $(OUTFILE) $(LDFLAGS)
	@echo LD $(OUTFILE)

debug: config ./build/debug.o
	@$(LD) -ggdb ./build/debug.o -o $(OUTFILE) $(LDFLAGS)
	@echo LD $(OUTFILE)

# Checks
config: $(CFGFILES)

$(CFGFILES):
	./configure.sh --check

# Compilation
./build/main.o: $(OBJECTS)
	@$(LD) -r $^ -o $@
	@echo LD $@
	
./build/debug.o: $(OBJ_DEBUG)
	@$(LD) -ggdb -r $^ -o $@
	@echo LD $@

./build/parser.h: ./build/parser.c
./build/parser.c: ./src/parser.bison
	@mkdir -p $(shell dirname $@)
	@$(YACC) $< $(YACCFLAGS) -o build/parser.c --defines=build/parser.h
	@echo YACC $<

./build/parser.c.o: ./build/parser.c $(HEADERS) Makefile
	@mkdir -p $(shell dirname $@)
	@$(CC) -c $< $(CCFLAGS) -o $@
	@echo CC $<

./build/parser.c.debug.o: ./build/parser.c $(HDR_DEBUG) Makefile
	@mkdir -p $(shell dirname $@)
	@$(CC) -c $< $(FLAGS_DEBUG) -o $@
	@echo CC $<

./build/%.o: ./src/% $(HEADERS) Makefile
	@mkdir -p $(shell dirname $@)
	@$(CC) -c $< $(CCFLAGS) -o $@
	@echo CC $<

./build/%.debug.o: ./src/% $(HDR_DEBUG) Makefile
	@mkdir -p $(shell dirname $@)
	@$(CC) -c $< $(FLAGS_DEBUG) -o $@
	@echo CC $<

# Clean
clean:
	rm -f $(OBJECTS) ./comp ./build/parser.* ./build/*.o
	rm -rf $(shell find build/* -type d)

# Install the thing
install: config ./comp
	sudo ./install.sh
