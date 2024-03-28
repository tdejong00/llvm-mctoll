#include <stdio.h>

int main(void) {
    int A = 15, B = 2;
    int Y = A >> B;
    printf("%d\n", Y); // Expected output: 3
    return 0;
}
