CC=gcc -I ../include -Wall -L../lib

all: build test

.PHONY: build test

build:
	find . -name \*.c -exec $(CC) -c -o /dev/null {} \;

test:
	$(CC) common_test.c -lsql && ./a.out && rm a.out
