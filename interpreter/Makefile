CXX=g++
CXXFLAGS = -I../include -L../lib -lsql -lpthread -lbsd -ledit -g -std=c++11
SHELL=/bin/bash
OUT=a.out

all: clean testbuild $(OUT)

testbuild: $(wildcard *.cpp)
	$(foreach file, $(wildcard *.cpp), $(CXX) -c $(file) -o /dev/null $(CXXFLAGS);)

SRC := $(wildcard *.cpp)

$(OUT): $(SRC)
	$(CXX) $^ -o $@ $(CXXFLAGS)

clean:
	rm -f $(OUT)
