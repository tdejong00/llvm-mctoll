// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: index: 3
// CHECK: index: -1

#include <stdio.h>

#define N 8

int A[N] = { 2, 3, 5, 9, 13, 17, 28, 54 };

int binarySearch(int A[], int l, int r, int x) {
    while (l <= r) {
        int m = l + (r - l) / 2;
    
        if (A[m] == x) {
            return m;
        }
        if (A[m] < x) {
            l = m + 1;
        }
        if (A[m] >= x) {
            r = m - 1;
        }
    }
    return -1;
}

int main(void) {
    int y1 = binarySearch(A, 0, N - 1, 9);
    int y2 = binarySearch(A, 0, N - 1, 25);
    printf("index: %d\n", y1);
    printf("index: %d\n", y2);
    return 0;
}
