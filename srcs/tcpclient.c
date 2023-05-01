#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

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

    // Envoi et réception de messages avec le serveur
    int connected =1;
    while(connected) {
        printf("Entrez votre message: ");
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0)
             error("Erreur: impossible d'écrire sur le socket");
        bzero(buffer,256);
        n = read(sockfd,buffer,255);
        if (n < 0)
             error("Erreur: impossible de lire depuis le socket");
        size_t newline_pos = strcspn(buffer, "\n");  // find position of newline character
        buffer[newline_pos] = '\0';  // overwrite the newline character with null character
        if (!(strcmp(buffer,("end")))) {
                    connected = 0;
                }
        else {printf("Message reçu du serveur: %s\n",buffer);}
    }

    // Fermeture du socket client
    close(sockfd);
    return 0;
}
