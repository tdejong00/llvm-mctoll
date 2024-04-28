// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Result:
// CHECK: 1 2 3
// CHECK: 4 5 6
// CHECK: 7 8 9

#include <stdio.h>

#define ROWS 3
#define COLUMNS 3

int A[ROWS][COLUMNS] = {
    {1, 2, 3}, 
    {4, 5, 6}, 
    {7, 8, 9}
};

int B[ROWS][COLUMNS];

void copy(int M1[][COLUMNS], int M2[][COLUMNS]) {
    for (int I = 0; I < ROWS; I++) {
        for (int J = 0; J < COLUMNS; J++) {
            M2[I][J] = M1[I][J];
        }
    }
}

void display(int M[][COLUMNS]) {
    for (int I = 0; I < COLUMNS; I++) {
        for (int J = 0; J < COLUMNS; J++) {
            printf("%d ", M[I][J]);
        }
        printf("\n");
    }
}

int main(void) {
    copy(A, B);
    printf("Result:\n");
    display(B);
    return 0;
}
