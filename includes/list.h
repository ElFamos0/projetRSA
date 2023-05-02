#include <stdbool.h>

#ifndef __LIST_H__
#define __LIST_H__

struct _element_t
{
    char * cle;
    char * val;
};

typedef struct _element_t element_t;

struct _list_t
{
    struct _list_t * tail;
    element_t * head;
};

typedef struct _list_t list_t;

list_t *list_create();

void list_destroy(list_t *one_list);

bool list_is_empty(list_t *one_list);

void empty_list(list_t *one_list);

void list_append(list_t *one_list, char *one_key, char *one_value);

void list_prepend(list_t *one_list, char *one_key, char *one_value);

void element_print(element_t *one_element);

void list_print(list_t *one_list);

bool list_contains(list_t *one_list, char *one_key);

char *list_find(list_t *one_list, char *one_key);

char *** list_to_arr(list_t * one_list, int * size_ptr);

void destroy_list_arr(char *** arr, int size);

#endif /* __LIST_H__ */
