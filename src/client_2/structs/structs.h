#include <gtk/gtk.h>
/*
Struttura che contiene i puntatori ai widget della chat_view
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
Struttura che contiene i puntatori ai widget della login_view
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
enum della mode
*/
enum chat_mode {TIMESTAMP_MODE, RECEIVE_MODE};

/*
struttura usata per contenere le informazioni sul login
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


