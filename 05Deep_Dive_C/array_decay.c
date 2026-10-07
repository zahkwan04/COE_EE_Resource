#include <stdio.h>

void print_size(int arr[], int length) {
    printf("\nInside function: number of elements = %d\n", length);

    // Now you can iterate safely
    for (int i = 0; i < length; i++) {
        printf("  arr[%d] = %d\n", i, arr[i]);
    }
}

int main(void) {
    int numbers[5] = {10, 20, 30, 40, 50};
    int length = sizeof(numbers) / sizeof(numbers[0]);

    printf("\n This is the sizeof(numbers[0])=%d", sizeof(numbers[0] ));
    printf("\n This is the sizeof(numbers) value=%d", sizeof(numbers));

    print_size(numbers, length);
    return 0;
}