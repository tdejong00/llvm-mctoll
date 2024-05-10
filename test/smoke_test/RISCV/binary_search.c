// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: index: 3
// CHECK: index: -1

#include <stdio.h>

#define N 8

int A[N] = { 2, 3, 5, 9, 13, 17, 28, 54 };

int binarySearch(int A[], int L, int R, int X) {
    while (L <= R) {
        int M = L + (R - L) / 2;
    
        if (A[M] == X) {
            return M;
        }
        if (A[M] < X) {
            L = M + 1;
        }
        if (A[M] >= X) {
            R = M - 1;
        }
    }
    return -1;
}

int main(void) {
    int Y1 = binarySearch(A, 0, N - 1, 9);
    int Y2 = binarySearch(A, 0, N - 1, 25);
    printf("index: %d\n", Y1);
    printf("index: %d\n", Y2);
    return 0;
}
