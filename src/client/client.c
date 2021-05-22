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
#include <sys/ioctl.h>
#include <pthread.h>

#include "../utility/dinamic_list.h"
#include "structs.h"

#define MESSAGE_BUFFER_SIZE 256

login_data *init_connection();
int start_chat(login_data *);


void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main (int argc, char **argv) {
    if (argc != 1) {
    perror("Please launch the application without arguments");
    exit(1);
    }
    puts("Starting");
  
    /*
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    printf ("lines %d\n", w.ws_row);
    printf ("columns %d\n", w.ws_col);
    */
    
    fflush(stdout);
    puts("___________________________________________________________________________________________");
    puts("|    _    _      _                            _          _____  _____ _           _        |");
    puts("|   | |  | |    | |                          | |        /  __ \\/  __ \\ |         | |       |");
    puts("|   | |  | | ___| | ___ ___  _ __ ___   ___  | |_ ___   | /  \\/| /  \\/ |__   __ _| |_      |");
    puts("|   | |/\\| |/ _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\ | __/ _ \\  | |    | |   | '_ \\ / _` | __|     |");
    puts("|   \\  /\\  /  __/ | (_| (_) | | | | | |  __/ | || (_) | | \\__/\\| \\__/\\ | | | (_| | |_      |");
    puts("|    \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___|  \\__\\___/   \\____/ \\____/_| |_|\\__,_|\\__|     |");
    puts("|__________________________________________________________________________________________|\n");
    fflush(stdout);

    login_data *log_data = init_connection();
    if (log_data == NULL){
        error("An error ocurred.");
    }

    start_chat(log_data);



    return 0;
}

char *get_current_time() {

    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    return asctime (timeinfo);
}

int print_message(char* message) {
    printf("\r%s", message);
    return 0;
}

int send_message(dinamic_list *msg, login_data *log_data) {
    puts("sending");
    unsigned int msg_length = 256 * msg->last;
    char *msg_buffer = calloc(msg_length, sizeof(char));
    for (long i = 0; i < msg->last; i++) {
        strcat(msg_buffer, msg->list[i]);
    }
    write(log_data->fd, msg_buffer, msg_length);

    return 0;
}

/**
 * this functions reads and returns an ar
*/
char* readinput() {
#define CHUNK 200
   char* input = NULL;
   char tempbuf[CHUNK];
   size_t inputlen = 0, templen = 0;
   do {
       fgets(tempbuf, CHUNK, stdin);
       templen = strlen(tempbuf);
       input = realloc(input, inputlen+templen+1);
       strcpy(input+inputlen, tempbuf);
       inputlen += templen;
    } while (templen==CHUNK-1 && tempbuf[CHUNK-2]!='\n');
    return input;
}



void *reader(void *log_data) {
    #define SIZE_OF_BUF 200
        char buf[SIZE_OF_BUF];

    while (1) {
        if(read(((login_data *)log_data)->fd, buf, SIZE_OF_BUF-1) > 0) {
            printf("\r");
            printf("\033[1A");
            printf("\033[K");
            printf("%s\n\n", buf);
            printf("\033[1B");
            memset(buf, 0, SIZE_OF_BUF);
        }
    }
}



int start_chat(login_data *log_data) {
    // build the message format
    pthread_t tid;

    pthread_create(&tid, NULL, &reader, (void *)log_data);

    while (1) {
        // read from cmd the text

        char *buff = readinput();

        if(strlen(buff) > 0) {
            write(log_data->fd, buff, strlen(buff));
        }

        free(buff);
        //log here
    }
}


login_data *init_connection() {

    login_data *data;                                   // struct that will contain the info for the login

    struct sockaddr_in *serv_addr;                      // struct that will contain the info of the server

    // int portno;                                         // variable that will contain the port number

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);       // file descriptor of the socket

    if(sockfd < 0) {
        error("Error while openeng socket.\n");
        return NULL;
    }

    data = malloc(sizeof(login_data));
    data->fd = sockfd;
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
                perror("Please enter a valid server address.");
                n = scanf("%s",data->host);
                fflush(stdin);
            }

            fflush(stdin);
            if(inet_pton(AF_INET, data->host, &(serv_addr->sin_addr)) > 0) {
                
                puts("Please enter the port: ");

                n = scanf("%d",&data->port);
                fflush(stdin);

                while ( n <= 0) {
                    perror("Please enter a valid port. ");
                    n = scanf("%d",&data->port);
                    fflush(stdin);
                }
                if(0 > data->port || data->port > 65536)
                    perror("Invalid port.");
                else {
                    serv_addr->sin_port = htons(data->port);
                    condition = 0;
                }
            }
            else{
                fprintf(stderr,"ERROR, no such host\n");
            }
        }

        // connecting to the server
        printf("Connecting to the server %d:%d\n", serv_addr->sin_addr.s_addr, serv_addr->sin_port);


        

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


    fflush(stdin);
    fflush(stdout);

    data->nickname = calloc(20, sizeof(char));
    puts("Enter your nickname: ");
    char response[2];
    response[1] = '\0';
    int n = scanf("%20s",data->nickname);


    while ( n <= 0) {
        printf("%d", n);
        puts("Please enter a valid nickname.");
        sleep(1);
        fflush(stdin);
        n = fscanf(stdin, "%20s",data->nickname);
    }

    write(sockfd, data->nickname, strlen(data->nickname));

    read(sockfd, response, 1);

    printf("%s", response);

    while (response[0] == '0') {
        puts("invalid nickname, please chose a new one: ");
        n = scanf("%20s",data->nickname);
        fflush(stdin);

        while ( n <= 0) {
            puts("Please enter a valid nickname.");
            n = scanf(".%20s.",data->nickname);
            printf("%s", data->nickname);
        }

        write(sockfd, data->nickname, strlen(data->nickname));

        
        if(read(sockfd, response, 1) <= 0) {
            perror("Server disconnected");
            close(sockfd);
            return NULL;
        }
        printf("response = %s", response);

    }
    puts(">>> Joined");

    return data;
}
