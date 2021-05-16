#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <pthread.h>

#include "../utility/dinamic_list.h"


void *chat_start(void *);


int main(int argc, char *argv[]) {

    unsigned int port_in = 5000;
    unsigned int queue_length = 15;
    
    dinamic_list *threads = dinamic_list_new();

    int sockfd, clifd;
    
    struct sockaddr_in serv_addr, cli_addr;

    socklen_t s;

    int port = 32000;

    // creating the socket of the server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //SOCK_SEQPACKET?

    if(sockfd < 0){
        perror("Error while opening the socket.");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port_in);       // TODO: get this from the argv

    if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error while binding the socket to the socket address.");
        exit(1);
    };

    listen(sockfd, queue_length);

    while (1) {

        clifd = accept(sockfd, &cli_addr, sizeof(struct sockaddr));
        if(clifd < 0) {
            perror("Error while accepting th econnection.");
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, )


    }
}













