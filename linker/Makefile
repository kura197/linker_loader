
CXXFLAGS := -Wall -g -O0 -fsanitize=address -std=c++17

CFLAGS := -Wall -fno-builtin --static

.PHONY: all clean

all: dynlink sample

dynlink: dynlink.cpp linklib.cpp chflg
	$(CXX) dynlink.cpp linklib.cpp -o $@ $(CXXFLAGS) 
	./chflg $@

test_main.o: test_main.c
	$(CC) $< -c -o $@ $(CFLAGS)

sample.o: sample.c
	$(CC) $< -c -o $@ $(CFLAGS)

sample2.o: sample2.c
	$(CC) $< -c -o $@ $(CFLAGS)

sample: test_main.o sample.o sample2.o
	$(CC) $^ -o $@ $(CFLAGS)

chflg: chflg.cpp

clean:
	rm dynlink sample *.o chflg
