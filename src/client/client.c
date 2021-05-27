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
#include "../utility/structs.h"
#include "../utility/chat_log.h"
#include "../utility/chat_message.h"

#define MESSAGE_BUFFER_SIZE 256
#define HOST_LEN 41
#define PORT_LEN 11
#define NICK_LEN 195

login_data *init_connection();
int start_chat(login_data *);


void error(char *msg)
{
    perror(msg);
    exit(0);
}

int get_term_width() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

int main (int argc, char **argv) {
    if (argc != 1) {
    perror("Please launch the application without arguments");
    exit(1);
    }

    printf("\033[2J");
    
    fflush(stdout);
    puts("\033[32m___________________________________________________________________________________________");
    puts("|    _    _      _                            _          _____  _____ _           _        |");
    puts("|   | |  | |    | |                          | |        /  __ \\/  __ \\ |         | |       |");
    puts("|   | |  | | ___| | ___ ___  _ __ ___   ___  | |_ ___   | /  \\/| /  \\/ |__   __ _| |_      |");
    puts("|   | |/\\| |/ _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\ | __/ _ \\  | |    | |   | '_ \\ / _` | __|     |");
    puts("|   \\  /\\  /  __/ | (_| (_) | | | | | |  __/ | || (_) | | \\__/\\| \\__/\\ | | | (_| | |_      |");
    puts("|    \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___|  \\__\\___/   \\____/ \\____/_| |_|\\__,_|\\__|     |");
    puts("|__________________________________________________________________________________________|\n\033[0m ");
    fflush(stdout);

    login_data *log_data = init_connection();
    if (log_data == NULL){
        error("An error ocurred.");
    }

    start_chat(log_data);



    return 0;
}

int print_message(char* message) {
    printf("%s", message);
    return 0;
}


void *reader(void *log_data) {
    char *buf;

    // FILE *fp = fopen("local_log.txt", "ab+");

    while (1) {
        buf = read_fd(((login_data *)log_data)->fd);
        if(buf != NULL) {
            //printf("\033[s");   // save the cursor point
            print_message(buf);
            free(buf);
        }
        else {
            perror("\033[31mServer closed the connection\033[0m");
            close(((login_data *)log_data)->fd);
            exit(1);
        }
    }
}



int start_chat(login_data *log_data) {
    // build the message format
    pthread_t tid;

    char m[2];
    enum chat_mode mode;

    read(log_data->fd, m, 1);

    if (m[0] == '0')
        mode = TIMESTAMP_MODE;
    else 
        mode = RECEIVE_MODE;

    pthread_create(&tid, NULL, &reader, (void *)log_data);

    while (1) {
        // read from cmd the text

        char *buff = readinput();

        int w = get_term_width();
        int lines_to_remove = strlen(buff)/w;

        for (int j = 0; j <= lines_to_remove; j++) {
            printf("\r");
            printf("\033[1A");
            printf("\033[K"); 
        }

        if(strlen(buff) > 1) {
            chat_message *new_message = malloc(sizeof(chat_message));
            new_message->sender = log_data->nickname;
            new_message->message = buff;
            new_message->time = get_current_time();
            if (mode == RECEIVE_MODE)
                write(log_data->fd, buff, strlen(buff));
            else {
                unsigned int msg_len = strlen(new_message->sender) + strlen(new_message->time) + strlen(new_message->message) + 7;
                char assembled_message[msg_len];
                snprintf(assembled_message, msg_len, "[%s, %s] %s\n", new_message->sender, new_message->time, new_message->message);
                write(log_data->fd, assembled_message, msg_len);
            }
            chat_log(new_message, TIMESTAMP_MODE);
        }
        else {  // the user wrote a \n only
            // printf("\r");
            // printf("\033[1A");
            // printf("\033[K");
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
    data->host = calloc(HOST_LEN, sizeof(char));
    
    serv_addr = malloc(sizeof(struct sockaddr_in));
    serv_addr->sin_family = AF_INET;

    int connection_succeded;
    
    int correct_host = 1;
    do {


        do {
            puts("Please enter the server address: ");

            fgets(data->host, HOST_LEN - 1, stdin);
            data->host[strcspn(data->host, "\n")] = '\0';
            printf("\r");
            printf("\033[1A");
            printf("\033[K");
            printf("\033[1A");
            printf("\033[K");
            

            if(inet_pton(AF_INET, data->host, &(serv_addr->sin_addr)) > 0) {
                
                puts("Please enter the port: ");

                char *p, s[PORT_LEN];
                memset(s, 0, PORT_LEN);

                while (fgets(s, PORT_LEN - 1, stdin)) {
                    data->port = atoi(s);
                    if (data->port == 0) {
                        printf("Please enter an integer: ");
                    } else break;
                }
                printf("\r");
                printf("\033[1A");
                printf("\033[K");
                printf("\033[1A");
                printf("\033[K");
                printf("\033[1A");
                printf("\033[K");
                fflush(stdin);

                if(0 > data->port || data->port > 65536)
                    perror("Invalid port.");
                else {
                    serv_addr->sin_port = htons(data->port);
                    correct_host = 0;
                }
            }
            else{
                fprintf(stdout,"ERROR, no such host\n");
            }
        } while (correct_host);

        // connecting to the server
        printf("Connecting to the server %d:%d\n", serv_addr->sin_addr.s_addr, data->port);


        

        if( (connection_succeded = connect(sockfd, (struct sockaddr *)serv_addr, sizeof(*serv_addr))) < 0) {
            printf("\n Error : Connect Failed \n");
            printf("Retry with %s:%d? Y/n", data->host, data->port);
            char c[1];
            scanf("%1s", c);
            printf("\r");
            printf("\033[1A");
            printf("\033[K");
            printf("\033[1A");
            printf("\033[K");
            fflush(stdin);
            if ( (int) c[0] != (int) 'Y' && (int) c[0] != (int) 'y')
                correct_host = 1;
        }
        else {
            puts("Connected to the server\n");
        }        

    } while (connection_succeded < 0);


    fflush(stdin);
    fflush(stdout);

    data->nickname = calloc(NICK_LEN, sizeof(char));
    puts("Enter your nickname: ");
    fgets(data->nickname, NICK_LEN - 1, stdin);
    data->nickname[strcspn(data->nickname, "\n")] = '\0';
    printf("\r");
    printf("\033[1A");
    printf("\033[K");
    printf("\033[1A");
    printf("\033[K");
    fflush(stdin);
    fflush(stdout);

    write(sockfd, data->nickname, strlen(data->nickname));
    char response[2];
    response[1] = '\0';

    read(sockfd, response, 1);

    while (response[0] == '0') {
        puts("Nickname already in use, please chose a new one: ");
        fgets(data->nickname, NICK_LEN - 1, stdin);
        data->nickname[NICK_LEN - 1] = '\0';
        data->nickname[strcspn(data->nickname, "\n")] = '\0';

        printf("\r");
        printf("\033[1A");
        printf("\033[K");
        printf("\033[1A");
        printf("\033[K");
        
        fflush(stdin);
        fflush(stdout);

        write(sockfd, data->nickname, strlen(data->nickname));

        
        if(read(sockfd, response, 1) <= 0) {
            perror("Server disconnected");
            close(sockfd);
            return NULL;
        }

    }
    printf("\r");
    printf("\033[1A");
    printf("\033[K");

    int c;
    while ((c = getchar()) != '\n' && c != EOF );
    fflush(stdout);

    return data;
}
