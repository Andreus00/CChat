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

char * myfifo = "./server_fifo.txt";

int write_fifo_fd, read_fifo_fd;

pthread_mutex_t fifo_mutex;

unsigned int port_in;
unsigned int queue_length = 15;
char *mode;

dinamic_list *cli_data_list;

int sockfd, clifd, clilen;

struct sockaddr_in *serv_addr, *cli_addr;

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

// argv : port, mode

int main(int argc, char *argv[]) {
    puts("init mutex\n");
    if (pthread_mutex_init(&fifo_mutex, NULL) != 0) {
        fprintf(stderr, "Error in pthread_mutex_init()\n");
        exit(EXIT_FAILURE);
    }
    puts("init fifo\n");
    //init fifo
    //check if fifo already exists
    

    struct stat res;

    if (stat(myfifo, &res)) {
        if (mkfifo(argv[1], S_IRUSR | S_IWUSR)) {
        perror("Unable to create named pipe");
        exit(6);
        }
    }
    else if (!S_ISFIFO(res.st_mode)) { 
        perror("File already exists and is not a named pipe\n");
        exit(5);
    }

    puts("open fifo write\n");
    write_fifo_fd = open(myfifo, O_WRONLY); // threads uses this to write in the fifo
    puts("open fifo read\n");
    read_fifo_fd = open(myfifo, O_RDONLY);  // the thread that will 
    /////////////////////


    if (argc == 1) {
        port_in = 50000;
    }
    else{
        port_in = atoi(argv[1]);
        mode = argv[2];
    }
    puts("init dinamic list\n");
    cli_data_list = dinamic_list_new();

    socklen_t n;

    // creating the socket of the server
    puts("init socket\n");
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

    while (1) {

        puts("waiting for a connection\n");

        clifd = accept(sockfd, (struct sockaddr *) &cli_addr, &n);
        if(clifd < 0) {
            perror("Error while accepting the connection.");
            continue;
        }

        pthread_t tid;
        cli_data data =  {clifd, cli_addr};
        dinamic_list_add(cli_data_list, &data);
        if (pthread_create(&tid, 0, &chat_start, &data) != 0){
            perror("Error while creating the thread");
            //close(clifd);
        }    
    }
    
    close(sockfd);
}

void *chat_start(void *info) {

    cli_data * data = (cli_data *) info;
    int clifd = data->clifd;
    struct sockaddr *cli_addr = (struct sockaddr *) data->cli_addr;
    int n;
    char buffer[BUFFER_SIZE];
    while (1) {
        n = read(clifd, buffer, BUFFER_SIZE - 1);
        write(write_fifo_fd, buffer, BUFFER_SIZE - 1);
        n = write(clifd, "Message got", 12);
    }

    return NULL;
}














