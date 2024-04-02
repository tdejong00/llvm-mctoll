#include <stdio.h>

void func(int A, int B) {
    if (A == B) {
        puts("equal");
    } else {
        puts("not equal");
    }
}

int main(void) {
    func(4, 4);
    return 0;
}
