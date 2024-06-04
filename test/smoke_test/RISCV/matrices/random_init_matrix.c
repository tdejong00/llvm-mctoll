// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t --include-files=/usr/include/stdio.h,/usr/include/stdlib.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Result:
// CHECK: 3 3 3
// CHECK: 0 9 1
// CHECK: 5 2 6

#include <stdio.h>
#include <stdlib.h>

#define ROWS 3
#define COLUMNS 3

int A[ROWS][COLUMNS];

void init(int M[][COLUMNS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            int x = rand() % 10;
            M[i][j] = x;
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
    srand(123);
    init(A);
    printf("Result:\n");
    display(A);
    return 0;
}
