#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define BUF 1024

int main(){

    int client_fd;
    struct sockaddr_un socket_addr;
    char buffer[BUF] = "bbbbbbbbbbbbbbbbbbbbbbbbbb";
    ssize_t bytes = strlen(buffer);
    buffer[bytes] = '\0';

    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sun_family = AF_UNIX;
    strncpy(socket_addr.sun_path, SOCKET_PATH, sizeof(socket_addr.sun_path)-1);

    connect(client_fd, (struct sockaddr*)&socket_addr, sizeof(socket_addr));

    int count = 0;

    sleep(1.5);

    while(count < 60){
        write(client_fd, buffer, bytes);
        count++;
        usleep(100000);
    }

    close(client_fd);

    return 0;
}