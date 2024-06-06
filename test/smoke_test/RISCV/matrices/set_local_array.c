// NOTE: failing because arrays and structs on stack are not supported yet

// UNSUPPORTED: not-implemented
// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 0
// CHECK: 2

#include <stdio.h>

int main(void) {
    int A[2] = { 1, 0 };

    printf("%d\n", A[1]);
    A[1] = 2;
    printf("%d\n", A[1]);
}
