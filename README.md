# Projet RSA 

## Application tchat d’assistance multi-niveaux

## BRUNET Jules - DAMIEN Malo

### Présentation

Voici notre application de tchat. Elle fonctionne comme suit : 
On lance un serveur qui s'occupe de gérer les tchats. Il est nécessaire de lui fournir 2 ports de connexions : 1 pour les clients du tchat, et 1 pour les staffs (techniciens & experts) du tchat.
Lien du git : https://gitlab.telecomnancy.univ-lorraine.fr/Malo.Damien/projetrsa

#### Côté client

Lorsqu'un client se connecte, il fait face à un chatbot très basique, qui, si on lui donne un mot clé qu'il connait, affiche une réponse (liste disponible avec le mot clé 'help').
S'il ne reconnait pas le mot clé, alors il transmet le client vers un technicien, s'il y en a un de disponible. Le client discute alors avec ce dernier, et si le technicien estime qu'il n'est pas en mesure d'aider le client, il peut le forwarder vers un expert, s'il y en a un, encore une fois, de disponible.
Une fois que l'expert estime avoir fini son travail / ne peut toujours pas résoudre le problème, alors il met fin à la communication.
Notons que si à un moment donné, un technicien ou un expert se déconnecte alors qu"il est en discussion avec un client, alors le serveur va tenté d'envoyer le client vers un autre staff du même niveau.
Si, lors d'un forward, aucun membre du niveau requis n'est disponible, le client est simplement déconnecté du serveur.
Enfin, un client peut quitter le chat à tout moment en écrivant le mot clé 'exit'.

#### Côté staff

Un staff se connecte via le second port du serveur, où on lui demande un mot de passe. S'il entre 'tech', alors il sera login en tant que Technicien, et s'il entre 'expert', il sera login en tant qu'Expert. Une fois connecté, il est placé en attente, et le serveur le préviendra dès qu'un client lui sera assigné. Une fois un client trouvé, il peut discuter avec lui pour tenter de résoudre son problème. Les techniciens peuvent forward un client vers un expert en utilisant le mot clé 'forward'. De même, les experts peuvent mettre fin à une consversation avec le même mot clé.
Une fois la conversation terminée avec un client, le staff est replacé en file d'attente. 
Tout comme les clients, les staffs peuvent quitter l'application à tout moment en utilisant le mot clé 'exit'.

### Lancer l'application

```bash
make all
./tcpserver 5454 5456 # Lance le serveur 
./tcpclient localhost 5454  # Lance un client lambda
./tcpclient localhost 5456  # Lance un client 'staff'
```