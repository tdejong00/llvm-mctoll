#include <stdio.h>

void func(int A, int B) {
    if (A != B) {
        puts("not equal");
    } else {
        puts("equal");
    }
}

int main(void) {
    func(5, 4);
    return 0;
}
