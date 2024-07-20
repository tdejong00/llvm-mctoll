// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Result:
// CHECK: 10 20 30 
// CHECK: 40 50 60 
// CHECK: 70 80 90

#include <stdio.h>

#define ROWS 3
#define COLUMNS 3

int A[ROWS][COLUMNS] = { 
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
};

void multiply(int M[ROWS][COLUMNS], int S) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            printf("%d ", M[i][j] * S);
        }
        printf("\n");
    }
}

int main(void) {
    printf("Result:\n");
    multiply(A, 10);
    return 0;
}
