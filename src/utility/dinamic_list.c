#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


#define MIN_SIZE 10

#define MAX(a,b) (((a)>(b))?(a):(b))



typedef struct {

    long allocated_size;    // lunghezza della allocazione in memoria

    long last;      // posizione dell'ultimo elemento della lista

    int *list;     // questo è il puntatore alla lista dinamica

} dinamic_list;

/*
this struct is used to return a value from the list.
the int value stored in the list is "value".
"error" is 0 if there wasn't any error, 1 otherwise. If "error" is 1, "value" will be 0.
*/
typedef struct {

    int value;

    bool error;

} element;

int check_long_overflow(long*, long, long);
int dinamic_list_expand(dinamic_list *);
int dinamic_list_reduce(dinamic_list *);

bool dinamic_list_add(dinamic_list *list, int el) {
    long *last = &((* list).last);

    if (((* last) + 1) == (* list).allocated_size) {
        if(dinamic_list_expand(list)) {
            perror("Unable to add the element.\n");
            return 1;
        };
    }
    (* list).last++;
    (* list).list[(* last)] = el;

    return 0;
}

element dinamic_list_pop_last(dinamic_list *list) {
    long *last = &((* list).last);

    if ((* last) == -1) {
        perror("List is empty");
        element e = {0, 1};
        return e;
    }

    if (((* last)) < (long)((* list).allocated_size / 2)) {
        dinamic_list_reduce(list);
    }

    int popped = (* list).list[(* last)];

    (* list).list[(* last)] = 0;
    (* list).last--;
    element e = {popped, 0};
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
    
    (* list).list = realloc((* list).list, sizeof(int)*res);

    return 0;
}

int dinamic_list_reduce(dinamic_list *list) {
    long *p_length = &((* list).allocated_size);
    long reduce = ((long) ((* p_length)/2));

    long res = MAX((* p_length) - reduce, MIN_SIZE);
    
    * p_length = res;
    
    (* list).list = realloc((* list).list, sizeof(int)*res);

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
    (* list).list = calloc(MIN_SIZE, sizeof(int));

    return list;
}

void dinamic_list_print(dinamic_list *list) {
    long i = 0;
    while (i <= list->last) {
        printf("element %ld : %d\n", i, list->list[i]);
        i++;
    }
}


element dinamic_list_pop(dinamic_list *list, long i) {
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

    return *ret;
}

int dinamic_list_insert(dinamic_list *list,int element, long index) {
    if (index < 0 || index > list->last) {
        perror("Insert index out of range")
        return 1;
    }
    if (dinamic_list_add(list, element)) {
        perror("Unable to insert the element");
        return 1;
    }

    for(int i = list->last; i > index; i--) {
        list->list[i] = list->list[i - 1];
    }
    list->list[index] = element;

    return 0;
}


int main() {
    dinamic_list *d = dinamic_list_new();


    for (int i = 0; i < 20; i++){
            dinamic_list_add(d, i);
            printf("added\t%d   new length: %ld\n",i , (* d).allocated_size);
    }

    // element e = dinamic_list_pop(d, 30);
    // printf("el : %d\terr: %d\n", e.value, e.error);
    // e = dinamic_list_pop(d, 2);
    // printf("el : %d\terr: %d\n", e.value, e.error);
    // e = dinamic_list_pop(d, 14);
    // printf("el : %d\terr: %d\n", e.value, e.error);
    // e = dinamic_list_pop(d, 17);
    // printf("el : %d\terr: %d\n", e.value, e.error);
    // dinamic_list_add(d, 1);
    // printf("added\t%d   new length: %ld\n",1 , (* d).allocated_size);
    // dinamic_list_add(d, 1);
    // printf("added\t%d   new length: %ld\n",1 , (* d).allocated_size);
    // dinamic_list_add(d, 1);
    // printf("added\t%d   new length: %ld\n",1 , (* d).allocated_size);
    // e = dinamic_list_pop(d, 7);
    // printf("el : %d\terr: %d\n", e.value, e.error);
    // e = dinamic_list_pop(d, 17);
    // printf("el : %d\terr: %d\n", e.value, e.error);

    dinamic_list_print(d);

    dinamic_list_insert(d, -1, -10);


    dinamic_list_insert(d, -1, 5);
    puts("\n-------------------------\n");

    dinamic_list_print(d);
    

    
    // srand(time(NULL));   // Initialization, should only be called once.
    // for (int i = 0; i < 50; i++){

    //     int r = rand();

    //     if (r > 1344784680) {
    //         dinamic_list_add(d, i);
    //         printf("added\t%d   new length: %ld\n",i , (* d).allocated_size);
    //     }
    //     else {
    //         element j = dinamic_list_pop_last(d);
    //         printf("remove\t%d Error: %d  new length %ld\n", j.value, j.error, (* d).allocated_size);
    //     }

    // }

    // printf("last %d\n", (* d).list[(*d).last]);
    // for (int i = 0; i < 25; i++){

    // }

    // for (int i = 0; i < (* d).last + 1; i++){
    //     printf("element: %d\n", (* d).list[i]);
    // }

    return 0;

}