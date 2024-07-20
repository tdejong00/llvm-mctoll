// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 3
// CHECK: 1

#include <stdio.h>

int func(int x) {
    int a = 0;
    if (x < 3) {
        a = 1;
    } else {
        a = 3;
    }
    return a;
}

int main(void) {
    printf("%d\n", func(4)); // Expected output: 3
    printf("%d\n", func(2)); // Expected output: 1
    return 0;
}
