/**
 * General utilities
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/**
 * convert string to integer number
*/
int str_to_number(char *str) {

    int x = 1, result = 0;
    int str_len = strlen(str);

    for (int i = str_len - 1; i >= 0; i--) {
        result += (str[i] - 48) * x;
        x *= 10;
    }

    return result;
}

/**
 * make a letter-only string and a number-only string from a source string
 * if make successfully, return 1
 * else return 0
*/
int char_number_split(char *str, char *char_str, char *num_str) {

    memset(char_str, '\0', sizeof(char_str));
    memset(num_str, '\0', sizeof(num_str));

    for (int i = 0; i < strlen(str); i++) {

        if (('a' <= str[i] && str[i] <= 'z') || ('A' <= str[i] && str[i] <= 'Z')) {
            strncat(char_str, str + i, 1);
            continue;
        }

        if ('0' <= str[i] && str[i] <= '9') {
            strncat(num_str, str + i, 1);
            continue;
        }

        return 0;
    }

    char_str[strlen(char_str)] = '\0';
    num_str[strlen(num_str)] = '\0';
    
    return 1;
}

/**
 * Print error message
*/
void report_error() {
    perror("Error");
    printf("\n");
}

/**
 * Clear stdin buffer
*/
void clear_stdin_buff() {
    char ch;
    while ((ch = getchar()) != EOF && ch != '\n');
}