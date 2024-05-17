// REQUIRES: system-linux
// REQUIRES: riscv64-linux-gnu-gcc
// RUN: riscv64-linux-gnu-gcc -fno-stack-protector -o %t %s
// RUN: llvm-mctoll -d -debug %t -I /usr/include/stdio.h
// RUN: lli %t-dis.ll | FileCheck %s
// CHECK: 3
// CHECK: 7
// CHECK: 11
// CHECK: 15

#include <stdio.h>

#define N 4

int A[N] = {2,4,6,8};
int B[N] = {1,3,5, 7};

int main(void) {
    for (int i = 0; i < N; i ++) { 
        printf("%d\n", A[i] + B[i]);
    }
    return 0;
}
