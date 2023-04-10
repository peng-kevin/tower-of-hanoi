#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

// Strings used for rendering
#define EMPTY " "
#define DISK "#"
#define ROD "|"

#define NUM_POLES 3
#define SPACE_BETWEEN_POLES 3 // The number of empty spaces between poles when rendering
#define ARRAY_RESIZE_INCREMENT 256; // For dynamic array allocation

#define COLORMAP_FILE "CET-I1.csv" // The csv file containing the colormap used


/*
 * Stores an rgb value as a triplet of bytes
 */
struct Color {
    int r, g, b;
};

/*
 * Stores a color map for converting from a scalar to a color.
 * It is stored as an array where each color is equally spaced. THe length is
 * the number of color values.
 */
struct ColorMap {
    struct Color *colors;
    int length;
};

/*
 * Stores information about a single disk
 */
struct Disk {
    int size; // The size of the disk
    struct Color color; // The color of the disk
};

/*
 * Stores information about a single pole
 */
struct Pole {
    int size; // The max size of the pole
    int num_disk; // The current number of disks on the pole
    struct Disk *disks; // Array storing the disk with lower ones closer to bottom.
};

/*
 * Stores information about the overall state of the game
 */
struct GameState {
    struct Pole *poles; // Points to array of poles
    int num_layers; // The total number of layers
    // Tracks the number of moves made so far
    // It is a long due to the exponential growth of the number of moves.
    long num_moves; 
};

/*
 * Attempts to malloc size bytes, exits on failure
 */
void* malloc_or_die(size_t size) {
    void *ptr = malloc(size);
    if(ptr == NULL) {
        perror("malloc failed");
        exit(1);
    }
    return ptr;
}

/*
 * Reads in a csv file containing the ColorMap. The file must contain three
 * columns representing red, green and blue in that order. Each row defines a color
 * as a triplet of numbers from 0-255 progressing from the lowest to highest
 * color. The colors are assumed to be equally spaced. There can be any number
 * of colors included.
 *
 * In the event of failure, may print an error and returns a colormap where
 * colormap.colors is not allocated and length is set to -1.
 */
struct ColorMap load_colormap(const char *filename) {
    struct ColorMap cmap;
    int arraysize = ARRAY_RESIZE_INCREMENT;
    int r, g, b;

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error");
        cmap.length = -1;
        return cmap;
    }
    // Begin reading
    cmap.colors = malloc_or_die(arraysize * sizeof(*cmap.colors));
    int i = 0;
    while(1) {
        int n = fscanf(file, "%d,%d,%d\n", &r, &g, &b);
        // Check for end of file or invalid format
        if (n == EOF && !ferror(file)) {
            break;
        } else if(n != 3) {
            // The format must be invalid
            fprintf(stderr, "Error: colormap file format invalid\n");
            free(cmap.colors);
            cmap.length = -1;
            return cmap;
        }
        // Check validity
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
            fprintf(stderr, "Error: record %d: %d %d %d out of range\n", i + 1, r, g, b);
            free(cmap.colors);
            cmap.length = -1;
            return cmap;
        }
        // Resize array if it is full
        if (i == arraysize) {
            arraysize += ARRAY_RESIZE_INCREMENT;
            cmap.colors = realloc(cmap.colors, arraysize * sizeof(*cmap.colors));
            if (cmap.colors == NULL) {
                perror("Error");
                free(cmap.colors);
                cmap.length = -1;
                return cmap;
            }
        }
        cmap.colors[i].r = r;
        cmap.colors[i].g = g;
        cmap.colors[i].b = b;
        i++;
    }
    fclose(file);
    
    // Shrink the array if necessary
    if (i != arraysize) {
        // In case realloc fails
        struct Color *newptr = realloc(cmap.colors, i * sizeof(*newptr));
        if(newptr != NULL) {
            cmap.colors = newptr;
        }
    }
    cmap.length = i;
    return cmap;
}

/*
 * Frees dynamically allocated memory
 */
void destroy_colormap(struct ColorMap colormap) {
    free(colormap.colors);
}

/*
 * Prints a string n times
 */
