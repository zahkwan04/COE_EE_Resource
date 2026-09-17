#include <stdio.h>

int main(int argc, char *argv[]) {
    // argc = argument count, argv = argument vector (array of strings)
    
    if (argc > 1) {
        // If a name was provided in the command line
        printf("Hello, %s!\n", argv[1]);
    } else {
        // Fallback if no name is provided
        printf("Hello, World!\n");
    }

    return 0;
}