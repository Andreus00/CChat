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

int dinamic_list_pop_last(dinamic_list *list) {
    long *last = &((* list).last);

    if ((* last) == -1) {
        perror("List is empty");
        return NULL;
    }

    if (((* last)) < (long)((* list).allocated_size / 2)) {
        dinamic_list_reduce(list);
    }

    int popped = (* list).list[(* last)];

    (* list).list[(* last)] = 0;
    (* list).last--;

    return popped;
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



int main() {
    dinamic_list *d = dinamic_list_new();


    for (int i = 0; i < 20; i++){
            dinamic_list_add(d, i);
            printf("added\t%d   new length: %ld\n",i , (* d).allocated_size);
    }

    
    srand(time(NULL));   // Initialization, should only be called once.
    for (int i = 0; i < 50; i++){

        int r = rand();

        if (r > 1344784680) {
            dinamic_list_add(d, i);
            printf("added\t%d   new length: %ld\n",i , (* d).allocated_size);
        }
        else {
            int j = dinamic_list_pop_last(d);
            printf("remove\t%d   new length %ld\n", j, (* d).allocated_size);
        }

    }

    // printf("last %d\n", (* d).list[(*d).last]);
    // for (int i = 0; i < 25; i++){

    // }

    // for (int i = 0; i < (* d).last + 1; i++){
    //     printf("element: %d\n", (* d).list[i]);
    // }

    return 0;

}