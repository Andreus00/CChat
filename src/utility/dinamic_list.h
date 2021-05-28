/*
Mia implementazione delle liste dinamiche. Esse si comportano come arraylist di java.
Usano dei puntatori void * per mantenere il riferimento all'oggetto da mettere nella lista.
Ogni volta che si aggiunge un elemento e no nc'è abbastanza spazio, la lista si espande.
Ogni volta che invece si rimuove un elemento e c'è troppa memoria inutilizzata, la lista si riduce.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>


#define MIN_SIZE 10     // grandezza minima della lista

#define MAX(a,b) (((a)>(b))?(a):(b))    // direttiva che definisce MAX tra due numeri


/*
struttura che rappresenta la lista dinamica.
*/
typedef struct {

    long allocated_size;    // lunghezza della allocazione in memoria

    long last;      // posizione dell'ultimo elemento della lista

    void **list;     // puntatore alla lista dinamica (ovvero un puntatore a puntatori)

    pthread_mutex_t *mutex; // mutex usato qundo si devono fare operazioni sulla lista

} dinamic_list;

/*
Struttura usata per ritornare un elemento eliminato dalla lista.
*/
typedef struct {

    void *value;    // puntatore all'oggetto tolto dalla lista

    bool error;     // valore che viene settato a 1 se c'è stato qualche errore durante la rimozione dell'elemento (value in questo caso è 0).

} element;

int check_long_overflow(long*, long, long);
int dinamic_list_expand(dinamic_list *);
int dinamic_list_reduce(dinamic_list *);

/*
Funzione che aggiunge a list l'elemento el.
Per fare ciò controlla se bisogna espandere la lista e che l'espanzione della lista non causi overflow.
*/
int dinamic_list_add(dinamic_list *list, void *el) {

    pthread_mutex_lock(list->mutex);    // lock sul mutex della lista

    long *last = &((* list).last);      // recupero dell'indice dell'ultimo elemento valido

    // se aggiungere el porta a un riempimento dello spazio attualmente allocato per la lista, viene allocato nuovo spazio.
    // se non è possibile aggiungere un nuovo spazio viene ritornato 1.
    if (((* last) + 1) == (* list).allocated_size) {
        if(dinamic_list_expand(list)) {
            perror("Unable to add the element.\n");
            pthread_mutex_unlock(list->mutex);
            return 1;
        };
    }
    // aggiornamento di last e aggiunta dell'elemento
    (* list).last++;
    (* list).list[(* last)] = el;

    // release del lock
    pthread_mutex_unlock(list->mutex);

    return 0;
}

/*
Funzione che rimuove dalla lista l'ultimo elemento e lo ritorna incapsulato in un element
*/
element dinamic_list_pop_last(dinamic_list *list) {

    pthread_mutex_lock(list->mutex);    // lock del mutex dela lista

    long *last = &((* list).last);      // recupero della posizione dell'ultimo elemento

    // controllo sul valore di last. Se è -1 la lista è vuota e ritorno un element con error 1 e value 0
    if ((* last) == -1) {
        perror("List is empty");
        element e = {0, 1};
        pthread_mutex_unlock(list->mutex);
        return e;
    }
    // controllo per vedere se l'ultimo elemento si trova prima della metà della lunghezza
    // attualmente allocata per la dinamic_list. Se infatti è così la lista viene ridotta
    if (((* last)) < (long)((* list).allocated_size / 2)) {
        dinamic_list_reduce(list);
    }

    // recupero un puntatore all'ultimo elemento
    void *popped = (* list).list[(* last)];

    // set dell'ultimo elemento a 0 e aggiornamento di last
    (* list).list[(* last)] = 0;
    (* list).last--;

    // costruzione dell'element da ritornare e unlock del mutex
    element e = {popped, 0};
    pthread_mutex_unlock(list->mutex);
    return e;
}

/*
Funzione che espande la grandezza della lista allocando più memoria.
Se allocare più memoria non è possibile ritorna 1.
*/
int dinamic_list_expand(dinamic_list *list) {
    long *p_length = &((* list).allocated_size);    // recupero la lunghezza dell'allocazione attuale
    long increment = ((long) ((* p_length)/2));     // calcolo l'incremento

    long res = 0;   // variabile dove verrà messo da cehck_long_overflow la somma di p_length e increment.

    // check per veder se è ancora possibile espandere la lista
    if(check_long_overflow(&res, *p_length, increment)){
        if(*p_length == LONG_MAX) {     //se la lista è già al massimo viene ritornato il codice di errore 1
            perror("List is full.\n");
            return 1;
        }// altrimenti viene settata la lunghezza massima
        res = LONG_MAX;
    }
    // viene aggiornato p_length a res
    * p_length = res;
    
    (* list).list = realloc((* list).list, sizeof(p_length)*res);   // riallocazione della lista

    return 0;
}

/*
funzine che riduce la dimensione della allocazione in memoria della lista.
*/
int dinamic_list_reduce(dinamic_list *list) {
    // recupero la lunghezza della allocazione in memoria e calcolo di quanto devo ridurre la lista
    long *p_length = &((* list).allocated_size);
    long reduce = ((long) ((* p_length)/2));
    // la lista non può essere più piccola di MIN_SIZE, quindi metto in res il massimo tra MIN_SIZE e la lunghezza calcolata
    long res = MAX((* p_length) - reduce, MIN_SIZE);
    // aggiorno la lunghezza della dinamic_list
    *p_length = res;
    // rialloco la lista
    (* list).list = realloc((* list).list, sizeof(p_length)*res);

    return 0;
}

