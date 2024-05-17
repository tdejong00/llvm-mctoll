// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 0
// CHECK: 2

#include <stdio.h>

int A[2] = { 1, 0 };

int main(void) {
    printf("%d\n", A[1]);
    A[1] = 2;
    printf("%d\n", A[1]);
}
