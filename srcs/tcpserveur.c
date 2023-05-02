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
#include <fcntl.h>  // for fcntl()
#include <errno.h>  // for errno

// Global variables
pthread_t * threads;
pthread_mutex_t mutex;
arg_t ** threads_arg;
int nb = 10000;

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
    arg->in = strdup("\0");
    arg->out = strdup("\0");
    arg->ended = 0;
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
    arg->out = strdup(out);
}


void * new_client_routine(void * arg) {
    arg_t * arglist = (arg_t * ) arg;
    pthread_mutex_t mutex = arglist->mutex;
    int n;
    char * reponse = "\033[1;31mWelcome !\033[0m\nYou're currently connected to our chatbot ! Type '\033[1;31mhelp\033[0m' for more infos !\n";
    n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
    sleep(0.5);
    
    int connected = 1;
    char buffer[256];
    int is_connected_to_technician = 0;
    int is_connected_to_expert = 0;
    int tech_id;
    int tech_thread_id =0;
    int exp_thread_id;
    int exp_id;

    // set the socket to non-blocking mode
    int flags = fcntl(arglist->sock_id, F_GETFL, 0);
    fcntl(arglist->sock_id, F_SETFL, flags | O_NONBLOCK);

    while(connected) {
        bzero(buffer,256);
        n = read(arglist->sock_id,buffer,255);
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
        }
        else {
            // data received
            
            char *last_newline = strrchr(buffer, '\n');
            if (last_newline != NULL) {
                *last_newline = '\0';
            } // On enlève le dernier '\n'
            printf("Message reçu du client : %s\n",buffer);
            //write(arglist->sock_id,"Message sent",13);

            if (!(strcmp(buffer,("exit")))) {
                n = write(arglist->sock_id,"end",4);
                if (n < 0) error("ERROR writing to socket");
                close(arglist->sock_id);
                arglist->ended = 1;
                connected = 0;
            }
            else {
                char * reponse = handle_command(buffer);
                // printf("Réponse: %s\n", reponse);
                if (strcmp(reponse,"unknown") == 0) {
                    n = write(arglist->sock_id,"Unknown keyword - Forwarding you to a technician.\n",51);
                        if (n < 0) error("ERROR writing to socket");
                    pthread_mutex_lock(&mutex);    
                    element_t * temp = list_pop(arglist->info->techniciens);
                    pthread_mutex_unlock(&mutex);
                    if (temp == NULL) {
                        n = write(arglist->sock_id,"No Technician available - Try again later.\n",44);
                    
                        if (n < 0) error("ERROR writing to socket");
                        n = write(arglist->sock_id,"end",4);
            
                        if (n < 0) error("ERROR writing to socket");
                        close(arglist->sock_id);
                        arglist->ended = 1;
                        connected = 0;
                    } 
                    else {
                        is_connected_to_technician = 1;
                        connected = 0;
                        tech_id = atoi(temp->val);
                        int i = 0;
                        int done = 0;
                        pthread_mutex_lock(&mutex);
                        while (i < nb && threads_arg[i] != NULL && done == 0) {
                            if((threads_arg[i]->sock_id) == tech_id && threads_arg[i]->ended != 1) {
                                tech_thread_id = i;
                                done =1;
                            }
                            i++;
                        }
                        pthread_mutex_unlock(&mutex);
                        printf("Client forwarded\n");
                        n = write(arglist->sock_id,"You've been forwarded to a technician - Please ask your question again\n",72);
                        if (n < 0) error("ERROR writing to socket");
                         pthread_mutex_lock(&mutex);
                        arg_new_in(threads_arg[tech_thread_id],"A new user needs your help !");
                        pthread_mutex_unlock(&mutex);
                        }   
                }
                else {
                    n = write(arglist->sock_id,reponse,strlen(reponse));
                        if (n < 0) error("ERROR writing to socket");
                    }
                }
            }

        
    }
    char * client_buffer = strdup("\0");
    while(is_connected_to_technician) {
    
        // On regarde si le client a envoyé un message au serveur
       
        bzero(buffer,256);
        n = read(arglist->sock_id,buffer,255);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // no data available on the socket, continue loop
                //do nothing
            }
            else {
                error("ERROR reading from socket");
            }
        }
        else if (n == 0) {
            // connection closed
            connected = 0;
        }
        else {
            // data received
            
            char *last_newline = strrchr(buffer, '\n');
            if (last_newline != NULL) {
                *last_newline = '\0';
            } // On enlève le dernier '\n'
            printf("Message reçu du client : %s\n",buffer);

            if (!(strcmp(buffer,("exit")))) {
                pthread_mutex_lock(&mutex);
                arg_new_in(threads_arg[tech_thread_id],"Client exited the room. Sending you back to available queue.\n");
                pthread_mutex_unlock(&mutex);

                n = write(arglist->sock_id,"end",4);
                if (n < 0) error("ERROR writing to socket");
                close(arglist->sock_id);
                arglist->ended = 1;
                is_connected_to_technician = 0;
                char sockid[100];
                sprintf(sockid, "%d", tech_id);  
                pthread_mutex_lock(&mutex);
                list_append(arglist->info->techniciens, sockid,sockid);
                pthread_mutex_unlock(&mutex);
            }
            else { // Il a envoyé un message, on le transmet à notre technicien via son arg->in . 
                
                pthread_mutex_lock(&mutex);
                arg_new_in(threads_arg[tech_thread_id],buffer);
                pthread_mutex_unlock(&mutex);
            }
            
        
        }
        // On regarde si le technicien ne nous a pas envoyé de message. 
            pthread_mutex_lock(&mutex);
            free(client_buffer);
            client_buffer = strdup(threads_arg[tech_thread_id]->out);
            arg_new_out(threads_arg[tech_thread_id],"\0");
            pthread_mutex_unlock(&mutex);

            if (!(strcmp(client_buffer,"\0") == 0)) {
                //On regarde si ce message est un forward / exit
                
                if (strcmp(client_buffer,"forward")==0) {
                    n = write(arglist->sock_id,"Technician is unable to solve the issue - Forwarding you to an Expert.\n",72);
                        if (n < 0) error("ERROR writing to socket");
                    pthread_mutex_lock(&mutex);
                    element_t * temp = list_pop(arglist->info->experts);
                    char sockid[100];
                    sprintf(sockid, "%d", tech_id);  
                    list_append(arglist->info->techniciens, sockid,sockid);
                    arg_new_in(threads_arg[tech_thread_id],"Client forwarded. Sending you back to available queue.\n");
                    pthread_mutex_unlock(&mutex);

                    if (temp == NULL) {
                        n = write(arglist->sock_id,"No Expert available - Try again later.\n",40);
                        if (n < 0) error("ERROR writing to socket");
                        n = write(arglist->sock_id,"end",4);
                        if (n < 0) error("ERROR writing to socket");
                        close(arglist->sock_id);
                        arglist->ended = 1;
                        connected = 0;
                    } 
                    else {
                        is_connected_to_expert = 1;
                        is_connected_to_technician = 0;
                        exp_id = atoi(temp->val);
                        int i = 0;
                        int done = 0;
                        pthread_mutex_lock(&mutex);
                        while (i < nb && threads_arg[i] != NULL && done == 0) {
                            if((threads_arg[i]->sock_id) == exp_id && threads_arg[i]->ended != 1) {
                                exp_thread_id = i;
                                done =1;
                            }
                            i++;
                        }
                        pthread_mutex_unlock(&mutex);
                        printf("Client forwarded\n");
                        n = write(arglist->sock_id,"You've been forwarded to an Expert - Please ask your question again\n",69);
                        if (n < 0) error("ERROR writing to socket");
                         pthread_mutex_lock(&mutex);
                        arg_new_in(threads_arg[exp_thread_id],"A new user needs your help !");
                        pthread_mutex_unlock(&mutex);
                        }   
                }
                else if (strcmp(client_buffer,"exit")==0) {
                    n = write(arglist->sock_id,"Technicien unexpectedly left the chat room. Trying to find a new one.\n",71);
                        if (n < 0) error("ERROR writing to socket");
                    pthread_mutex_lock(&mutex);    
                    element_t * temp = list_pop(arglist->info->techniciens);
                    arg_new_out(threads_arg[tech_thread_id],"\0");
                    pthread_mutex_unlock(&mutex);
                    free(client_buffer);
                    client_buffer = strdup("\0");

                    if (temp == NULL) {
                        n = write(arglist->sock_id,"No Technician available - Try again later.\n",44);
                        printf("Wrote no tech\n");
                        if (n < 0) error("ERROR writing to socket");
                        n = write(arglist->sock_id,"end",4);
                        printf("Wrote end\n");
                        if (n < 0) error("ERROR writing to socket");
                        close(arglist->sock_id);
                        arglist->ended = 1;
                        is_connected_to_technician = 0;
                    } 
                    else {
                        is_connected_to_technician = 1;
                        connected = 0;
                        tech_id = atoi(temp->val);
                        int i = 0;
                        int done = 0;
                        pthread_mutex_lock(&mutex);
                        while (i < nb && threads_arg[i] != NULL && done == 0) {
                            if((threads_arg[i]->sock_id) == tech_id&& threads_arg[i]->ended != 1) {
                                tech_thread_id = i;
                                done =1;
                            }
                            i++;
                        }
                        pthread_mutex_unlock(&mutex);
                        printf("Client forwarded\n");
                        n = write(arglist->sock_id,"You've been forwarded to a technician - Please ask your question again\n",72);
                        if (n < 0) error("ERROR writing to socket");
                         pthread_mutex_lock(&mutex);
                        arg_new_in(threads_arg[tech_thread_id],"A new user needs your help !");
                        pthread_mutex_unlock(&mutex);
                        }
                }
                else { // On passe le message à notre client
                    n = write(arglist->sock_id,client_buffer,strlen(client_buffer));
                        if (n < 0) error("ERROR writing to socket");
                }
            }
    }
    while(is_connected_to_expert) {
       
        // On regarde si le client a envoyé un message au serveur
        
        bzero(buffer,256);
        n = read(arglist->sock_id,buffer,255);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // no data available on the socket, continue loop
                //do nothing
            }
            else {
                error("ERROR reading from socket");
            }
        }
        else if (n == 0) {
            // connection closed
            connected = 0;
        }
        else {
            // data received
            
            char *last_newline = strrchr(buffer, '\n');
            if (last_newline != NULL) {
                *last_newline = '\0';
            } // On enlève le dernier '\n'
            printf("Message reçu du client : %s\n",buffer);

            if (!(strcmp(buffer,("exit")))) {
                pthread_mutex_lock(&mutex);
                arg_new_in(threads_arg[exp_thread_id],"Client exited the room. Sending you back to available queue.\n");
                pthread_mutex_unlock(&mutex);

                n = write(arglist->sock_id,"end",4);
                if (n < 0) error("ERROR writing to socket");
                close(arglist->sock_id);
                arglist->ended = 1;
                is_connected_to_expert = 0;
                char sockid[100];
                sprintf(sockid, "%d", exp_id);  
                pthread_mutex_lock(&mutex);
                list_append(arglist->info->experts, sockid,sockid);
                pthread_mutex_unlock(&mutex);
            }
            else { // Il a envoyé un message, on le transmet à notre expert via son arg->in . 
                
                pthread_mutex_lock(&mutex);
                arg_new_in(threads_arg[exp_thread_id],buffer);
                pthread_mutex_unlock(&mutex);
            }
            
        
        }
        // On regarde si l'expert' ne nous a pas envoyé de message. 
            pthread_mutex_lock(&mutex);
            free(client_buffer);
            client_buffer = strdup(threads_arg[exp_thread_id]->out);
            arg_new_out(threads_arg[exp_thread_id],"\0");
            pthread_mutex_unlock(&mutex);

            if (!(strcmp(client_buffer,"\0") == 0)) {
                //On regarde si ce message est un forward / exit
                
                if (strcmp(client_buffer,"forward")==0) {
                    n = write(arglist->sock_id,"Expert is unable to solve the issue - Terminating connection.\n",63);
                        if (n < 0) error("ERROR writing to socket");
                    pthread_mutex_lock(&mutex);
                    arg_new_in(threads_arg[exp_thread_id],"Client evacuated sucessfully. Sending you back to available queue.\n");
                    pthread_mutex_unlock(&mutex);

                    n = write(arglist->sock_id,"end",4);
                    if (n < 0) error("ERROR writing to socket");
                    close(arglist->sock_id);
                    arglist->ended = 1;
                    is_connected_to_expert = 0;
                    char sockid[100];
                    sprintf(sockid, "%d", exp_id);  
                    pthread_mutex_lock(&mutex);
                    list_append(arglist->info->experts, sockid,sockid);
                    pthread_mutex_unlock(&mutex); 
                }
                else if (strcmp(client_buffer,"exit")==0) {
                    n = write(arglist->sock_id,"Expert unexpectedly left the chat room. Trying to find a new one.\n",67);
                        if (n < 0) error("ERROR writing to socket");
                    
                    pthread_mutex_lock(&mutex);
                    element_t * temp = list_pop(arglist->info->experts);
                    arg_new_out(threads_arg[exp_thread_id],"\0");
                    pthread_mutex_unlock(&mutex);
                    free(client_buffer);
                    client_buffer = strdup("\0");

                    if (temp == NULL) {
                        n = write(arglist->sock_id,"No Expert available - Try again later.\n",40);
                      
                        if (n < 0) error("ERROR writing to socket");
                        n = write(arglist->sock_id,"end",4);
                      
                        if (n < 0) error("ERROR writing to socket");
                        close(arglist->sock_id);
                        arglist->ended = 1;
                        is_connected_to_expert = 0;
                    } 
                    else {
                        is_connected_to_expert = 1;
                        is_connected_to_technician = 0;
                        exp_id = atoi(temp->val);
                        int i = 0;
                        int done = 0;
                        pthread_mutex_lock(&mutex);
                        while (i < nb && threads_arg[i] != NULL && done == 0) {
                            if((threads_arg[i]->sock_id) == exp_id && threads_arg[i]->ended != 1) {
                                exp_thread_id = i;
                                done =1;
                            }
                            i++;
                        }
                        pthread_mutex_unlock(&mutex);
                        printf("Client forwarded\n");
                        n = write(arglist->sock_id,"You've been forwarded to an Expert - Please ask your question again\n",69);
                        if (n < 0) error("ERROR writing to socket");
                         pthread_mutex_lock(&mutex);
                        arg_new_in(threads_arg[exp_thread_id],"A new user needs your help !");
                        pthread_mutex_unlock(&mutex);
                        } 
                }
                else { // On passe le message à notre client
                    n = write(arglist->sock_id,client_buffer,strlen(client_buffer));
                        if (n < 0) error("ERROR writing to socket");
                }
            }
    }
    free(client_buffer);
    printf("Client %d exited\n", arglist->sock_id);
    pthread_exit(NULL);
}


