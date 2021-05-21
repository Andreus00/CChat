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

void show_chatroom(chatroom_widgets * );
void destroy_chatroom(chatroom_widgets * );
void init_chatroom(GtkWidget *, chatroom_widgets *, int);

int sockfd;

chatroom_widgets *wid;

int send_message() {
    puts("Sending message");
    GtkTextIter start_iter, end_iter;

    gtk_text_buffer_get_bounds (wid->textBuffer, &start_iter, &end_iter);

    char *text = gtk_text_buffer_get_text(wid->textBuffer, &start_iter, &end_iter, FALSE);
    if (strlen(text) != 0){
        write(sockfd, text, strlen(text));

        memset(&start_iter, 0, sizeof(start_iter));
        free(text);
        
        gtk_text_buffer_get_iter_at_offset (wid->textBuffer, &start_iter, 0);

        gtk_text_buffer_delete (wid->textBuffer, &start_iter, &end_iter);

        char buff[14];

        read(sockfd, buff, 14);

        printf("%s\n", buff);
    }

    return 0;
}

void init_chatroom(GtkWidget *window, chatroom_widgets *widgets, int fd) {

    puts("init chatroom");

    sockfd = fd;

    wid = widgets;

    widgets->main_frame = gtk_frame_new ("Chatroom");                    // creation of the new window
    
    // vBox will contain the text area and the chat.
    widgets->vBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(widgets->vBox), FALSE);

    // text area setting
    widgets->textBuffer = gtk_text_buffer_new(NULL);
    widgets->textView = gtk_text_view_new_with_buffer(widgets->textBuffer);
    gtk_widget_set_halign ((GtkWidget *) widgets->textView, GTK_ALIGN_FILL);

    
        // put the textView inside the scroll area
    widgets->scroll_text_view = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_min_content_width ((GtkScrolledWindow *) widgets->scroll_text_view, 300);
    gtk_scrolled_window_set_max_content_height ((GtkScrolledWindow *) widgets->scroll_text_view, 150);
    gtk_scrolled_window_set_min_content_height ((GtkScrolledWindow *) widgets->scroll_text_view, 50);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) widgets->scroll_text_view, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand ((GtkWidget *) widgets->scroll_text_view, TRUE);
    gtk_widget_set_halign ((GtkWidget *) widgets->scroll_text_view, GTK_ALIGN_FILL);

    widgets->bottom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(widgets->bottom_hbox), FALSE);

    char *send_button_text = "send";

    widgets->send_button = gtk_button_new_with_label(send_button_text);
    gtk_widget_set_size_request (widgets->send_button, 60, 20);
    gtk_widget_set_hexpand ((GtkWidget *) widgets->send_button, FALSE);
    gtk_widget_set_halign ((GtkWidget *) widgets->send_button, GTK_ALIGN_FILL);

    printf("\n\ntextBuffer pointer = %p\n\n", widgets->textBuffer);
    g_signal_connect(G_OBJECT(widgets->send_button), "clicked", G_CALLBACK(send_message), NULL);

    // chat view settings

    widgets->chat_view = gtk_text_view_new();
    gtk_text_view_set_cursor_visible ((GtkTextView *) widgets->chat_view, FALSE);
    gtk_text_view_set_editable((GtkTextView *) widgets->chat_view, FALSE);

    // set up the disposition of the containers
    gtk_box_pack_end(GTK_BOX(widgets->bottom_hbox), widgets->send_button, FALSE, FALSE, 20);

    gtk_container_add(GTK_CONTAINER (widgets->scroll_text_view), widgets->textView);
    gtk_box_pack_start(GTK_BOX(widgets->bottom_hbox), widgets->scroll_text_view, TRUE, TRUE, 20);



    gtk_box_pack_end(GTK_BOX(widgets->vBox), widgets->bottom_hbox, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(widgets->vBox), widgets->chat_view, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER (widgets->main_frame), widgets->vBox);
    
    gtk_container_add (GTK_CONTAINER (window), widgets->main_frame);

}

void show_chatroom(chatroom_widgets * widgets) {

    gtk_widget_show_all (widgets->main_frame);                                 // show the window

}

void destroy_chatroom(chatroom_widgets * widgets) {

    gtk_widget_destroy (widgets->main_frame);                                 // show the window

}
