#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Fonction pour gérer les erreurs
void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    // Vérification des arguments de ligne de commande
    if (argc < 2) {
        fprintf(stderr,"Usage: %s port\n", argv[0]);
        exit(1);
    }

    // Création du socket serveur
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
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

    // Attente de connexion des clients
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
                (struct sockaddr *) &cli_addr,
                &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    // Lecture et écriture de messages avec le client
    while(1) {
        bzero(buffer,256);
        n = read(newsockfd,buffer,255);
        if (n < 0) error("ERROR reading from socket");
        printf("Message reçu du client: %s\n",buffer);
        n = write(newsockfd,"Message reçu",13);
        if (n < 0) error("ERROR writing to socket");
    }

    // Fermeture du socket serveur
    close(sockfd);
    return 0;
}