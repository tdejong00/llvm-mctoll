// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: L: 0
// CHECK: L: 1
// CHECK: L: 2
// CHECK: L: 3
// CHECK: L: 4
// CHECK: L: 5
// CHECK: L: 6
// CHECK: L: 7
// CHECK: Y1: 8
// CHECK: Y2: -1

#include <stdio.h>

int loop(int l, int r) {
    while (l <= r) {
        int m = l;
        if (m >= r) {
            return l;
        }
        printf("L: %d\n", l);
        l = m + 1;
    }
    return -1;
}

int main(void) {
    int y1 = loop(0, 8);
    printf("Y1: %d\n", y1);
    int y2 = loop(9, 8);
    printf("Y2: %d\n", y2);
    return 0;
}
