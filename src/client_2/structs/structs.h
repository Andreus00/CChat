#include <gtk/gtk.h>
/*
    this struct is used to contain all the widgets of the chatroom view
*/
typedef struct {
    GtkWidget *main_frame;
    GtkWidget *textView;
    GtkTextBuffer *textBuffer;
    GtkWidget *vBox;
    GtkWidget *scroll_text_view;
    GtkWidget *bottom_hbox;
    GtkWidget *send_button;
    GtkWidget *chat_view;
}chatroom_widgets;

/*
    this struct is used to contain all the widgets of the login view
*/
typedef struct {
    GtkWidget *main_frame;
    GtkWidget *nickname_field;
    GtkWidget *vBox;
    GtkWidget *connect_button;
    GtkWidget *host_field;
    GtkWidget *port_field;
} login_widgets;

/*
    once the user enters the login data, this struct will contain all the pointers pointing to the information.
*/
typedef struct {
    const char *nickname;
    const char *host;
    const char *port;
}login_data;
