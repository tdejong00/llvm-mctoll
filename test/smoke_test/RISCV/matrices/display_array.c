// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: Array A:
// CHECK: 1 2 3 4 5

#include <stdio.h>

#define COLUMNS 5

int A[COLUMNS] = { 1, 2, 3, 4, 5 };

void display(int M[]) {
    for (int i = 0; i < COLUMNS; i++) {
        printf("%d ", M[i]);
    }
    printf("\n");
}

int main(void) {
    puts("Array A:");
    display(A);
}
