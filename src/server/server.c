/*
Andrea Sanchietti

Server per CChat.

Il server accetta come argomenti la porta, la mode (TIMESTAMP_MODE O RECEIVE_MODE)
e il tempo (in millisecondi) che il reader thread deve dormire una volta che ha letto
e inviato i messaggi.

Il server usa il thread principale per inizializzare le impostazioni passate da linea di comando, per aprire il socket
e per iniziare ad ascoltare sul socket.
Ogni volta che una connessione con un nuovo client viene accettata, il server prende un thread dalla thread pool o crea un thread che dovrà gestire
l'arrivo degli input da parte dell'utente.
A questo thread vengono passate le informazioni del client impacchettate in una struttura receiver_thread_param.

Il reader thread invece è quello al quale vengono passati i messaggi arrivati (i messaggi vengono accodati in una lista) e ha il compito di
rispedirli a tutti i client. Per fare ciò dispone di un puntatore alla lista dei messaggi (usa una dinamic_list come fosse una queue) e un puntatore alla
lista dei client connessi.

In base alla mode scelta durante il lancio del server, i timestamp dei messaggi verranno imposti dal server o dai client.
Quando il server è in TIMESTAMP_MODE, ogni messaggio aspetta un tot di tempo (equivalente al tempo passato come argomento quando si fa partire il server)
in modo tale da dare la possibilità ai messaggi che sono stati mandati prima dai client, ma che hanno trovato difficoltà ad arrivare al server,
di essere reinoltrati prima dei messaggi che invece sono stati mandati dopo ma sono arrivati prima.
Il server in TIMESTAMP_MODE non inoltrerà quindi i messaggi troppo "giovani".
Per fare in modo di avere una mafggiore precisione di quali messaggi sono arrivati prima in TIMESTAMP_MODE, i client mandano al server il tempo
in microsecondi anziché in secondi

*/
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

// numero di thread della thread pool
#define THREAD_POOL_N 10

void * receiver_start(void *);
void *reader_start(void *);

// numero di thread disponibili e mutex da usare con available_threads
int available_threads = THREAD_POOL_N;
pthread_mutex_t *available_threads_mutex;

// mode del server. Viene inizializzato un avolta parsati gli argomenti del main
enum chat_mode mode;
// millisecondi che deve aspettare il reader thread durante lo sleep e millisecondi
// che devono aspettare i messaggi prima di essere inoltrati.
int millis_check = 1;
/*
Funzione che stampa il messaggio di errore e chiama la exit(1)
*/
void error(char *msg)
{
    perror(msg);
    exit(1);
}
/*
Funzione che stampa a schermo l' usage del programma e chiama exit(1)
*/
void print_usage() {
    printf("Usage: ./server [port] [[mode] [millis_check]].\n You can put only the port, or the port and the mode.\nIf the mode is TIMESTAMP_MODE, you can choose the millis_check.\n");
    exit(1);
}
/*
inizializzatore di un chat_mutex. Il chat_mutex viene usato per garantire
un'accesso sicuro e senza errori alla lista dei messaggi.
*/
chat_mutex *chat_mutex_new() {
    // allocazione dello spazio per il chat_mutex, per il mutex e per la cond.
    chat_mutex * m = malloc(sizeof(chat_mutex));

    m->mutex = malloc(sizeof(pthread_mutex_t));
    m->cond = malloc(sizeof(pthread_cond_t));
    // inizializzazione del mutex e della cond
    pthread_mutex_init(m->mutex, NULL);
    pthread_cond_init(m->cond , NULL);

    return m;
}


