// REQUIRES: system-linux
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 6

#include <stdio.h>

int A[4] = {1,2,3,4};

int main(void) {
    int B[4] = {2,3,4,5};
    printf("%d\n", A[1] + B[2]); // Expected output: 2 + 4 = 6
    return 0;
}
