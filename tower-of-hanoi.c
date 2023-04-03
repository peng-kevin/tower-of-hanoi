#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Strings used for rendering
#define EMPTY " "
#define DISK "#"
#define ROD "|"

#define NUM_POLES 3
#define SPACE_BETWEEN_POLES 3 // The number of empty spaces between poles when rendering


struct Disk {
    int size; // The size of the disk
};

struct Pole {
    int size; // The max size of the pole
    int num_disk; // The current number of disks on the pole
    struct Disk *disks; // Array storing the disk with lower ones closer to bottom.
};

// Prints a string n times
void print_repeat(char str[], int count) {
    for (int i = 0; i < count; i++) {
        fputs(str, stdout);
    }
}

// Renders a single layer of a pole. layer is zero indexed
void render_layer_pole(struct Pole pole, int num_layers, int layer) {
    if (pole.num_disk <= layer) {
        // No disk at this layer
        print_repeat(EMPTY, num_layers - 1);
        fputs(ROD, stdout);
        print_repeat(EMPTY, num_layers - 1);
    } else {
        // Disk at this layer
        int disk_size = pole.disks[layer].size;
        print_repeat(EMPTY, num_layers - disk_size);
        print_repeat(DISK, disk_size * 2 - 1);
        print_repeat(EMPTY, num_layers - disk_size);
    }
}


// Renders a single layer of poles. layer is zero indexed
void render_layer(struct Pole poles[NUM_POLES], int num_layers, int layer) {
    for (int i = 0; i < NUM_POLES; i++) {
        render_layer_pole(poles[i], num_layers, layer);
        if (i != NUM_POLES - 1) {
            print_repeat(EMPTY, SPACE_BETWEEN_POLES);
        }
    }
}

// Renders the number of steps and poles
void render( struct Pole poles[NUM_POLES], int num_layers) {
    for (int i = num_layers - 1; i >= 0; i--) {
        render_layer(poles, num_layers, i);
        printf("\n");
    }
    printf("\n\n");
}

// Intializes an array of NUM_POLES poles, with the first being full and the rest
// empty
void initialize_poles(struct Pole poles[NUM_POLES], int num_layers) {
    // Initialize all poles as empty
    for (int i = 0; i < NUM_POLES; i++) {
        poles[i].size = num_layers;
        poles[i].num_disk = 0;
        poles[i].disks = malloc(num_layers * sizeof(*(poles[i].disks)));
    }
    // Fill first pole
    poles[0].num_disk = num_layers;
    for (int i = 0; i < num_layers; i++) {
        poles[0].disks[i].size = num_layers - i;
    }
}

// Frees dynamicall allocated memory
void destroy_poles(struct Pole poles[NUM_POLES]) {
    for (int i = 0; i < NUM_POLES; i++) {
        free(poles[i].disks);
    }
}

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
            // Read failed or reached end of file
            if (ferror(stdin)) {
                perror("Error");
            }
            return -1;
        }
        // Strip trailing newline if necessary
        int length = strlen(buf);
        if (length > 0 && buf[length - 1] == '\n') {
            buf[length -1] = '\0';
        }
        if (buf[0] == '\0') {
            // Skip empty lines
            continue;
        }
        int num_layers = string_to_num_layers(buf);
        if (num_layers == -1) {
            // Failed to convert to int
            continue;
        }
        return num_layers;
    }
}

// Returns index of pole that is not pole1 or pole2
int get_spare(int pole1, int pole2) {
    for (int i = 0; i < NUM_POLES; i++) {
        if (i != pole1 && i != pole2) {
            return i;
        }
    }
    // Should not occur
    printf("Error: could not find spare with pole1 = %d, pole2 = %d, NUM_POLES = %d\n", pole1, pole2, NUM_POLES);
    exit(1);
}

// Moves the top disk of src to dest
void move_disk(struct Pole *src, struct Pole *dest) {
    if (src->num_disk <= 0) {
        printf("Error: src pole is empty\n");
        return;
    }
    if (dest->num_disk >= dest->size) {
        printf("Error: dest pole is full\n");
        return;
    }
    dest->disks[dest->num_disk] = src->disks[src->num_disk - 1];
    dest->num_disk++;
    src->num_disk--;

    // Ensure correctness for debugging. Should not occur.
    if (dest->num_disk > 1) {
        int top_size = dest->disks[dest->num_disk - 1].size;
        int under_top_size = dest->disks[dest->num_disk - 2].size;
        if (top_size > under_top_size) {
            printf("Error: attempted illegal move. This should not occur");
            exit(1);
        }
    }
}

// Move a stack of "size" disks from the src to dest pole. Renders each step.
void move_stack(struct Pole poles[NUM_POLES], int num_layers, int size, int src, int dest) {
    if (size == 1) {
        move_disk(&poles[src], &poles[dest]);
        render(poles, num_layers);
    } else {
        int spare = get_spare(src, dest);
        move_stack(poles, num_layers, size - 1, src, spare);
        move_stack(poles, num_layers, 1, src, dest);
        move_stack(poles, num_layers, size - 1, spare, dest);
    }
}

void solve_hanoi(struct Pole poles[NUM_POLES], int num_layers) {
    move_stack(poles, num_layers, num_layers, 0, NUM_POLES - 1);
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        printf("Usage: %s [num_layers]\n", argv[0]);
        exit(1);
    }

    // Get num_layers
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

    // Initialize poles
    struct Pole poles[NUM_POLES];
    initialize_poles(poles, num_layers);

    render(poles, num_layers);
    solve_hanoi(poles, num_layers);

    destroy_poles(poles);
}
