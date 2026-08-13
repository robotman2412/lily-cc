
MAKEFLAGS += --silent --no-print-directory -j$(shell nproc)
TEST      ?=
VGFLAGS   ?=
DESTDIR   ?= /

.PHONY: all
all: build

.PHONY: build
build:
	[ -d build ] || meson setup build .
	meson compile -C build

.PHONY: install
install: build
	meson install -C build --destdir=$(shell realpath $(DESTDIR))

.PHONY: clang-tidy
clang-tidy:
	clang-tidy -p build $(shell find src -name '*.c')

.PHONY: test
test: build
	meson test -C build

.PHONY: valgrind-test
valgrind-test: build
	meson test -C build --setup valgrind

.PHONY: gdb-test
gdb-test: build
	meson test -C build --gdb

.PHONY: clean
clean:
	rm -rf build
