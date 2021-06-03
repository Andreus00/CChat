/*
Andrea Snachietti

client con gui per CChat

Versione con la gui del client. Utilizza la libreria grafica gtk3+.
Per compilare il codice è necessario installare la libreria per i dev di gtk3+.
L'installazione è automatica se si usa il makefile con make all-gu
*/

#include <stdio.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>


#include "../structs/structs.h"
#include "../../utility/chat_message.h"
#include "../../utility/chat_log.h"


int init_connection(login_data *);
void check_data();
void init_login();
void destroy_login(login_widgets *);
void destroy_chatroom(chatroom_widgets *);
void init_chatroom(struct application_data *);

// struttura usata per mantenere le informazioni sull'applicazione
struct application_data *app_data;
// struttura usata per mantenere le informazioni sui parametri usati per il login
login_data *data;

int run_app = 1;

// funzione chiamata una volta che il login è completo. Essa inizializza la gui della chatroom.
void login_callback(int fd) {
  app_data->servfd = fd;
  init_chatroom(app_data);
}

// funzione usata per lo startup dell'applicaizone.
// viene connessa a gtk che la chiamerà nel momento dello startup.
// La funzione aloca lo spazio che servirà all'applicazione
void startup(GtkApplication* app, gpointer user_data){
    app_data->widgets = malloc(sizeof(chatroom_widgets));
    app_data->lgn_widgets = malloc(sizeof(login_widgets));
    app_data->lgn_widgets->nickname_field = malloc(sizeof(char *));
    app_data->window = malloc(sizeof(GtkWindow));
    puts("Startup done");
}

// funzione chiamata da gtk dopo la startup.
// crea la finestra, inizializza la gui del login e fa apparire la finestra a schermo.
void activate (GtkApplication* app, gpointer user_data) {

    puts("Activating");
    app_data->window = gtk_application_window_new (GTK_APPLICATION (app));

    init_login(app_data);
    gtk_widget_show (app_data->window);
    puts("Activated");
}
// funzione chiamata quando l'applicazione deve essere chiusa.
// chiude il file descriptor.
void shutdown_app (GtkApplication* app, gpointer user_data) {
  close(app_data->servfd);
  run_app = 0;

  // g_object_unref (app);
}

// funzione main del client.
// Per prima cosa viene controllato che l'utente non abbia passato argomenti,
// poi viene allcoato spazio per app_data, viene inizializzata l'appicazione,
// vengono connessi i segnali di startup, activate e shutdown alle funzioni corrispondenti
// e infine viene fatta partire l'applicazione.
int main (int argc, char **argv) {
  if (argc != 1) {
    perror("Prease launch the application without arguments");
    exit(1);
  }
  // allocazione di memoria per la struttura app_data
  app_data = malloc(sizeof(struct application_data));

  puts("Starting");
  // puntatore all'istanza dell'applicazione
  GtkApplication *app;
  // return status dell'applicazione
  int status;           

  // a questo punto devo generare un nome unico per l'applicazione che sto inizializzando (altrimenti la presenza di più applicazioni con lo stesso nome potrebbe far crashare entrambe)
  // Per fare ciò genero un numero randomico e lo includo nel nome (le possibilità di due nomi identici così è quasi impossibile);
  srand(time(NULL));
  int r = rand();

  char buff[30];    // buffer per il nome
  memset(buff, 0, 30);

  snprintf(buff, 30, "it.sanchietti.cchat%d", r);

  // creazione dell'applicazione
  app = gtk_application_new (buff, G_APPLICATION_FLAGS_NONE);

  // connessione all'applicazione dei segnali di startup, activate e shutdown
  g_signal_connect (app, "startup", G_CALLBACK (startup), NULL);

  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  g_signal_connect (app, "shutdown", G_CALLBACK (shutdown_app), NULL);

  // avvio dell'applicazione
  status = g_application_run (G_APPLICATION (app), argc, argv);

  return status;
}

