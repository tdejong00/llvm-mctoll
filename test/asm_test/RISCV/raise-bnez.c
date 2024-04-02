#include <stdio.h>

void func(int A) {
    if (A == 0) {
        puts("equals zero");
    } else {
        puts("does not equal zero");
    }
}

int main(void) {
    func(0);
    return 0;
}
