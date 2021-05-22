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
#include "../client/structs.h"


#define BUFFER_SIZE 255


void *chat_start(void *);
void *reader_start(void *);

void error(char *msg)
{
    perror(msg);
    exit(1);
}

chat_mutex *chat_mutex_new() {
    chat_mutex * m = malloc(sizeof(chat_mutex));

    m->mutex = malloc(sizeof(pthread_mutex_t));
    m->cond = malloc(sizeof(pthread_cond_t));

    pthread_mutex_init(m->mutex, NULL);
    pthread_cond_init(m->cond , NULL);
    m->msg_n = 0;

    return m;
}


int main(int argc, char *argv[]) {

    unsigned int port_in;

    unsigned int queue_length = 15;

    dinamic_list *cli_data_list;

    int sockfd, clifd;

    struct sockaddr_in *serv_addr, *cli_addr;

    // init chat message list
    chat_message_list *msg_list = calloc(1, sizeof(chat_message_list));

    puts("A");
    msg_list->message_list = dinamic_list_new();
    puts("A");

    msg_list->mutex = chat_mutex_new();
    puts("A");
    // init cli_data_list
    cli_data_list = dinamic_list_new();

    //////
    if (argc == 1) {
        port_in = 50000;
    }
    else if (argc == 2) {
        port_in = atoi(argv[1]);
    }
    puts("init dinamic list");
    // creating the socket of the server
    puts("init socket");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //SOCK_SEQPACKET?

    if(sockfd < 0){
        error("Error while opening the socket.");
    }

    serv_addr = malloc(sizeof(struct sockaddr_in));//memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr->sin_port = htons(port_in);       // TODO: get this from the argv

    if(bind(sockfd, (struct sockaddr*) serv_addr, sizeof(*serv_addr)) < 0) {
        error("Error while binding the socket to the socket address.");
    };

    listen(sockfd, queue_length);
    
    // INIT READER THREAD
    pthread_t tid;
    reader_thread_param *reader_param = malloc(sizeof(reader_thread_param));
    reader_param->msg_list = msg_list;
    reader_param->cli_data_list = cli_data_list;

    if( pthread_create(&tid, 0, &reader_start, reader_param) != 0) {       //starting the reader thread.
        error("Error while starting the reader thread");
    }

    receiver_thread_param *receiver_param;


    socklen_t n = sizeof(struct sockaddr_in);
    cli_addr = malloc(n);

    while (1) {

        puts("waiting for a connection");

        clifd = accept(sockfd, (struct sockaddr *) cli_addr, &n);

        if(clifd < 0) {
            perror("Error while accepting the connection.");
            continue;
        }
        printf("Connection acepted: %d\n", clifd);

        receiver_param = malloc(sizeof(receiver_thread_param));
        receiver_param->cli_data_list = cli_data_list;
        receiver_param->msg_list = msg_list;
        receiver_param->data = malloc(sizeof(cli_data));
        receiver_param->data->clifd = clifd;
        receiver_param->data->cli_addr = cli_addr;

        if (pthread_create(&tid, 0, &chat_start, receiver_param) != 0){
            perror("Error while creating the thread");
            close(clifd);
        }

    }
}

char *get_current_time() {
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char *ascii_time = asctime (timeinfo);
    int l = strlen(ascii_time) - 1;
    char *ret_ascii_time = malloc(l * sizeof(char));
    strncpy(ret_ascii_time, ascii_time, l);
    return ret_ascii_time;
}

void *reader_start(void *info) {

    reader_thread_param *reader_param = (reader_thread_param *) info;

    dinamic_list *cli_data_list = reader_param->cli_data_list;
    chat_message_list *msg_list = reader_param->msg_list;

    while (1) {
        printf("\033[0;31m READER THREAD:    LOCKING THE MUTEX\033[0m\n");
        pthread_mutex_lock(msg_list->mutex->mutex);
        printf("%ld, %ld\n", msg_list->mutex->msg_n, msg_list->message_list->last);
        if (msg_list->message_list->last < 0) {
            printf("\033[0;31m READER THREAD:    WAITING CONDITION\033[0m\n");
            pthread_cond_wait(msg_list->mutex->cond, msg_list->mutex->mutex);
        }

        for (int i = msg_list->message_list->last; i >= 0; i--) {
            element e = dinamic_list_pop(msg_list->message_list, 0);
            chat_message *msg= e.value;
            fprintf(stdout, "[%s, %s] %s\n", msg->sender, msg->time, msg->message);
            msg_list->mutex->msg_n--;
        }
        printf("\033[0;31m READER THREAD:    UNLOCKING THE MUTEX\n");
        pthread_mutex_unlock(msg_list->mutex->mutex);

        usleep(10000);  // sleep 10 millis
    }
}


/*
1 if in the list, 0 if not
*/
int check_name(dinamic_list *l, char *n) {
    puts("\033[33m Checking name : lock on mutex\033[0m");
    pthread_mutex_lock(l->mutex);

    printf("%ld\n", l->last);

    puts("\033[33m Checking name : locked\033[0m");
    int ret = 0;
    for (long i = 0; i <= l->last; i++) {

        printf("\033[33m Checking name : getting data\033[0m");
        cli_data *data = (cli_data *) l->list[i];

        printf("\033[33m Checking name : %s\033[0m", data->nickname);
        if (strcmp(data->nickname, n) == 0) {
            ret = 1;
            break;
        }
    }

    puts("\033[33m Checking name : unlock on mutex\033[0m");
    pthread_mutex_unlock(l->mutex);
    return ret;
}


void *chat_start(void *info) {
    #define NICK_SIZE 21

    receiver_thread_param *param = (receiver_thread_param *) info;
    cli_data *data = param->data;
    dinamic_list *cli_data_list = param->cli_data_list;
    chat_message_list *msg_list = param->msg_list;
    int clifd = data->clifd;
    char *buffer = calloc(BUFFER_SIZE, sizeof(char));
    char nickname[NICK_SIZE];
    int do_loop = 0;
    do {
        if (do_loop) {
            write(clifd, "0", 1);
        }
        memset(nickname, 0, NICK_SIZE);

        printf("%d wants to join.\n", clifd);
        if (read(clifd, nickname, NICK_SIZE - 1) <= 0) {
            printf("Client %d disconnected", clifd);
            close(clifd);
            return NULL;
        }
        printf("%s\n", nickname);

    } while ((do_loop = check_name(cli_data_list, nickname)));

    printf("Name accepted: %s", nickname);

    write(clifd, "1", 1);

    data->nickname = nickname;

    dinamic_list_add(cli_data_list, data);
    int n;

    while (1) {
        n = read(clifd, buffer, BUFFER_SIZE);

        if (n <=0) {
            close(clifd);
            dinamic_list_remove_element(cli_data_list, (void *) data);
            printf("%s left the chat.\n", nickname);
            return data;
        }
        else if(n > 1) {
            pthread_mutex_lock(msg_list->mutex->mutex);
            chat_message *new_message = malloc(sizeof(chat_message));
            new_message->sender = nickname;
            new_message->message = buffer;
            new_message->time = get_current_time();
            printf("Buffer = %s\n", buffer);

            dinamic_list_add(msg_list->message_list,(void *) new_message);
            msg_list->mutex->msg_n++;

            pthread_cond_signal(msg_list->mutex->cond);

            pthread_mutex_unlock(msg_list->mutex->mutex);

            //n = write(clifd, buffer, strlen(buffer));
            buffer = calloc(BUFFER_SIZE, sizeof(char));
        }

    }
}


