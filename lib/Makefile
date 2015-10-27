LIB=libsql.a
CFLAGS = -I ../include -Wall -lpthread
SRC_FILES := $(shell find . -name \*.c)
OBJ_FILES = $(addsuffix .o, $(basename $(SRC_FILES)))

.PHONY: clean

all: $(LIB) clean

$(LIB): $(OBJ_FILES)
	ar cr $(LIB) $(OBJ_FILES)

$(OBJ_FILES): $(SRC_FILES)

clean:
	rm -rf *.o
