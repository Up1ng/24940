#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
#include <ulimit.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
void print_id_user() {
    printf("Real ID %d\n", getuid());
    printf("Effective ID: %d\n", geteuid());
}

void become_a_proc_leader() {
    if (setpgid(0, 0) == -1) {
        printf("Fail setpgid\n");
    } else {
        printf("Successful setpgid\n");
    }
}

void print_id_proc() {
    printf("Process: PID=%d\nParent PID=%d\nGroup PID=%d\n", getpid(), getppid(), getpgrp());
}

void print_ulimit() {
    long max_proc = sysconf(_SC_CHILD_MAX);
    if (max_proc != -1) {
        printf("%ld\n", max_proc);
    } else {
        perror("Error getting process limit");
    }
}


void print_size_corfile() {
    struct rlimit limit;
    if (getrlimit(RLIMIT_CORE, &limit) == 0) {
        if (limit.rlim_cur == RLIM_INFINITY) {
            printf("Size cor-file: undefinded\n");
        } else {
            printf("Max size cor-file: %lld\n", (long long) limit.rlim_cur);
        }
    } else {
        printf("Error getrlimit\n");
    }
}

void print_current_directory() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, PATH_MAX) != NULL) {
        printf("Current directory: %s\n", cwd);
    } else {
        perror("getcwd");
    }
}

void print_environment() {
    extern char **environ;
    char **env = environ;
    printf("Environment variables:\n");
    while (*env) {
        printf("%s\n", *env);
        env++;
    }
}

void set_environment_variable(char *str) {
    char *str_copy = strdup(str);
    int res = putenv(str_copy);
    if (res == 0) {
        printf("Set environment variable successful\n");
    } else {
        free(str_copy);
        perror("Error changing or creating environment variable");
    }
}

void set_core_size(const char *str) {
    char *endptr;
    char *str_copy = strdup(str);
    long new_size = strtol(str_copy, &endptr, 10);
    if (*endptr != '\0' || errno == ERANGE) {
        perror("Invalid core size value");
    } else {
        struct rlimit limit;
        limit.rlim_cur = (rlim_t) new_size;
        limit.rlim_max = (rlim_t) new_size;
        if (setrlimit(RLIMIT_CORE, &limit) == -1) {
            perror("setrlimit");
        } else {
            printf("Core file size limit changed to: %ld bytes\n", new_size);
        }
    }
    free(str_copy);
}

void set_new_ulimit(const char *str) {
    char *endptr;
    long new_val = strtol(str, &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Invalid ulimit value: %s\n", str);
    } else {
        long result = ulimit(UL_SETFSIZE, new_val);
        if (result != -1) {
            printf("New process limit: %ld\n", new_val);
        } else {
            perror("Error setting process limit");
        }
    }
}
int main(int argc, char *argv[]) {
    int c;
    int count = 0;

    if (argc <= 1) {
        printf("No arguments\n");
        return 0;
    }

    while ((c = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        count++;
        switch (c) {
            case 'i':
                print_id_user();
                break;
            case 's':
                become_a_proc_leader();
                break;
            case 'p':
                print_id_proc();
                break;
            case 'u':
                print_ulimit();
                break;
            case 'c':
                print_size_corfile();
                break;
            case 'd':
                print_current_directory();
                break;
            case 'v':
                print_environment();
                break;
            case 'V':
                if (optarg) {
                    set_environment_variable(optarg);
                } else {
                    fprintf(stderr, "Option -V requires an argument\n");
                }
                break;
            case 'C':
                if (optarg) {
                    set_core_size(optarg);
                } else {
                    fprintf(stderr, "Option -C requires an argument\n");
                }
                break;
            case 'U':
                if (optarg) {
                    set_new_ulimit(optarg);
                } else {
                    fprintf(stderr, "Option -U requires an argument\n");
                }
                break;
            case '?':
                printf("Invalid option %c\n", optopt);
                break;
        }
    }

    if (count == 0) {
        printf("No options\n");
    }
    return 0;
}