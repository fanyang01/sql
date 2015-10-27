LIB=libsql.a
CFLAGS = -I ../include -Wall -lpthread
SRC_FILES := $(wildcard *.c)
OBJ_FILES := $(patsubst %.c, %.o, $(SRC_FILES))

.PHONY: clean

all: $(LIB) clean

$(LIB): $(OBJ_FILES)
	ar cr $(LIB) $(OBJ_FILES)

$(OBJ_FILES): $(SRC_FILES)

clean:
	rm -rf *.o
