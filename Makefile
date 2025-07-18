
MAKEFLAGS += --silent --no-print-directory -j$(shell nproc)
TEST      ?=
VGFLAGS   ?=

.PHONY: all
all: build

.PHONY: build
build:
	cmake -B build src
	cmake --build build

.PHONY: test
test: build
	./build/test/lily-test $(TEST)

.PHONY: valgrind-test
valgrind-test: build
	LILY_TEST_FORK=0 valgrind $(VGFLAGS) ./build/test/lily-test $(TEST)

.PHONY: gdb-test
gdb-test: build
	LILY_TEST_FORK=0 gdb ./build/test/lily-test

.PHONY: clean
clean:
	rm -rf build
