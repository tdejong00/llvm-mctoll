// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Result:
// CHECK: 21 27 33 
// CHECK: 57 72 87 
// CHECK: 93 117 141

#include <stdio.h>

#define ROWS 3
#define COLUMNS 3

int A[ROWS][COLUMNS] = { 
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
};

int B[ROWS][COLUMNS] = { 
    {7, 8, 9},
    {1, 2, 3},
    {4, 5, 6}
};

void multiply(int M1[][COLUMNS], int M2[][COLUMNS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            int c = 0;
            for (int k = 0; k < COLUMNS; k++) {
                c += M1[i][k] * M2[k][j];
            }
            printf("%d ", c);
        }
        printf("\n");
    }
}

int main(void) {
    printf("Result:\n");
    multiply(A, B);
    return 0;
}
