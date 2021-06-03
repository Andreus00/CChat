#include "enums.h"
/*
Struttura utilizzata per mantenere i dati per il log degli utenti
*/
typedef struct {
    char *nickname;     // puntatore al nickname dell'utente
    char *host;         // puntatore all'ip del server
    int port;           // porta a cui deve colegarsi il client
    int fd;             // file descriptor del server
}login_data;

/*
struttura usata dal server per mnantenere le informazioni riguardanti i client connessi  
 */
typedef struct {
    int clifd;                      // file descriptor dell'utente
    char *nickname;                 // puntatore al nickname dell'utente
    struct sockaddr_in *cli_addr;   // putnatore alla struttura contenetre le informazioni del socket dell'utente
} cli_data;

/*
struttura usata per un accesso ai messaggi che sfrutta lock e conditions in modo da non avere problemi di concorrenza
*/
typedef struct {
    pthread_mutex_t *mutex;         // puntatore almutex usato quando si vuole aggiungere un messaggio alla lista dei messaggi
    pthread_cond_t *cond;           // puntatore alla condition usata dal reader thread del server per capire quando ci sono messaggi in coda (producer - consumer)
}chat_mutex;

/*
struttura che contiene le informazioni riguardanti un messaggio
*/
typedef struct {
    char *sender;   // puntatore al nickname di chi ha mandato il messaggio
    char *message;  // puntatore al contenuto del messaggio
    char *time;     // puntatore al time del messaggio
}chat_message;

/*
struttura che unisce una dinamic_list (di chat_messge) e un chat_mutex.
Il chat mutex gestisce l'accesso e la lettura dalla lista da parte dei thread del server
*/
typedef struct {
    dinamic_list *message_list;     // puntatore alla dinamic_list contenente dei messaggi
    chat_mutex *mutex;              // puntatore al mutex da lockare quando si vuole lavorare con la message_list
} chat_message_list;

/*
struttura che racchiude le informazioni che servono ai thread che ricevono i client.
Le informazioni vengono usate per la "registrazione" dell'utente e per gestire l'arrivo dei messaggi.
*/
typedef struct {
    cli_data *data;                 // puntatore a una struttura contenente le informazioni sul client
    dinamic_list *cli_data_list;    // puntatore a una dinamic list contenente tutti gli utenti connessi
    chat_message_list *msg_list;    // putnatore alla lista dei messaggi
} receiver_thread_param;

/*
Struttura usata per mantenere le informazioni riguardanti i thread attualmente attivi.
*/
typedef struct { 
    receiver_thread_param *receiver_param;  // parametri da passare al thread riguardanti il client
    pthread_t tid;  // tid del thread
    pthread_mutex_t *mutex; // mutex usato con la cond
    pthread_cond_t *cond;   // cond usata per attendere che il thread principale metta le informazioni riguardanti il client nel receiver_param
    int running;    // intero che indica se il thread deve runanre la task. Usato per la condition. (1 per runnare e 0 per non runnare)
    int kill;   // intero che indica se, una volta completata la task, il thread ritornare o deve rimanere in attesa di un altra task. (0 per non killare, 1 per killare)
} receiver_info;

/*
struttura che racchiude le informazioni che servono al reader thread.
*/
typedef struct {
    dinamic_list *cli_data_list;    // puntatore alla lista dei clienti
    chat_message_list *msg_list;    // puntatore alla lista dei messaggi
} reader_thread_param;