void print_repeat(char str[], int count) {
    for (int i = 0; i < count; i++) {
        fputs(str, stdout);
    }
}

/*
 * Prints a string n times with a certain color
 */
void print_repeat_color(char str[], int count, struct Color color) {
    int buf_size = 256;
    char colored_str[buf_size];
    snprintf(colored_str, buf_size, "\033[38;2;%d;%d;%dm%s\033[0;0m", color.r, color.g, color.b, str);
    for (int i = 0; i < count; i++) {
        fputs(colored_str, stdout);
    }
}

/*
 * Renders a single layer of a pole. layer is zero indexed
 */
void render_layer_pole(struct Pole pole, int num_layers, int layer) {
    if (pole.num_disk <= layer) {
        // No disk at this layer
        print_repeat(EMPTY, num_layers - 1);
        fputs(ROD, stdout);
        print_repeat(EMPTY, num_layers - 1);
    } else {
        // Disk at this layer
        int disk_size = pole.disks[layer].size;
        struct Color disk_color = pole.disks[layer].color;
        print_repeat(EMPTY, num_layers - disk_size);
        print_repeat_color(DISK, disk_size * 2 - 1, disk_color);
        // print_repeat(DISK, disk_size * 2 - 1);
        print_repeat(EMPTY, num_layers - disk_size);
    }
}


/*
 * Renders a single layer of poles. layer is zero indexed
 */
void render_layer(struct GameState game_state, int layer) {
    for (int i = 0; i < NUM_POLES; i++) {
        render_layer_pole(game_state.poles[i], game_state.num_layers, layer);
        if (i != NUM_POLES - 1) {
            print_repeat(EMPTY, SPACE_BETWEEN_POLES);
        }
    }
}

/*
 * Clears the previosly drawn image
 */
void erase_drawing(struct GameState game_state) {
    for (int i = 0; i < game_state.num_layers + 1; i++) {
        printf("\033[A");
        printf("\r");
        printf("\033[J");
    }
}


/*
 * Draws the number of steps and poles
 */
void draw(struct GameState game_state) {
    printf("Moves: %ld / %ld\n", game_state.num_moves, (1l << (long) game_state.num_layers) - 1l);
    for (int i = game_state.num_layers - 1; i >= 0; i--) {
        render_layer(game_state, i);
        printf("\n");
    }
    sleep(1);
}

/*
 * Replaces the previous drawing
 */
void redraw(struct GameState game_state) {
    erase_drawing(game_state);
    draw(game_state);
}

/*
 * Intializes an array of NUM_POLES poles, with the first being full and the rest
 * empty
 */
void initialize_poles(struct Pole poles[NUM_POLES], int num_layers, struct ColorMap colormap) {
    // The minus one is because we use zero indexing when going through the map
    // Special case to avoid dividing by zero
    int colormap_increment;
    if (num_layers == 1) {
        colormap_increment = 0;
    } else{
        colormap_increment = colormap.length / (num_layers - 1);
    }
    if (colormap_increment < 1) {
        colormap_increment = 1;
    }
    // Initialize all poles as empty
    for (int i = 0; i < NUM_POLES; i++) {
        poles[i].size = num_layers;
        poles[i].num_disk = 0;
        poles[i].disks = malloc_or_die(num_layers * sizeof(*(poles[i].disks)));
    }
    // Fill first pole
    poles[0].num_disk = num_layers;
    for (int i = 0; i < num_layers; i++) {
        poles[0].disks[i].size = num_layers - i;
        int colormap_index = colormap_increment * i;
        if (colormap_index >= colormap.length) {
            colormap_index = colormap.length - 1;
        }
        poles[0].disks[i].color = colormap.colors[colormap_index];
    }
}

/*
 * Frees dynamically allocated memory
 */
void destroy_poles(struct Pole poles[NUM_POLES]) {
    for (int i = 0; i < NUM_POLES; i++) {
        free(poles[i].disks);
    }
}

/*
 * Takes a string representing the number of layers for the Tower of Hanoi
 * and converts it to an int while doing error and bounds checking.
 * If the string is invalid, it prints an error message and returns -1.
 * Returns the number of layers, or -1 on error
 */
