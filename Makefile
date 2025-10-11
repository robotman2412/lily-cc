
MAKEFLAGS += --silent --no-print-directory -j$(shell nproc)
TEST      ?=
VGFLAGS   ?=

.PHONY: all
all: build

.PHONY: build
build:
	cmake -B build src
	cmake --build build --target lilycc
	cmake --build build --target lily-explainer

.PHONY: clang-tidy
clang-tidy:
	clang-tidy -p build $(shell find src -name '*.c')

.PHONY: build-test
build-test:
	cmake -B build src
	cmake --build build --target lily-test

.PHONY: test
test: build-test
	./build/test/lily-test $(TEST)

.PHONY: valgrind-test
valgrind-test: build-test
	LILY_TEST_FORK=0 valgrind $(VGFLAGS) ./build/test/lily-test $(TEST)

.PHONY: gdb-test
gdb-test: build-test
	LILY_TEST_FORK=0 gdb ./build/test/lily-test

.PHONY: clean
clean:
	rm -rf build
