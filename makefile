CC = gcc
LIBS = -pthread

client : bin-client
	$(CC) $(LIBS) src/client/client.c -o bin/client/client

server : bin-server
	$(CC) $(LIBS) src/server/server.c -o bin/server/server

bin-client : bin
	mkdir -p bin/client
bin-server : bin
	mkdir -p bin/server
bin :
	mkdir -p bin

all : client server

clear : 
	rm -rf bin