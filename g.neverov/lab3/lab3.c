#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

void print_ids() {
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
}

void try_open_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        printf("File opened successfully\n");
        fclose(file);
    } else {
        perror("Failed to open file");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];

    printf("=== Before setuid ===\n");
    print_ids();

    try_open_file(filename);

    if (setuid(geteuid()) == -1) {
        perror("setuid failed");
        return 1;
    }

    printf("\n=== After setuid ===\n");
    print_ids();
    try_open_file(filename);

    return 0;
}