#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef LEN_BUFFER
#define LEN_BUFFER 10
#endif

typedef struct Node
{
    char* string;
    struct Node* next;
} Node;

Node* add_node(Node* prev, char* string)
{
    Node* ans = malloc(sizeof(Node));
    if (ans == NULL)
        return NULL;
    
    if (prev != NULL)
        prev->next = ans;
    
    ans->next = NULL;
    ans->string = string;
    return ans;
}

void print_sanitized(const char *s)
{
    for (const unsigned char *p = (const unsigned char *)s; *p != '\0'; p++)
    {
        if (isprint(*p))
            putchar(*p);
        else
            printf("\\x%02X", *p);
    }
    putchar('\n');
}

void print_list(Node* cur)
{
    if (cur != NULL)
    {
        print_sanitized(cur->string);
        print_list(cur->next);
    }
}

void free_list(Node* cur)
{
    if (cur != NULL)
    {
        free_list(cur->next);
        free(cur->string);
        free(cur);
    }
}

void exit_program(int code, char* message)
{
    if (message != NULL)
        perror(message);
    exit(code);
}

int get_list_from_user(Node** startList)
{
    char buffer[LEN_BUFFER];
    char* string = NULL;
    Node *prev = NULL, *start = NULL;
    size_t string_len = 0;

    while (fgets(buffer, LEN_BUFFER, stdin))
    {
        // Удаляем символ новой строки если он есть
        size_t buf_len = strlen(buffer);
        if (buf_len > 0 && buffer[buf_len - 1] == '\n')
        {
            buffer[buf_len - 1] = '\0';
            buf_len--;
        }

        // Проверяем пустую строку
        if (buf_len == 0)
            continue;

        // Проверяем на точку в начале строки
        if (buffer[0] == '.')
        {
            if (buf_len == 1) // Только точка
                break;
            
            // Точка с текстом - копируем без точки
            string = strdup(buffer + 1);
        }
        else
        {
            string = strdup(buffer);
        }

        if (string == NULL)
            return 1;

        // Дополняем строку пока не достигнем конца ввода
        while (buf_len == LEN_BUFFER - 1 && buffer[LEN_BUFFER - 2] != '\0')
        {
            if (!fgets(buffer, LEN_BUFFER, stdin))
                break;

            buf_len = strlen(buffer);
            if (buf_len > 0 && buffer[buf_len - 1] == '\n')
            {
                buffer[buf_len - 1] = '\0';
                buf_len--;
            }

            char* new_string = realloc(string, strlen(string) + buf_len + 1);
            if (new_string == NULL)
            {
                free(string);
                return 1;
            }
            string = new_string;
            strcat(string, buffer);
        }

        prev = add_node(prev, string);
        if (prev == NULL)
            return 1;

        if (start == NULL)
            start = prev;
    }

    if (ferror(stdin))
        return 1;

    *startList = start;
    return 0;
}

int main()
{
    Node* start = NULL;

    switch (get_list_from_user(&start))
    {
    case 1:
        exit_program(EXIT_FAILURE, "get_list_from_user failed");
        break;
    case 2:
        fprintf(stderr, "get_list_from_user failed: EOF\n");
        exit_program(EXIT_FAILURE, NULL);
    }

    puts("strings:");
    print_list(start);
    free_list(start);

    exit_program(EXIT_SUCCESS, NULL);
}