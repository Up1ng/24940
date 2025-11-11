#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

void disable_echo_canonical(struct termios *original_settings) {
    struct termios new_settings;
    tcgetattr(STDIN_FILENO, original_settings);
    
    new_settings = *original_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
}

void restore_terminal(struct termios *original_settings) {
    tcsetattr(STDIN_FILENO, TCSANOW, original_settings);
}

int main() {
    struct termios original_settings;
    disable_echo_canonical(&original_settings);

    char input_string[41] = {0};
    int input_len = 0;
    char ch;
    int last_space = -1;

    while (1) {
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n <= 0) {
            break;
        }

        switch (ch) {
            case 127: // ERASE
                if (input_len > 0) {
                    input_len--;
                    write(STDOUT_FILENO, "\b \b", 3);
                    input_string[input_len] = '\0';
                
                    if (input_len == last_space) {
                        last_space = -1;
                        for (int i = input_len - 1; i >= 0; i--) {
                            if (input_string[i] == ' ') {
                                last_space = i;
                                break;
                            }
                        }
                    }
                }
                break;
            case 21: // KILL
                while (input_len > 0) {
                    write(STDOUT_FILENO, "\b \b", 3);
                    input_len--;
                }
                input_string[0] = '\0';
                last_space = -1;
                break;
            case 4: // CTRL+D
                if (input_len == 0) {
                    restore_terminal(&original_settings);
                    return 0;
                }
                break;
            case 23: // CTRL+W
                if (input_len > 0) {
                    while (input_len > 0 && input_string[input_len - 1] == ' ') {
                        write(STDOUT_FILENO, "\b \b", 3);
                        input_len--;
                    }
                    while (input_len > 0 && input_string[input_len - 1] != ' ') {
                        write(STDOUT_FILENO, "\b \b", 3);
                        input_len--;
                    }
                    input_string[input_len] = '\0';
                    last_space = -1;
                    for (int i = input_len - 1; i >= 0; i--) {
                        if (input_string[i] == ' ') {
                            last_space = i;
                            break;
                        }
                    }
                }
                break;
            case 10:
                input_string[input_len] = '\n';
                input_string[input_len + 1] = '\0';
                write(STDOUT_FILENO, "\n", 1);
                input_len = 0;
                last_space = -1;
                break;
            default:     
                if (ch < 32 || ch == 127) {
                    write(STDOUT_FILENO, "\a", 1);
                    write(STDOUT_FILENO, "^G", 2);
                } else {
                    if (input_len < 40) {
                        input_string[input_len++] = ch;
                        write(STDOUT_FILENO, &ch, 1);
                        
                        if (ch == ' ') {
                            last_space = input_len - 1;
                        }
                    } else {
                        if (ch == ' ') {
                            input_string[input_len] = '\n';
                            input_string[input_len + 1] = '\0';
                            write(STDOUT_FILENO, "\n", 1);
                            write(STDOUT_FILENO, &ch, 1);
                            input_len = 1;
                            input_string[0] = ch;
                            last_space = 0;
                        } else {
                            if (last_space != -1) {
                                int chars_to_move = input_len - last_space - 1;
                                
                                write(STDOUT_FILENO, "\n", 1);
                                
                                for (int i = last_space + 1; i < input_len; i++) {
                                    write(STDOUT_FILENO, &input_string[i], 1);
                                }
                                
                                write(STDOUT_FILENO, &ch, 1);
                                
                                for (int i = 0; i < chars_to_move; i++) {
                                    input_string[i] = input_string[last_space + 1 + i];
                                }
                                input_string[chars_to_move] = ch;
                                input_len = chars_to_move + 1;
                                
                                last_space = -1;
                                for (int i = 0; i < input_len; i++) {
                                    if (input_string[i] == ' ') {
                                        last_space = i;
                                    }
                                }
                            } else {
                                input_string[input_len] = '\n';
                                input_string[input_len + 1] = '\0';
                                write(STDOUT_FILENO, "\n", 1);
                                write(STDOUT_FILENO, &ch, 1);
                                input_len = 1;
                                input_string[0] = ch;
                                last_space = -1;
                            }
                        }
                    }
                }
                break;
        }
    }

    restore_terminal(&original_settings);
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}