
CXXFLAGS := -Wall -g -O0 -fsanitize=address -std=c++17

.PHONY: all clean

all: loader

loader: loader.cpp
	$(CXX) $< -o $@ $(CXXFLAGS)

clean:
	rm loader
