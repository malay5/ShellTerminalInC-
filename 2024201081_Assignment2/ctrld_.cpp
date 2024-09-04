#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <signal.h>

struct termios orig_termios;

void reset_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void set_conio_mode() {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(reset_terminal_mode);
    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void sighup_handler(int signum) {
    // Handle SIGHUP
    std::cout<<"J";
    printf("Received SIGHUP\n");
}

int main() {
    // set_conio_mode();
    
    signal(SIGHUP, sighup_handler);

    char ch;
    while (1) {
        ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);
        if (bytesRead == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        } else if (bytesRead == 0) {
            // End of file or no more input
            break;
        }
        // Process the character
        printf("Read: %c\n", ch);
    }

    return 0;
}
