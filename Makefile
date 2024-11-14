
MAKEFLAGS += --silent --no-print-directory
TEST      ?=

.PHONY: all
all: build

.PHONY: build
build:
	cmake -B build src
	cmake --build build

.PHONY: test
test: build
	./build/lily-cc-test $(TEST)