int main(int argc, char *argv[]) {
    // variabile dove viene messa la porta inserita dall'utente
    int port_in;
    // parse degli argomenti del main. Se non specificati, la porta è 5000 e la mode è RECEIVE_MODE con 1 come millis_check
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

    // lunghezza della queue della listen
    unsigned int queue_length = 15;
    // puntatore alla lista dei client
    dinamic_list *cli_data_list;
    // variabili al cui interno andreanno il file descriptor del socked e del client.
    // il clifd viene sovrascritto ogni volta che un nuovo client si connette e il valore
    // viene passato al thread che si occuperà di quel client.
    int sockfd, clifd;
    // puntatori alle strutture sockaddr_in del server e del client
    struct sockaddr_in *serv_addr, *cli_addr;

    // inizializzazione della lista dei messaggi. Essa funzionerà da queue ordinata per i messaggi.
    // ogni volta cha a un receiver thread arriva un messaggio, esso verrà inserito in questa lista.
    // Se mode == TIMESTAMP_MODE, il messaggio dovrà essere ordinato secondo il timestamp (i messaggi
    // più giovani vanno alla fine), altrimenti viene semplicemente accodato.
    chat_message_list *msg_list = calloc(1, sizeof(chat_message_list));
    // inizializzazione della lista all'interno della chat_message_list
    msg_list->message_list = dinamic_list_new();
    // mutex da lockare quando si vuole accedere ai dati della lista. Non basta il lock della lista
    // dinamica perchè esso ha controllo solo la concorrenza delle funzioni implementate in dinamic_list.h
    // usando quel lock si verrebbe a creare uno stallo nel momento in cui a un reader thread arriva un messaggio,
    // fa il lock e poi prova ad aggiungere un elemento alla lista con una delle funzioni.
    msg_list->mutex = chat_mutex_new();
    // inizializzazione della lista che conterrà le informaizoni dei client incapsulate in una struttura cli_data
    cli_data_list = dinamic_list_new();

    // inizializzazione del socket
    puts("init socket");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //SOCK_SEQPACKET?
    // check sul socket appena creato
    if(sockfd < 0){
        error("Error while opening the socket.");
    }
    // allocazione e inizializzazione di serv_addr. Viene usata la port_in passata in input.
    serv_addr = malloc(sizeof(struct sockaddr_in));
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr->sin_port = htons(port_in);

    // bind del serv_addr con il socket e check sulgli errori.
    if(bind(sockfd, (struct sockaddr*) serv_addr, sizeof(*serv_addr)) < 0) {
        error("Error while binding the socket to the socket address.");
    };

    // listen
    if (listen(sockfd, queue_length) < 0) {
        error("Error while listening.");
    };
    
    // inizializzazione del reader thread e dei parametri da passargli. Esso si occuperà di inviare ai client i
    // messaggi della cli_message_list. Usa la cond della cli_data_list per capire quando 
    // qualcuno manda un messaggio.
    pthread_t tid;
    reader_thread_param *reader_param = malloc(sizeof(reader_thread_param));
    reader_param->msg_list = msg_list;
    reader_param->cli_data_list = cli_data_list;

    if(pthread_create(&tid, 0, &reader_start, reader_param) != 0) {       // check sul reader thread
        error("Error while starting the reader thread");
    }

    // una volta inizializzato il reader thread, devo mettermi ad inizializzare i receiver thread.

    // puntatore alla struttura da inizializzare e da passare ai reader thread
    receiver_thread_param *receiver_param;
    // parametro da passare alla accept
    socklen_t n = sizeof(struct sockaddr_in);

    available_threads_mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(available_threads_mutex, NULL);

    // inizializzazione della thread pool
    dinamic_list *thread_list = dinamic_list_new();
    for(int i = 0; i < THREAD_POOL_N; i++) {
        // alloco lo spazio per le informazioni che serviranno al thread
        receiver_info *t_info = malloc(sizeof(receiver_info));
        // alloco lo spazio per i parametri della chat
        t_info->receiver_param = malloc(sizeof(receiver_thread_param));
        // imposto i puntatori alla lista dei client e alla lista dei messaggi
        t_info->receiver_param->cli_data_list = cli_data_list;
        t_info->receiver_param->msg_list = msg_list;
        // alloco lo spazio per le informazioni del client
        t_info->receiver_param->data = malloc(sizeof(cli_data));
        // alloco e inizializzo il mutex e la cond
        t_info->mutex = malloc(sizeof(pthread_mutex_t));
        t_info->cond = malloc(sizeof(pthread_cond_t));
        pthread_mutex_init(t_info->mutex, NULL);
        pthread_cond_init(t_info->cond, NULL);
        // Impostro kill a 0 per dire che il thread non deve mai terminare, ma fare un loop continuo e attendere ogni volta
        // che il thread principale inserisca le informazioni riguardanti un client appena connesso all'interno di t_info->receiver_param->data
        t_info->kill = 0;
        // imposto running a 0 per dire che ancora non sono validi i valori all'interno di t_info->receiver_param->data e che quindi il
        // thread deve andare in attesa.
        t_info->running = 0;
        // creo il thread e controllo gli errori
        if (pthread_create(&t_info->tid, 0, &receiver_start, t_info)) {
            perror("Error while initializing a thread of the thread pool");
            pthread_mutex_lock(available_threads_mutex);
            available_threads--;
            pthread_mutex_unlock(available_threads_mutex);
        }
        // aggiungo il thread alla lista della thread pool
        dinamic_list_add(thread_list, t_info);
        printf("Thread %d initialized\n", i);
    }

    // while infinito al cui interno vengono accettati i client e passati ai reader thread
    while (1) {

        // allocazione in memoria di spazio per cli_addr (struct sockaddr_in)
        cli_addr = malloc(n);

        puts("waiting for a connection");
        // accept della connessione
        clifd = accept(sockfd, (struct sockaddr *) cli_addr, &n);
        // check su ciò ceh è stato ritornato dalla accept
        if(clifd < 0) {
            perror("Error while accepting the connection.");
            continue;
        }
        printf("Connection acepted: %d\n", clifd);
        // ora devo passare il client a un thread.
        // Se ci sono thread della thread pool liberi uso uno di essi per il client appena accettato.
        pthread_mutex_lock(available_threads_mutex);
        if (available_threads > 0) {
            // trovo il primo thread della thread_pool in attesa e gli passo le informazioni del client.
            pthread_mutex_lock(thread_list->mutex);
            int first_free_thread = 0;
            while (first_free_thread < THREAD_POOL_N) { // while al cui interno vengono scorsi i thread della thread pool
                receiver_info *receiver_thread = (receiver_info *) thread_list->list[first_free_thread];
                // check per controllare se il thread è libero.
                pthread_mutex_lock(receiver_thread->mutex);
                if (!receiver_thread->running) {
                    // se è libero imposto il thread a running, faccio puntare i parametri del thread alla struttura cli_addr e imposto il clifd del client
                    // appena accettato. Infine mando una signal al thread per dirgli di far partire la comunicazione con il client.
                    receiver_thread->running = 1;
                    receiver_thread->receiver_param->data->cli_addr = cli_addr;
                    receiver_thread->receiver_param->data->clifd = clifd;
                    pthread_cond_signal(receiver_thread->cond);
                    pthread_mutex_unlock(receiver_thread->mutex);
                    // aggiorno il numero di thread disponibili
                    available_threads--;
                    printf("Thread %d used. Threadpool index : %d\n",(int) receiver_thread->tid, first_free_thread);
                    break;
                }
                else pthread_mutex_unlock(receiver_thread->mutex);
                first_free_thread++;
            }
            pthread_mutex_unlock(thread_list->mutex);
            
        }
        // se invece non ci sono thread disponibili dalla thread pool viene creato un nuovo thread con kill = 1 in modo da farlo ritornare una volta che
        // il client si disconnette, running = 1 in modo da far partire subito la chat senza aspettare la signal della cond, i parametri che servono
        // per la chat in t_info->receiver_param
        else {
            // allocazione in memoria dello spazio per i parametri da passare al receiver thread
            receiver_info *t_info = malloc(sizeof(receiver_info));
            // allocazione per i parametri
            t_info->receiver_param = malloc(sizeof(receiver_thread_param)); 
            // set del puntatore alla lista dei client
            t_info->receiver_param->cli_data_list = cli_data_list;  
            // set del puntatore alla lista dei messaggi
            t_info->receiver_param->msg_list = msg_list;
            // allocazione per i dati del client
            t_info->receiver_param->data = malloc(sizeof(cli_data));
            // file descriptor del client
            t_info->receiver_param->data->clifd = clifd;
            // puntatore al cli_addr del client
            t_info->receiver_param->data->cli_addr = cli_addr;          
            // inizializzazione del mutex e della cond
            t_info->mutex = malloc(sizeof(pthread_mutex_t));
            t_info->cond = malloc(sizeof(pthread_cond_t));
            pthread_mutex_init(t_info->mutex, NULL);
            pthread_cond_init(t_info->cond, NULL);
            // set di kill a 1 in modo da far terminare il thread una volta che la connessione al client termina
            t_info->kill = 1;
            // set di running a 1 per non far aspettare al thread una signal
            t_info->running = 1;
            // creazione e check del thread
            if (pthread_create(&t_info->tid, 0, &receiver_start, t_info)) {
                perror("Error while initializing a thread of the thread pool");
            }
            else
                printf("Thread %d used.\n",(int) t_info->tid);
        }
        pthread_mutex_unlock(available_threads_mutex);  // rilascio il lock usato per la variabile contatore available_threads
        
    }
}
/*
Funzione che, dati due messaggi al cui interno il time è espresso in microsecondi, ritorna
0 se i tempi sono uguali, un numero maggiore di 0 se il primo è maggiore, altrimenti un
numero minore di 0
*/
int compare_time(chat_message *t1, chat_message *t2) {
    return atol(t1->time) - atol(t2->time);
}
/*
Funzione che accoda i messaggi alla chat_message_list passata come input.
In base alla mode del server, farà operazioni diverse.
Per la RECEIVE_MODE basterà imporre il current time e il text.
Per la TIMESTAMP_MODE invece, la funzione deve recuperarsi dal text il timestamp
e il text dato che il client invia tutto nella stessa stringa.
*/
void queue_message(chat_message_list *msg_list,char *nickname, char *text) {
    // lock sul mutex dell amessage list per evitare concottenza
    pthread_mutex_lock(msg_list->mutex->mutex);
    puts("queuing messag");
    // allocazione dello spazio in memoria per il nuovo messaggio e set del puntatore al nickname
    chat_message *new_message = malloc(sizeof(chat_message));
    new_message->sender = nickname;

    // se mode == RECEIVE_MODE è il server che impone il timestamp, e all'interno di text c'è
    // solamente il messaggio mandato dall'utente.
    if (mode == RECEIVE_MODE) {
        new_message->message = text;
        new_message->time = get_current_time();
    }
    // se invece mode == TIEMSTAMP_MODE, devo andare a recuperare il timestamp e il messaggio dal text mandato
    // dal client.
    else {
        // calcolo la lunghezza del testo e il punto dove inizia il timestamp (variabile 'i').
        int text_len = strlen(text);
        int i = strlen(nickname) + 3;
        // inizializzo j al valore di i e scorro verso destra finchè non trovo la fine del timstamp
        // (delimitata da una parentesi quadra chiusa)
        int j = i;
        while (j < text_len && text[j] != ']') j++;
        j += 2;
        if (j >= text_len - 1 || i == j) {
            printf("Error while queuing the message.\n");
            return;
        }
        // allocazione dello spazio per il messaggio e copia del massaggio da text
        new_message->message = malloc(sizeof(char) * (text_len) - j + 3);
        memcpy(new_message->message, &text[j], (text_len) - j);
        // allocazione dello spazio per il time e copia del time da text
        new_message->time = malloc(sizeof(char) * j - i + 1);
        memcpy(new_message->time, &text[i], j - i);
        // set dell'ultimo elemento a '\0'
        new_message->message[(text_len) - j] = '\0';
        new_message->time[j - i] = '\0';
        // free di text
        free(text);
    }

    // dopo aver costruito il messaggio devo accordarlo in maniera corretta.
    // Se mode == TIMESTAMP_MODE devo inserire il messaggio mantenendo un ordine
    // crescente del timestamp in modo da avere i timestamp maggiori in coda.
    // In questo modo se arriva un messaggio che è stato inviato prima ma che ha fatto ritardo
    // viene accodato prima di quelli che sono stati madati dopo ma sono arrivati prima di lui.
    if (mode == TIMESTAMP_MODE) {
        // indice usato per scorrer i messaggi
        int i;
        // variabile usata come check per vedere se il messaggio è stato inserito
        int added = 0;
        // all' interno di questo for scorro i messaggi e cerco la posizione in cui devo inserire il messaggio
        for (i = 0; i <= msg_list->message_list->last; i++) {
            chat_message *msg_in_list = msg_list->message_list->list[i];
            if (compare_time(new_message, msg_in_list) < 0) {
                dinamic_list_insert(msg_list->message_list, new_message, i);
                printf("Added message in place %d\n", i);
                added = 1;
                break;
            }
        }
        // se i == 0 && !added (ovver non c'erano messaggi nella lista e quindi no è stato ciclato nemmeno una volta)
        // o il messaggio non è stato aggiunto alla lista (perchè ha il timestamp più alto), lo aggiungo con una add (che accoda alla fine il messaggio)
        if ((i == 0 && !added) || !added) dinamic_list_add(msg_list->message_list,(void *) new_message);
    }
    // se invece mode == RECEIVE_MODE il messaggio è stato già impacchettato e basta accodarlo.
    else if (mode == RECEIVE_MODE) {
        dinamic_list_add(msg_list->message_list,(void *) new_message);
    }
    // uso la signal per dire al reader thread che c'è un messaggio da inviare
    pthread_cond_signal(msg_list->mutex->cond);
    puts("queued message");
    // unlock del mutex
    pthread_mutex_unlock(msg_list->mutex->mutex);
}
/*
Funzione che manda i messaggi a tutti i client. Prende come input un messaggio e una lista di client.
*/
void send_message_to_everyone(chat_message  *msg, dinamic_list *cli_data_list) {
    // lock sulla lista dei client per non far eliminare/aggiungere client durante l'invio
    pthread_mutex_lock(cli_data_list->mutex);
    // se mode == TIMESTAMP_MODE il timestamp è stato passato dal client come long (in microsecondi).
    // deve quindi essere convertito a stringa dopo aver tolto la parte riguardante microsecondi e millisecondi (basta dividere per 1000000)
    if (mode == TIMESTAMP_MODE) {
        // in questo passaggio il timestamp passa dall'essere in heap all'essere in stack.
        // faccio quindi la free del vecchio timestamp che si trova in heap.
        char *old = msg->time;
        msg->time = get_ascii_time_from_long((atol(msg->time)) / 1000000);
        free(old);
    }
    // invio del messaggio ai client
    for (int i = 0; i <= cli_data_list->last; i++) {        
        dprintf(((cli_data *)(cli_data_list->list[i]))->clifd, "[%s, %s] \033[32m|\033[0m %s\n", msg->sender, msg->time, msg->message);
    }
    // unlock del mutex e log del messaggio
    pthread_mutex_unlock(cli_data_list->mutex);
    chat_log(msg, mode);
}
/*
funzione che rimuove dalla memoria il messaggio e la struct chat_message.
Il nickname può ancora servire mentre il timestamp si trova in stack
*/
void free_mesage(chat_message *msg) {
    free(msg->message);
    free(msg);
}
/*
Funzione eseguita dal reader thread. Essa esegue un ciclo infinito, al cui interno c'è una cond, in attessa
di messaggi da inviare. I print di questa funzione sono rossi per essere diferenziati dagli altri
*/
void *reader_start(void *info) {
    // recupero delle informazioni passate dal thread principale
    reader_thread_param *reader_param = (reader_thread_param *) info;
    dinamic_list *cli_data_list = reader_param->cli_data_list;
    chat_message_list *msg_list = reader_param->msg_list;

    // while infinito in attesa dei messaggi. (consumer)
    while (1) {
        printf("\033[0;31m READER THREAD:    LOCKING THE MUTEX\033[0m\n");
        // lock sul mutex e controllo del numero di messaggi in lista.
        pthread_mutex_lock(msg_list->mutex->mutex);
        if (msg_list->message_list->last < 0) { // last indica l'indice dell'ultimo elemento della dinamic_list. se non ci sono elementi last è -1
            printf("\033[0;31m READER THREAD:    WAITING CONDITION\033[0m\n");
            // se non ci sono messaggi il thread si mette in attesa dei messaggi.
            pthread_cond_wait(msg_list->mutex->cond, msg_list->mutex->mutex);
        }
        // una volta che ci sono messaggi, il thread li scorre e li manda ai client.
        for (int i = msg_list->message_list->last; i >= 0; i--) {
            chat_message *msg;
            // Se mode == TIMESTAMP_MODE i messaggi non devono essere mandati se non sono stati inviati da abbastanza tempo.
            // Questo "limite" server per dare la possibilità ai messaggi che sono stati mandati prima ma che sono arrivati dopo
            // di essere inseriti e mandati prima. Dato che i messaggi sono ordinati per time crescente, appena trovo un messaggio
            // che ha aspettato troppo smette di ciclare il for dato che i prossimi sicuramente avranno timestamp maggiori.
            if (mode == TIMESTAMP_MODE) {
                msg = msg_list->message_list->list[i];
                if ((atol(get_current_time_u()) - atol(msg->time)) < (millis_check * 1000)){ 
                    break;
                }
            }
            // poppo il primo elemento, recupero il valore
            element e = dinamic_list_pop(msg_list->message_list, 0);
            if (e.error == 1) { // c'è stato un'errore
                return NULL;
            }
            msg= e.value;
            // il messaggio viene mandato a tutti e il messaggio viene tolto dall'heap
            send_message_to_everyone(msg, cli_data_list);
            free_mesage(msg);
        }
        printf("\033[0;31m READER THREAD:    UNLOCKING THE MUTEX\n");
        // unlock del mutex
        pthread_mutex_unlock(msg_list->mutex->mutex);
        // il thread va in sleep
        usleep(millis_check * 1000);  // sleep
    }
    return NULL;
}


