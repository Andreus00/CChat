CC := gcc
LIBS := -pthread
GTKLIB := `pkg-config --libs gtk+-3.0`
CFLAFS := `pkg-config --cflags gtk+-3.0`

CLIBIN := ../../../bin/client/

gui_client : gui_client.c gui_dependencies
	$(CC) $(CFLAFS) $(LIBS) $(PKGCONF) $< -o  $(CLIBIN)$@ $(GTKLIB)

.PHONY gui_dependencies :
	sudo apt install libgtk-3-dev 