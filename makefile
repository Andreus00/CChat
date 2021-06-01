CC = gcc
LIBS = -pthread
CLIBIN = bin/client/


client : bin-client
	$(CC) $(LIBS) src/client/client.c -o $(CLIBIN)client

gui_client : gui_dependencies bin-client
	$(CC) $(LIBS) `pkg-config --cflags gtk+-3.0` src/client_2/client/client.c -o  $(CLIBIN)/gui_client `pkg-config --libs gtk+-3.0`

server : bin-server
	$(CC) $(LIBS) src/server/server.c -o bin/server/server

bin-client : bin
	mkdir -p bin/client
bin-server : bin
	mkdir -p bin/server
bin :
	mkdir -p bin

gui_dependencies :
	sudo apt install libgtk-3-dev

all : client server

all-gui : all gui_client

clear : 
	rm -rf bin

clear-dep:
	sudo apt remove libgtk-3-dev