/*
funzione che controlla se il nome n è già usato da un altro client.
ritorna 1 se il nome è già utilizzato, 0 altrimenti.
*/
int check_name(dinamic_list *l, char *n) {
    // il nome deve essere almento da un carattere
    if (strlen(n) < 1)
        return 1;
    // lock del mutex della lista dei clienti
    puts("\033[33m Checking name : lock on mutex\033[0m");
    pthread_mutex_lock(l->mutex);
    puts("\033[33m Checking name : locked\033[0m");
    // intero al cui interno verrà messo il risultato da ritornare
    int ret = 0;
    // for al cui interno vengono controllati i nomi dei client connessi
    for (long i = 0; i <= l->last; i++) {
        puts("\033[33m Checking name : getting data\033[0m");
        cli_data *data = (cli_data *) l->list[i];

        printf("\033[33m Checking name : %s\033[0m\n", data->nickname);
        if (strcmp(data->nickname, n) == 0) {   // se i nomi combaciano ritorno 1
            ret = 1;
            break;
        }
    }

    puts("\033[33m Checking name : unlock on mutex\033[0m");
    pthread_mutex_unlock(l->mutex);
    return ret;
}

/*
Metodo usato dal server per inviare i messaggi di join e di leave.
In base alla mode impacchetta il messaggio in modo diverso.
*/
void _server_send_message(chat_message_list *msg_list, char *nickname, char *text) {
    // se si è in TIEMSTAMP_MODE bisogna mettere il timestamp in microsecondi e
    // impacchettare nickname, timestamp e messaggio nella stessa stringa.
    // una volta impacchettato il messaggio si usa il metodo queue_message per aggiungerlo alla
    // lista dei messaggi.
    if(mode == TIMESTAMP_MODE) {
        char *time = get_current_time_u();  // recupero il timestamp
        unsigned int msg_len = strlen(time) + strlen(text) + strlen(nickname) + 6;  // calcolo la lunghezza del messaggio
        char *assembled_message = calloc(msg_len, sizeof(char));    // alloco il messaggio in memoria
        snprintf(assembled_message, msg_len, "[%s, %s] %s", nickname, time, text);  // assemblo il messaggio
        queue_message(msg_list, nickname, assembled_message);   // metto il messaggio nella lista con il metodo queue_message
    }
    // se invece si è in RECIEVE_MODE il messaggio sarà impacchettato dalla queue_message.
    else if (mode == RECEIVE_MODE) {
        char *allocated_text = calloc(strlen(text), sizeof(char));
        strcpy(allocated_text, text);
        queue_message(msg_list, nickname, allocated_text);
    }
}

