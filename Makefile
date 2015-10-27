SHELL=/bin/bash
INCLUDE_DIR=./include
LIB_DIR=./lib
BUILD_DIR=./build

CC=gcc -Wall -I $(INCLUDE_DIR)

SUBDIRS = $(basename $(shell find . -mindepth 2 -maxdepth 2 -name Makefile -printf '%h\n'))

all: subdirs testbuild

.PHONY: subdirs $(SUBDIRS) copy testbuild build

subdirs: $(SUBDIRS)

copy:
	find . -path $(INCLUDE_DIR) -prune -o -name \*.h -exec cp {} $(INCLUDE_DIR) \;
	find . -path $(LIB_DIR) -prune -o -name \*_test.c -prune -o -name \*.c -exec cp {} $(LIB_DIR) \;

testbuild: copy
	find $(LIB_DIR) -name \*.c -exec $(CC) -c -o /dev/null {} \;

$(SUBDIRS): copy
	@$(MAKE) -C $@
