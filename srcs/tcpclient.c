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
    char buffer[1024];
    int n;
    list_t * li;

    // set the socket to non-blocking mode
    int flags = fcntl(arglist->sockid, F_GETFL, 0);
    fcntl(arglist->sockid, F_SETFL, flags | O_NONBLOCK);

    while (connected) {
        pthread_mutex_lock(&mutex);
        connected = (!arglist->ended);
        pthread_mutex_unlock(&mutex);
        bzero(buffer,1024);
        n = read(arglist->sockid,buffer,1023);
        
        if (connected) {

        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // no data available on the socket, continue loop
                // do nothing
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
            // printf("%d\n",n);
            // printf("%d %s\n",(int) strlen(buffer),buffer);
            // data received
            int i;
            int count = 0;
            char temp_msg[1024];
            int null_count =0;
            bzero(temp_msg,1024);
            for (i = 0; i < 1022; i++) {
                if (buffer[i] == '$') {
                    temp_msg[count] = '\0';
                    char *last_newline = strrchr(temp_msg, '\n');
                    if (last_newline != NULL) {
                    *last_newline = '\0';
                    } // On enlève le dernier '\n'
                    //On ajoute le message à la liste des messages
                    // printf("%s\n",temp_msg);
                    char id[20];
                    sprintf(id, "%d", arglist->msg_nb);  
                    arglist->msg_nb +=1;
                    list_append(arglist->msg->messages,id,temp_msg);
                    // On reset le message temporaire
                    count = 0;
                    bzero(temp_msg,1024);
                    null_count = 0;
                }
                else if (buffer[i] == '\0') {
                    // On skip le caractère
                    null_count++;
                }
                else {
                    temp_msg[count] = buffer[i];
                    count++;
                    null_count = 0;
                }
                if (null_count >2) {
                    break;
                }
            }
        

        }

        // print all messages in the message list
        li = arglist->msg->messages;
        while (li->tail != NULL) {
            if(strcmp(li->head->val,"end") !=0) {
                // Si le message n'est pas un message de fin, on le print
                printf("Message received : %s \n",li->head->val);
               // printf("%d %d \n",strlen(li->head->val),(strcmp(li->head->val,"end")));
            }
            else {
                // Sinon, on stop la connection
                connected = 0;
                pthread_mutex_lock(&mutex);
                arglist->ended = 1;
                pthread_mutex_unlock(&mutex);
            }
            li = li->tail;
        }
        empty_list(arglist->msg->messages);
        }
    }
    printf("Listening thread ended\n");
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

    char buffer[1024];

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
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    fd_set set;
    struct timeval timeout;

    while(connected) {
        // Use select to check for input from stdin
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms


        pthread_mutex_lock(&mutex);
        connected = (!threads_arg[0]->ended);
        pthread_mutex_unlock(&mutex);
        if (connected) {
            bzero(buffer,1024);
            if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0) {
                fgets(buffer, 1023, stdin);
                n = strlen(buffer);
                char *last_newline = strrchr(buffer, '\n');
                if (last_newline != NULL) {
                    *last_newline = '\0';
                } // On enlève le dernier '\n'
            }
            if (buffer[0] == '\0') {
                // do nothing
            } else {
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)
                    error("Erreur: impossible d'écrire sur le socket");

                //printf("Message sent : %s\n",buffer);
                char *last_newline = strrchr(buffer, '\n');
                if (last_newline != NULL) {
                    *last_newline = '\0';
                } // On enlève le dernier '\n'

                if(strcmp(buffer,"leave") ==0) {
                    pthread_mutex_lock(&mutex);
                    (threads_arg[0]->ended = 1);
                    pthread_mutex_unlock(&mutex);
                    sleep(1);
                    connected = 0;
                }
               
            }
            // fgets(buffer,255,stdin);
            
        }
        else {
            sleep(1);
        }
    }
    list_destroy(server_messages->messages);
    free(server_messages);
    free(threads_arg);
    free(threads);
    // Fermeture du socket client
    printf("Main Ended\n");
    sleep(1);
    close(sockfd);
    return 0;
}
