CC=g++
CFLAGS=-Wall -std=c++11 -g -lcurl -lboost_system -lboost_filesystem -lboost_thread

all: daemon 

daemon: daemon.cpp inotify.o directory_tree.o confClass.o
	$(CC) $(CFLAGS) inotify.o directory_tree.o confClass.o daemon.cpp -o daemon -DBOOST_ASIO_SEPARATE_COMPILATION /usr/local/lib/libtorrent-rasterbar.a -lssl -lcrypto

inotify.o: inotify.cpp inotify.hpp
	$(CC) $(CFLAGS) -c inotify.cpp 

confClass.o: confClass.cpp confClass.hpp
	$(CC) $(CFLAGS) -c confClass.cpp -DBOOST_ASIO_DYN_LINK -lssl -lcrypto

directory_tree.o: directory_tree.cpp directory_tree.hpp
	$(CC) $(CFLAGS) -c directory_tree.cpp

clean:
	rm -f daemon inotify.o directory_tree.o confClass.o
