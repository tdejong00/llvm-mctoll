// REQUIRES: system-linux
// REQUIRES: riscv64-unknown-linux-gnu-gcc
// RUN: riscv64-unknown-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 6

#include <stdio.h>

#define N 4

int A[N] = {2,4,6,8};

int main(void) {
    int *a = (int *)(A + 2);
    printf("%d\n", *a);
    return 0;
}