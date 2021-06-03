CC := gcc
LIBS := -pthread
CLI := src/client/
GUICLI := src/client_2/client/
SERV := src/server/


#client : src/client/client.c bin-client
#	$(CC) $(LIBS) src/client/client.c -o $(CLIBIN)client

# gui_client : src/client_2/client/gui_client gui_dependencies bin-client
# 	$(CC) $(LIBS) `pkg-config --cflags gtk+-3.0` src/client_2/client/gui_client.c -o  $(CLIBIN)/gui_client `pkg-config --libs gtk+-3.0`

# server : bin-server
# 	$(CC) $(LIBS) src/server/server.c -o $(SERVBIN)server

bin :
	mkdir -p bin/server
	mkdir -p bin/client


all : bin
	cd $(SERV) && $(MAKE)
	cd $(CLI) && $(MAKE)

all-gui : all
	cd $(GUICLI) && $(MAKE)

.PHONY clean : 
	rm -rfi bin

.PHONY clean-dep:
	sudo apt remove libgtk-3-dev