GCC := gcc
LIBS := -pthread
CLI := src/client/
GUICLI := src/client_2/client/
SERV := src/server/

.PHONY : bin
bin :
	mkdir -p bin/server
	mkdir -p bin/client


all : bin
	$(MAKE) -C $(SERV)
	$(MAKE) -C $(CLI)

all-gui : all
	$(MAKE) -C $(GUICLI)

.PHONY : clean
clean : 
	rm -rfi bin

.PHONY : clean-dep
clean-dep:
	sudo apt remove libgtk-3-dev
