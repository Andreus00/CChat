#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>
#include <string.h>

#include "../utility/dinamic_list.h"


#define BUFFER_SIZE 255


typedef struct {
    int clifd;
    struct sockaddr_in *cli_addr;
} cli_data;

void *chat_start(void *);

void error(char *msg)
{
    perror(msg);
    exit(0);
}


int main(int argc, char *argv[]) {

    unsigned int port_in;
    unsigned int queue_length = 15;

    dinamic_list *cli_data_list;

    int sockfd, clifd;

    struct sockaddr_in *serv_addr, *cli_addr;

    socklen_t n = sizeof(struct sockaddr_in);

    if (argc == 1) {
        port_in = 50000;
    }
    else if (argc == 2) {
        port_in = atoi(argv[1]);
    }
    puts("init dinamic list");
    cli_data_list = dinamic_list_new();

    // creating the socket of the server
    puts("init socket");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //SOCK_SEQPACKET?

    if(sockfd < 0){
        perror("Error while opening the socket.");
        exit(1);
    }

    serv_addr = malloc(sizeof(struct sockaddr_in));//memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr->sin_port = htons(port_in);       // TODO: get this from the argv

    if(bind(sockfd, (struct sockaddr*) serv_addr, sizeof(*serv_addr)) < 0) {
        perror("Error while binding the socket to the socket address.");
        exit(1);
    };

    listen(sockfd, queue_length);
    
    pthread_t tid;

    while (1) {

        puts("waiting for a connection");

        clifd = accept(sockfd, (struct sockaddr *) cli_addr, &n);

        printf("Connection acepted: %d\n", clifd);
        if(clifd < 0) {
            perror("Error while accepting the connection.");
            continue;
        }

        cli_data *data = malloc(sizeof(cli_data));
        data->clifd = clifd;
        data->cli_addr = cli_addr;
        dinamic_list_add(cli_data_list, &data);
        if (pthread_create(&tid, 0, &chat_start, data) != 0){
            perror("Error while creating the thread");
            close(clifd);
        }

    }
}

void *chat_start(void *info) {

    cli_data * data = (cli_data *) info;
    int clifd = data->clifd;
    struct sockaddr *cli_addr = (struct sockaddr *) data->cli_addr;
    int n;
    char buffer[BUFFER_SIZE];
    char nickname[20];
    read(clifd, nickname, 20);

    write(clifd, "1", 1);

    while (1) {
        n = read(clifd, buffer, BUFFER_SIZE - 1);

        printf("%s", buffer);

        if (n == 0) {
            close(clifd);
            return data;
        }
        n = write(clifd, "Message got", 12);
        memset(buffer, 0, BUFFER_SIZE);
    }
}


