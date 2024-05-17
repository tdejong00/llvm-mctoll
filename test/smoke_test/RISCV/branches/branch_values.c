// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 7
// CHECK: 3

#include <stdio.h>

int func(int x) {
    int a = 0, b = 0;
    if (x < 3) {
        a = 1, b = 2;
    } else {
        a = 3, b = 4;
    }
    return a + b;
}

int main(void) {
    printf("%d\n", func(4)); // Expected output: 7
    printf("%d\n", func(2)); // Expected output: 3
    return 0;
}
