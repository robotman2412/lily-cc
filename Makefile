
CC=gcc
LD=gcc
YACC=bison

TARGET =$(shell cat build/current_arch)
VERSION=$(shell car version)
SOURCES=./build/parser.c\
		$(shell find ./src ! -path './src/debug/*' ! -path './src/arch/*' -name '*.c')\
		$(shell find ./src/arch/$(TARGET) -name '*.c')
HEADERS=./build/config.h ./build/parser.h ./build/version_number.h\
		$(shell find ./src ! -path './src/debug/*' ! -path './src/arch/*' -name '*.h')\
		$(shell find ./src/arch/$(TARGET) -name '*.h')
OBJECTS=$(shell echo $(SOURCES) | sed -e 's/src/build/g;s/\.c/.c.o/g')

CCFLAGS=-ggdb -DENABLE_DEBUG_LOGS -DDEBUG_GENERATOR
LDFLAGS=
YACCFLAGS=-v -Wnone -Wconflicts-sr -Wconflicts-rr

.PHONY: all debug clean

all: ./comp

./comp: $(OBJECTS)
	$(LD) $^ -o $@ $(LDFLAGS)

./build/parser.c: ./src/parser.bison
	$(YACC) $< $(YACCFLAGS) -o build/parser.c --defines=build/parser.h

./build/parser.c.o: ./build/parser.c $(HEADERS)
	$(CC) $< $(CCFLAGS) -o $@

./build/%.o: ./src/% $(HEADERS)
	$(CC) $< $(CCFLAGS) -o $@

clean:
	rm -f $(OBJECTS) ./comp ./build/parser.c
