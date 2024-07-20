// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: A:
// CHECK: 1 2 3
// CHECK: 4 5 6
// CHECK: 7 8 9
// CHECK: Result:
// CHECK: -1 -1 -1
// CHECK: -1 -1 -1
// CHECK: -1 -1 -1

#include <stdio.h>

#define ROWS 3
#define COLUMNS 3

int A[ROWS][COLUMNS] = {
    {1, 2, 3}, 
    {4, 5, 6}, 
    {7, 8, 9}
};

void reset(int M[ROWS][COLUMNS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            M[i][j] = -1;
        }
    }
}

void display(int M[][COLUMNS]) {
    for (int i = 0; i < COLUMNS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            printf("%d ", M[i][j]);
        }
        printf("\n");
    }
}

int main(void) {
    printf("A:\n");
    display(A);

    reset(A);

    printf("Result:\n");
    display(A);

    return 0;
}
