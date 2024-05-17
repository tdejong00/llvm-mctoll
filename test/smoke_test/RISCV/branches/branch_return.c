// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 3
// CHECK: 1

#include <stdio.h>

int func(int x) {
    if (x < 3) {
        return 1;
    }
    return 3;
}

int main(void) {
    printf("%d\n", func(4)); // Expected output: 3
    printf("%d\n", func(2)); // Expected output: 1
    return 0;
}
