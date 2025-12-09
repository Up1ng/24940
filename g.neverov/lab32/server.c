#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define SOCKET_PATH  "./socket"
#define BUF 1024
#define MAX_CLIENTS 5

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) { perror("fcntl F_GETFL"); return; }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) { perror("fcntl F_SETFL"); }
}

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

    struct pollfd fds[MAX_CLIENTS + 1];
    int client_count = 1;

    char mixed_buffer[BUF * 10] = {0};
    int mixed_pos[MAX_CLIENTS + 1] = {0};

    char connection_time[MAX_CLIENTS + 1][64];

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    set_nonblocking(server_fd);

    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sun_family = AF_UNIX;
    strncpy(socket_addr.sun_path, SOCKET_PATH, sizeof(socket_addr.sun_path)-1);

    unlink(SOCKET_PATH);
    bind(server_fd, (struct sockaddr*)&socket_addr, sizeof(socket_addr));
    listen(server_fd, MAX_CLIENTS);

    memset(fds, 0, sizeof(fds));
    memset(connection_time, 0, sizeof(connection_time));

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    printf("Сервер запущен...\n");

    while (1) {
        int ready = poll(fds, client_count, 100);

        for (int i = 0; i < client_count; i++) {
            if (fds[i].revents == 0) continue;

            if (fds[i].fd == server_fd) {
                if (fds[i].revents & POLLIN) {
                    while (1) {
                        client_fd = accept(server_fd, NULL, NULL);
                        if (client_fd == -1) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        }

                        if (client_count < MAX_CLIENTS + 1) {
                            set_nonblocking(client_fd);
                            fds[client_count].fd = client_fd;
                            fds[client_count].events = POLLIN;

                            get_current_time(connection_time[client_count], sizeof(connection_time[client_count]));
                            char current_time[64];
                            get_current_time(current_time, sizeof(current_time));
                            printf("[%s] Новое подключение клиента %d\n", current_time, client_count);

                            mixed_pos[client_count] = 0;
                            client_count++;
                        } else {
                            close(client_fd);
                        }
                    }
                }
            } else if (fds[i].revents & POLLIN) {
                bytes = read(fds[i].fd, buffer, 1);

                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    buffer[0] = toupper(buffer[0]);

                    if (mixed_pos[i] < sizeof(mixed_buffer) - 1) {
                        mixed_buffer[mixed_pos[i]++] = buffer[0];
                        mixed_buffer[mixed_pos[i]] = '\0';
                    }

                    if (mixed_pos[i] >= 30) {
                        char current_time[64];
                        get_current_time(current_time, sizeof(current_time));
                        printf("[%s] Клиент %d: %s\n", current_time, fds[i].fd, mixed_buffer);
                        mixed_pos[i] = 0;
                    }
                } else if (bytes == -1) {
                    if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                        close(fds[i].fd);
                        fds[i].fd = -1;
                    }
                } else if (bytes == 0) {
                    char current_time[64];
                    get_current_time(current_time, sizeof(current_time));

                    time_t now = time(NULL);
                    struct tm tm_connect;
                    memset(&tm_connect, 0, sizeof(tm_connect));
                    sscanf(connection_time[i], "%d-%d-%d %d:%d:%d",
                           &tm_connect.tm_year, &tm_connect.tm_mon, &tm_connect.tm_mday,
                           &tm_connect.tm_hour, &tm_connect.tm_min, &tm_connect.tm_sec);
                    tm_connect.tm_year -= 1900;
                    tm_connect.tm_mon -= 1;
                    time_t connect_time = mktime(&tm_connect);

                    int duration = (int)difftime(now, connect_time);
                    printf("[%s] Клиент %d отключился, время подключения: %d секунд\n",
                           current_time, fds[i].fd, duration);

                    close(fds[i].fd);
                    fds[i].fd = -1;
                    memset(connection_time[i], 0, sizeof(connection_time[i]));
                    mixed_pos[i] = 0;
                }
            }
        }

        for (int i = 0; i < client_count; i++) {
            if (fds[i].fd == -1) {
                for (int j = i; j < client_count - 1; j++) {
                    fds[j] = fds[j + 1];
                    strncpy(connection_time[j], connection_time[j + 1], sizeof(connection_time[j]));
                    mixed_pos[j] = mixed_pos[j + 1];
                }
                i--;
                client_count--;
            }
        }
    }

    for (int i = 0; i < client_count; i++) {
        if (fds[i].fd != -1) close(fds[i].fd);
    }

    unlink(SOCKET_PATH);
    return 0;
}
