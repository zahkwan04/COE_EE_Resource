#include <stdio.h>

int main() {
    int a = 10;
    int b = 20;
    int *ptr_a = &a; // Pointer to a
    int *ptr_b = &b; // Pointer to b

    printf("size of *ptr_a = %d\n", sizeof(*ptr_a));
    printf("size of ptr_a = %d\n", sizeof(ptr_a));

    printf("Before swapping:\n");
    printf("a = %d, b = %d\n", a, b);

    printf("Addresses: ptr_a = %p, ptr_b = %p\n", (void*)ptr_a, (void*)ptr_b);
    printf("Values: *ptr_a = %d, *ptr_b = %d\n", *ptr_a, *ptr_b);
    printf("Addresses of a and b: &a = %p, &b = %p\n", (void*)&a, (void*)&b);

    // Swap values using pointers
    int temp = *ptr_a;
    *ptr_a = *ptr_b;
    *ptr_b = temp;

    printf("After swapping:\n");
    printf("a = %d, b = %d\n", a, b);

    return 0;
}