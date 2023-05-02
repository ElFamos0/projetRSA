#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/select.h>
#include "../includes/command_handler.h"
#include "../includes/tcpserver.h"

// Global variables
pthread_t * threads;
pthread_mutex_t mutex;
arg_t ** threads_arg;

// Fonction pour gérer les erreurs
void error(const char *msg) {
    perror(msg);
    exit(1);
}

arg_t * arg_create(int id, pthread_mutex_t mutex,info_t * info){
    arg_t * arg = (arg_t *)calloc(1,sizeof(arg_t));
    arg->sock_id = id;
    arg->mutex = mutex;
    arg->info = info;
    return arg;
}

void arg_destroy(arg_t * arg) {

    if (!(arg->in == NULL)) {
        free(arg->in);
    }
    if (!(arg->out == NULL)) {
        free(arg->out);
    }
    free(arg);
}

void arg_new_in(arg_t* arg, char * in) {
     if (!(arg->in == NULL)) {
        free(arg->in);
    }
    arg->in = strdup(in);

}


void arg_new_out(arg_t* arg, char * out){
    if (!(arg->out == NULL)) {
        free(arg->out);
    }
    arg->in = strdup(out);
}


void * new_client_routine(void * arg) {
    arg_t * arglist = (arg_t * ) arg;
    pthread_mutex_t mutex = arglist->mutex;

    //printf("Hello world! i = %d\n",*id);
    pthread_mutex_lock(&mutex);
    // do something
    pthread_mutex_unlock(&mutex);
    int n;
    char * reponse = "\033[1;31mWelcome !\033[0m\nYou're currently connected to our chatbot ! Type '\033[1;31mhelp\033[0m' for more infos !\n";
    n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
    sleep(0.5);
    
    int connected = 1;
    char buffer[256];
    while(connected) {
        bzero(buffer,256);
        n = read(arglist->sock_id,buffer,255);
        if (n < 0) error("ERROR reading from socket");
        printf("Message reçu du client: %s\n",buffer);

        char *last_newline = strrchr(buffer, '\n');
            if (last_newline != NULL) {
                *last_newline = '\0';
            } // On enlève le dernier '\n'

        if (!(strcmp(buffer,("exit")))) {
            n = write(arglist->sock_id,"end",4);
            if (n < 0) error("ERROR writing to socket");
            close(arglist->sock_id);
            connected = 0;
        }
        else {
            char * reponse = handle_command(buffer);
            // printf("Réponse: %s\n", reponse);
            if (strcmp(reponse,"unknown") == 0) {
                n = write(arglist->sock_id,"Unknown keyword - Forwarding you to a technician.\n",51);
                    if (n < 0) error("ERROR writing to socket");
                // TO DO NEXT
            }
            else {
                n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
                }
            }
    }
    printf("Client %d exited\n", arglist->sock_id);
    pthread_exit(NULL);
}


void * new_staff_routine(void * arg) {
    arg_t * arglist = (arg_t * ) arg;
    int n;
    pthread_mutex_t mutex = arglist->mutex;
    char * reponse = "Veuillez entrer le mot de passe :";
    n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");

    char sockid[100];

    sprintf(sockid, "%d", arglist->sock_id);    

    sleep(0.5);
    int connection_await= 1;
    char buffer[256];
    int status = -1;
    while(connection_await) {
        bzero(buffer,256);
        n = read(arglist->sock_id,buffer,255);
        if (n < 0) error("ERROR reading from socket");
        
        char *last_newline = strrchr(buffer, '\n');
        if (last_newline != NULL) {
            *last_newline = '\0';
        } // On enlève le dernier '\n'

        if (!(strcmp(buffer,("tech")))) {
            status = 1;
            connection_await = 0;
        }
        else if (!(strcmp(buffer,("expert")))) {
            status = 2;
            connection_await = 0;
        }
        else {
            connection_await = 0;
            }
    }
    if (status == -1) {
        printf("Staff failed to connect\n");
        reponse = "Login failed";
        n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
        sleep(0.3);
        reponse = "end";
        n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
        close(arglist->sock_id);
        pthread_exit(NULL);
    }
    else if (status == 1) {
        printf("Technicien logged in !\n");
        reponse = "You logged in as a Technicien !";
        n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
        pthread_mutex_lock(&mutex);
        list_append(arglist->info->techniciens, sockid,sockid);
        pthread_mutex_unlock(&mutex);

    }
    else {
        printf("Expert logged in !\n");
        reponse = "You logged in as an Expert !";
        n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
        pthread_mutex_lock(&mutex);
        list_append(arglist->info->experts, sockid,sockid);
        pthread_mutex_unlock(&mutex);
    }
    sleep(0.3);
    int connected = 1;
    while(connected) {
        bzero(buffer,256);
            n = read(arglist->sock_id,buffer,255);
            if (n < 0) error("ERROR reading from socket");
            printf("Message reçu du staff: %s\n",buffer);

            char *last_newline = strrchr(buffer, '\n');
            if (last_newline != NULL) {
                *last_newline = '\0';
            } // On enlève le dernier '\n'
            if (!(strcmp(buffer,("exit")))) {
                n = write(arglist->sock_id,"end",4);
                if (n < 0) error("ERROR writing to socket");
                close(arglist->sock_id);
                connected = 0;
            }
            else {
                char * reponse = handle_command(buffer);
                printf("Réponse: %s\n", reponse);

                n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
                }
    }
        printf("Staff %d exited\n", arglist->sock_id);
        pthread_exit(NULL);
    }


