#include <stdio.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <time.h>
#include <stdbool.h>

struct message {
  int sender;       //id of the sender
  time_t send_time; //when the message was sent
  bool continues;   //true if the message is part of an bigger message and this is not the last segment of it.If a message is too big to stay in a single message, it will be divided into different messages.
  int seq_number;   //sequence number of this message. If there is a too big message, it will be divided into different messages. This number is the sequence number of this message. It starts at 0.
  char msg[300];    //message.
};


static void startup (GtkApplication* app, gpointer user_data) {
    //initialise logs, set up a connection with the server, 
    puts("Logs initialization complete\n");

    puts("Connected to the server\n");
}

static void activate (GtkApplication* app, gpointer user_data) {

  puts("init gui");

  GtkWidget *window; // pointer that will point to the main window
  GtkWidget *textView;
  GtkTextBuffer *textBuffer;
  GtkWidget *vBox;
  GtkWidget *scroll_text_view;
  GtkWidget *bottom_hbox;
  GtkWidget *send_button;
  GtkWidget *chat_view;

  window = gtk_application_window_new (app);                    // creation of the new window
  gtk_window_set_title (GTK_WINDOW (window), "Window");         // setting the window
  gtk_window_set_default_size (GTK_WINDOW (window), 500, 500);  // ...

  // vBox will contain the text area and the chat.
  vBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_set_homogeneous(GTK_BOX(vBox), FALSE);

  // text area setting
  textBuffer = gtk_text_buffer_new(NULL);
  textView = gtk_text_view_new_with_buffer(textBuffer);
  gtk_widget_set_halign ((GtkWidget *) textView, GTK_ALIGN_FILL);

  
    // put the textView inside the scroll area
  scroll_text_view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_min_content_width ((GtkScrolledWindow *) scroll_text_view, 300);
  gtk_scrolled_window_set_max_content_height ((GtkScrolledWindow *) scroll_text_view, 150);
  gtk_scrolled_window_set_min_content_height ((GtkScrolledWindow *) scroll_text_view, 50);
  gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scroll_text_view, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand ((GtkWidget *) scroll_text_view, TRUE);
  gtk_widget_set_halign ((GtkWidget *) scroll_text_view, GTK_ALIGN_FILL);

  bottom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_set_homogeneous(GTK_BOX(bottom_hbox), FALSE);

  char *send_button_text = "send";

  send_button = gtk_button_new_with_label(send_button_text);
  gtk_widget_set_size_request (send_button, 60, 20);
  gtk_widget_set_hexpand ((GtkWidget *) send_button, FALSE);
  gtk_widget_set_halign ((GtkWidget *) send_button, GTK_ALIGN_FILL);

  // chat view settings

  chat_view = gtk_text_view_new();
  gtk_text_view_set_cursor_visible ((GtkTextView *) chat_view, FALSE);
  gtk_text_view_set_editable((GtkTextView *) chat_view, FALSE);




  // set up the disposition of the containers
  gtk_box_pack_end(GTK_BOX(bottom_hbox), send_button, FALSE, FALSE, 20);

  gtk_container_add(GTK_CONTAINER (scroll_text_view), textView);
  gtk_box_pack_start(GTK_BOX(bottom_hbox), scroll_text_view, TRUE, TRUE, 20);



  gtk_box_pack_end(GTK_BOX(vBox), bottom_hbox, FALSE, FALSE, 8);
  gtk_box_pack_start(GTK_BOX(vBox), chat_view, TRUE, TRUE, 0);


  gtk_container_add (GTK_CONTAINER (window), vBox);



  gtk_widget_show_all (window);                                 // show the window
}







int main (int argc, char **argv) {
  GtkApplication *app;  // pointer to the app instance
  int p = 0;
  int status;           // return status of the application

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);    // creation of the app
  g_signal_connect (app, "startup", G_CALLBACK (startup), &p);
  g_signal_connect (app, "activate", G_CALLBACK (activate), &p);      // connect the app to the activate function.
  status = g_application_run (G_APPLICATION (app), argc, argv);         // run the app. 
  g_object_unref (app);

  return status;
}