////////////////////////////////////////////////////////////// LOGIN VIEW
/*
funzione usata per printare nello stderr l'errore passato in input e far
terminare l'applicazione ritornando 1.
*/
void error(char *msg)
{
    perror(msg);
    exit(1);
}
/*
Funzione che usa i dati passati in input e inizializza la connessione con il server.
Se ci sono errori durante la connessione o il server non accetta il nickname, la funzione ritorna -1.
Se non ci sono errori viene ritornato il file descriptor del server.
*/
int init_connection(login_data *data) {

    // connecting to the server
    puts("Connecting to the server\n");

    //inizializzazione delle strutture e dei parametri che servono ai fini della connessione
    struct sockaddr_in *serv_addr;
    serv_addr = malloc(sizeof(struct sockaddr_in));

    int portno = atoi(data->port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // check sul socket
    if(sockfd < 0) {
      perror("Error while openeng socket.\n");
      free(serv_addr);
      return -1;
    }
    
    // inizializzazione di serv_addr
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(portno); 
    if(inet_pton(AF_INET, data->host, &(serv_addr->sin_addr))<=0) {
        perror("\n inet_pton error occured\n");
        free(serv_addr);
        return -1;
    }
    // tentativo di connessione e check degli errori
    if( connect(sockfd, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) < 0) {
       perror("\n Error : Connect Failed \n");
       free(serv_addr);
       return -1;
    }
    puts("Connected to the server\n");
    // comunicazione del nickname al server
    if (write(sockfd, data->nickname, strlen(data->nickname)) < 0) {
        perror("Error while sending the nickname to the server");
        free(serv_addr);
        close(app_data->servfd);
        return -1;
    }

    // lettura della risposta del server.
    // se il server non accetta il nickname la funzione ritorna -1
    char response[2];
    response[0] = '\0';
    response[1] = '\0';

    if (read(sockfd, response, 1) <= 0) {
        free(serv_addr);
        close(app_data->servfd);
        perror("The server closed the connection");
        return -1;
    }

    if (response[0] == 48) {
      puts("nickname already in use.");
      free(serv_addr);
      close(app_data->servfd);
      return -1;
    }
    // lettura della mode del server
    if (read(sockfd, response, 1) <= 0) {
        free(serv_addr);
        close(app_data->servfd);
        perror("The server closed the connection");
        return -1;
    }

    if (response[0] == '0') {
        data->mode = TIMESTAMP_MODE;
        printf("mode : %d", data->mode);
    }
    else {
        data->mode = RECEIVE_MODE;
        printf("mode : %d", data->mode);
    }

    return sockfd;
}

/*
Funzione usata dal reader thread. Essa legge dal file descriptor del server i messaggi in arrivo e li mostra a schermo.
*/
void *reader(void *info) {

    struct application_data *app_data = (struct application_data *)info;
    //recupero del text buffer della chat
    GtkTextBuffer *textBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW((app_data->widgets->chat_view)));
    // crea i tag per colorare il testo 
    gtk_text_buffer_create_tag(textBuffer, "lmarg", "left_margin", 5, NULL);
    gtk_text_buffer_create_tag(textBuffer, "red_fg", "foreground", "red", NULL); 
    // recupera un GtkTextIter del buffer. Un GtkTextIter non è altro che un cursore del GtkTextBuffer usato per scrivere o leggere da esso.
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset(textBuffer, &iter, 0);
    char *buf;
    // while infinito per la lettura
    while (run_app) {
        // lettura dal fine descriptor del messaggio
        buf = read_fd(app_data->servfd);
        // check sul buffer
        if(buf != NULL) {
            // rimozione dei caratteri speciali usati dal server per colorare l'output da terminale
            int read_len = strlen(buf);
            char *filtered_text = calloc(read_len, sizeof(char));
            int i = 0;
            int j = 0;
            while (j < read_len) {
              if (buf[j] == '\033') {
                j += 4;
                if (buf[j] == 'm') j++;
              }
              else filtered_text[i++] = buf[j++];
            }
            // scrittura del testo filtrato nel text buffer.
            gtk_text_buffer_insert(textBuffer, &iter, filtered_text, -1);
            free(buf);
        }
        // se il buffer è NULL il server ha chiuso la connessione. Quindi ritorno nella schermata di login.
        else {
            perror("\033[31mServer closed the connection\033[0m");
            gtk_text_buffer_insert_with_tags_by_name(textBuffer, &iter, "Server Disconnected\n", -1, "red_fg", "lmarg",  NULL);
            sleep(5);
            destroy_chatroom(app_data->widgets);
            close(app_data->servfd);
            init_login();
            return 0;
        }
    }
}
/*
Funzione colelgata al bottone di connessione al server.
Quando il bottone viene premuto, la funzione va a recuperare le informazioni dai campi, controlla
che essi vadano bene e prova a connettersi al server. Se la connessione avviene con successo
viene distrutta la schermata di login e viene chiamata la funzione login_callback che crea e mostra
la schermata della chat.
*/
void check_data() {
    // valore di guardia usato per controllare che i dati inseriti siano corretti
    int correct = 1;
    // allocazione dello spazio per i dati
    data = malloc(sizeof(login_data));
    // recupero del nickname
    const char *buf = gtk_entry_get_text (GTK_ENTRY(app_data->lgn_widgets->nickname_field));
    if (strlen(buf) == 0) {
        perror("enter a nickname");
        correct = 0;
    }
    data->nickname = malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(data->nickname, buf);
    // recupero dell'host
    data->host = gtk_entry_get_text (GTK_ENTRY(app_data->lgn_widgets->host_field));
    // recupero della porta
    data->port = gtk_entry_get_text (GTK_ENTRY(app_data->lgn_widgets->port_field));
    
    // struttura usata per le informazioni sull'host
    struct in_addr address;
    // conversione dell'host da stringa a addres per AF_INET
    if(!inet_pton(AF_INET, data->host, &(address))){
        gtk_entry_set_text (GTK_ENTRY(app_data->lgn_widgets->host_field),"");
        correct = 0;
    }
    printf("host : %s\n", data->host);
    // check sulla porta
    int port = atoi(data->port);
    if(port > 65535 || port <= 0){
        gtk_entry_set_text (GTK_ENTRY(app_data->lgn_widgets->port_field),"");
        correct = 0;
    }
    printf("port : %d\n", port);
    // se host, porta e nickname sono corretti
    if (correct) {
        puts("Data are correct.");
        // tentativo di connessione
        int fd = init_connection(data);
        // se in tentativo di connessione è andato a buon fine passo alla chat view
        if (fd >= 0) {
            destroy_login(app_data->lgn_widgets);
            login_callback(fd);
            // creo il reader thread per la chat
            pthread_t tid;
            if(pthread_create(&tid, NULL, &reader, app_data) < 0) {
                error("Error while creating the reader thread");
            }
        }
        else{
            perror("Failed to init connection");
            free(data->nickname);
            free(data);
        }
    }
    else{
        perror("Wrong host or port");
        free(data->nickname);
        free(data);
    }    
}


