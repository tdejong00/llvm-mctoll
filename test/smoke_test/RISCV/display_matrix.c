// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Array A:
// CHECK: 1 2 3 4 5
// CHECK: 6 7 8 9 10
// CHECK: 11 12 13 14 15
// CHECK: 16 17 18 19 20
// CHECK: 21 22 23 24 25

#include <stdio.h>

#define ROWS 5
#define COLUMNS 5

int A[ROWS][COLUMNS] = {
    {1, 2, 3, 4, 5},
    {6, 7, 8, 9, 10},
    {11, 12, 13, 14, 15},
    {16, 17, 18, 19, 20},
    {21, 22, 23, 24, 25}
};

void display(int M[][COLUMNS]) {
    for (int I = 0; I < COLUMNS; I++) {
        for (int J = 0; J < COLUMNS; J++) {
            printf("%d ", M[I][J]);
        }
        printf("\n");
    }
}

int main(void) {
    puts("Array A:");
    display(A);
}