void * new_staff_routine(void * arg) {
    arg_t * arglist = (arg_t * ) arg;
    int n;
    pthread_mutex_t mutex = arglist->mutex;
    char * reponse = "Please type in a password :";
    n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");

    char sockid[100];

    sprintf(sockid, "%d", arglist->sock_id);    

    sleep(0.5);
    int connection_await= 1;
    char buffer[256];
    int status = -1;

    // set the socket to non-blocking mode
    int flags = fcntl(arglist->sock_id, F_GETFL, 0);
    fcntl(arglist->sock_id, F_SETFL, flags | O_NONBLOCK);

    while(connection_await) {
        bzero(buffer,256);
        n = read(arglist->sock_id,buffer,255);
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
            connection_await = 0;
        }
        else {
            // data received
            
            char *last_newline = strrchr(buffer, '\n');
            if (last_newline != NULL) {
                *last_newline = '\0';
            } // On enlève le dernier '\n'
            printf("Message reçu du staff : %s\n",buffer);

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
        arglist->ended = 1;
        pthread_exit(NULL);
    }
    else if (status == 1) {
        printf("Technicien logged in !\n");
        reponse = "You logged in as a Technician !\n Wait patientally, a new user requiring your help will arrive soon!";
        n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
        pthread_mutex_lock(&mutex);
        list_append(arglist->info->techniciens, sockid,sockid);
        pthread_mutex_unlock(&mutex);

    }
    else {
        printf("Expert logged in !\n");
        reponse = "You logged in as an Expert !\n Wait patientally, a new user requiring your help will arrive soon!";
        n = write(arglist->sock_id,reponse,strlen(reponse));
                    if (n < 0) error("ERROR writing to socket");
        pthread_mutex_lock(&mutex);
        list_append(arglist->info->experts, sockid,sockid);
        pthread_mutex_unlock(&mutex);
    }
    sleep(0.3);
    int connected = 1;
    char * staff_buffer = strdup("\0");
    while(connected) {
        // Staff code to implement
        bzero(buffer,256);
        n = read(arglist->sock_id,buffer,255);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // no data available on the socket, continue loop
                // do nothing;
            }
            else {
                error("ERROR reading from socket");
            }
        }
        else if (n == 0) {
            // connection closed
            connected = 0;
        }
        else {
            // data received
            
            char *last_newline = strrchr(buffer, '\n');
            if (last_newline != NULL) {
                *last_newline = '\0';
            } // On enlève le dernier '\n'
            printf("Message reçu du staff : %s\n",buffer);
            if (!(strcmp(buffer,("exit")))) {
                pthread_mutex_lock(&mutex);
                arg_new_out(arglist,"exit");
                pthread_mutex_unlock(&mutex);
                sleep(1);
                n = write(arglist->sock_id,"end",4);
                if (n < 0) error("ERROR writing to socket");
                close(arglist->sock_id);
                arglist->ended = 1;
                connected = 0;
                if (status == 1) {
                    list_remove(arglist->info->techniciens,sockid);
                }
                else {
                    list_remove(arglist->info->experts,sockid);
                }
                
            }
            else {
                pthread_mutex_lock(&mutex);
                arg_new_out(arglist,buffer);
                pthread_mutex_unlock(&mutex);
            }

            
        }
        // Read the in parameter, to check if the client just sent the staff a message or not
            pthread_mutex_lock(&mutex);
            //printf("%s\n",arglist->in);
            free(staff_buffer);
            staff_buffer = strdup(arglist->in);
            arg_new_in(arglist,"\0");
            pthread_mutex_unlock(&mutex);

            if (!(strcmp(staff_buffer,"\0") == 0)) {
                n = write(arglist->sock_id,staff_buffer,strlen(staff_buffer));
                        if (n < 0) error("ERROR writing to socket");
            
                
            }
    }
        free(staff_buffer);
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
        //printf("select() timeout\n");
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
