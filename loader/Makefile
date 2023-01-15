
CXXFLAGS := -Wall -g -O0 -fsanitize=address

.PHONY: all clean

all: chflg loader

chflg: chflg.cpp

loader: loader.cpp
	$(CXX) $< -o $@ $(CXXFLAGS) -Wl,-T $(PWD)/space.scr

clean:
	rm chflg loader
