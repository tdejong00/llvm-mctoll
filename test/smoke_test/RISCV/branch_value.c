// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 3
// CHECK: 1

#include <stdio.h>

int func(int X) {
    int A = 0;
    if (X < 3) {
        A = 1;
    } else {
        A = 3;
    }
    return A;
}

int main(void) {
    printf("%d\n", func(4)); // Expected output: 3
    printf("%d\n", func(2)); // Expected output: 1
    return 0;
}
