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


#include "../structs/structs.h"
//#include "../ui/login_view.h"
#include "../ui/chat_view.h"


void login_callback(int);
int init_connection(login_data *);
void check_data();
void init_login(GtkWidget *);
void show_login();
void destroy_login();

chatroom_widgets *widgets;
login_widgets *lgn_widgets;
GtkWidget *window;
int servfd;

void login_callback(int fd) {
  servfd = fd;
  init_chatroom(window, widgets, fd);
  show_chatroom(widgets);
}


void startup(GtkApplication* app, gpointer user_data){
    widgets = malloc(sizeof(chatroom_widgets));
    lgn_widgets = malloc(sizeof(lgn_widgets));
    window = malloc(sizeof(GtkWindow));
    puts("Startup done");
}

void activate (GtkApplication* app, gpointer user_data) {

    puts("Activating");
    window = gtk_application_window_new (GTK_APPLICATION (app));

    init_login(window);
    show_login();
    gtk_widget_show (window);
    puts("Activated");
}

void shutdown_ (GtkApplication* app, gpointer user_data) {
  close(servfd);
}


int main (int argc, char **argv) {
  if (argc != 1) {
    perror("Prease launch the application without arguments");
    exit(1);
  }
  puts("Starting");
  GtkApplication *app;  // pointer to the app instance
  int p = 0;
  int status;           // return status of the application

  srand(time(NULL));   // Initialization, should only be called once.
  int r = rand();

  char buff[30];

  snprintf(buff, 30, "it.sanchietti.cchat%d", r);

  app = gtk_application_new (buff, G_APPLICATION_FLAGS_NONE);    // creation of the app

  g_signal_connect (app, "startup", G_CALLBACK (startup), &p);

  g_signal_connect (app, "activate", G_CALLBACK (activate), &p);      // connect the app to the activate function.

  status = g_application_run (G_APPLICATION (app), argc, argv);         // run the app.

  g_object_unref (app);

  return status;
}

login_data *data;

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int init_connection(login_data *data) {

    // connecting to the server
    puts("Connecting to the server\n");

    struct sockaddr_in *serv_addr;

    int portno = atoi(data->port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0) {
      error("Error while openeng socket.\n");
      return -1;
    }
    
    serv_addr = malloc(sizeof(struct sockaddr_in));
    
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(portno); 
    if(inet_pton(AF_INET, data->host, &(serv_addr->sin_addr))<=0) {
        printf("\n inet_pton error occured\n");
        return -1;
    }

    if( connect(sockfd, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) < 0) {
       printf("\n Error : Connect Failed \n");
       return -1;
    }
    puts("Connected to the server\n");

    return sockfd;
}

void check_data() {
    int correct = 1;
    data = malloc(sizeof(login_data));
    
    data->nickname = gtk_entry_get_text (GTK_ENTRY(lgn_widgets->nickname_field));
    
    data->host = gtk_entry_get_text (GTK_ENTRY(lgn_widgets->host_field));
    
    data->port = gtk_entry_get_text (GTK_ENTRY(lgn_widgets->port_field));
    

    struct in_addr address;

    if(!inet_pton(AF_INET, data->host, &(address))){
        gtk_entry_set_text (GTK_ENTRY(lgn_widgets->host_field),"");
        correct = 0;
    }

    int port = atoi(data->port);
    printf("%d\n", port);
    if(port > 65535 || port <= 0){
        gtk_entry_set_text (GTK_ENTRY(lgn_widgets->port_field),"");
        correct = 0;
    }

    if (correct) {
        puts("Data are correct.");
        int fd = init_connection(data);
        if (fd > 0) {
            destroy_login();

            login_callback(fd);
            // TODO call the function that shows the chat and pass the fd.
        }
        else{
            perror("Failed to init connection");
            free(data);
        }
    }
    else{
        free(data);
    }    
}


/*
this file contains the function that sets up the connection.
returns the informations of the connection.
*/

void init_login(GtkWidget *window) {
    puts("init login");
    lgn_widgets->main_frame = gtk_frame_new("Login");              // creation of the new window
    
    // vBox will contain the login text areas.
    lgn_widgets->vBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(lgn_widgets->vBox), FALSE);
  puts("1");
    // nickname field
    lgn_widgets->nickname_field = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(lgn_widgets->nickname_field), 0.5f);
    gtk_entry_set_max_length (GTK_ENTRY(lgn_widgets->nickname_field), 20);
    // host_field field
    puts("2");
    lgn_widgets->host_field = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(lgn_widgets->host_field), 0.5f);
    // port field
    puts("3");
    lgn_widgets->port_field = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(lgn_widgets->port_field), 0.5f);
    char send_button_text[8];
    snprintf(send_button_text, 8, "connect");
  puts("4");
    lgn_widgets->connect_button = gtk_button_new_with_label(send_button_text);
    gtk_widget_set_size_request (lgn_widgets->connect_button, 60, 20);
    gtk_widget_set_hexpand ((GtkWidget *) lgn_widgets->connect_button, FALSE);
    gtk_widget_set_halign ((GtkWidget *) lgn_widgets->connect_button, GTK_ALIGN_FILL);
  puts("5");
    g_signal_connect(G_OBJECT(lgn_widgets->connect_button), "clicked", G_CALLBACK(check_data), NULL);
  puts("6");
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), lgn_widgets->nickname_field, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), lgn_widgets->host_field, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), lgn_widgets->port_field, FALSE, FALSE, 20);

    gtk_box_pack_start(GTK_BOX(lgn_widgets->vBox), lgn_widgets->connect_button, FALSE, FALSE, 20);

    gtk_container_add (GTK_CONTAINER (lgn_widgets->main_frame), lgn_widgets->vBox);

    gtk_container_add (GTK_CONTAINER (window), lgn_widgets->main_frame);
    puts("end login");
}

void show_login() {

    gtk_widget_show_all (lgn_widgets->main_frame);                                 // show the window

}

void destroy_login() {

    gtk_widget_destroy (lgn_widgets->main_frame);                                 // show the window
    
}
