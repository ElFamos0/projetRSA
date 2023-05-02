#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>  // for fcntl()
#include <errno.h>  // for errno

#include "../includes/tcpclient.h"

// Global variables
pthread_t * threads;
pthread_mutex_t mutex;
arg_t ** threads_arg;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

arg_t * arg_create(msg_t* msg, pthread_mutex_t mutex, int sockid ) {

    arg_t * arg = (arg_t * )malloc(sizeof(arg_t));
    arg->msg_nb = 0;
    arg->msg = msg;
    arg->mutex = mutex;
    arg-> sockid = sockid;
    arg->ended = 0;
    return arg;
}


void arg_destroy(arg_t * arg) {

    free(arg);
}

void * read_server_messages_routine(void * arg) {

    arg_t * arglist = (arg_t * ) arg;
    pthread_mutex_t mutex = arglist->mutex;

    int connected = 1;
    char buffer[256];
    int n;
    list_t * li;

    // set the socket to non-blocking mode
    int flags = fcntl(arglist->sockid, F_GETFL, 0);
    fcntl(arglist->sockid, F_SETFL, flags | O_NONBLOCK);

    while (connected) {
        bzero(buffer,256);
        n = read(arglist->sockid,buffer,255);

        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // no data available on the socket, continue loop
                continue;
            }
            else {
                error("ERROR reading from socket");
            }
        }
        else if (n == 0) {
            // connection closed
            connected = 0;
            pthread_mutex_lock(&mutex);
            arglist->ended = 1;
            pthread_mutex_unlock(&mutex);
        }
        else {
            // data received
            char *last_newline = strrchr(buffer, '\n');
            if (last_newline != NULL) {
                *last_newline = '\0';
            } // On enlève le dernier '\n'

            pthread_mutex_lock(&mutex);
            char id[20];
            sprintf(id, "%d", arglist->msg_nb);  
            arglist->msg_nb +=1;
            list_append(arglist->msg->messages,id,buffer);
            pthread_mutex_unlock(&mutex);
        }

        // print all messages in the message list
        li = arglist->msg->messages;
        while (li->tail != NULL) {
            printf("Message reçu du serveur : %s \n",li->head->val);
            li = li->tail;
        }
        empty_list(arglist->msg->messages);
    }

    pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int nb = 50;
    pthread_mutex_init(&mutex, NULL);
    threads = malloc(nb * sizeof(pthread_t));
    threads_arg =  (arg_t **)calloc(nb,sizeof(arg_t *));
    msg_t * server_messages = (msg_t *)malloc(sizeof(msg_t));
    server_messages->messages = list_create();

    char buffer[256];

    // Vérification des arguments de la ligne de commande
    if (argc < 3) {
       fprintf(stderr,"Utilisation: %s hostname port\n", argv[0]);
       exit(0);
    }

    // Récupération du numéro de port et de l'adresse IP du serveur
    portno = atoi(argv[2]);
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"Erreur: l'hôte n'existe pas\n");
        exit(0);
    }

    // Création du socket client
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Erreur: impossible de créer le socket");

    // Configuration des paramètres du socket client
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);

    // Connexion au serveur
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("Erreur: connexion au serveur impossible");

    // Début de la communication avec le serveur
    int connected = 1;
    // On lance un thread qui s'occupe de la lecture des messages
    threads_arg[0] = arg_create(server_messages,mutex,sockfd);
    pthread_create(&threads[0], NULL, read_server_messages_routine, threads_arg[0]);
    // Boucle qui s'occupe de l'envoie de messages vers le serveur
    while(connected) {
        pthread_mutex_lock(&mutex);
        connected = (!threads_arg[0]->ended);
        pthread_mutex_unlock(&mutex);
        if (connected) {
            bzero(buffer,256);
            fgets(buffer,255,stdin);
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0)
                error("Erreur: impossible d'écrire sur le socket");
        }
    }

    list_destroy(server_messages->messages);
    free(server_messages);
    free(threads_arg);
    free(threads);
    // Fermeture du socket client
    close(sockfd);
    return 0;
}
