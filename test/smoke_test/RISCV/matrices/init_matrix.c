// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Result:
// CHECK: -1 -1 -1
// CHECK: -1 -1 -1
// CHECK: -1 -1 -1

#include <stdio.h>

#define ROWS 3
#define COLUMNS 3

int A[ROWS][COLUMNS];

void init(int M[][COLUMNS]) {
    for (int I = 0; I < ROWS; I++) {
        for (int J = 0; J < COLUMNS; J++) {
            M[I][J] = -1;
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
    init(A);
    printf("Result:\n");
    display(A);
    return 0;
}
