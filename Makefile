SHELL=/bin/bash

SUBDIRS = $(basename $(shell find . -mindepth 2 -maxdepth 2 -name Makefile -printf '%h\n'))

all: subdirs testbuild

.PHONY: subdirs $(SUBDIRS) copy testbuild

subdirs: $(SUBDIRS)

copy:
	find . -path ./include -prune -o -name \*.h -exec cp {} include/ \;
	find . -path ./lib -prune -o -name \*.c -exec cp {} lib/ \;

testbuild: copy
	find ./lib -name \*.c -exec gcc -Wall -I ./include -c -o /dev/null {} \;

$(SUBDIRS): copy
	@$(MAKE) -C $@

