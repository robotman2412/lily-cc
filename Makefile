
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
	valgrind $(VGFLAGS) ./build/test/lily-test $(TEST)

.PHONY: clean
clean:
	rm -rf build
