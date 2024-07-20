// NOTE: failing because "Illegal instruction"

// UNSUPPORTED: non-functional
// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Original array: 5 2 8 1 3 6 4 7 
// CHECK: Sorted array:   1 2 3 4 5 6 7 8

#include <stdio.h>
#include <stdlib.h>

#define N 8

int A[N] = { 5, 2, 8, 1, 3, 6, 4, 7 };

int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

void dump() {
    for (int i = 0; i < N; i++) {
        printf("%d ", A[i]);
    }
    printf("\n");
}

int main() {
    printf("Original array: ");
    dump();

    qsort(A, N, sizeof(int), compare);

    printf("Sorted array:   ");
    dump();

    return 0;
}
