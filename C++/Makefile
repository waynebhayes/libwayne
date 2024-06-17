CC=gcc
all: mt19937 FutureAsync

mt19937: MTGenerator.hpp test-mt.c mt19937.cpp mt19937.h
	$(CC) -c test-mt.c
	$(CXX) -std=c++11 -c mt19937.cpp
	$(CXX) -o mt19937 test-mt.o mt19937.o

FutureAsync: FutureAsync.hpp test-async.c FutureAsync.cpp FutureAsync.h
	$(CC) -I../include -c test-async.c
	$(CXX) -I../include -std=c++11 -c FutureAsync.cpp
	$(CXX) -o FutureAsync test-async.o FutureAsync.o

clean:
	@/bin/rm -f *.o mt19937 FutureAsync
