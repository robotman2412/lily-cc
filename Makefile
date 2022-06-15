
CC=gcc
LD=gcc
YACC=bison

TARGET =$(shell cat build/current_arch)
VERSION=$(shell cat version)
SOURCES=./build/parser.c\
		$(shell find ./src ! -path './src/debug/*' ! -path './src/arch/*' -name '*.c')\
		$(shell find ./src/arch/$(TARGET) -name '*.c')
HEADERS=./build/config.h ./build/parser.h ./build/version_number.h\
		$(shell find ./src ! -path './src/debug/*' ! -path './src/arch/*' -name '*.h')\
		$(shell find ./src/arch/$(TARGET) -name '*.h')
OBJECTS=$(shell echo $(SOURCES) | sed -e 's/src/build/g;s/\.c/.c.o/g')
INCLUDES=-Isrc -Isrc/arch/$(TARGET) -Isrc/asm -Isrc/objects -Isrc/util -Isrc/debug\
		-Ibuild

CCFLAGS=$(INCLUDES)
LDFLAGS=
YACCFLAGS=-v -Wnone -Wconflicts-sr -Wconflicts-rr

CFGFILES=build build/config.h build/current_arch build/

.PHONY: all config debug debugsettings clean config

# Commands for the user.
all: ./build/notdebug config ./comp

debug: ./build/yesdebug config debugsettings ./comp

# Auto clean when switching between yes and not debug.
./build/notdebug:
	rm -rf ./build/yesdebug
	$(MAKE) clean
	touch ./build/notdebug
	
./build/yesdebug:
	rm -rf ./build/notdebug
	$(MAKE) clean
	touch ./build/yesdebug

# Checks
config: $(CFGFILES)

$(CFGFILES):
	./configure.sh --check

debugsettings:
	$(eval CCFLAGS = $(CCFLAGS) -DDEBUG_COMPILER -DDEBUG_GENERATOR)

# Compilation
./comp: $(OBJECTS)
	$(LD) $^ -o $@ $(LDFLAGS)

./build/parser.c: ./src/parser.bison
	mkdir -p $(shell dirname $@)
	$(YACC) $< $(YACCFLAGS) -o build/parser.c --defines=build/parser.h

./build/parser.c.o: ./build/parser.c $(HEADERS)
	mkdir -p $(shell dirname $@)
	$(CC) -c $< $(CCFLAGS) -o $@

./build/%.o: ./src/% $(HEADERS)
	mkdir -p $(shell dirname $@)
	$(CC) -c $< $(CCFLAGS) -o $@

# Clean
clean:
	rm -f $(OBJECTS) ./comp ./build/parser.*
	rm -rf $(shell find build/* -type d)
