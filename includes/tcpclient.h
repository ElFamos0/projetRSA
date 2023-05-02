#ifndef __client_H__
#define __client_H__

#include <pthread.h>
#include "./list.h"


struct _msg_t
{
    list_t * messages;
};

typedef struct _msg_t msg_t;

struct _arg_element_t
{
    msg_t* msg;
    int msg_nb;
    pthread_mutex_t mutex;
    int sockid;
    int ended;
};
typedef struct _arg_element_t arg_t;
arg_t * arg_create(msg_t* msg, pthread_mutex_t mutex, int sockid);
void arg_destroy(arg_t * arg);
#endif