int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno,sockfdserv;
    info_t * info = (info_t *) malloc(sizeof(info_t));
    info->techniciens = list_create();
    info->experts = list_create();
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int socket_count = 0; // a changer par une struct de liste
    int nb = 10000;
    pthread_mutex_init(&mutex, NULL);
    threads = malloc(nb * sizeof(pthread_t));
    threads_arg =  (arg_t **)calloc(nb,sizeof(arg_t *));

    

    // Vérification des arguments de ligne de commande
    if (argc < 3) {
        fprintf(stderr,"Usage: %s port1 port2 \n" , argv[0]);
        exit(1);
    }

    // Création du socket serveur
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    
    // Création du socket serveur bis
    sockfdserv = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfdserv < 0)
        error("ERROR opening socket");

    // Configuration des paramètres du socket serveur
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Liaison du socket serveur avec l'adresse et le port
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    
     // Configuration des paramètres du socket serveur bis
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[2]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

     // Liaison du socket serveur bis avec l'adresse et le port
    if (bind(sockfdserv, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // Attente de connexion des clients et au serv bis
    listen(sockfd,10);
    listen(sockfdserv,10);
    clilen = sizeof(cli_addr);
    int is_main = 1;
    int newsockfdserv = -1;
    fd_set master_set;
    FD_ZERO(&master_set);
    FD_SET(sockfd, &master_set);
    FD_SET(sockfdserv, &master_set);
    while (is_main) {
    // Make a copy of the master set
    fd_set read_set = master_set;
    
    // Wait for one of the sockets to become ready or the timeout to expire
    struct timeval timeout;
    timeout.tv_sec = 100;  // Timeout after 5 seconds
    timeout.tv_usec = 0;
    int num_ready = select(FD_SETSIZE, &read_set, NULL, NULL, &timeout);
    
    if (num_ready == -1) {
        perror("select");
        exit(EXIT_FAILURE);
    }
    else if (num_ready == 0) {
        // Timeout expired
        printf("select() timeout\n");
    }
    else {
        // At least one socket is ready for reading
        printf("select() %d\n",num_ready);
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_set)) {
                if (i == sockfd) {
                    // A new client is trying to connect to sockfd
                    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                    if (newsockfd < 0) {
                        error("ERROR on accept");
                    }
                    else {
                        //Création d'un nouveau thread pour le nouveau client
                        printf("New client connection, %d ! \n",newsockfd);
                        threads_arg[socket_count] = arg_create(newsockfd,mutex,info);
                        pthread_create(&threads[socket_count], NULL, new_client_routine, threads_arg[socket_count]);
                        socket_count +=1;
                    }
                }
                else if (i == sockfdserv) {
                    // A new request has been received on sockfdserv
                    newsockfdserv = accept(sockfdserv, (struct sockaddr *) &cli_addr, &clilen);
                    if (newsockfdserv < 0) {
                        error("ERROR on accept");
                    }
                    else {
                       //Création d'un nouveau thread pour le nouveau staff
                        printf("New staff connection, %d ! \n",newsockfdserv);
                        threads_arg[socket_count] = arg_create(newsockfdserv,mutex,info);
                        pthread_create(&threads[socket_count], NULL, new_staff_routine, threads_arg[socket_count]);
                        socket_count +=1;
                    }
                }
                else {
                    // A socket is ready for reading
                    // TODO: handle the incoming data on the socket
                    }
                }
            }
        }
    }
    list_destroy(info->experts);
    list_destroy(info->techniciens);
    free(info);
    free(threads_arg);
    free(threads);
    close(sockfd);
    return 0;
    }
