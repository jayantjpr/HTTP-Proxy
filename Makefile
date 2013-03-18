CC=g++
CFLAGS=-c -g -Wall -Werror

all: proxy

proxy: proxy.o http_header_util.o socket_util.o
	$(CC) proxy.o http_header_util.o socket_util.o -o proxy

proxy.o: proxy.cpp
	$(CC) $(CFLAGS) proxy.cpp
	
http_header_util.o: http_header_util.cpp
	$(CC) $(CFLAGS) http_header_util.cpp

socket_util.o: socket_util.c
	$(CC) $(CFLAGS) socket_util.c

clean:
	rm -rf proxy *.o

#tar:
#	tar -cvzf cos461_ass2_$(USER).tgz proxy.c README Makefile
