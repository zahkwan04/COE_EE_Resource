#include <stdio.h>
#include <stdint.h>

union FloatBits {
    float    f;
    uint32_t bits;
};

void print_float_hex(float value) {
    union FloatBits u;
    u.f = value;
    printf("%g = 0x%08X\n", value, u.bits);
}

int main(void) {
    print_float_hex(1.0f);       // 0x3F800000
    print_float_hex(2.0f);       // 0x40000000
    print_float_hex(0.5f);       // 0x3F000000
    print_float_hex(-1.0f);      // 0xBF800000
    return 0;
}