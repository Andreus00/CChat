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
#include "../utility/login_view.h"
#include "../utility/chat_view.h"

int portno, sockfd;


char *host_name;

chatroom_widgets widgets;
login_widgets lgn_widgets;

int send_message() {

    GtkTextIter start_iter, end_iter;

    gtk_text_buffer_get_bounds (widgets.textBuffer, &start_iter, &end_iter);

    char *text = gtk_text_buffer_get_text(widgets.textBuffer, &start_iter, &end_iter, FALSE);

    write(sockfd, text, strlen(text));

    memset(&start_iter, 0, sizeof(start_iter));
    
    gtk_text_buffer_get_iter_at_offset (widgets.textBuffer, &start_iter, 0);

    gtk_text_buffer_delete (widgets.textBuffer, &start_iter, &end_iter);

    return 0;
}

static void activate (GtkApplication* app, gpointer user_data) {
  printf("%p", init_connection);
  show_login(app, &lgn_widgets);
  printf("done\n");
}


int main (int argc, char **argv) {
  if (argc != 1) {
    perror("Prease launch the application without arguments");
    exit(1);
  }
  // host_name = "0.0.0.0";
  // init_connection();
  GtkApplication *app;  // pointer to the app instance
  int p = 0;
  int status;           // return status of the application

  app = gtk_application_new ("it.sanchietti.cchat", G_APPLICATION_FLAGS_NONE);    // creation of the app

  g_signal_connect (app, "activate", G_CALLBACK (activate), &p);      // connect the app to the activate function.

  status = g_application_run (G_APPLICATION (app), argc, argv);         // run the app. 

  g_object_unref (app);

  return status;
}