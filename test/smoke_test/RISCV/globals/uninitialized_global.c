// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 3

#include <stdio.h>

int A;

int main(void) {
    A = 3;
    printf("%d\n", A);
    return 0;
}
