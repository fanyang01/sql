SHELL=/bin/bash
INCLUDE_DIR=./include
LIB_DIR=./lib
BUILD_DIR=./build

CC=gcc -Wall -I $(INCLUDE_DIR)

SUBDIRS := $(filter-out lib/, \
	$(dir \
	$(shell find . -mindepth 2 -maxdepth 2 -name Makefile -printf '%P\n')))

all: echo subdirs lib

echo:
	@echo subdirectory: $(SUBDIRS)

.PHONY: subdirs $(SUBDIRS) copy testbuild lib

copy:
	find . -path $(INCLUDE_DIR) -prune -o -name \*.h -exec cp {} $(INCLUDE_DIR) \;
	find . -path $(LIB_DIR) -prune -o -name \*_test.c -prune -o -name \*.c -exec cp {} $(LIB_DIR) \;

subdirs: $(SUBDIRS)

lib: copy
	@$(MAKE) -C $@

$(SUBDIRS): lib
	@$(MAKE) -C $@

testbuild: copy
	find $(LIB_DIR) -name \*.c -exec $(CC) -c -o /dev/null {} \;

