#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <poll.h>
#include <time.h>

#define SOCKET_PATH  "./socket"
#define BUF 1024
#define MAX 5

void get_current_time(char *time_buf, size_t buf_size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(time_buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un socket_addr;
    char buffer[BUF];
    ssize_t bytes;
    int count_ds = 1;

    struct pollfd fds[MAX + 1];
    char connection_time[MAX + 1][64];

    time_t start_time;
    time(&start_time);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sun_family = AF_UNIX;
    strncpy(socket_addr.sun_path, SOCKET_PATH, sizeof(socket_addr.sun_path)-1);

    unlink(SOCKET_PATH);

    bind(server_fd, (struct sockaddr*)&socket_addr, sizeof(socket_addr));
    listen(server_fd, 5);

    printf("Сервер запущен\n");

    memset(fds, 0, sizeof(fds));
    memset(connection_time, 0, sizeof(connection_time));

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1) {
        int ready = poll(fds, count_ds, 100);

        for (int i = 0; i < count_ds; i++) {
            if (fds[i].revents == 0) {
                continue;
            }

            if (fds[i].fd == server_fd) {
                if (fds[i].revents & POLLIN) {
                    client_fd = accept(server_fd, NULL, NULL);

                    if (count_ds < MAX + 1) {
                        fds[count_ds].fd = client_fd;
                        fds[count_ds].events = POLLIN;

                        get_current_time(connection_time[count_ds], sizeof(connection_time[count_ds]));

                        char current_time[64];
                        get_current_time(current_time, sizeof(current_time));

                        printf("[%s] Клиент подключен: id=%d, всего клиентов: %d\n", 
                               current_time, client_fd, count_ds);

                        count_ds++;
                    } else {
                        char current_time[64];
                        get_current_time(current_time, sizeof(current_time));
                        printf("[%s] Достигнут лимит подключений, отказ клиенту\n", current_time);
                        close(client_fd);
                    }
                }
            } 
            else if (fds[i].revents & POLLIN) {
                bytes = read(fds[i].fd, buffer, BUF - 1);

                if (bytes > 0) {
                    buffer[bytes] = '\0';

                    for (int j = 0; j < bytes; j++) {
                        buffer[j] = toupper(buffer[j]);
                    }

                    char current_time[64];
                    get_current_time(current_time, sizeof(current_time));

                    printf("[%s] Сообщение от клиента id=%d : %s\n", current_time, fds[i].fd, buffer);
                    fflush(stdout);
                } 
                else if (bytes == 0) {
                    char current_time[64];
                    get_current_time(current_time, sizeof(current_time));

                    time_t connect_time_t, disconnect_time_t;
                    time(&disconnect_time_t);

                    struct tm tm_connect;
                    memset(&tm_connect, 0, sizeof(tm_connect));
                    sscanf(connection_time[i], "%d-%d-%d %d:%d:%d", 
                           &tm_connect.tm_year, &tm_connect.tm_mon, &tm_connect.tm_mday,
                           &tm_connect.tm_hour, &tm_connect.tm_min, &tm_connect.tm_sec);
                    tm_connect.tm_year -= 1900;
                    tm_connect.tm_mon -= 1;
                    connect_time_t = mktime(&tm_connect);

                    int connection_duration = (int)difftime(disconnect_time_t, connect_time_t);

                    printf("[%s] Клиент отключился: id=%d, время соединения: %d секунд, подключен был: %s\n", 
                           current_time, fds[i].fd, connection_duration, connection_time[i]);

                    close(fds[i].fd);
                    fds[i].fd = -1;

                    memset(connection_time[i], 0, sizeof(connection_time[i]));
                } 
            }
        }

        for (int i = 0; i < count_ds; i++) {
            if (fds[i].fd == -1) {
                for (int j = i; j < count_ds - 1; j++) {
                    fds[j] = fds[j + 1];
                    strncpy(connection_time[j], connection_time[j + 1], sizeof(connection_time[j]));
                }
                i--; 
                count_ds--;
            }
        }
    }

    for (int i = 0; i < count_ds; i++) {
        if (fds[i].fd != -1) {
            close(fds[i].fd);
        }
    }

    time_t end_time;
    time(&end_time);
    printf("Сервер завершил работу, программа работала %ld секунд\n", (long)difftime(end_time, start_time));

    unlink(SOCKET_PATH);
    return 0;
}
