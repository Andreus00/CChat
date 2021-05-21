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

void login_callback(int);
void destroy_login(login_widgets * );
int init_connection(login_data *);
void check_data();
void init_login(GtkWidget *, login_widgets *);
void show_login(login_widgets * );
void destroy_login(login_widgets * );

login_widgets *w;
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
    
    data->nickname = gtk_entry_get_text (GTK_ENTRY(w->nickname_field));
    
    data->host = gtk_entry_get_text (GTK_ENTRY(w->host_field));
    
    data->port = gtk_entry_get_text (GTK_ENTRY(w->port_field));
    

    struct in_addr address;

    if(!inet_pton(AF_INET, data->host, &(address))){
        gtk_entry_set_text (GTK_ENTRY(w->host_field),"");
        correct = 0;
    }

    int port = atoi(data->port);
    printf("%d\n", port);
    if(port > 65535 || port <= 0){
        gtk_entry_set_text (GTK_ENTRY(w->port_field),"");
        correct = 0;
    }

    if (correct) {
        puts("Data are correct.");
        int fd = init_connection(data);
        if (fd > 0) {
            destroy_login(w);

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

void init_login(GtkWidget *window, login_widgets *widgets) {
    puts("init login");
    w = widgets;

    widgets->main_frame = gtk_frame_new("Login");              // creation of the new window
    
    // vBox will contain the login text areas.
    widgets->vBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(widgets->vBox), FALSE);

    // nickname field
    widgets->nickname_field = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(widgets->nickname_field), 0.5f);
    gtk_entry_set_max_length (GTK_ENTRY(widgets->nickname_field), 20);
    // host_field field
    widgets->host_field = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(widgets->host_field), 0.5f);
    // port field
    widgets->port_field = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(widgets->port_field), 0.5f);
    char *send_button_text = "connect";

    widgets->connect_button = gtk_button_new_with_label(send_button_text);
    gtk_widget_set_size_request (widgets->connect_button, 60, 20);
    gtk_widget_set_hexpand ((GtkWidget *) widgets->connect_button, FALSE);
    gtk_widget_set_halign ((GtkWidget *) widgets->connect_button, GTK_ALIGN_FILL);

    g_signal_connect(G_OBJECT(widgets->connect_button), "clicked", G_CALLBACK(check_data), NULL);

    gtk_box_pack_start(GTK_BOX(widgets->vBox), widgets->nickname_field, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(widgets->vBox), widgets->host_field, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(widgets->vBox), widgets->port_field, FALSE, FALSE, 20);

    gtk_box_pack_start(GTK_BOX(widgets->vBox), widgets->connect_button, FALSE, FALSE, 20);

    gtk_container_add (GTK_CONTAINER (widgets->main_frame), widgets->vBox);

    gtk_container_add (GTK_CONTAINER (window), widgets->main_frame);
    puts("end login");
}

void show_login(login_widgets * widgets) {

    gtk_widget_show_all (widgets->main_frame);                                 // show the window

}

void destroy_login(login_widgets * widgets) {

    gtk_widget_destroy (widgets->main_frame);                                 // show the window
    
}
