# test need input
# include ../subdir.mk
CC=gcc -I ../include -Wall -L../lib -lpthread -lbsd -g
SRC_FILE := $(shell find . \( \! -name \*_test.c \) -a -name \*.c -printf "%P ")

all: $(SRC_FILE)
	@$(foreach file, $(SRC_FILE), $(CC) $(file) -c -o /dev/null ;)
