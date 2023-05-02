#include "../includes/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


list_t* list_create() {

    list_t * liste = (list_t *)calloc(1,sizeof(list_t));
    return liste;

}

void list_destroy(list_t* one_list) {

    if (one_list!= NULL) {
        list_t* nxt;

        while (!(one_list->tail == NULL)) {

            nxt = one_list->tail;
            free(one_list->head->cle);
            free(one_list->head->val);
            free(one_list->head);
            free(one_list);
            one_list = nxt;
        
        }
        free(one_list->head);
        free(one_list);
    }
    

}

bool list_is_empty(list_t* one_list) {

    if(one_list == NULL) {
        return true;
    }
    if (one_list->head == NULL && one_list->tail == NULL) {
        return true;
    }

    return false;
}

void empty_list(list_t *one_list) {

    if (!(one_list== NULL)) {

        list_destroy(one_list->tail);
        if (one_list->head != NULL) {
            free(one_list->head->cle);
            free(one_list->head->val);
            free(one_list->head);
        }
        one_list->head = NULL;
        one_list->tail = NULL;

    }
}

void list_append(list_t* one_list, char* one_key, char* one_value) {

    while (one_list->tail != NULL) {

        one_list = one_list->tail;
    }

    element_t * elt  = malloc(sizeof(element_t));
    //elt->cle = malloc(sizeof(char *));
    //elt->val = malloc(sizeof(char *));
    elt->cle = strdup(one_key);
    elt->val = strdup(one_value);
    one_list->head = elt;
    list_t * new_list = list_create();
    one_list->tail = new_list;


}

void list_prepend(list_t* one_list, char* one_key, char* one_value) {

    element_t * elt  = malloc(sizeof(element_t));
    //elt->cle = malloc(sizeof(char *));
    //elt->val = malloc(sizeof(char *));

    elt->cle = strdup(one_key);
    elt->val = strdup(one_value);

    list_t * new_list = list_create();
    new_list->head = one_list->head;
    new_list->tail = one_list->tail;


    one_list->head = elt;
    one_list->tail = new_list;

}

void element_print(element_t* one_element) {

    char * key = one_element->cle;
    char * val = one_element->val;

    printf("%s : %s", key,val);
}

void list_print(list_t* one_list) {

    printf("{");
    while (one_list->tail != NULL) {

        element_print(one_list->head);
        one_list = one_list->tail;
        if (!(list_is_empty(one_list))) {
            printf(",");
        }
    }
    printf("}");
}

bool list_contains(list_t* one_list, char* one_key) {

    bool state = false;
    while (!(list_is_empty(one_list))) {

        if (strcmp(one_list->head->cle,one_key) == 0) {
            //printf("%s,%s\n",one_list->head->cle,one_key);
            state = true;
        }
        one_list = one_list->tail;
        
    }
    return state;
}

element_t * list_pop(list_t * one_list) {

    if (one_list != NULL) {

        element_t * temp = one_list->head;
        if (one_list ->tail != NULL) {
            one_list->head = one_list->tail->head;
            one_list->tail = one_list->tail->tail;
        } else {
            one_list->head = NULL;
            one_list->tail = NULL;
        }
        return temp;

    } else {
        return NULL;
    }

}


char* list_find(list_t* one_list, char* one_key) {

    if (!(list_contains(one_list,one_key))) {
        return NULL;
    }

    else {

        char * val = NULL;
        //printf("%s,%s\n",one_list->head->cle,one_key);
        while (!(list_is_empty(one_list)) && val == NULL) {
            
            if (strcmp(one_list->head->cle,one_key)==0) {
                val = one_list->head->val;
            }

            one_list = one_list->tail;

        }

        return val;
    }
}

void list_remove(list_t * one_list, char* one_key) {


    if (!(list_contains(one_list,one_key))) {
     // printf("No key found");
    }

    else {

        char * val = NULL;
        while (!(list_is_empty(one_list)) && val == NULL) {
            //printf("%s,%s\n",one_list->head->cle,one_key);
            if (strcmp(one_list->head->cle,one_key)==0) {
                val = one_list->head->val;
                list_pop(one_list);
            }

            one_list = one_list->tail;

        }
    }

}

char *** list_to_arr(list_t * one_list, int * size_ptr) {

    int size = 0;
    char * mot;
    char *** arr = (char ***)calloc(1,sizeof(char **)) ;
    (*arr) = (char **) malloc(sizeof(char*));


    while (!(list_is_empty(one_list))) {
        
        mot = strdup(one_list->head->cle);
        size+=1;
        (*arr) = (char **) realloc((*arr),(size+1)*sizeof(char[30]));
        (*arr)[size-1] = mot;
        one_list = one_list->tail;
        
        
    }
    // void * realloc( void * pointer, size_t memorySize )
    // (*arr) = (char**)malloc(size*sizeof(char*)) ;
    // for(int i = 0; i<(int)pow(3,size); i++ ) {

    //     (*char)[i] = create_pattern(val_arr[i],word);
    // }

    (* size_ptr) = size;
    return arr;

}

void destroy_list_arr(char *** arr, int size) {

    for(int i = 0 ; i< size; i++) {

        free((*arr)[i]);

    }

    free(*arr);
    free(arr);

}