/*
Funzione usata dai thread che gestiscono la connessione con i client e la ricezione dei messaggi.
All'interno della funzione viene letto il nickname e viene inviata al client una risposta di accettazione/rifiuto
del nickname, una vlta accettato il nickname viene comunicata al client la mode, viene aggiunto il client alla lista
dei client, viene mandato a tutti il messaggio di join, e infine si entra in un ciclo all'interno del quale il thread
attende che il client scriva un mesaggio per metterlo nella queue.
*/
int chat_start(void *info) {
    #define NICK_SIZE 200
    // recupero delle informazioni dall'argomento passato in input
    receiver_thread_param *param = (receiver_thread_param *) info;
    cli_data *data = param->data;   // data del client
    dinamic_list *cli_data_list = param->cli_data_list; // puntatore alla lista dei client
    chat_message_list *msg_list = param->msg_list;      // puntatore alla msg_list
    int clifd = data->clifd;                            // file descriptor del client
    char *nickname;                                     // nickname del client
    int do_loop = 0;    // intero che fa ciclare il do..while finchè il client non inserisce un nickname valido
    // do while per il nickname
    do {
        if (do_loop) {  // se si entra in questo if vuol dire che non è il primo ciclo e che quindi il client ha inserito un nickname non valido.
            write(clifd, "0", 1);   // invio al client uno "0" per dire che il nickname non è stato accettato.
        }

        printf("\033[33m %d wants to join.\n\033[0m", clifd);
        // leggo il nickname che mi manda il client
        nickname = read_fd(clifd);
        //check sul nickname
        if (nickname == NULL) {
            printf("\033[33m Client %d disconnected\n\033[0m", clifd);
            close(clifd);
            return -1;
        }
        // sostituisco uno \0 al posto dello \n
        nickname[strcspn(nickname, "\n\0")] = '\0';
        printf("\033[33m name : %s\n\033[0m", nickname);

    } while ((do_loop = check_name(cli_data_list, nickname)));  // check sul nome e assegnazione di do_loop.

    write(clifd, "1", 1);   // invio del messaggio di accettazione del nickname al client

    data->nickname = nickname;

    // aggiunta del client alla lista
    dinamic_list_add(cli_data_list, data);

    // comunicazione al client della mode del server
    write(data->clifd, mode == RECEIVE_MODE ? "1" : "0", 2);
    // invio del messaggio di join
    _server_send_message(msg_list, data->nickname, "\033[32m-- Join the chat\n\033[0m");

    char *buffer;   // puntatore al buffer che verrà letto dal file descriptor del client.
    // while infinito al cui interno viene letto il messaggio del client e viene accodato all alista dei messaggi.
    while (1) {
        // lettura dal file descriptor
        buffer = read_fd(clifd);
        // check per capire se il client ha lasciato la chat.
        // quando un client lascia la chat, il thread manda lo rimuove dal cli_data_list e libera la memoria
        // dalle informazioni allocate riguardanti il client.
        if (buffer == NULL) {
            close(clifd);
            if (dinamic_list_remove_element(cli_data_list, (void *) data) == NULL) {
                puts("\033[33m USER NOT FOUND\033[0m");
            }
            _server_send_message(msg_list, data->nickname, "\033[31m-- Left the chat\n\033[0m");
            // aspetta che venga mandato il messaggio per liberare lo spazio allocato per il nickname e per le informazioni riguardanti l'address del client
            sleep(1);   
            free(data->nickname);
            data->nickname = NULL;
            free(data->cli_addr);
            data->cli_addr = NULL;
            data->clifd = -1;
            return 1;
        }    
        else {// se invece è stato letto qualcosa, il messaggio viene accodato.
            queue_message(msg_list, nickname, buffer);
        }
    }
}

