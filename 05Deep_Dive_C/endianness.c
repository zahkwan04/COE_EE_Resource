#include <stdio.h>
#include <stdint.h>

int main(void) {
    uint32_t value = 0x12345678;
    uint8_t *p = (uint8_t *)&value;

    printf("Value: 0x%08X\n\n", value);
    printf("Address stored: %p\n", (void*)(p));
    printf("Memory layout:\n");
    printf("  Address   Byte\n");
    printf("  -------   ----\n");
    for (int i = 0; i < 4; ++i) {
        printf("  %p  %02X\n", (void*)(p + i), p[i]);
    }

    // Detect endianness
    if (p[0] == 0x78) {
        printf("\nResult: LITTLE-endian\n");
    } else if (p[0] == 0x12) {
        printf("\nResult: BIG-endian\n");
    }
    return 0;
}