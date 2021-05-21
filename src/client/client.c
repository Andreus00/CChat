#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>


#include "structs.h"

int init_connection();


void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main (int argc, char **argv) {
    if (argc != 1) {
    perror("Prease launch the application without arguments");
    exit(1);
    }
    puts("Starting");

    puts(" _    _      _                            _          _____  _____ _           _   ");
    puts("| |  | |    | |                          | |        /  __ \\/  __ \\ |         | |  ");
    puts("| |  | | ___| | ___ ___  _ __ ___   ___  | |_ ___   | /  \\/| /  \\/ |__   __ _| |_ ");
    puts("| |/\\| |/ _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\ | __/ _ \\  | |    | |   | '_ \\ / _` | __|");
    puts("\\  /\\  /  __/ | (_| (_) | | | | | |  __/ | || (_) | | \\__/\\| \\__/\\ | | | (_| | |_ ");
    puts(" \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___|  \\__\\___/   \\____/ \\____/_| |_|\\__,_|\\__|");
    puts("\n");
    
    init_connection();

    return 0;
}


int init_connection() {

    login_data *data;                                   // struct that will contain the info for the login

    struct sockaddr_in *serv_addr;                      // struct that will contain the info of the server

    int portno;                                         // variable that will contain the port number

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);       // file descriptor of the socket

    if(sockfd < 0) {
        error("Error while openeng socket.\n");
        return -1;
    }

    data = malloc(sizeof(login_data));

    data->host = calloc(40, sizeof(char));
    
    serv_addr = malloc(sizeof(struct sockaddr_in));
    serv_addr->sin_family = AF_INET;

    int connection_succeded;
    
    int condition = 1;

    do {


        while (condition) {
            puts("Please enter the server address: ");

            int n = scanf("%s",data->host);
            fflush(stdin);

            while ( n <= 0) {
                puts("Please enter a valid server address.");
                n = scanf("%s",data->host);
                fflush(stdin);
            }

            fflush(stdin);
            if(inet_pton(AF_INET, data->host, &(serv_addr->sin_addr)) > 0) {
                
                puts("Please enter the port: ");

                n = scanf("%d",&data->port);
                fflush(stdin);

                while ( n <= 0) {
                    puts("Please enter a valid port.");
                    n = scanf("%d",&data->port);
                    fflush(stdin);
                }
                if(0 > data->port || data->port > 65536)
                    puts("Invalid port.");
                else
                    condition = 0;
            }
        }

        // connecting to the server
        puts("Connecting to the server\n");

        

        if( (connection_succeded = connect(sockfd, (struct sockaddr *)serv_addr, sizeof(*serv_addr))) < 0) {
            printf("\n Error : Connect Failed \n");
            printf("Retry with %s:%d? Y/n", data->host, data->port);
            char c[1];
            fflush(stdin);
            scanf("%1s", c);
            if ( (int) c[0] != (int) 'Y' && (int) c[0] != (int) 'y')
                condition = 1;
            fflush(stdin);
        }
        else {
            puts("Connected to the server\n");
        }        

    } while (connection_succeded < 0);


    puts("Enter your nickname: ");

    int n = scanf("%20s",data->nickname);

    char response[2];
    fflush(stdin);

    while ( n <= 0) {
        puts("Please enter a valid nickname.");
        n = scanf("%20s",data->nickname);
        fflush(stdin);
    }

    write(sockfd, data->nickname, strlen(data->nickname));

    read(sockfd, response, 1);

    while (response[0] == 0) {
        puts("invalid nickname, please chose a new one: ");
        n = scanf("%20s",data->nickname);

        fflush(stdin);

        while ( n <= 0) {
            puts("Please enter a valid nickname.");
            n = scanf("%20s",data->nickname);
            fflush(stdin);
        }

        write(sockfd, data->nickname, strlen(data->nickname));

        read(sockfd, response, 1);
    }

    return sockfd;
}
