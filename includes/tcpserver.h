#ifndef __serv_H__
#define __serb_H__

#include <pthread.h>
#include "./list.h"


struct _info_t
{
    list_t * techniciens;
    list_t * experts;
};

typedef struct _info_t info_t;

struct _arg_element_t
{
    char* in;
    char* out;
    int sock_id;
    int sock_to;
    info_t * info;
    pthread_mutex_t mutex;
    int ended;
    int thread_id;
};

typedef struct _arg_element_t arg_t;
arg_t * arg_create(int id, int thread_id, pthread_mutex_t mutex, info_t * info);
void arg_destroy(arg_t * arg);
void arg_new_in(arg_t* arg, char * in);
void arg_new_out(arg_t* arg, char * out);


#endif