/*
Funzione che inizializza la gui per il login
*/

void init_login() {


    // recupero dei widgets
    login_widgets *lgn_widgets = app_data->lgn_widgets;
    puts("init login");
    // creo il main frame
    lgn_widgets->main_frame = gtk_frame_new("Login");              // creation of the new window
    gtk_widget_set_size_request (GTK_WIDGET(lgn_widgets->main_frame), 500, 500);
    
    // vBox che conterrà i field e i label del nickname, dell'host e della porta
    lgn_widgets->vBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(lgn_widgets->vBox), FALSE);


    // title label
    GtkWidget *title_label = gtk_label_new ("Welcome to CChat");
    PangoFontDescription *df;
    gtk_label_set_markup(GTK_LABEL(title_label),
    "<span face=\"Ubuntu Mono\" size=\"40960\" weight=\"bold\" color=\"green\">Welcome to CChat</span>");
    gtk_label_set_xalign ((GtkLabel *) title_label, 0.5);


    // nickname field
    lgn_widgets->nickname_field = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(lgn_widgets->nickname_field), 0.5f);
    gtk_entry_set_max_length (GTK_ENTRY(lgn_widgets->nickname_field), 20);
    // nickname label
    GtkWidget *nickname_label = gtk_label_new ("Nickname");
    gtk_label_set_xalign ((GtkLabel *) nickname_label, 0.5);
    gtk_label_set_justify ((GtkLabel *) nickname_label, GTK_JUSTIFY_CENTER);
    // host_field field
    lgn_widgets->host_field = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(lgn_widgets->host_field), 0.5f);
    // host label
    GtkWidget *host_label = gtk_label_new ("Host");
    gtk_label_set_xalign ((GtkLabel *) host_label, 0.5);
    gtk_label_set_justify ((GtkLabel *) host_label, GTK_JUSTIFY_CENTER);
    // port field
    lgn_widgets->port_field = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(lgn_widgets->port_field), 0.5f);
    // port label
    GtkWidget *port_label = gtk_label_new ("Port");
    gtk_label_set_xalign ((GtkLabel *) port_label, 0.5);
    gtk_label_set_justify ((GtkLabel *) port_label, GTK_JUSTIFY_CENTER);

    // inizializzazione del bottone di connessione
    char connect_button_text[8];
    snprintf(connect_button_text, 8, "connect");
    lgn_widgets->connect_button = gtk_button_new_with_label(connect_button_text);
    gtk_widget_set_size_request (lgn_widgets->connect_button, 60, 20);
    gtk_widget_set_hexpand ((GtkWidget *) lgn_widgets->connect_button, FALSE);
    gtk_widget_set_halign ((GtkWidget *) lgn_widgets->connect_button, GTK_ALIGN_FILL);
    // connessione del bottone alla funzione di connessione
    g_signal_connect(G_OBJECT(lgn_widgets->connect_button), "clicked", G_CALLBACK(check_data), app_data);

    // posizionamento dei vari Widgets all'iterno della finestra
    /*
    window
        |main frame
            |vBox
                |title label
                |nickname label
                |nickname field
                |host label
                |host field
                |port label
                |port field
                |conenction button
    */
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), title_label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), nickname_label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), lgn_widgets->nickname_field, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), host_label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), lgn_widgets->host_field, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), port_label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), lgn_widgets->port_field, FALSE, FALSE, 20);

    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), lgn_widgets->connect_button, FALSE, FALSE, 20);

    gtk_container_add (GTK_CONTAINER (lgn_widgets->main_frame), lgn_widgets->vBox);

    gtk_container_add (GTK_CONTAINER (app_data->window), lgn_widgets->main_frame);

    gtk_widget_show_all (app_data->lgn_widgets->main_frame);                                 // show the window
    puts("end login");
}
/*
funzione che distrugge i widgets del login
*/
void destroy_login(login_widgets *lgn_widgets) {

    gtk_widget_destroy (lgn_widgets->main_frame);                                 // show the window
    
}