/*
funzione che, dato un puntatore a un long (result) e due long (a, b), controlla se a + b da overflow.
Mette la somma in result e ritorna 0 se non c'è overflow. Ritorna -1 altrimenti
*/
int check_long_overflow(long* result, long a, long b)
 {
     *result = a + b;   // metto il risultato della somma in result
     if(a > 0 && b > 0 && *result < 0)  // controllo se c'è stato overflow
         return -1;
     if(a < 0 && b < 0 && *result > 0)  // controllo se c'è stato overflow
         return -1;
     return 0;
 }
/*
Funzione che ritorna una nuova dinamic_list inizializzata e allocata in memoria.
*/
dinamic_list *dinamic_list_new() {
    dinamic_list *list = malloc(sizeof(dinamic_list));  // alloco in memoria spazio per la dinamic_list
    // inizializza i valori della dinamic list e ritorna il puntatore alla struttura
    (* list).last = -1;
    (* list).allocated_size = MIN_SIZE;
    (* list).list = calloc(MIN_SIZE, sizeof(list));
    list->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(list->mutex, NULL);
    return list;
}

/*
Funzione che rimuove dalla lista passata in input l'elemento passato in input.
Ritorna l'elemento eliminato o NULL se non è stato trovato.
*/
void *dinamic_list_remove_element(dinamic_list *list, void *el) {
    // lock del mutex della lista
    pthread_mutex_lock(list->mutex);
    // dichiarazione del puntatore da ritornare
    void *ret = NULL;

    // for all'interno del quale scorro la lista alla ricerca dell'elemento
    for(int i = 0; i <= list->last; i++) {
        // se trovo l'elemento, procedo con la rimozione.
        if (list->list[i] == el) {
            ret = list->list[i];
            // for che porta l'elemento alla fine della lista senza cambiare l'ordine degli altri elementi
            // In pratica copia il puntatore seguente al posto di quello corrente per far scorrere tutti di 1.
            for (long j = i; j < list->last; j++) {
                list->list[j] = list->list[j+1];
            }
            // setta a 0 l'ultimo elemento e aggiorna last.
            list->list[list->last] = 0;
            list->last--;
            pthread_mutex_unlock(list->mutex);
            return ret;
        }
    }
    //unlock del mutex
    pthread_mutex_unlock(list->mutex);
    return ret;
}

/*
Funzione che elimina dalla lista e ritorna l'elemento in i-esima posizione.
Ritorna il valore incapsulato in un element.
*/
element dinamic_list_pop(dinamic_list *list, long i) {
    // lock del mutex e recupero dell'indice dell'ultimo elemento
    pthread_mutex_lock(list->mutex);
    int last = (* list).last;
    // allocazione in memoria della struttura di tipo element
    element *ret = malloc(sizeof(element));
    // check sull'indice dell'elemento da eliminare
    if (i > last || i < 0) {
        perror("Index out of range");
        ret->value = 0;
        ret->error = 1;
    }
    else {  // se l'indice è corretto, salvo il puntatore all'i-esimo elemento, setto error a 0,
            // scorro di una posizione a sinistra tutti gli elementi che si trovano dopo l'i-esimo elemento
        ret->value = (* list).list[i];
        ret->error = 0;

        for (long j = i; j < last; j++) {   // for che fa scorrere di una posizione a sx tutti gli elementi alla destra di i
            list->list[j] = list->list[j+1];
        }
        // setto l'ultimo elemento a 0 e aggiorno last.
        list->list[last] = 0;

        list->last--;
    }
    // unlock del mutex e return
    pthread_mutex_unlock(list->mutex);
    return *ret;
}
/*
Funzione che inseriscre l'elemento elemento nella posizione index della lista.
ritorna 1 in caso di errore e 0 in caso di successo.
*/
int dinamic_list_insert(dinamic_list *list,void *element, long index) {
    // lock del mutex: devo ancora portare all'i-esima posizione l'elementos
    pthread_mutex_lock(list->mutex);
    long *last = &((* list).last);      // recupero dell'indice dell'ultimo elemento valido
    // check sull'indice passato come argomento
    if (index < 0 || index > *last) {
        perror("Insert index out of range");
        return 1;
    }

    // se aggiungere element porta a un riempimento dello spazio attualmente allocato per la lista, viene espansa la lista.
    // se non è possibile aggiungere un nuovo spazio viene ritornato 1.
    if (((* last) + 1) == (* list).allocated_size) {
        if(dinamic_list_expand(list)) {
            perror("Unable to add the element.\n");
            pthread_mutex_unlock(list->mutex);
            return 1;
        };
    }
    // aggiornamento di last
    (* list).last++;
    // spostamento di una posizione a destra di tutti gli elementi che si trovano alla destra di index.
    for(int i = list->last; i > index; i--) {
        list->list[i] = list->list[i - 1];
    }
    // assegnazione all'i-esimo elemento di index.
    list->list[index] = element;

    // unlock del mutex e return
    pthread_mutex_unlock(list->mutex);

    return 0;
}
