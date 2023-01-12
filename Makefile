
CXXFLAGS := -Wall -g -O0

.PHONY: all clean

all: elfdump

elfdump: elfdump.cpp

clean:
	rm elfdump
