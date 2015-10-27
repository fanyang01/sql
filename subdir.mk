CC=gcc -I ../include -Wall -L../lib
SHELL=/bin/bash

all: build test

.PHONY: build test

SRC_FILE := $(shell find . \( \! -name \*_test.c \) -a -name \*.c -printf "%P ")

build: $(SRC_FILE)
	@$(foreach file, $(SRC_FILE), $(CC) $(file) -c -o /dev/null ;)

test: $(wildcard *_test.c)
	@$(foreach file, $(wildcard *_test.c), $(CC) $(file) -lsql && ./a.out && rm a.out ;)
