// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Array A:
// CHECK: 1 2 3 4 5

#include <stdio.h>

#define COLUMNS 5

int A[COLUMNS] = { 1, 2, 3, 4, 5 };

void display(int M[]) {
    for (int I = 0; I < COLUMNS; I++) {
        printf("%d ", M[I]);
    }
    printf("\n");
}

int main(void) {
    puts("Array A:");
    display(A);
}
