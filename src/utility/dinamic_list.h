#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>


#define MIN_SIZE 10

#define MAX(a,b) (((a)>(b))?(a):(b))



typedef struct {

    long allocated_size;    // lunghezza della allocazione in memoria

    long last;      // posizione dell'ultimo elemento della lista

    void **list;     // questo è il puntatore alla lista dinamica

    pthread_mutex_t *mutex; // this mutex can be used to modify the list

} dinamic_list;

/*
this struct is used to return a value from the list.
the int value stored in the list is "value".
"error" is 0 if there wasn't any error, 1 otherwise. If "error" is 1, "value" will be 0.
*/
typedef struct {

    void *value;

    bool error;

} element;

int check_long_overflow(long*, long, long);
int dinamic_list_expand(dinamic_list *);
int dinamic_list_reduce(dinamic_list *);

int dinamic_list_add(dinamic_list *list, void *el) {

    pthread_mutex_lock(list->mutex);

    long *last = &((* list).last);

    if (((* last) + 1) == (* list).allocated_size) {
        if(dinamic_list_expand(list)) {
            perror("Unable to add the element.\n");
            pthread_mutex_unlock(list->mutex);
            return 1;
        };
    }
    (* list).last++;
    (* list).list[(* last)] = el;
    pthread_mutex_unlock(list->mutex);

    return 0;
}

element dinamic_list_pop_last(dinamic_list *list) {
    pthread_mutex_lock(list->mutex);
    long *last = &((* list).last);

    if ((* last) == -1) {
        perror("List is empty");
        element e = {0, 1};
        pthread_mutex_unlock(list->mutex);
        return e;
    }

    if (((* last)) < (long)((* list).allocated_size / 2)) {
        dinamic_list_reduce(list);
    }

    void *popped = (* list).list[(* last)];

    (* list).list[(* last)] = 0;
    (* list).last--;
    element e = {popped, 0};
    pthread_mutex_unlock(list->mutex);
    return e;
}

int dinamic_list_expand(dinamic_list *list) {
    long *p_length = &((* list).allocated_size);
    long increment = ((long) ((* p_length)/2));

    long res = 0;

    if(check_long_overflow(&res, *p_length, increment)){
        if(*p_length == LONG_MAX) {
            perror("List is full.\n");
            return 1;
        }
        res = LONG_MAX;
    }
    
    * p_length = res;

    dinamic_list **old = (dinamic_list **) list->list;
    
    (* list).list = realloc((* list).list, sizeof(p_length)*res);

    for (int i = *p_length; i == 0; i-- )
        free(old[i]);

    return 0;
}

int dinamic_list_reduce(dinamic_list *list) {
    long *p_length = &((* list).allocated_size);
    long reduce = ((long) ((* p_length)/2));

    long res = MAX((* p_length) - reduce, MIN_SIZE);
    
    *p_length = res;

    dinamic_list **old = (dinamic_list **) list->list;
    
    (* list).list = realloc((* list).list, sizeof(p_length)*res);

    for (int i = *p_length; i == 0; i-- )
        free(old[i]);

    return 0;
}


int check_long_overflow(long* result, long a, long b)
 {
     *result = a + b;
     if(a > 0 && b > 0 && *result < 0)
         return -1;
     if(a < 0 && b < 0 && *result > 0)
         return -1;
     return 0;
 }

dinamic_list *dinamic_list_new() {
    dinamic_list *list = malloc(sizeof(dinamic_list));

    (* list).last = -1;
    (* list).allocated_size = MIN_SIZE;
    (* list).list = calloc(MIN_SIZE, sizeof(list));
    list->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(list->mutex, NULL);
    return list;
}

void dinamic_list_print(dinamic_list *list) {
    long i = 0;
    while (i <= list->last) {
        element *value = ((element *)(list->list[i]));
        int *val = (int *)(value->value);
        printf("element %ld : %d\n", i, *val);
        i++;
    }
}

void *dinamic_list_remove_element(dinamic_list *list, void *el) {
    pthread_mutex_lock(list->mutex);
    void *ret = NULL;
    for(int i = 0; i <= list->last; i++) {
        if (list->list[i] == el) {
            for (long j = i; j < list->last; j++) {
                list->list[j] = list->list[j+1];
            }
            ret = list->list[list->last];
            list->list[list->last] = 0;
            list->last--;
            pthread_mutex_unlock(list->mutex);
            return ret;
        }
    }
    pthread_mutex_unlock(list->mutex);
    return ret;
}


element dinamic_list_pop(dinamic_list *list, long i) {
    pthread_mutex_lock(list->mutex);
    int last = (* list).last;

    element *ret = malloc(sizeof(element));

    if (i > last || i < 0) {
        perror("Index out of range");
        ret->value = 0;
        ret->error = 1;
    }
    else {
        ret->value = (* list).list[i];
        ret->error = 0;

        for (long j = i; j < last; j++) {
            list->list[j] = list->list[j+1];
        }
        list->list[last] = 0;

        list->last--;
    }
    pthread_mutex_unlock(list->mutex);
    return *ret;
}

int dinamic_list_insert(dinamic_list *list,void *element, long index) {
    if (index < 0 || index > list->last) {
        perror("Insert index out of range");
        return 1;
    }
    if (dinamic_list_add(list, element)) {
        perror("Unable to insert the element");
        return 1;
    }

    pthread_mutex_lock(list->mutex);

    for(int i = list->last; i > index; i--) {
        list->list[i] = list->list[i - 1];
    }
    list->list[index] = element;

    pthread_mutex_unlock(list->mutex);

    return 0;
}
