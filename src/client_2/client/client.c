#include <stdio.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>


#include "../structs/structs.h"
#include "../ui/login_view.h"
#include "../ui/chat_view.h"

chatroom_widgets *widgets;
login_widgets *lgn_widgets;
GtkWidget *window;


void login_callback(int fd) {
    init_chatroom(window, widgets, fd);
    show_chatroom(widgets);
}


void startup(GtkApplication* app, gpointer user_data){
    widgets = malloc(sizeof(chatroom_widgets));
    lgn_widgets = malloc(sizeof(lgn_widgets));
    window = malloc(sizeof(GtkWindow));
}

void activate (GtkApplication* app, gpointer user_data) {
    window = gtk_application_window_new (GTK_APPLICATION (app));
    init_login(window, lgn_widgets);
    show_login(lgn_widgets);
    gtk_widget_show (window);
}


int main (int argc, char **argv) {
  if (argc != 1) {
    perror("Prease launch the application without arguments");
    exit(1);
  }
  GtkApplication *app;  // pointer to the app instance
  int p = 0;
  int status;           // return status of the application

  app = gtk_application_new ("it.sanchietti.cchat", G_APPLICATION_FLAGS_NONE);    // creation of the app

  g_signal_connect (app, "startup", G_CALLBACK (startup), &p);

  g_signal_connect (app, "activate", G_CALLBACK (activate), &p);      // connect the app to the activate function.

  status = g_application_run (G_APPLICATION (app), argc, argv);         // run the app. 

  g_object_unref (app);

  return status;
}