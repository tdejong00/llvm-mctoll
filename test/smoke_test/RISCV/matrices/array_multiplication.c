// REQUIRES: system-linux
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
    int C = 0;
    for (int I = 0; I < COLUMNS; I++) {
        C += M1[I] * M2[I];
    }
    printf("%d\n", C);
}

int main(void) {
    printf("Result:\n");
    multiply(A, B);
    return 0;
}
