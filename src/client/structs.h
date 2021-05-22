/*
    once the user enters the login data, this struct will contain all the pointers pointing to the information.
*/
typedef struct {
    char *nickname;
    char *host;
    int port;
    int fd;
}login_data;


/**
 * this struct has all the  
 */
typedef struct {
    int clifd;
    char *nickname;
    struct sockaddr_in *cli_addr;
} cli_data;

typedef struct {
    pthread_mutex_t *mutex;         // every thread must lock this when he wants to write a message to the other clients
    pthread_cond_t *cond;           // cond used bu the reader
    long msg_n;
}chat_mutex;

typedef struct {
    char *sender;
    char *message;
    char *time;
}chat_message;

typedef struct {
    dinamic_list *message_list;
    chat_mutex *mutex;
} chat_message_list;

typedef struct {
    cli_data *data;
    dinamic_list *cli_data_list;
    chat_message_list *msg_list;
} receiver_thread_param;

typedef struct {
    dinamic_list *cli_data_list;
    chat_message_list *msg_list;
} reader_thread_param;