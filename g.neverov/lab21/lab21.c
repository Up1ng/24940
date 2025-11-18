#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

int sigint_count = 0;

void handle_sigint(int sig) {
    sigint_count++;
    write(STDOUT_FILENO, "\a", 1);
    printf("Beep! (SIGINT received: %d times)\n", sigint_count);
}

void handle_sigquit(int sig) {
    printf("\nSIGQUIT received. Program completed. Total SIGINT signals: %d\n", sigint_count);
    exit(0);
}

int main() {
    
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("Failed to set SIGINT handler");
        return 1;
    }
    
    if (signal(SIGQUIT, handle_sigquit) == SIG_ERR) {
        perror("Failed to set SIGQUIT handler");
        return 1;
    }

    printf("Program started on Solaris:\n");
    printf("- Press Ctrl+C to make a beep sound\n");
    printf("- Press Ctrl+\\\\ to exit program\n");
    printf("Waiting for signals...\n");
    
    while (1) {
        pause();
    }

    return 0;
}