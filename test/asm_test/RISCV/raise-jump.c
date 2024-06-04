#include <stdio.h>

int main(void) {

    int N = 5;
    int i = 0;

L1:
    printf("%d\n", i);
    if (i < N) {
        i++;
        goto L1;
    } else {
        goto L2;
    }
L2: 
    return 0;
}
