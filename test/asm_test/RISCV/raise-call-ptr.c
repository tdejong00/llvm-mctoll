#include <stdio.h>

void increment(int *X) {
    *X += 1;
}

int main(void) {
    int X = 3;
    increment(&X);
    printf("%d\n", X); // Expected output: 4
    return 0;
}
