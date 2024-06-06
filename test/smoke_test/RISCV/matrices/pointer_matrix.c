// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Result:
// CHECK: -1 -1 -1
// CHECK: -1 -1 -1
// CHECK: -1 -1 -1

#include <stdio.h>
#include <stdlib.h>

#define ROWS 3
#define COLUMNS 3

void init(int **M) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            M[i][j] = -1;
        }
    }
}

void display(int **M) {
    for (int i = 0; i < COLUMNS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            printf("%d ", M[i][j]);
        }
        printf("\n");
    }
}

int main(void) {
    printf("Allocating matrix...\n");
    int **A = (int **)malloc(ROWS * sizeof(int*));
    for (int i = 0; i < ROWS; i++) {
        A[i] = (int *)malloc(COLUMNS * sizeof(int));
    }

    init(A);
    printf("Result:\n");
    display(A);

    printf("Cleaning up...\n");
    for (int i = 0; i < ROWS; i++) {
        free(A[i]);
    }
    free(A);

    return 0;
}
