
CXXFLAGS := -Wall -g -O0 -fsanitize=address

.PHONY: all clean

all: elfdump chflg

elfdump: elfdump.cpp

chflg: chflg.cpp

clean:
	rm elfdump
