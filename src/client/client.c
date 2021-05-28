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

// lunghezza dell'host
#define HOST_LEN 41
// lunghezza della porta
#define PORT_LEN 11
// lunghezza del nickname
#define NICK_LEN 195

login_data *init_connection();
int start_chat(login_data *);

/*
funzione che stampa un messaggio passato come input nello stderr e chiama la exit();
*/
void error(char *msg)
{
    perror(msg);
    exit(0);
}
/*
Ritorna la larghezza del terminale. Questa informazione è usata quando biogna eliminare i messaggi scritti dall'utente quando li invia.
*/
int get_term_width() {
    // inizializzo la struttura nella quale verrà messa la grandezza dello schermo
    struct winsize w;
    // funzione che mette all'interno di w le informazioni riguardanti il terminale.
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

int main (int argc, char **argv) {
    /*
    il client non accetta argomenti.
    */
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
    
    /*
    inizializzazione della connessione
    */
    login_data *log_data = init_connection();
    /*
    check sui dati tornati da init_connection
    */
    if (log_data == NULL){
        error("An error ocurred.");
    }

    /*
    funzione che fa partire la chat vera e propria
    */
    start_chat(log_data);
    
    return 0;
}

/*
stampa un messaggio in console
*/
int print_message(char* message) {
    printf("%s", message);
    return 0;
}

/*
Funzione usata dal thread incaricato a leggere continuamente dal
file descriptor del server e stampare i messaggi in arrivo.
*/
void *reader(void *log_data) {
    // poiner al buffer in entrata
    char *buf;
    /* all'interno del while:
        1) viene letto il file descriptor del server con la funzione read_fd
        2) viene fatto un check su ciò che viene ritornato dalla read_fd. 
           Se infatti viene ritornato NULL vuol dire che il client si è disconnesso e
           si deve chiudere il file descriptor del server.
           Altrimenti si mostra il messaggio a schermo.
    */
    while (1) {
        buf = read_fd(((login_data *)log_data)->fd);
        if(buf != NULL) {
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


/*
funzione che rinchiude la parte di codice che gestisce il dialogo con il server
per impostare il nickname, la mode e iniziare lo scambio dei messaggi
*/
int start_chat(login_data *log_data) {
    // tid del reader thread
    pthread_t tid;
    // buffer usato per leggere dal socket la mode del server.
    char m[2];
    // variabile dove viene messa la mode
    enum chat_mode mode;
    // lettura della modalità (il server scrive 1 o 0 in base alla mode)
    read(log_data->fd, m, 1);
    // check e set della mode
    if (m[0] == '0')
        mode = TIMESTAMP_MODE;
    else 
        mode = RECEIVE_MODE;
    // Creazione del reader thread. Ad esso viene passato log_data
    pthread_create(&tid, NULL, &reader, (void *)log_data);

    // viene inizializzato il chat_message con all'interno il nickname e il messaggio
    chat_message *new_message = malloc(sizeof(chat_message));
    new_message->sender = log_data->nickname;

    /*
    while all'interno del quale:
            1) viene letto l'input dell'utente con la funzione readinput
            2) viene "pulito" lo stdout dall'input dell'utente
            3) vinen controllato che siano stati letti almeno due byte (un carattere e lo \n almeno)
            4) se il test viene passato, l'input dell'utente viene inpacchettato in una struct chat_message
               e viene mandato al server. Se la mode è RECEIVE_MODE viene mandato al server solo il buffer,
               altrimenti viene mandato anche il nickname e il tempo. Infine il messaggio viene loggato.
    */
    while (1) {
        // leggo l'input dell'utente dal terminale
        char *buff = readinput();

        // cancello dal termnale ciò che l'user ha appena scritto
        int w = get_term_width();
        int lines_to_remove = strlen(buff)/w;
        for (int j = 0; j <= lines_to_remove; j++) {
            printf("\r");
            printf("\033[1A");
            printf("\033[K"); 
        }

        // check su quanto è stato letto
        if(strlen(buff) > 1) {
            new_message->message = buff;
            // se mode == RECEIVE_MODE è il server a decidere il timestamp, quindi al
            // client serve solo per il log e basta avere il current time con il format Day Month dd hh:mm:ss yyyy.
            // Inoltre basta mandare al server il solo messaggio.
            if (mode == RECEIVE_MODE) {
                new_message->time = get_current_time();
                write(log_data->fd, buff, strlen(buff));
            }
            // se invece la mode è TIMESTAMP_MODE, bisogna mandare al server il messaggio formattato 
            // con all'interno anche il nickname e il tempo in microsecondi (per essere più precisi con il reinvio dei messaggi).
            else {
                // prendo il tempo in microsecondi come stringa
                new_message->time = get_current_time_u();
                // calcolo la lunghezza del messaggio
                unsigned int msg_len = strlen(new_message->sender) + strlen(new_message->time) + strlen(new_message->message) + 8;
                // inizializzo il buffer dove metterò insieme il nickname, il time e il messaggio
                char assembled_message[msg_len];
                // asemblo il messaggio
                snprintf(assembled_message, msg_len, "[%s, %s] %s\n", new_message->sender, new_message->time, new_message->message);
                // mando il mesaggio
                write(log_data->fd, assembled_message, msg_len);
            }
            // faccio il log del messaggio
            chat_log(new_message, mode);

            if (mode == TIMESTAMP_MODE) free(new_message->time);    // la funzione get_current_time_u, usata solo se 
                                                                    // mode == TIMESTAMP_MODE, alloca in memoria il time.
                                                                    // bisogna quindi liberarla una volta che non serve più

        }
        // rilascio la memoria allocata dalla readinput
        free(buff);
    }
}

/*
funzione che inizializza la connessione con il server. Chiede all'utente l'ip del server, la porta e, una volta connesso, il nickname.
La funzione chiede un nickname finchè il server non da l'ok per il nome scelto (infatti il server non ammette omonmi contemporanemente).
*/
login_data *init_connection() {
    // struttura che conterrà le informazioni per il login da ritornare
    login_data *data;

    // struttura che conterrà le informazioni del server
    struct sockaddr_in *serv_addr;

    // inizializzazione del socket e salvataggio del file descriptor all'interno di una variabile
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // check del file descriptor
    if(sockfd < 0) {
        error("Error while openeng socket.\n");
    }
    // alloco spazio in heap per i dati da ritornare
    data = malloc(sizeof(login_data));
    data->fd = sockfd;
    data->host = calloc(HOST_LEN, sizeof(char));
    
    // alloco spazio al sockaddr_in e setto AF_INET
    serv_addr = malloc(sizeof(struct sockaddr_in));
    serv_addr->sin_family = AF_INET;

    // intero che viene usato per controllare che la connessione sia avvenuta con successo. Finché è minore di 0 il do..while esterno si ripete
    int connection_succeded;
    // intero usato per controllare che l'host inserito sia corretto. Finché è 1 il do..while interno si ripete
    int correct_host = 1;

    //do while esterno che controlla se la connessione ha avuto successo
    do {

        // do while interno che controlla se l'host inserito è corretto
        do {
            puts("Please enter the server address: ");
            // fgets dell'host, rimozione dello \n finale e pulizia delle scritte a schermo
            fgets(data->host, HOST_LEN - 1, stdin);
            data->host[strcspn(data->host, "\n")] = '\0';
            printf("\r");
            printf("\033[1A");
            printf("\033[K");
            printf("\033[1A");
            printf("\033[K");
            
            // check dell'host e sua traduzione a address in bit con la funzione inet_pton
            // se il check va a buon fine si procede con la richiesta della porta
            if(inet_pton(AF_INET, data->host, &(serv_addr->sin_addr)) > 0) {
                
                puts("Please enter the port: ");
                // array dove verrà inserito l'input dell'utente
                char p[PORT_LEN];
                memset(p, 0, PORT_LEN);
                // while che non fa continuare la funzione finchè l'utente non inserirà un intero
                while (fgets(p, PORT_LEN - 1, stdin)) {
                    data->port = atoi(p);
                    if (data->port == 0) {
                        printf("Please enter an integer: ");
                    } else break;
                }
                //pulizia del terminale
                printf("\r");
                printf("\033[1A");
                printf("\033[K");
                printf("\033[1A");
                printf("\033[K");
                printf("\033[1A");
                printf("\033[K");
                fflush(stdin);
                // check sulla porta inserita
                if(0 > data->port || data->port > 65536)
                    perror("Invalid port.");
                else {
                    // conversione della porta
                    serv_addr->sin_port = htons(data->port);
                    correct_host = 0;
                }
            }
            else{
                fprintf(stdout,"ERROR, no such host\n");
            }
        } while (correct_host);

        
        printf("Connecting to the server %d:%d\n", serv_addr->sin_addr.s_addr, data->port);

        // assegnazione e check di connection_succeded. Se la connessione fallisce viene chiesto se si vuole provare la riconnessione con gli stessi parametri.
        // se invece la connessione no fallisce, connecion_succeded viene settato a un intero maggiore o uguale a 0 e il while esterno viene fermato.
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
            if ( (int) c[0] != (int) 'Y' && (int) c[0] != (int) 'y')
                correct_host = 1;
        }
        else {
            puts("Connected to the server\n");
        }        

    } while (connection_succeded < 0);

    fflush(stdout);

    // ora si deve chiedere all'user un nickname da mandare al server e successivamente controllare se il server accetta il nickname.
    // In caso il server non accetti viene chiesto all'utente un nuovo nickname.
    puts("Enter your nickname: ");
    // leggo l'input dall'utente
    data->nickname = readinput();
    printf("\r");
    printf("\033[1A");
    printf("\033[K");
    printf("\033[1A");
    printf("\033[K");
    fflush(stdout);
    // mando al server il nickname
    write(sockfd, data->nickname, strlen(data->nickname));

    // buffer dove viene messa la risposta del server riguardante l'accettazione o meno del nickname
    char response[2];
    response[1] = '\0';

    // lettura della risposta
    read(sockfd, response, 1);

    // finchè il server non accetta il nickname, questo while continua all'infinito.
    while (response[0] == '0') {
        // lettura del nuovo nickname dall'user
        puts("Nickname already in use, please chose a new one: ");
        data->nickname = readinput();

        printf("\r");
        printf("\033[1A");
        printf("\033[K");
        printf("\033[1A");
        printf("\033[K");
        
        fflush(stdout);

        // invio del nickname al server
        write(sockfd, data->nickname, strlen(data->nickname));

        // check sulla write
        if(read(sockfd, response, 1) <= 0) {
            perror("Server disconnected");
            close(sockfd);
            return NULL;
        }

    }

    // rimozione dello \n dal nome
    data->nickname[strcspn(data->nickname, "\n")] = '\0';

    printf("\r");
    printf("\033[1A");
    printf("\033[K");

    fflush(stdout);

    return data;
}