/*
Funzione che gestisce sia i thread della thread pool che i thread in più creati per i client.
Riceve una struct di tipo receiver_info al cui interno ci sono:
- receiver_thread_param *receiver_param : la struttura da passare alla funzione chat start
- pthread_t tid : il tid del thread
- pthread_cond_t *cond : la condition usata dal thread per capire se deve far partire la chat
- pthread_mutex_t *mutex : il mutex del thread (usato con la condition)
- int running : intero che server per capire se il thread sta runnando o no. viene lockato il mutex prima di accedere a questa variabile
- int kill : indica se il thread deve fermarsi e ritornare una volta ceh il client attualmente connesso si scollega.
*/
void * receiver_start(void *info) {
    // recupero delle informazioni passate dal main thread
    receiver_info *rec_info = (receiver_info *) info;
    do {
        // lock sul mutex delle informazioni di questo thread e controllo sulla variabile running.
        // Se running è 1 vuol dire che le informazioni all'interno di rec_info->receiver_param riguardano un client che si
        // è appena connesso e con il quale bisogna far partire la chat.
        // Se invece running è 0 il thread va in attesa che il thread principale metta all'interno di rec_info->receiver_param informazioni
        // valide riguardanti un client che si è appena connesso.
        // I thread della thread pool si potranno fermare in attesa mentre i thread creati per servire i client quanto nella thread pool
        // non ci soo thread liberi non si fermeranno mai ad aspettare (dato che devono essere dei thread "one shot" e devono terminare una volta
        // completato il loro compito).
        pthread_mutex_lock(rec_info->mutex);
        if(!rec_info->running) {
            puts("Going to sleep");
            pthread_cond_wait(rec_info->cond, rec_info->mutex);
            puts("unlocked");
        }
        // a questo punto le informazioni all'interno di rec_info->receiver_param riguardano un client collegato in attesa di iniziare la chat.
        // lascio il lock e chiamo la funzione chat_start.
        pthread_mutex_unlock(rec_info->mutex);
        puts("starting chat");
        chat_start(rec_info->receiver_param);
        
        // una volta finita la chat imposto running a 0
        pthread_mutex_lock(rec_info->mutex);
        rec_info->running = false;
        pthread_mutex_unlock(rec_info->mutex);

        // se il thread non deve essere killato perchè fa parte della thread pool va ad aumentare il contatore dei thread disponibili.
        if (!rec_info->kill) {
            pthread_mutex_lock(available_threads_mutex);
            available_threads++;   // se il thread non deve ritornare, kill è 0. Quindi il thread torna in attesa nella thread pool
            pthread_mutex_unlock(available_threads_mutex);
        }

    } while (!rec_info->kill);  // se il thread non deve ritornare (e fa quindi parte della thread pool) cicla e va ad aspettare che il main thread
                                // inserisca dei dati validi in rec_info->receiver_param
    
    // se invece il thread deve terminare va ad eliminare dall'heap tutte le strutture che non serviranno più e ritorna
    free(rec_info->cond);
    free(rec_info->mutex);
    free(rec_info->receiver_param->data);
    free(rec_info->receiver_param);
    free(rec_info);
    return NULL;
}


