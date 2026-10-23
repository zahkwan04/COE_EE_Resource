#include <stdio.h>
#include <stdint.h>

void check_memory(uint32_t data[]) {
    printf("Size inside function: %zu\n", sizeof(data));
}

void process_data(uint32_t *data, size_t length) {
    printf("Size of array in function: %d\n", length);
    for(size_t i = 0; i < length; i++) {
        printf("data[%zu] = %u\n", i, data[i]);
        // Safe!
    }
}

int main() {
    uint32_t buffer[5] = {10, 20, 30, 40, 50};
    uint32_t *ptr = buffer;
    
    printf("Size in main: %zu\n", sizeof(buffer));
    printf("Pointer size: %zu\n", sizeof(ptr));
    
    ptr = ptr + 3;
    check_memory(buffer);
    process_data(buffer, sizeof(buffer) / sizeof(buffer[0]));
    
    return 0;
}