int string_to_num_layers(char arg[]) {
    char* str_end;
    long n = strtol(arg, &str_end, 10);
    if (n == LONG_MIN || n == LONG_MAX) {
        perror("Error: Invalid num_layers");
        return -1;
    }
    if (*str_end != '\0') {
        fprintf(stderr, "Error: num_layers must be an integer\n");
        return -1;
    }
    if (n <= 0) {
        fprintf(stderr, "Error: num_layers greater than zero\n");
        return -1;
    }
    if (n > INT_MAX) {
        fprintf(stderr, "Error: num_layers must be less than %d\n", INT_MAX);
        return -1;
    }
    return (int) n;
}


/*
 * Asks the user enter the number of layers and returns it. Repeats if the user
 * gives an invalid input.
 * 
 * Returns the number of layers, or -1 on error
 */
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

/*
 * Returns index of pole that is not pole1 or pole2
 */
int get_spare(int pole1, int pole2) {
    for (int i = 0; i < NUM_POLES; i++) {
        if (i != pole1 && i != pole2) {
            return i;
        }
    }
    // Should not occur
    fprintf(stderr, "Error: could not find spare with pole1 = %d, pole2 = %d, NUM_POLES = %d\n", pole1, pole2, NUM_POLES);
    exit(1);
}

/*
 * Moves the top disk of src to dest
 *
 * If the attempted move is invalid, print an error message and exit the program.
 */
void move_disk(struct GameState *game_state, int src, int dest) {
    struct Pole *src_pole = &game_state->poles[src];
    struct Pole *dest_pole = &game_state->poles[dest];
    // Ensure that the src pole is not empty and the dest pole is not full for debugging.
    // Should not occur
    if (src_pole->num_disk <= 0) {
        redraw(*game_state);
        fprintf(stderr, "Error: src pole (%d) is empty\n", src);
        exit(1);
    }
    if (dest_pole->num_disk >= dest_pole->size) {
        redraw(*game_state);
        fprintf(stderr, "Error: dest pole (%d) is full\n", dest);
        exit(1);
    }

    // Move the disk
    dest_pole->disks[dest_pole->num_disk] = src_pole->disks[src_pole->num_disk - 1];
    dest_pole->num_disk++;
    src_pole->num_disk--;
    game_state->num_moves++;

    // Ensure there is not a larger piece on a smaller piece for debugging.
    // Should not occur.
    if (dest_pole->num_disk > 1) {
        int top_size = dest_pole->disks[dest_pole->num_disk - 1].size;
        int under_top_size = dest_pole->disks[dest_pole->num_disk - 2].size;
        if (top_size > under_top_size) {
            redraw(*game_state);
            fprintf(stderr, "Error: attempted illegal move. This should not occur");
            exit(1);
        }
    }

    redraw(*game_state);
}

/*
 * Move a stack of "size" disks from the src to dest pole. Renders each step.
 */
void move_stack(struct GameState *game_state, int size, int src, int dest) {
    if (size == 1) {
        move_disk(game_state, src, dest);
    } else {
        int spare = get_spare(src, dest);
        move_stack(game_state, size - 1, src, spare);
        move_stack(game_state, 1, src, dest);
        move_stack(game_state, size - 1, spare, dest);
    }
}

void solve_hanoi(struct GameState *game_state) {
    move_stack(game_state, game_state->num_layers, 0, NUM_POLES - 1);
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
    printf("Number of layers: %d\n", num_layers);

    // Load colormap
    struct ColorMap colormap = load_colormap(COLORMAP_FILE);
    if (colormap.length == -1) {
        fprintf(stderr, "Error: Failed to load colormap from %s\n", COLORMAP_FILE);
        exit(1);
    }

    // Initialize poles
    struct Pole poles[NUM_POLES];
    initialize_poles(poles, num_layers, colormap);
    destroy_colormap(colormap);

    // Initialize game state
    struct GameState game_state;
    game_state.poles = poles;
    game_state.num_layers = num_layers;
    game_state.num_moves = 0;

    draw(game_state);
    solve_hanoi(&game_state);

    destroy_poles(poles);
}
