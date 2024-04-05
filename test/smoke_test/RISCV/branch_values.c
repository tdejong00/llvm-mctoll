// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 7
// CHECK: 3

#include <stdio.h>

int func(int X) {
    int A = 0, B = 0;
    if (X < 3) {
        A = 1, B = 2;
    } else {
        A = 3, B = 4;
    }
    return A + B;
}

int main(void) {
    printf("%d\n", func(4)); // Expected output: 7
    printf("%d\n", func(2)); // Expected output: 3
    return 0;
}
