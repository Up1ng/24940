#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/time.h>
#include <ctype.h>
#include <time.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 10
#define CHUNK_SIZE 1024
#define COLLECTION_TIME_SEC 20

typedef struct {
    int fd;
    char *buffer;
    size_t size;
} ClientData;

void append_data(ClientData *client, char *new_data, int len) {
    client->buffer = realloc(client->buffer, client->size + len + 1);
    if (!client->buffer) {
        perror("realloc");
        exit(1);
    }
    memcpy(client->buffer + client->size, new_data, len);
    client->size += len;
    client->buffer[client->size] = '\0';
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    
    ClientData clients[MAX_CLIENTS] = {0}; 
    
    fd_set master, read_fds;
    int max_fd;
    char temp_buffer[CHUNK_SIZE];
    
    struct timeval start_time, current_time, timeout;
    double elapsed_time;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); exit(1); }

    unlink(SOCKET_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) { perror("bind"); exit(1); }
    if (listen(server_fd, MAX_CLIENTS) == -1) { perror("listen"); exit(1); }

    FD_ZERO(&master);
    FD_SET(server_fd, &master);
    max_fd = server_fd;

    printf("Server started. Collecting messages for %d seconds...\n", COLLECTION_TIME_SEC);

    gettimeofday(&start_time, NULL);

    while (1) {
        gettimeofday(&current_time, NULL);
        elapsed_time = (current_time.tv_sec - start_time.tv_sec) + 
                       (current_time.tv_usec - start_time.tv_usec) / 1000000.0;

        if (elapsed_time >= COLLECTION_TIME_SEC) {
            break;
        }

        double remaining = COLLECTION_TIME_SEC - elapsed_time;
        timeout.tv_sec = (long)remaining;
        timeout.tv_usec = (long)((remaining - timeout.tv_sec) * 1000000);

        read_fds = master;
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity == -1) {
            perror("select");
            break;
        }
        
        if (activity == 0) {
            continue; 
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            client_fd = accept(server_fd, NULL, NULL);
            if (client_fd != -1) {
                int placed = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == 0) {
                        clients[i].fd = client_fd;
                        clients[i].buffer = NULL;
                        clients[i].size = 0;
                        FD_SET(client_fd, &master);
                        if (client_fd > max_fd) max_fd = client_fd;
                        placed = 1;
                        break;
                    }
                }
                if (!placed) close(client_fd);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i].fd;
            if (fd > 0 && FD_ISSET(fd, &read_fds)) {
                ssize_t r = read(fd, temp_buffer, CHUNK_SIZE);
                
                if (r <= 0) {
                    close(fd);
                    FD_CLR(fd, &master);
                    clients[i].fd = -1;
                } else {
                    for (int k = 0; k < r; k++) {
                        temp_buffer[k] = toupper(temp_buffer[k]);
                    }
                    append_data(&clients[i], temp_buffer, r);
                }
            }
        }
    }

    printf("\n--- Time is up! Processing results ---\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].buffer != NULL && clients[i].size > 0) {
            printf("Client #%d sent:\n%s\n----------------\n", i + 1, clients[i].buffer);
            free(clients[i].buffer);
        }
        if (clients[i].fd > 0) close(clients[i].fd);
    }

    gettimeofday(&current_time, NULL);
    double total_time = (current_time.tv_sec - start_time.tv_sec) + 
                        (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    printf("Total program execution time: %.4f seconds\n", total_time);
    unlink(SOCKET_PATH);

    return 0;
}