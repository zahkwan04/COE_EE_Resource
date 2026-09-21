#include <stdio.h>

int main(int argc, char *argv[]) {
    // argc = argument count, argv = argument vector (array of strings)
    
    if (argc > 1) {
        // If a name was provided in the command line
        printf("this is argv[0], %s!" , argv[0]);
        printf("\nHello, %s!\n", argv[1]);
    } else {
        // Fallback if no name is provided
        printf("Hello, World!\n");
    }

    return 0;
}