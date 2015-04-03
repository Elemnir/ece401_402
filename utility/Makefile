CC=g++
CFLAGS=-Wall -g -std=c++11 -lboost_system -lboost_filesystem

all: daemon 

daemon: daemon.cpp inotify.o directory_tree.o
	$(CC) $(CFLAGS) inotify.o directory_tree.o daemon.cpp -o daemon

inotify.o: inotify.cpp inotify.hpp
	$(CC) $(CFLAGS) -c inotify.cpp 

directory_tree.o: directory_tree.cpp directory_tree.hpp
	$(CC) $(CFLAGS) -c directory_tree.cpp

clean:
	rm -f daemon inotify.o directory_tree.o 
