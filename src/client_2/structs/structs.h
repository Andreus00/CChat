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
    GtkWidget *scroll_chat_view;
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

enum chat_mode {TIMESTAMP_MODE, RECEIVE_MODE};

/*
    once the user enters the login data, this struct will contain all the pointers pointing to the information.
*/
typedef struct {
    char *nickname;
    const char *host;
    const char *port;
    enum chat_mode mode;
}login_data;


struct application_data{
  chatroom_widgets *widgets;
  login_widgets *lgn_widgets;
  GtkWidget *window;
  int servfd;
};


/*
struttura che contiene le informazioni riguardanti un messaggio
*/
typedef struct {
    const char *sender;   // puntatore al nickname di chi ha mandato il messaggio
    char *message;  // puntatore al contenuto del messaggio
    char *time;     // puntatore al time del messaggio
}chat_message;


