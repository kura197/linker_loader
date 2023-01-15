
CXXFLAGS := -Wall -g -O0 -fsanitize=address

.PHONY: all clean

all: dynlink

dynlink: dynlink.cpp linklib.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS) 

clean:
	rm dynlink
