#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
/*
funzione che prende in input un long e lo trasforma in una stringa del tipo "day month dd hh:mm:ss yyyy".
*/
char *get_ascii_time_from_long(long t) {
    struct tm * timeinfo;           // puntatore alla struttura dove verranno messe le informazioni ricavate dal long passato in input
    timeinfo = localtime ( &t );    // assegno il puntatore a ciò ceh ritorna la funzione localtime 
                                    // che, da descrizione "Return the `struct tm' representation of *TIMER in the local timezone."
    char *ascii_time = asctime (timeinfo);  // trasformo il timeinfo in ascii con la funzione asciitime
    ascii_time[strlen(ascii_time) - 1] = '\0';  // rimuovo lo \n dalla fine della stringa
    return ascii_time;
}
/*
funzione che ritorna il tempo corrente sottoforma di stringa del tipo "day month dd hh:mm:ss yyyy".
*/
char *get_current_time() {
    time_t rawtime; // inizializzo la variabile time_t (è un typedef int)
    time(&rawtime); // uso la funzione time per mettere all'interno del current time il tempo (è in pratica il conto dei secondi dall'1 gennaio 1970)
    return get_ascii_time_from_long(rawtime);
}
/*
Funzione che ritorna il tempo corrente in microsecondi sottoforma di stringa.
Usata quando il client deve mandare un tempo più preciso dell'invio del messaggio (solo in TIMESTAMP_MODE)
in modo da dare la possibilità al server di accodare il mesaggio con più precisione.
*/
char *get_current_time_u() {
    #define RET_LEN 20      // definisco la lunghezza della stringa ritornata
    struct timeval  t;      // inizializzo la struttura dove verranno messe le informazioni riguardanti i seconndi e i microsecondi
    gettimeofday(&t, NULL); // recuopero le informazioni
    char *t_str = calloc(RET_LEN, sizeof(char));    // alloco memoria per la stringa da ritornare
    snprintf(t_str, RET_LEN, "%lu", t.tv_sec * 1000000 + t.tv_usec);    // combino insieme il tempo in secondi e quello in microsecondi e poi metto il risultato nella stringa
    return t_str;
}

/*
Funzione che prende un messaggio e la mode con cui si sta lavorando e va a scrivere il messaggio in un file.
*/
void chat_log(chat_message *msg, enum chat_mode m) {
    char log_file_name[16];             // array di caratteri che conterrà il nome del file con cui lavorare
    strcpy(log_file_name, "log_file");  // inserisco all'interno di log_file_name la prima parte del nome
    FILE *f;                            // variabile al cui interno metterò il file di log
    
    // scelta del suffisso in base alla mode
    if(m == RECEIVE_MODE) {
        strcat(log_file_name, "RCV.log");
    }
    else {
        strcat(log_file_name, "TMS.log");
    }

    // calcolo della lunghezza del messaggio e suo assemblamento
    unsigned int msg_len = strlen(msg->sender) + strlen(msg->time) + strlen(msg->message) + 7;  
    char assembled_message[msg_len];
    snprintf(assembled_message, msg_len, "[%s, %s] %s\n", msg->sender, msg->time, msg->message);

    // apertura del file e scrittura del messaggio
    f = fopen(log_file_name, "a");
    fwrite(assembled_message, sizeof(char), msg_len - 1, f);

    //chiusura del file
    fclose(f);
}