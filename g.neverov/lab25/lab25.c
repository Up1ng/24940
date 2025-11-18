#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    int pipefd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(pipefd[1]);

        ssize_t nbytes = read(pipefd[0], buffer, BUFFER_SIZE);
        if (nbytes == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        for (ssize_t i = 0; i < nbytes; i++) {
            buffer[i] = toupper(buffer[i]);
        }

        write(STDOUT_FILENO, buffer, nbytes);
        write(STDOUT_FILENO, "\n", 1);

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    } else {
        close(pipefd[0]);

        if (argc > 1) {
            write(pipefd[1], argv[1], strlen(argv[1]));
        } else {
            printf("Введите текст: ");
            fflush(stdout);
            
            char input[BUFFER_SIZE];
            if (fgets(input, BUFFER_SIZE, stdin) != NULL) {
                input[strcspn(input, "\n")] = 0;
                write(pipefd[1], input, strlen(input));
            } else {
                write(pipefd[1], "default text", 12);
            }
        }

        close(pipefd[1]);
        wait(NULL);
    }

    return 0;
}