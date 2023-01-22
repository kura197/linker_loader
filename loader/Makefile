
CXXFLAGS := -Wall -g -O0 -fsanitize=address

.PHONY: all clean

all: loader

loader: loader.cpp
	$(CXX) $< -o $@ $(CXXFLAGS)

clean:
	rm loader