//////////////////////////////////////////////////////// CHATROOM VIEW

/*
Funzione associata al bottone per mandare il messaggio.
Questa funzione recupera il testo scritto dall'utente, toglie i caratteri '\n' e ' ' dalla fine,
trova il primo carattere che non sia uno '\n' o uno ' ', controlla che l'utente abbia scritto almeno un carattere e,
se è così, manda il messaggioal server (seguendo la mode del server), fa il log del messaggio.

*/
int send_message() {
    // recupero gli iter dell'inizio e della fine del messaggio scritto dall'utente
    GtkTextIter start_iter, end_iter;

    gtk_text_buffer_get_bounds (app_data->widgets->textBuffer, &start_iter, &end_iter);
    // recupero il messaggio
    char *text = gtk_text_buffer_get_text(app_data->widgets->textBuffer, &start_iter, &end_iter, FALSE);

    // creo una struttura mew_message per impacchettare il messaggio
    chat_message *new_message = malloc(sizeof(chat_message));
    // imposto il sender del new_message
    new_message->sender = data->nickname;
    // elimino tutti gli '\n' e i ' ' finali
    int textlen = strlen(text);
    int last_char = textlen - 1;
    while (last_char > 0 && (text[last_char] == '\n' || text[last_char] == ' ')) text[last_char--] = '\0';
    // vado a recuperare l'indice del primo carattere che non sia uno '\n' o uno ' '
    int first_char = 0;
    while (first_char < textlen && (text[first_char] == '\n' || text[first_char] == ' ')) first_char++;
    // check per vedere se lo user non ha scritto niente o ha scritto solo '\n' e ' '
    if (first_char <= last_char){
        // vado a creare un filtered_message che no nè altro che un messaggio senza '\n' e ' ' all'inizio e alla fine
        puts("Sending message");
        char *filtered_message = calloc(last_char - first_char + 3, sizeof(char));
        strncpy(filtered_message, text + first_char, last_char - first_char + 1);
        strcat(filtered_message, "\n");
        new_message->message = filtered_message;

        // invio il messaggio al server seguendo la mode di esso
        if (data->mode == RECEIVE_MODE) {
            new_message->time = get_current_time();
            if (write(app_data->servfd, filtered_message, last_char - first_char + 4) < 0) {
                perror("error while sending the message");
            }
        }
        else {
            new_message->time = get_current_time_u();

            unsigned int msg_len = strlen(new_message->sender) + strlen(new_message->time) + strlen(new_message->message) + 8;
            
            char assembled_message[msg_len];
            
            snprintf(assembled_message, msg_len, "[%s, %s] %s", new_message->sender, new_message->time, new_message->message);
            
            if (write(app_data->servfd, assembled_message, msg_len) < 0) {
                perror("Error while sending the message");
            }
        }
        // log del messaggio e free del testo (la memoria occupata da filtered_message sarà liberata da chat_log)
        chat_log(new_message, data->mode);
        free(text);

        // elimino il messaggio dal GtkTextBuffer dell'input
        memset(&start_iter, 0, sizeof(start_iter));

        gtk_text_buffer_get_iter_at_offset (app_data->widgets->textBuffer, &start_iter, 0);

        gtk_text_buffer_delete (app_data->widgets->textBuffer, &start_iter, &end_iter);
    }

    return 0;
}
/*
Funzione che inizializza la gui della chat
*/
void init_chatroom(struct application_data *app_data) {

    puts("init chatroom");
    
    chatroom_widgets *widgets = app_data->widgets;

    // creazione del main frame
    widgets->main_frame = gtk_frame_new ("CChat");
    
    // vBox che conterrà in alto la chat e in basso una hBox con la zona per l'input e quella per il bottone di invio dei messaggi
    widgets->vBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(widgets->vBox), FALSE);

    // creazione e set del GtkTextBuffer per l'input
    widgets->textBuffer = gtk_text_buffer_new(NULL);
    widgets->textView = gtk_text_view_new_with_buffer(widgets->textBuffer);
    gtk_widget_set_halign ((GtkWidget *) widgets->textView, GTK_ALIGN_FILL);

    
    // creo una scroll view al cui interno mettere la zona per l'input
    widgets->scroll_text_view = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_min_content_width ((GtkScrolledWindow *) widgets->scroll_text_view, 300);
    gtk_scrolled_window_set_max_content_height ((GtkScrolledWindow *) widgets->scroll_text_view, 150);
    gtk_scrolled_window_set_min_content_height ((GtkScrolledWindow *) widgets->scroll_text_view, 50);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) widgets->scroll_text_view, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand ((GtkWidget *) widgets->scroll_text_view, TRUE);
    gtk_widget_set_halign ((GtkWidget *) widgets->scroll_text_view, GTK_ALIGN_FILL);

    // creo una scroll view al cui interno mettere la zona per la chat
    widgets->scroll_chat_view = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) widgets->scroll_text_view, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand ((GtkWidget *) widgets->scroll_text_view, TRUE);
    gtk_widget_set_halign ((GtkWidget *) widgets->scroll_text_view, GTK_ALIGN_FILL);

    // hBox inferiore al cui interno mettere la zona per l'input e il bottone per l'invio dei messaggi
    widgets->bottom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(widgets->bottom_hbox), FALSE);

    // creazione del bottone per l'invio dei messaggi e connessione del bottone alla funzione send_message()
    char *send_button_text = "send";
    widgets->send_button = gtk_button_new_with_label(send_button_text);
    gtk_widget_set_size_request (widgets->send_button, 60, 20);
    gtk_widget_set_hexpand ((GtkWidget *) widgets->send_button, FALSE);
    gtk_widget_set_halign ((GtkWidget *) widgets->send_button, GTK_ALIGN_FILL);

    g_signal_connect(G_OBJECT(widgets->send_button), "clicked", G_CALLBACK(send_message), NULL);

    // creazione della zona per la chat

    widgets->chat_view = gtk_text_view_new();
    gtk_text_view_set_cursor_visible ((GtkTextView *) widgets->chat_view, FALSE);
    gtk_text_view_set_editable((GtkTextView *) widgets->chat_view, FALSE);

    // posizionamento dei vari widget all'interno della finestra
    /*
    window
        |main frame
            |vBox
                |scroll_chat_view
                |   |chat_view
                |
                |bottom_hbox
                    |send_button
                    |scroll_text_view
                        |textView
                    
    */

    gtk_container_add(GTK_CONTAINER (widgets->scroll_chat_view), widgets->chat_view);
    gtk_container_add(GTK_CONTAINER (widgets->scroll_text_view), widgets->textView);
    gtk_box_pack_end(GTK_BOX(widgets->bottom_hbox), widgets->send_button, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(widgets->bottom_hbox), widgets->scroll_text_view, TRUE, TRUE, 20);



    gtk_box_pack_end(GTK_BOX(widgets->vBox), widgets->bottom_hbox, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(widgets->vBox), widgets->scroll_chat_view, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER (widgets->main_frame), widgets->vBox);
    
    gtk_container_add (GTK_CONTAINER (app_data->window), widgets->main_frame);

    gtk_widget_show_all (widgets->main_frame);

}

/*
Funzione che distrugge la chat_view
*/
void destroy_chatroom(chatroom_widgets * widgets) {

    gtk_widget_destroy (widgets->main_frame);                                 // show the window

}
