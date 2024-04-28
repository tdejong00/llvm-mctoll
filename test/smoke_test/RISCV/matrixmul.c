// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Matrix A:
// CHECK: 1 2 3 
// CHECK: 4 5 6 
// CHECK: 7 8 9 
// CHECK: Matrix B:
// CHECK: 7 8 9 
// CHECK: 1 2 3 
// CHECK: 4 5 6 
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

int C[ROWS][COLUMNS] = { 
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};

void multiply(int M3[ROWS][COLUMNS], int M1[ROWS][COLUMNS], int M2[ROWS][COLUMNS]) {
    for (int I = 0; I < ROWS; I++) {
        for (int J = 0; J < COLUMNS; J++) {
            M3[I][J] = 0;
            for (int K = 0; K < COLUMNS; K++) {
                M3[I][J] += M1[I][K] * M2[K][J];
            }
        }
    }
}

void display(int M[ROWS][COLUMNS]) {
    for (int I = 0; I < ROWS; I++) {
        for (int J = 0; J < COLUMNS; J++) {
            printf("%d ", M[I][J]);
        }
        printf("\n");
    }
}

int main(void) {
    printf("Matrix A:\n");
    display(A);

    printf("Matrix B:\n");
    display(B);

    multiply(C, A, B);


    printf("Result:\n");
    display(C);

    return 0;
}
