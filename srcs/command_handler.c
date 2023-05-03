#include <stdio.h>
#include <string.h>

char* handle_command(char* buffer) {
    if (strcmp(buffer, "help") == 0) {
        return "Help Menu - Here is the list of keyword to use :\n help,hello,chat,forward,tech,expert,leaving\n";
    }
    else if (strcmp(buffer, "hello") == 0) {
        return "Hello, World!";
    }
    else if (strcmp(buffer, "chat") == 0) {
        return "This chat bot is very simple ! It's only purpose is to demonstrate automatic response to a keyword given as an input by a user.\n";
    }
    else if (strcmp(buffer, "forward") == 0) {
        return "If your input doesn't match any keyword, you will be forwarded to a technician. If none is available, your connection will unfortunately end.\n";
    }
    else if (strcmp(buffer, "tech") == 0) {
        return "Technicians are real humans, connected to the server, who are can help you solve your problem. If they can't handle it, they can always forward you to an expert!\n";
    }
     else if (strcmp(buffer, "expert") == 0) {
        return "Experts are your last resort. If they can't solve your issue, nobody can. That's why they can terminate the discussion at any time if they desire to do so !\n";
    }
    else if (strcmp(buffer, "leaving") == 0) {
        return "You can leave at any time by typing in the keyword '\033[1;31mexit\033[0m'.\n";
    }
    else {
        return "unknown";
    }
}
