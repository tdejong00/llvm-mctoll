#include <stdio.h>

void func(int X, int N) {
    if (X < N) {
        puts("smaller");
    } else {
        puts("bigger");
    }
}

int main(void) {
    func(4, 5);
    return 0;
}
