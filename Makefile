SHELL=/bin/bash

.PHONY: copy testbuild

copy:
	find . -path ./include -prune -o -name *.h -exec cp {} include/ \;
	find . -path ./lib -prune -o -name *.c -exec cp {} lib/ \;

testbuild: copy
	find . -name *.c -exec gcc -I ./include -c -o /dev/null {} \;
