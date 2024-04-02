#include <stdio.h>

void func(int X, int N) {
    if (X >= N) {
        puts("bigger");
    } else {
        puts("smaller");
    }
}

int main(void) {
    func(5, 4);
    return 0;
}
