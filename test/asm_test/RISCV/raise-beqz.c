#include <stdio.h>

void func(int A) {
    if (A != 0) {
        puts("does not equal zero");
    } else {
        puts("equals zero");
    }
}

int main(void) {
    func(1);
    return 0;
}
