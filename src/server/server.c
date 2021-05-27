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
#include "../utility/structs.h"
#include "../utility/chat_log.h"
#include "../utility/chat_message.h"


enum chat_mode mode;

int millis_check = 1;

void *chat_start(void *);
void *reader_start(void *);

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void print_usage() {
    printf("Usage: ./server [port] [[mode] [millis_check]].\n You can put only the port, or the port and the mode.\nIf the mode is TIMESTAMP_MODE, you can choose the millis_check.\n");
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

    int port_in;

    if (argc == 1) {
        port_in = 5000;
        mode = RECEIVE_MODE;
        printf("port in = 5000\n");
    }
    else if(argc == 2) {
        port_in = atoi(argv[1]);
        printf("Port in: %d\n", port_in);
        mode = RECEIVE_MODE;
        puts("Log mode not detected: setting RECEIVE_MODE");
    }
    else if (argc == 3 || argc == 4) {
        port_in = atoi(argv[1]);
        printf("Port in: %d\n", port_in);
        if(strcmp(argv[2], "TIMESTAMP_MODE") == 0) {
            puts("Log mode : TIMESTAMP_MODE");
            mode = TIMESTAMP_MODE;
        }
        else if (strcmp(argv[2], "RECEIVE_MODE") == 0){
            puts("Log mode : RECEIVE_MODE");
            mode = RECEIVE_MODE;
        }
        else {
            puts("Log mode not detected: setting RECEIVE_MODE");
            mode = RECEIVE_MODE;
        }

        if (argc == 4) millis_check = atoi(argv[3]);

        
    }    
    else {
        print_usage();
    }
    if (port_in == 0 || millis_check == 0) print_usage();

    unsigned int queue_length = 15;

    dinamic_list *cli_data_list;

    int sockfd, clifd;

    struct sockaddr_in *serv_addr, *cli_addr;

    // init chat message list
    chat_message_list *msg_list = calloc(1, sizeof(chat_message_list));

    msg_list->message_list = dinamic_list_new();

    msg_list->mutex = chat_mutex_new();
    // init cli_data_list
    cli_data_list = dinamic_list_new();

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

int compare_time(chat_message *t1, chat_message *t2) {
    return atoi(t1->time) - atoi(t2->time);
}

void queue_message(chat_message_list *msg_list,char *nickname, char *text) {
    pthread_mutex_lock(msg_list->mutex->mutex);
    puts("queuing message");
    chat_message *new_message = malloc(sizeof(chat_message));    
    new_message->sender = nickname;

    if (mode == RECEIVE_MODE) {
        new_message->message = text;
        new_message->time = get_current_time();
    }
    else {
        // devo capire dove inizia il timestamp: scorro il text fino alla fine del nome.
        int text_len = strlen(text);
        int i = 0;  //i will be the index of the first character after the user's name.
        while (i < (text_len - 1) && text[i + 1] == nickname[i]) {
            i++;
        }
        i += 2;    // the offset of the timestamp is 2 chars after the end of the nickname (we have 'nickname, timestamp' ).
        int j = i;      // j will be the index of the ']'.
        while (text[j] != ']') j++;
        printf("Malloc for message = %d\n", (text_len) - j + 1);
        if (j <= 0) {
            printf("Received a message without timestamp.\n");
            return;
        }
        new_message->message = malloc(sizeof(char) * (text_len) - j + 3);
        memcpy(new_message->message, &text[j + 2], (text_len) - j + 3);
        printf("Malloc for time = %d\n", j - i + 1);
        new_message->time = malloc(sizeof(char) * j - i + 1);
        memcpy(new_message->time, &text[i], j);
        new_message->message[(text_len) - j] = '\0';
        new_message->time[j - i] = '\0';
        free(text);
    }
    if (mode == TIMESTAMP_MODE) {
        int i;
        int added = 0;
        for (i = 0; i <= msg_list->message_list->last; i++) {
            chat_message *msg_in_list = msg_list->message_list->list[i];
            if (compare_time(new_message, msg_in_list) < 0) {
                dinamic_list_insert(msg_list->message_list, new_message, i);
                printf("Added message in place %d\n", i);
                added = 1;
                break;
            }
        }
        if (i == 0 || !added) dinamic_list_add(msg_list->message_list,(void *) new_message);    // i add the message if i == 0 (aka there were no messages in the list) or if the message was not added to the list during the loop(because it's the last mesage);
    }
    else if (mode == RECEIVE_MODE) {
        dinamic_list_add(msg_list->message_list,(void *) new_message);
    }
    printf("%ld", msg_list->message_list->last);

    msg_list->mutex->msg_n++;

    pthread_cond_signal(msg_list->mutex->cond);
    puts("queued message");

    pthread_mutex_unlock(msg_list->mutex->mutex);
}

void send_message_to_everyone(chat_message  *msg, dinamic_list *cli_data_list) {
    chat_log(msg, mode);
    pthread_mutex_lock(cli_data_list->mutex);
    if (mode == TIMESTAMP_MODE) {
        msg->time = get_ascii_time_from_long((atol(msg->time)) / 1000000);
    }
    for (int i = 0; i <= cli_data_list->last; i++) {        
        dprintf(((cli_data *)(cli_data_list->list[i]))->clifd, "[%s, %s] \033[32m|\033[0m %s\n", msg->sender, msg->time, msg->message);
    }
    pthread_mutex_unlock(cli_data_list->mutex);
}

void free_mesage(chat_message *msg) {
    free(msg->message);
    free(msg);
}

void *reader_start(void *info) {

    reader_thread_param *reader_param = (reader_thread_param *) info;

    dinamic_list *cli_data_list = reader_param->cli_data_list;
    chat_message_list *msg_list = reader_param->msg_list;

    while (1) {
        printf("\033[0;31m READER THREAD:    LOCKING THE MUTEX\033[0m\n");
        pthread_mutex_lock(msg_list->mutex->mutex);
        if (msg_list->message_list->last < 0) {
            printf("\033[0;31m READER THREAD:    WAITING CONDITION\033[0m\n");
            pthread_cond_wait(msg_list->mutex->cond, msg_list->mutex->mutex);
        }

        for (int i = msg_list->message_list->last; i >= 0; i--) {
            chat_message *msg;
            if (mode == TIMESTAMP_MODE) {
                msg = msg_list->message_list->list[i];
                if ((atol(get_current_time_u()) - atol(msg->time)) < (millis_check * 1000)){ 
                    break;
                }
            }
            element e = dinamic_list_pop(msg_list->message_list, 0);
            msg= e.value;
            send_message_to_everyone(msg, cli_data_list);
            msg_list->mutex->msg_n--;
            free_mesage(msg);
        }
        printf("\033[0;31m READER THREAD:    UNLOCKING THE MUTEX\n");
        pthread_mutex_unlock(msg_list->mutex->mutex);

        usleep(millis_check * 1000);  // sleep
    }
}


/*
1 if in the list, 0 if not
*/
int check_name(dinamic_list *l, char *n) {
    if (strlen(n) < 1)
        return 1;
    puts("\033[33m Checking name : lock on mutex\033[0m");
    pthread_mutex_lock(l->mutex);

    puts("\033[33m Checking name : locked\033[0m");
    int ret = 0;
    for (long i = 0; i <= l->last; i++) {

        puts("\033[33m Checking name : getting data\033[0m");
        cli_data *data = (cli_data *) l->list[i];

        printf("\033[33m Checking name : %s\033[0m\n", data->nickname);
        if (strcmp(data->nickname, n) == 0) {
            ret = 1;
            break;
        }
    }

    puts("\033[33m Checking name : unlock on mutex\033[0m");
    pthread_mutex_unlock(l->mutex);
    return ret;
}


void _server_send_message(chat_message_list *msg_list, char *nickname, char *text) {
    if(mode == TIMESTAMP_MODE) {
        char *time = get_current_time();
        unsigned int msg_len = strlen(time) + strlen(text) + strlen(nickname) + 8;
        char *assembled_message = calloc(msg_len, sizeof(char));
        snprintf(assembled_message, msg_len - 1, "[%s, %s] %s", nickname, time, text);
        queue_message(msg_list, nickname, assembled_message);
    }
    else {
        char *allocated_text = calloc(strlen(text), sizeof(char));
        strcpy(allocated_text, text);
        queue_message(msg_list, nickname, allocated_text);

    }
}


void *chat_start(void *info) {
    #define NICK_SIZE 200

    receiver_thread_param *param = (receiver_thread_param *) info;
    cli_data *data = param->data;
    dinamic_list *cli_data_list = param->cli_data_list;
    chat_message_list *msg_list = param->msg_list;
    int clifd = data->clifd;
    char nickname[NICK_SIZE];
    int do_loop = 0;
    do {
        if (do_loop) {
            write(clifd, "0", 1);
        }
        memset(nickname, 0, NICK_SIZE);

        printf("\033[33m %d wants to join.\n\033[0m", clifd);
        int aa = read(clifd, nickname, (NICK_SIZE - 1) * sizeof(char));
        if (aa <= 0) {
            printf("\033[33m Client %d disconnected\n\033[0m", clifd);
            close(clifd);
            return NULL;
        }
        printf("\033[33m name : %s\n\033[0m", nickname);

    } while ((do_loop = check_name(cli_data_list, nickname)));

    write(clifd, "1", 1);

    data->nickname = nickname;

    dinamic_list_add(cli_data_list, data);


    write(data->clifd, mode == RECEIVE_MODE ? "1" : "0", 2);

    _server_send_message(msg_list, data->nickname, "\033[32m-- Join the chat\n\033[0m");

    char *buffer;

    while (1) {
        buffer = read_fd(clifd);
        printf("READED\n");
        if (buffer == NULL) {
            close(clifd);
            if (dinamic_list_remove_element(cli_data_list, (void *) data) == NULL) {
                puts("\033[33m USER NOT FOUND\033[0m");
            }
            _server_send_message(msg_list, data->nickname, "\033[31m-- Left the chat\n\033[0m");
            return data;
        }
        else {            
            queue_message(msg_list, nickname, buffer);
        }

    }
}


