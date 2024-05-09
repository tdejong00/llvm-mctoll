// REQUIRES: system-linux
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

int loop(int L, int R) {
    while (L <= R) {
        int M = L;
        if (M >= R) {
            return L;
        }
        printf("L: %d\n", L);
        L = M + 1;
    }
    return -1;
}

int main(void) {
    int Y1 = loop(0, 8);
    printf("Y1: %d\n", Y1);
    int Y2 = loop(9, 8);
    printf("Y2: %d\n", Y2);
    return 0;
}
