#include <stdio.h>
#include <string.h>

char* handle_command(char* buffer) {
    if (strcmp(buffer, "help") == 0) {
        return "bienvenue dans le help";
    }
    else if (strcmp(buffer, "hello") == 0) {
        return "Hello, World!";
    }
    else {
        return "Commande inconnue";
    }
}
