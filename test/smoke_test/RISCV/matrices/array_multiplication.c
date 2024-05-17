// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Result:
// CHECK: 620181

#include <stdio.h>

#define COLUMNS 3

int A[COLUMNS] = { 123, 234, 345 };

int B[COLUMNS] = { 789, 891, 912 };

void multiply(int M1[], int M2[]) {
    int c = 0;
    for (int i = 0; i < COLUMNS; i++) {
        c += M1[i] * M2[i];
    }
    printf("%d\n", c);
}

int main(void) {
    printf("Result:\n");
    multiply(A, B);
    return 0;
}
