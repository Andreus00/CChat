#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {

    int sockfd;
    
    struct sockaddr_in serv_addr, cli_addr;

    int port = 32000;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //SOCK_SEQPACKET?

    

}













