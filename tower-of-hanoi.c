#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Takes a string representing the number of layers for the Tower of Hanoi
// and converts it to an int while doing error and bounds checking.
// If the string is invalid, it prints an error message and returns -1.

// Returns the number of layers, or -1 on error
int string_to_num_layers(char arg[]) {
    char* str_end;
    long n = strtol(arg, &str_end, 10);
    if (n == LONG_MIN || n == LONG_MAX) {
        perror("Error: Invalid num_layers");
        return -1;
    }
    if (*str_end != '\0') {
        printf("Error: num_layers must be an integer\n");
        return -1;
    }
    if (n <= 0) {
        printf("Error: num_layers greater than zero\n");
        return -1;
    }
    if (n > INT_MAX) {
        printf("Error: num_layers must be less than %d\n", INT_MAX);
        return -1;
    }
    return (int) n;
}

// Asks the user enter the number of layers and returns it. Repeats if the user
// gives an invalid input.
//
// Returns the number of layers, or -1 on error
int get_num_layers_from_user() {
    int buf_size = 64;
    char buf[buf_size];
    while(true) {
        printf("Enter the number of layers: ");
        if (fgets(buf, buf_size, stdin) == NULL) {
            if (ferror(stdin)) {
                perror("Error");
            }
            return -1;
        }
        // strip trailing newline if necessary
        int length = strlen(buf);
        if (length > 0 && buf[length - 1] == '\n') {
            buf[length -1] = '\0';
        }
        if (buf[0] == '\0') {
            continue;
        }
        int num_layers = string_to_num_layers(buf);
        if (num_layers == -1) {
            continue;
        }
        return num_layers;
    }
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        printf("Usage: %s [num_layers]\n", argv[0]);
        exit(1);
    }
    int num_layers;
    if (argc == 2) {
        num_layers = string_to_num_layers(argv[1]);
    } else {
        num_layers = get_num_layers_from_user();
    }
    if (num_layers == -1) {
        exit(1);
    }

    printf("num_layers: %d\n", num_layers);
}
