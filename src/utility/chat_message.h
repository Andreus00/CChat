#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define BUFFER_SIZE 255
/*
funzione che legge dal FILE *f un input dell'utente di una qualsiasi lunghezza.
usa un do..while e un test che controlla se i buffer temporaneo è stato letto completamente o no.
*/
char* readinput_i(FILE *f) {
   char* ret = NULL;  // puntatore alla allocazione di memoria dove verrà pian piano composto il messaggio
   char tempbuf[BUFFER_SIZE]; // buffer temporaneo all'interno dek quale verrà messo ciò che a ogni ciclo viene letto dalla fgets
   size_t retlen = 0, templen = 0;    // lunghezza di input e di tempbuff
   do {
       fgets(tempbuf, BUFFER_SIZE, f);    // leggo dal file CHUNK bytes e li metto in tempbuff
       templen = strlen(tempbuf);   // metto all'interno di templen il numero di caratteri letti ed ora in tempbuff
       ret = realloc(ret, retlen+templen+1);    // ralloco spazio sufficiente per poter poi copiare tempbuff in ret
       strcpy(ret+retlen, tempbuf);             // copio tempbuff in ret. Uso l'aritmetica dei puntatorei per far scrivere  tempbuff alla fine
       retlen += templen;                       // aggiorno la lunghezza di ret
    } while (templen==BUFFER_SIZE-1 && tempbuf[BUFFER_SIZE-2]!='\n');   // controllo per capire se il buffer letto era l'ultimo
    return ret;
}
/*
funzione che legge dallo stdin un input di lunghezza arbitraria.
Chiama readinput_i con come argomento stdin
*/
char* readinput() {
    return readinput_i(stdin);
} 

/*
funzione che legge dal file descriptor passatto un numero di caratteri arbitrario.
Ritorna NULL se ci sono stati problemi durante la lettura o se il server ha chiuso la connessione,
Altrimenti ritorna un puntatore al primo carattere letto (il heap)
*/
char *read_fd(int fd) {
    char *buffer = NULL;    // putntatore al buffer da ritornare
    char temp_buff[BUFFER_SIZE];    // array temporaneo da usar per leggere dal file descriptor
    int read_len = 0, buffer_len = 0;   // lunghezza del temp_buff e del buffer

    do {   // do..while ceh si ripete finchè c'è qualcosa da leggere da fd
        memset(temp_buff, 0, BUFFER_SIZE);  // set a 0 di tutti i bit del temp_buff
        read_len = read(fd, temp_buff, BUFFER_SIZE - 1);    // letura del file descriptor e salvataggio del numero di caratteri letti in read_len
        // check su ciò che si ha letto per capire se il server ha chiuso la connessione
        if (read_len <= 0) {
            return NULL;
        }
        buffer = realloc(buffer, (buffer_len + read_len + 1));  // riallocamento del buffer per creare spazio in cui copiare il temp_buff
        memset(buffer + buffer_len, 0, read_len);               // set dei bit allocati in più dalla realloc a 0
        strcpy(buffer + buffer_len, temp_buff);                 // copia del temp_buff nello spazio allocato in più dalla realloc
        buffer_len += read_len;                                 // aggiornamento di buffer_len
    
    // check per vedere se il buffer letto era l'ultimo
    }while ((temp_buff[read_len-1]!='\0' && temp_buff[read_len-1]!='\n') && (read_len >= (BUFFER_SIZE - 1)));

    return buffer;
}